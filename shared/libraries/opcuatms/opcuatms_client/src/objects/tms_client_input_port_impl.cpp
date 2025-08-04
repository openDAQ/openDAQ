#include <open62541/daqbsp_nodeids.h>
#include <opcuatms/converters/variant_converter.h>
#include <opcuatms_client/objects/tms_client_input_port_impl.h>
#include <opcuatms/errors.h>
#include <opendaq/device_ptr.h>
#include <opcuatms/exceptions.h>

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
    try
    {
        *value = readValue<IBoolean>("RequiresSignal");
    }
    catch(...)
    {
        LOG_W("Failed to get requires signals on OpcUA client input port \"{}\"", this->globalId);
    }
    return OPENDAQ_SUCCESS;
}

ErrCode TmsClientInputPortImpl::setRequiresSignal(Bool value)
{
    return DAQ_MAKE_ERROR_INFO(OPENDAQ_ERR_NOTIMPLEMENTED);
}

ErrCode TmsClientInputPortImpl::acceptsSignal(ISignal* signal, Bool* accepts)
{
    return DAQ_MAKE_ERROR_INFO(OPENDAQ_ERR_OPCUA_CLIENT_CALL_NOT_AVAILABLE);

    //const ErrCode errCode = daqTry([&]()
    //{
    //    OpcUaNodeId methodId(NAMESPACE_DAQBSP, UA_DAQBSPID_INPUTPORTTYPE_ACCEPTSSIGNAL);

    //    auto signalNodeId = clientContext->getNodeId(signal);
    //    OpcUaVariant inputArg;
    //    inputArg.setScalar(*signalNodeId);

    //    OpcUaCallMethodRequest callRequest(methodId, nodeId, 1, inputArg.get());
    //    OpcUaObject<UA_CallMethodResult> result = client->callMethod(callRequest);
    //    if (OPCUA_STATUSCODE_FAILED(result->statusCode) || (result->outputArgumentsSize != 1))
    //        return DAQ_MAKE_ERROR_INFO(OPENDAQ_ERR_CALLFAILED);

    //    *accepts = OpcUaVariant(result->outputArguments[0]).toBool();
    //    
    //    return OPENDAQ_SUCCESS;
    //    
    //});
    //OPENDAQ_RETURN_IF_FAILED(errCode);
    //return errCode;
}

ErrCode TmsClientInputPortImpl::connect(ISignal* signal)
{
    const ErrCode errCode = daqTry([&]()
    {
        if (!isChildComponent(signal))
            DAQ_THROW_EXCEPTION(NotFoundException);

        const SignalPtr signalPtr = signal;
        const auto methodNodeId = getNodeId("Connect");

        StringPtr remoteId;
        signalPtr.asPtr<ITmsClientComponent>()->getRemoteGlobalId(&remoteId);
        const auto signalIdVariant = OpcUaVariant(remoteId.toStdString().c_str());

        auto request = OpcUaCallMethodRequest();
        request->objectId = nodeId.copyAndGetDetachedValue();
        request->methodId = methodNodeId.copyAndGetDetachedValue();
        request->inputArgumentsSize = 1;
        request->inputArguments = (UA_Variant*) UA_Array_new(request->inputArgumentsSize, &UA_TYPES[UA_TYPES_VARIANT]);
        request->inputArguments[0] = signalIdVariant.copyAndGetDetachedValue();

        auto response = client->callMethod(request);

        if (response->statusCode != UA_STATUSCODE_GOOD)
            throw OpcUaGeneralException();
    });
    OPENDAQ_RETURN_IF_FAILED(errCode);
    return errCode;
}

ErrCode TmsClientInputPortImpl::connectSignalSchedulerNotification(ISignal* signal)
{
    return DAQ_MAKE_ERROR_INFO(OPENDAQ_ERR_OPCUA_CLIENT_CALL_NOT_AVAILABLE);
}

ErrCode TmsClientInputPortImpl::disconnect()
{
    const ErrCode errCode = daqTry([&]()
    {
        const auto methodNodeId = getNodeId("Disconnect");

        auto request = OpcUaCallMethodRequest();
        request->objectId = nodeId.copyAndGetDetachedValue();
        request->methodId = methodNodeId.copyAndGetDetachedValue();
        request->inputArgumentsSize = 0;

        auto response = client->callMethod(request);

        if (response->statusCode != UA_STATUSCODE_GOOD)
            throw OpcUaGeneralException();
    });
    OPENDAQ_RETURN_IF_FAILED(errCode);
    return errCode;
}

ErrCode TmsClientInputPortImpl::getSignal(ISignal** signal)
{
    SignalPtr signalPtr;
    const ErrCode errCode = wrapHandlerReturn(this, &TmsClientInputPortImpl::onGetSignal, signalPtr);
    OPENDAQ_RETURN_IF_FAILED(errCode);

    *signal = signalPtr.detach();
    return errCode;
}

SignalPtr TmsClientInputPortImpl::onGetSignal()
{
    auto browser = clientContext->getReferenceBrowser();

    auto filter = BrowseFilter();
    filter.referenceTypeId = OpcUaNodeId(NAMESPACE_DAQBSP, UA_DAQBSPID_CONNECTEDTOSIGNAL);
    filter.direction = UA_BROWSEDIRECTION_FORWARD;

    browser->invalidate(nodeId);
    const auto& references = browser->browseFiltered(nodeId, filter);
    assert(references.byNodeId.size() <= 1);

    if (!references.byNodeId.empty())
    {
        auto connectedSignalNodeId = references.byNodeId.begin().key();
        return findSignal(connectedSignalNodeId);
    }

    return nullptr;
}

StringPtr TmsClientInputPortImpl::onGetRemoteId() const
{
    return String(remoteComponentId).detach();
}

ErrCode TmsClientInputPortImpl::getConnection(IConnection** connection)
{
    return DAQ_MAKE_ERROR_INFO(OPENDAQ_ERR_NOTIMPLEMENTED);
}

END_NAMESPACE_OPENDAQ_OPCUA_TMS
