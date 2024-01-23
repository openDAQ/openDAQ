#include "coretypes/validation.h"
#include "opcuatms_client/objects/tms_client_procedure_factory.h"
#include "opcuatms/converters/variant_converter.h"
#include "opcuatms/converters/list_conversion_utils.h"
#include <opendaq/custom_log.h>

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
    StringPtr lastProccessDescription = "";
    ErrCode errCode = daqTry([&]()
    {
        auto argsPtr = BaseObjectPtr::Borrow(args);
        OpcUaCallMethodRequest callRequest;

        if (!argsPtr.assigned())
        {
            lastProccessDescription = "Creating call request with no args";
            callRequest = OpcUaCallMethodRequest(methodId, parentId, 0);
        }
        else if (argsPtr.asPtrOrNull<IList>().assigned())
        {
            lastProccessDescription = "Creating call request with list of arguments";
            auto argsList = argsPtr.asPtrOrNull<IList>();
            OpcUaVariant varArgs = ListConversionUtils::ToVariantTypeArrayVariant(argsList, daqContext);
            callRequest = OpcUaCallMethodRequest(methodId, parentId, argsList.getCount(), (UA_Variant*) varArgs->data);
        }
        else
        {
            lastProccessDescription = "Creating call request with one arguments";
            OpcUaVariant varArgs = VariantConverter<IBaseObject>::ToVariant(argsPtr, nullptr, daqContext);
            callRequest = OpcUaCallMethodRequest(methodId, parentId, 1, &varArgs.getValue());
        }

        lastProccessDescription = "Calling procedure";
        OpcUaObject<UA_CallMethodResult> callResult = ctx->getClient()->callMethod(callRequest);
        if (OPCUA_STATUSCODE_FAILED(callResult->statusCode) || (callResult->outputArgumentsSize != 0))
            return OPENDAQ_ERR_CALLFAILED;
        
        return OPENDAQ_SUCCESS;
    });
    if (OPENDAQ_FAILED(errCode) && this->daqContext.getLogger().assigned())
    {
        auto loggerComponent = this->daqContext.getLogger().getOrAddComponent("OpcUaClientProcudure");
        LOG_W("Failed to call procedure on OpcUA client. Error: \"{}\"", lastProccessDescription);
    }
    return OPENDAQ_SUCCESS;
}

ErrCode TmsClientProcedureImpl::getCoreType(CoreType* coreType)
{
    OPENDAQ_PARAM_NOT_NULL(coreType);

    *coreType = ctProc;
    return OPENDAQ_SUCCESS;
}

END_NAMESPACE_OPENDAQ_OPCUA_TMS
