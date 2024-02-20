#include <iostream>
#include "opcuatms_server/objects/tms_server_input_port.h"
#include "opcuatms/converters/variant_converter.h"
#include "open62541/statuscodes.h"
#include "open62541/daqbsp_nodeids.h"
#include "open62541/di_nodeids.h"
#include "open62541/daqbsp_nodeids.h"
#include <opendaq/device_ptr.h>

BEGIN_NAMESPACE_OPENDAQ_OPCUA_TMS

using namespace opcua;

TmsServerInputPort::TmsServerInputPort(const InputPortPtr& object, const OpcUaServerPtr& server, const ContextPtr& context, const TmsServerContextPtr& tmsContext)
    : Super(object, server, context, tmsContext)
{
}

OpcUaNodeId TmsServerInputPort::getReferenceType()
{
    return OpcUaNodeId(NAMESPACE_DAQBSP, UA_DAQBSPID_HASINPUTPORT);
}

void TmsServerInputPort::addChildNodes()
{
    Super::addChildNodes();

    createConnectMethodNode();
    createDisconnectMethodNode();
}

OpcUaNodeId TmsServerInputPort::getTmsTypeId()
{
    return OpcUaNodeId(NAMESPACE_DAQBSP, UA_DAQBSPID_INPUTPORTTYPE);
}

void TmsServerInputPort::createConnectMethodNode()
{
    OpcUaNodeId nodeIdOut;
    AddMethodNodeParams params(nodeIdOut, nodeId);
    params.referenceTypeId = OpcUaNodeId(UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT));
    params.setBrowseName("Connect");
    params.inputArgumentsSize = 1;
    params.inputArguments = (UA_Argument*) UA_Array_new(params.inputArgumentsSize, &UA_TYPES[UA_TYPES_ARGUMENT]);

    params.inputArguments[0].name = UA_STRING_ALLOC("signalGlobalId");
    params.inputArguments[0].dataType = UA_TYPES[UA_TYPES_STRING].typeId;
    params.inputArguments[0].valueRank = UA_VALUERANK_SCALAR;

    auto methodNodeId = server->addMethodNode(params);

    auto callback = [this](NodeEventManager::MethodArgs args) -> UA_StatusCode
    {
        try
        {
            onConnectSignal(args);
        }
        catch (const OpcUaException& e)
        {
            return e.getStatusCode();
        }

        return UA_STATUSCODE_GOOD;
    };

    addEvent(methodNodeId)->onMethodCall(callback);
}

void TmsServerInputPort::createDisconnectMethodNode()
{
    OpcUaNodeId nodeIdOut;
    AddMethodNodeParams params(nodeIdOut, nodeId);
    params.referenceTypeId = OpcUaNodeId(UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT));
    params.setBrowseName("Disconnect");
    params.inputArgumentsSize = 0;

    auto methodNodeId = server->addMethodNode(params);

    auto callback = [this](NodeEventManager::MethodArgs args) -> UA_StatusCode
    {
        try
        {
            onDisconenctSignal(args);
        }
        catch (const OpcUaException& e)
        {
            return e.getStatusCode();
        }

        return UA_STATUSCODE_GOOD;
    };

    addEvent(methodNodeId)->onMethodCall(callback);
}

void TmsServerInputPort::onConnectSignal(NodeEventManager::MethodArgs args)
{
    assert(args.inputSize == 1);

    const auto globalId = OpcUaVariant(args.input[0]).toString();
    SignalPtr signal = tmsContext->findComponent(globalId).asPtrOrNull<ISignal>();

    if (!signal.assigned())
        throw OpcUaException(UA_STATUSCODE_BADNOTFOUND, "Signal not found");

    const auto refType = OpcUaNodeId(NAMESPACE_DAQBSP, UA_DAQBSPID_CONNECTEDTOSIGNAL);
    const auto sourceId = nodeId;
    const auto signalNodeId = findSignalNodeId(signal);

    deleteReferencesOfType(refType);
    addReference(signalNodeId, refType);
    browseReferences();

    object.connect(signal);
}

void TmsServerInputPort::onDisconenctSignal(NodeEventManager::MethodArgs args)
{
    assert(args.inputSize == 0);

    const auto refType = OpcUaNodeId(NAMESPACE_DAQBSP, UA_DAQBSPID_CONNECTEDTOSIGNAL);

    deleteReferencesOfType(refType);

    object.disconnect();
}

void TmsServerInputPort::bindCallbacks()
{
    addReadCallback("RequiresSignal", [this]() { return VariantConverter<IBoolean>::ToVariant(object.getRequiresSignal()); });

    Super::bindCallbacks();
}

void TmsServerInputPort::createNonhierarchicalReferences()
{
    auto connectedSignal = object.getSignal();
    if (connectedSignal.assigned())
    {
        auto connectedSignalNodeId = findSignalNodeId(connectedSignal);
        if (!connectedSignalNodeId.isNull())
            addReference(connectedSignalNodeId, OpcUaNodeId(NAMESPACE_DAQBSP, UA_DAQBSPID_CONNECTEDTOSIGNAL));
    }
}

END_NAMESPACE_OPENDAQ_OPCUA_TMS
