#include <opendaq/login_parameters_impl.h>
#include <opendaq/login_parameters_builder_ptr.h>
#include <opendaq/login_parameters_factory.h>
#include <opendaq/authentication_method_internal_ptr.h>
#include <coretypes/dictobject_factory.h>

BEGIN_NAMESPACE_OPENDAQ

LoginParametersImpl::LoginParametersImpl(ILoginParametersBuilder* loginParametersBuilder)
{
    const auto builderPtr = LoginParametersBuilderPtr::Borrow(loginParametersBuilder);
    authenticationMethod = builderPtr.getAuthenticationMethod();
    credentials = builderPtr.getCredentials();

    if (!authenticationMethod.assigned())
    {
        DAQ_THROW_EXCEPTION(InvalidParameterException, "Authentication method must be assigned when creating login parameters");
    }

    if (!credentials.assigned())
        credentials = Dict<IString, IString>();

    if (authenticationMethod.getFields().getCount() > 0 &&
        !authenticationMethod.asPtr<IAuthenticationMethodInternal>().acceptsCredentials(credentials))
    {
        DAQ_THROW_EXCEPTION(InvalidParameterException, "Credentials do not satisfy the authentication method's required fields");
    }
}

ErrCode LoginParametersImpl::getAuthenticationMethod(IAuthenticationMethod** method)
{
    OPENDAQ_PARAM_NOT_NULL(method);

    *method = this->authenticationMethod.addRefAndReturn();
    return OPENDAQ_SUCCESS;
}

ErrCode LoginParametersImpl::getCredentials(IDict** credentials)
{
    OPENDAQ_PARAM_NOT_NULL(credentials);

    *credentials = this->credentials.addRefAndReturn();
    return OPENDAQ_SUCCESS;
}

OPENDAQ_DEFINE_CLASS_FACTORY_WITH_INTERFACE_AND_CREATEFUNC(
    LIBRARY_FACTORY, LoginParameters,
    ILoginParameters, createLoginParametersFromBuilder,
    ILoginParametersBuilder*, builder
)

END_NAMESPACE_OPENDAQ
