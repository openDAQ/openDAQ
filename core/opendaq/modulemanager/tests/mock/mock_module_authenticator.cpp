#include "mock_module_authenticator.h"


BEGIN_NAMESPACE_OPENDAQ

MockModuleAuthenticatorImpl::MockModuleAuthenticatorImpl(IString* certPath)
    : certificatePath(certPath)
{

}

Bool MockModuleAuthenticatorImpl::onAuthenticateModuleBinary(IString* binaryPath)
{
    if (certificatePath == "mock/path")
    {
        return true;
    }
    else
    {
        return false;
    }
}

DictPtr<IString,IString> MockModuleAuthenticatorImpl::onGetAuthenticatedModules()
{
    return nullptr;
}

OPENDAQ_DEFINE_CLASS_FACTORY_WITH_INTERFACE(INTERNAL_FACTORY, MockModuleAuthenticator, IModuleAuthenticator, IString*, certPath)

END_NAMESPACE_OPENDAQ
