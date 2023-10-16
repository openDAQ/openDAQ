#include "open62541/tmsbsp_nodeids.h"
#include "opcuatms/converters/variant_converter.h"
#include "opcuatms_client/objects/tms_client_input_port_impl.h"
#include "opcuatms/errors.h"

BEGIN_NAMESPACE_OPENDAQ_OPCUA_TMS
    using namespace opcua;

TmsClientInputPortImpl::TmsClientInputPortImpl(const ContextPtr& ctx,
                                               const ComponentPtr& parent,
                                               const StringPtr& localId,
                                               const TmsClientContextPtr& tmsCtx,
                                               const opcua::OpcUaNodeId& nodeId)
    : TmsClientComponentBaseImpl(ctx, parent, localId, tmsCtx, nodeId)
{
}

ErrCode TmsClientInputPortImpl::getRequiresSignal(Bool* value)
{
    return daqTry([&]() {
        *value = readValue<IBoolean>("RequiresSignal");
        return OPENDAQ_SUCCESS;
    });
}

ErrCode TmsClientInputPortImpl::setRequiresSignal(Bool value)
{
    return OPENDAQ_ERR_NOTIMPLEMENTED;
}

ErrCode TmsClientInputPortImpl::acceptsSignal(ISignal* signal, Bool* accepts)
{
    return OPENDAQ_ERR_OPCUA_CLIENT_CALL_NOT_AVAILABLE;

    //return daqTry([&]() {
    //    OpcUaNodeId methodId(NAMESPACE_TMSBSP, UA_TMSBSPID_INPUTPORTTYPE_ACCEPTSSIGNAL);

    //    auto signalNodeId = clientContext->getNodeId(signal);
    //    OpcUaVariant inputArg;
    //    inputArg.setScalar(*signalNodeId);

    //    OpcUaCallMethodRequest callRequest(methodId, nodeId, 1, inputArg.get());
    //    OpcUaObject<UA_CallMethodResult> result = client->callMethod(callRequest);
    //    if (OPCUA_STATUSCODE_FAILED(result->statusCode) || (result->outputArgumentsSize != 1))
    //        return OPENDAQ_ERR_CALLFAILED;

    //    *accepts = OpcUaVariant(result->outputArguments[0]).toBool();
    //    
    //    return OPENDAQ_SUCCESS;
    //    
    //});
}

ErrCode TmsClientInputPortImpl::connect(ISignal* signal)
{
    return OPENDAQ_ERR_OPCUA_CLIENT_CALL_NOT_AVAILABLE;
    //return daqTry([&]() {
    //    OpcUaNodeId methodId(NAMESPACE_TMSBSP, UA_TMSBSPID_INPUTPORTTYPE_CONNECTSIGNAL);

    //    auto signalNodeId = clientContext->getNodeId(signal);
    //    OpcUaVariant inputArg;
    //    inputArg.setScalar(*signalNodeId);

    //    OpcUaCallMethodRequest callRequest(methodId, nodeId, 1, inputArg.get());
    //    OpcUaObject<UA_CallMethodResult> result = client->callMethod(callRequest);
    //    if (OPCUA_STATUSCODE_FAILED(result->statusCode))
    //        return OPENDAQ_ERR_CALLFAILED;
    //    else
    //    {
    //        referenceUtils.updateReferences(nodeId);
    //        return OPENDAQ_SUCCESS;
    //        
    //    }
    //});
}

ErrCode TmsClientInputPortImpl::disconnect()
{
    return OPENDAQ_ERR_OPCUA_CLIENT_CALL_NOT_AVAILABLE;

    //return daqTry([&]() {
    //    OpcUaNodeId methodId(NAMESPACE_TMSBSP, UA_TMSBSPID_INPUTPORTTYPE_DISCONNECTSIGNAL);

    //    OpcUaCallMethodRequest callRequest(methodId, nodeId, 0, nullptr);
    //    OpcUaObject<UA_CallMethodResult> result = client->callMethod(callRequest);
    //    if (OPCUA_STATUSCODE_FAILED(result->statusCode))
    //        return OPENDAQ_ERR_CALLFAILED;
    //    else
    //    {
    //        referenceUtils.updateReferences(nodeId);
    //        return OPENDAQ_SUCCESS;
    //    }
    //});
}

ErrCode TmsClientInputPortImpl::getSignal(ISignal** signal)
{
    SignalPtr signalPtr;
    ErrCode errCode = wrapHandlerReturn(this, &TmsClientInputPortImpl::onGetSignal, signalPtr);

    *signal = signalPtr.detach();
    return errCode;
}

SignalPtr TmsClientInputPortImpl::onGetSignal()
{
    OpcUaNodeId referenceTypeId(NAMESPACE_TMSBSP, UA_TMSBSPID_CONNECTEDTOSIGNAL);
    auto nodeIds = referenceUtils.getReferencedNodes(nodeId, referenceTypeId, true);
    assert(nodeIds.size() <= 1);

    if (!nodeIds.empty())
    {
        auto connectedSignalNodeId = *nodeIds.begin();
        return findSignal(connectedSignalNodeId);
    }

    return nullptr;
}

ErrCode TmsClientInputPortImpl::getConnection(IConnection** connection)
{
    return daqTry([&]() {
        // TODO: Implement. Awaits support to implement
        return OPENDAQ_ERR_NOTIMPLEMENTED;
    });
}

END_NAMESPACE_OPENDAQ_OPCUA_TMS
