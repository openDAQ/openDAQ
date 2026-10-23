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
    DictPtr<IString, IBaseObject> ret = f("authenticateModuleBinary", params);
    if (const auto errCode = static_cast<ErrCode>(ret.getOrDefault("ErrorCode", OPENDAQ_SUCCESS)); OPENDAQ_FAILED(errCode))
        throwExceptionFromErrorCode(errCode, ret.getOrDefault("ErrorMessage", ""));
    vendorKey = ret.getOrDefault("VendorKey", nullptr);
    return BoolPtr(ret.getOrDefault("BinaryValid", false));
}

OPENDAQ_DEFINE_CLASS_FACTORY_WITH_INTERFACE(LIBRARY_FACTORY, FunctionModuleAuthenticator, IModuleAuthenticator, IFunction*, function)

END_NAMESPACE_OPENDAQ
