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
        throw InvalidStateException("Device is not set.");
    if (!context.assigned())
        throw InvalidStateException("Context is not set.");

    server = std::make_shared<OpcUaServer>();
    server->setPort(opcUaPort);
    server->setAuthenticationProvider(context.getAuthenticationProvider());
    server->prepare();

    tmsContext = std::make_shared<TmsServerContext>(context, device);
    auto signals = device.getSignals();

    auto serverCapability = ServerCapability("OpenDAQOPCUAConfiguration", "OpenDAQOPCUA", ProtocolType::Configuration);
    serverCapability.setPrefix("daq.opcua");
    serverCapability.setConnectionType("TCP/IP");
    serverCapability.setPort(opcUaPort);
    serverCapability.addProperty(StringProperty("Path", opcUaPath == "/" ? "" : opcUaPath));
    device.getInfo().asPtr<IDeviceInfoInternal>().addServerCapability(serverCapability);

    tmsDevice = std::make_unique<TmsServerDevice>(device, server, context, tmsContext);
    tmsDevice->registerOpcUaNode(OpcUaNodeId(NAMESPACE_DI, UA_DIID_DEVICESET));
    tmsDevice->createNonhierarchicalReferences();

    server->start();
}

void TmsServer::stop()
{
    if (server)
        server->stop();
    
    server.reset();
    tmsDevice.reset();
}

END_NAMESPACE_OPENDAQ_OPCUA
