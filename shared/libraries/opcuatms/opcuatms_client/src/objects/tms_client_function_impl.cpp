#include <coretypes/validation.h>
#include <opendaq/custom_log.h>
#include "opcuatms_client/objects/tms_client_function_factory.h"
#include "opcuatms/converters/variant_converter.h"
#include "opcuatms/converters/list_conversion_utils.h"

BEGIN_NAMESPACE_OPENDAQ_OPCUA_TMS
    using namespace opcua;

TmsClientFunctionImpl::TmsClientFunctionImpl(const TmsClientContextPtr& ctx,
                                             const ContextPtr& daqContext,
                                             const OpcUaNodeId& parentId,
                                             const OpcUaNodeId& methodId)
    : ctx(ctx)
    , daqContext(daqContext)
    , parentId(parentId)
    , methodId(methodId)
{
}

ErrCode TmsClientFunctionImpl::call(IBaseObject* args, IBaseObject** result)
{
    auto argsPtr = BaseObjectPtr::Borrow(args);
    OpcUaCallMethodRequest callRequest;

    if (!argsPtr.assigned())
    {
        callRequest = OpcUaCallMethodRequest(methodId, parentId, 0);
    }
    else if (argsPtr.asPtrOrNull<IList>().assigned())
    {
        try {
            auto argsList = argsPtr.asPtrOrNull<IList>();
            OpcUaVariant varArgs = ListConversionUtils::ToVariantTypeArrayVariant(argsList, daqContext);
            callRequest = OpcUaCallMethodRequest(methodId, parentId, argsList.getCount(), (UA_Variant*) varArgs->data);
        }
        catch (...)
        {
            auto loggerComponent = this->daqContext.getLogger().getOrAddComponent("OpcUaClientFunction");
            LOG_W("Failed to prepapre list of input arguments for OPC UA client function");
            return OPENDAQ_IGNORED;
        }
    }
    else
    {
        try {
            OpcUaVariant varArgs = VariantConverter<IBaseObject>::ToVariant(argsPtr, nullptr, daqContext);
            callRequest = OpcUaCallMethodRequest(methodId, parentId, 1, &varArgs.getValue());
        }
        catch (...)
        {
            auto loggerComponent = this->daqContext.getLogger().getOrAddComponent("OpcUaClientFunction");
            LOG_W("Failed to prepapre input argument for OPC UA client function");
            return OPENDAQ_IGNORED;
        }
    }

    OpcUaObject<UA_CallMethodResult> callResult = ctx->getClient()->callMethod(callRequest);
    if (OPCUA_STATUSCODE_FAILED(callResult->statusCode) || (callResult->outputArgumentsSize != 1))
    {
        auto loggerComponent = this->daqContext.getLogger().getOrAddComponent("OpcUaClientFunction");
        LOG_W("Failed to call OPC UA client function");
        return OPENDAQ_IGNORED;
    }
    try {
        *result = VariantConverter<IBaseObject>::ToDaqObject(OpcUaVariant(callResult->outputArguments[0]), daqContext).detach();
    }
    catch (...)
    {
        auto loggerComponent = this->daqContext.getLogger().getOrAddComponent("OpcUaClientFunction");
        LOG_W("Failed to convert OPC UA client function result to daq object");
        return OPENDAQ_IGNORED;
    }
    return OPENDAQ_SUCCESS;
}

ErrCode TmsClientFunctionImpl::getCoreType(CoreType* coreType)
{
    OPENDAQ_PARAM_NOT_NULL(coreType);

    *coreType = ctFunc;
    return OPENDAQ_SUCCESS;
}

END_NAMESPACE_OPENDAQ_OPCUA_TMS
