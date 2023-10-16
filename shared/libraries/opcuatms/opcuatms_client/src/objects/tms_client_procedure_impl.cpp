#include "coretypes/validation.h"
#include "opcuatms_client/objects/tms_client_procedure_factory.h"
#include "opcuatms/converters/variant_converter.h"
#include "opcuatms/converters/list_conversion_utils.h"

BEGIN_NAMESPACE_OPENDAQ_OPCUA_TMS

using namespace opcua;

TmsClientProcedureImpl::TmsClientProcedureImpl(const TmsClientContextPtr& ctx,
                                             const ContextPtr& daqContext,
                                             const OpcUaNodeId& parentId,
                                             const OpcUaNodeId& methodId)
    : ctx(ctx)
    , daqContext(daqContext)
    , parentId(parentId)
    , methodId(methodId)
{
}

ErrCode TmsClientProcedureImpl::dispatch(IBaseObject* args)
{
    // TODO: Return more specific error depending on OPC UA status code
    return daqTry([&]()
    {
        auto argsPtr = BaseObjectPtr::Borrow(args);
        OpcUaCallMethodRequest callRequest;

        if (!argsPtr.assigned())
        {
            callRequest = OpcUaCallMethodRequest(methodId, parentId, 0);
        }
        else if (argsPtr.asPtrOrNull<IList>().assigned())
        {
            auto argsList = argsPtr.asPtrOrNull<IList>();
            OpcUaVariant varArgs = ListConversionUtils::ToVariantTypeArrayVariant(argsList, daqContext);
            callRequest = OpcUaCallMethodRequest(methodId, parentId, argsList.getCount(), (UA_Variant*) varArgs->data);
        }
        else
        {
            OpcUaVariant varArgs = VariantConverter<IBaseObject>::ToVariant(argsPtr, nullptr, daqContext);
            callRequest = OpcUaCallMethodRequest(methodId, parentId, 1, &varArgs.getValue());
        }

        OpcUaObject<UA_CallMethodResult> callResult = ctx->getClient()->callMethod(callRequest);
        if (OPCUA_STATUSCODE_FAILED(callResult->statusCode) || (callResult->outputArgumentsSize != 0))
            return OPENDAQ_ERR_CALLFAILED;
        
        return OPENDAQ_SUCCESS;
    });
}

ErrCode TmsClientProcedureImpl::getCoreType(CoreType* coreType)
{
    OPENDAQ_PARAM_NOT_NULL(coreType);

    *coreType = ctProc;
    return OPENDAQ_SUCCESS;
}

END_NAMESPACE_OPENDAQ_OPCUA_TMS
