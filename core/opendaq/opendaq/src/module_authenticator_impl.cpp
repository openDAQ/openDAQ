#include <opendaq/module_authenticator_impl.h>

BEGIN_NAMESPACE_OPENDAQ

FunctionModuleAuthenticatorImpl::FunctionModuleAuthenticatorImpl(const FunctionPtr& f)
    : f(f)
{
}

Bool FunctionModuleAuthenticatorImpl::onAuthenticateModuleBinary(StringPtr& vendorKey, const StringPtr& binaryPath)
{
    auto params = Dict<IString, IBaseObject>();
    params["BinaryPath"] = binaryPath;
    constexpr auto methodName = "authenticateModuleBinary";
    DictPtr<IString, IBaseObject> ret = f(methodName, params);

    const auto getKey = [&](const StringPtr&& key)
    {
        try
        {
            return ret.get(key);
        }
        catch (const NotFoundException&)
        {
            DAQ_THROW_EXCEPTION(NotFoundException, "{} function result is missing the \"{}\" key", methodName, key);
        }
    };

    const StringPtr key = getKey("VendorKey");
    vendorKey = key.assigned() ? key : String("");
    return BoolPtr(getKey("BinaryValid"));
}

OPENDAQ_DEFINE_CLASS_FACTORY_WITH_INTERFACE(LIBRARY_FACTORY, FunctionModuleAuthenticator, IModuleAuthenticator, IFunction*, function)

END_NAMESPACE_OPENDAQ
