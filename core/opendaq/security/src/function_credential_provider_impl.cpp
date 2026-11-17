#include <opendaq/function_credential_provider_impl.h>
#include <coretypes/dictobject_factory.h>

BEGIN_NAMESPACE_OPENDAQ

FunctionCredentialProviderImpl::FunctionCredentialProviderImpl(const FunctionPtr& f)
    : f(f)
{
    if (!f.assigned())
    {
        DAQ_THROW_EXCEPTION(ArgumentNullException, "Credential provider function must be assigned");
    }
}

ErrCode FunctionCredentialProviderImpl::getDescription(IString** description)
{
    OPENDAQ_PARAM_NOT_NULL(description);

    return daqTry(
        [&]
        {
            StringPtr ret = callFunction("getDescription", Dict<IString, IBaseObject>(), "Description");
            if (!ret.assigned())
                ret = String("");
            *description = ret.detach();
        });
}

ErrCode FunctionCredentialProviderImpl::requestCredentials(ICredentialRequest* request, IDict** credentials)
{
    OPENDAQ_PARAM_NOT_NULL(request);
    OPENDAQ_PARAM_NOT_NULL(credentials);

    return daqTry(
        [&]
        {
            auto params = Dict<IString, IBaseObject>();
            params["Request"] = request;
            DictPtr<IString, IString> ret = callFunction("requestCredentials", params, "Credentials");
            *credentials = ret.detach();
        });
}

BaseObjectPtr FunctionCredentialProviderImpl::callFunction(const char* methodName,
                                                           const DictPtr<IString, IBaseObject>& params,
                                                           const char* resultKey)
{
    const DictPtr<IString, IBaseObject> ret = f(methodName, params);
    try
    {
        return ret.get(resultKey);
    }
    catch (const NotFoundException&)
    {
        DAQ_THROW_EXCEPTION(NotFoundException, "{} function result is missing the \"{}\" key", methodName, resultKey);
    }
}

OPENDAQ_DEFINE_CLASS_FACTORY_WITH_INTERFACE(LIBRARY_FACTORY, FunctionCredentialProvider, ICredentialProvider, IFunction*, function)

END_NAMESPACE_OPENDAQ
