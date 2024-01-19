#include <iostream>
#include "opcuatms_server/objects/tms_server_input_port.h"
#include "opcuatms/converters/variant_converter.h"
#include "open62541/statuscodes.h"
#include "open62541/daqbsp_nodeids.h"
#include "open62541/di_nodeids.h"
#include "open62541/daqbsp_nodeids.h"

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

OpcUaNodeId TmsServerInputPort::getTmsTypeId()
{
    return OpcUaNodeId(NAMESPACE_DAQBSP, UA_DAQBSPID_INPUTPORTTYPE);
}

void TmsServerInputPort::bindCallbacks()
{

    addReadCallback("RequiresSignal", [this]() { return VariantConverter<IBoolean>::ToVariant(object.getRequiresSignal()); });

    //addEvent("AcceptsSignal")->onMethodCall([this](NodeEventManager::MethodArgs args)
    //{
    //    if (args.inputSize != 1 || args.outputSize != 1)
    //        return static_cast<UA_StatusCode>(UA_STATUSCODE_BADINTERNALERROR);

    //    auto nodeId = OpcUaVariant(args.input[0]).toNodeId();
    //    //TODO: check if signal can be accepted, for now allow all signals
    //    // Pseudocode:
    //    //   signal = lookUpSignal(nodeId)
    //    //   if (signal == nullptr)
    //    //     return static_cast<UA_StatusCode>(UA_STATUSCODE_BADNOTFOUND);
    //    //   Whatever else that needs to be done...

    //    bool accepts = true;
    //    
    //    OpcUaVariant result;
    //    result.setScalar(accepts);
    //    args.output[0] = result.getDetachedValue();
    //    return static_cast<UA_StatusCode>(UA_STATUSCODE_GOOD);
    //});

    //addEvent("ConnectSignal")->onMethodCall([this](NodeEventManager::MethodArgs args)
    //{
    //    if (args.inputSize != 1)
    //        return static_cast<UA_StatusCode>(UA_STATUSCODE_BADINTERNALERROR);

    //    auto nodeId = OpcUaVariant(args.input[0]).toNodeId();

    //    //TODO: Connect the signal to the input port
    //    // Pseudocode:
    //    //   signalNodeId = lookUpSignal(nodeId)
    //    //   if (signalNodeId == nullptr)
    //    //     return static_cast<UA_StatusCode>(UA_STATUSCODE_BADNOTFOUND);
    //    //   OpcUaNodeId referenceTypeId(NAMESPACE_TMSBSP, UA_TMSBSPID_CONNECTEDTO);
    //    //   server->addReference(signalNodeId, referenceTypeId, getNodeId(), true);
    //    //   Whatever else that needs to be done...

    //    return static_cast<UA_StatusCode>(UA_STATUSCODE_GOOD);
    //});

    //addEvent("DisconnectSignal")->onMethodCall([this](NodeEventManager::MethodArgs args)
    //{
    //    // TODO: Disconnect the signal to the input port
    //    // Pseudocode:
    //    //   auto signal = object.getSignal();
    //    //   if (signal == nullptr)
    //    //     return static_cast<UA_StatusCode>(UA_STATUSCODE_BADNOTFOUND);
    //    //   OpcUaNodeId referenceTypeId(NAMESPACE_TMSBSP, UA_TMSBSPID_CONNECTEDTO);
    //    //   server->deleteReference(this->getNodeId(), referenceTypeId, false);
    //    //   Whatever else that needs to be done...
    //    auto signal = object.getSignal();
    //    return static_cast<UA_StatusCode>(UA_STATUSCODE_GOOD);
    //});

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
