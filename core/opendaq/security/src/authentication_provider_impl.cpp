#include <opendaq/authentication_provider_impl.h>

BEGIN_NAMESPACE_OPENDAQ

AuthenticationProviderImpl::AuthenticationProviderImpl()
{
}

ErrCode INTERFACE_FUNC AuthenticationProviderImpl::authenticate(IString* usernanme, IString* password, IUser** user)
{
    return OPENDAQ_ERR_NOTIMPLEMENTED;
}

OPENDAQ_DEFINE_CLASS_FACTORY(LIBRARY_FACTORY, AuthenticationProvider)

END_NAMESPACE_OPENDAQ
