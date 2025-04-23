#include <opendaq/packet.h>
#include <opendaq/reader_factory.h>
#include <opcuatms_server/tms_server.h>
#include <open62541/di_nodeids.h>
#include <iostream>
#include <opendaq/device_info_factory.h>
#include <opendaq/device_info_internal_ptr.h>
#include <coreobjects/property_factory.h>

using namespace daq::opcua;
using namespace daq::opcua::tms;

BEGIN_NAMESPACE_OPENDAQ_OPCUA

TmsServer::~TmsServer()
{
    tmsContext = nullptr;
    stop();
}

TmsServer::TmsServer(const InstancePtr& instance)
    : TmsServer(instance.getRootDevice(), instance.getContext())
{
}

TmsServer::TmsServer(const DevicePtr& device, const ContextPtr& context)
    : device(device)
    , context(context)
{
}

void TmsServer::setOpcUaPort(uint16_t port)
{
    this->opcUaPort = port;
}

void TmsServer::setOpcUaPath(const std::string& path)
{
    this->opcUaPath = path;
}

void TmsServer::start()
{
    if (!device.assigned())
        DAQ_THROW_EXCEPTION(InvalidStateException, "Device is not set.");
    if (!context.assigned())
        DAQ_THROW_EXCEPTION(InvalidStateException, "Context is not set.");

    auto info = device.getInfo();
    if (info.hasServerCapability("OpenDAQOPCUAConfiguration"))
        DAQ_THROW_EXCEPTION(InvalidStateException, fmt::format("Device \"{}\" already has an OpenDAQOPCUAConfiguration server capability.", info.getName()));

    server = std::make_shared<OpcUaServer>();
    server->setPort(opcUaPort);
    server->setAuthenticationProvider(context.getAuthenticationProvider());
    server->setClientConnectedHandler(
        [this](const std::string& clientId)
        {
            const auto loggerComponent = context.getLogger().getOrAddComponent("TmsServer");
            LOG_I("New client connected, ID: {}", clientId);
            SizeT clientNumber = 0;
            if (device.assigned() && !device.isRemoved())
            {
                device.getInfo().asPtr<IDeviceInfoInternal>(true).addConnectedClient(
                    &clientNumber,
                    ConnectedClientInfo("", ProtocolType::Configuration, "OpenDAQOPCUA", "", ""));
            }
            registeredClientIds.insert({clientId, clientNumber});
        }
    );
    server->setClientDisconnectedHandler(
        [this](const std::string& clientId)
        {
            if (auto it = registeredClientIds.find(clientId); it != registeredClientIds.end())
            {
                const auto loggerComponent = context.getLogger().getOrAddComponent("TmsServer");
                LOG_I("Client disconnected, ID: {}", clientId);
                if (device.assigned() && !device.isRemoved() && it->second != 0)
                {
                    device.getInfo().asPtr<IDeviceInfoInternal>(true).removeConnectedClient(it->second);
                }
                registeredClientIds.erase(it);
            }
        }
    );
    server->prepare();

    tmsContext = std::make_shared<TmsServerContext>(context, device);

    auto serverCapability = ServerCapability("OpenDAQOPCUAConfiguration", "OpenDAQOPCUA", ProtocolType::Configuration);
    serverCapability.setPrefix("daq.opcua");
    serverCapability.setConnectionType("TCP/IP");
    serverCapability.setPort(opcUaPort);
    serverCapability.addProperty(StringProperty("Path", opcUaPath == "/" ? "" : opcUaPath));
    info.asPtr<IDeviceInfoInternal>(true).addServerCapability(serverCapability);

    tmsDevice = std::make_unique<TmsServerDevice>(device, server, context, tmsContext);
    tmsDevice->registerOpcUaNode(OpcUaNodeId(NAMESPACE_DI, UA_DIID_DEVICESET));
    tmsDevice->createNonhierarchicalReferences();

    server->start();
}

void TmsServer::stop()
{
    if (device.assigned() && !device.isRemoved())
    {
        const auto info = device.getInfo();
        const auto infoInternal = info.asPtr<IDeviceInfoInternal>();
        if (info.hasServerCapability("OpenDAQOPCUAConfiguration"))
            infoInternal.removeServerCapability("OpenDAQOPCUAConfiguration");
        for (const auto& [_, clientNumber] : registeredClientIds)
        {
            if (clientNumber != 0)
                infoInternal.removeConnectedClient(clientNumber);
        }
    }
    registeredClientIds.clear();

    if (server)
        server->stop();
    
    server.reset();
    tmsDevice.reset();
}

END_NAMESPACE_OPENDAQ_OPCUA
