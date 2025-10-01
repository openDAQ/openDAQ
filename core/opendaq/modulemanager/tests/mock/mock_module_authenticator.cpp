#include "mock_module_authenticator.h"


BEGIN_NAMESPACE_OPENDAQ

MockModuleAuthenticatorImpl::MockModuleAuthenticatorImpl(const StringPtr& certPath)
    : certificatePath(certPath)
{

}

Bool MockModuleAuthenticatorImpl::onAuthenticateModuleBinary(StringPtr& vendorKey, const StringPtr& binaryPath)
{
    StringPtr key("mockKey");
    vendorKey = key.detach();

    if (certificatePath == "mock/path")
    {
        return true;
    }
    else
    {
        return false;
    }
}

OPENDAQ_DEFINE_CLASS_FACTORY_WITH_INTERFACE(INTERNAL_FACTORY, MockModuleAuthenticator, IModuleAuthenticator, IString*, certPath)

END_NAMESPACE_OPENDAQ
