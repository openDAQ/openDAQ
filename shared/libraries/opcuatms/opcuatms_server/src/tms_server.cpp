#include <opendaq/packet.h>
#include <opendaq/reader_factory.h>
#include <opcuatms_server/tms_server.h>
#include <open62541/di_nodeids.h>
#include <iostream>

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

void TmsServer::setOpendaqVersion(const std::string& version)
{
    this->versionStr = version;
}

void TmsServer::start()
{
    if (!device.assigned())
        throw InvalidStateException("Device is not set.");
    if (!context.assigned())
        throw InvalidStateException("Context is not set.");

    server = std::make_shared<OpcUaServer>();
    server->setPort(opcUaPort);
    server->prepare();

    tmsContext = std::make_shared<TmsServerContext>(context, device);
    auto signals = device.getSignals();

    tmsDevice = std::make_unique<TmsServerDevice>(device, server, context, tmsContext);
    tmsDevice->registerOpcUaNode(OpcUaNodeId(NAMESPACE_DI, UA_DIID_DEVICESET));

    if (!versionStr.empty())
    {
        OpcUaNodeId newNodeId(0);
        AddVariableNodeParams params(newNodeId, tmsDevice->getNodeId());
        params.setBrowseName("OpenDaqPackageVersion");
        params.setDataType(OpcUaNodeId(UA_TYPES[UA_TYPES_STRING].typeId));
        params.typeDefinition = OpcUaNodeId(UA_NODEID_NUMERIC(0, UA_NS0ID_PROPERTYTYPE));
        const auto nodeId = server->addVariableNode(params);

        server->writeValue(nodeId, OpcUaVariant(versionStr.c_str()));
    }

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
