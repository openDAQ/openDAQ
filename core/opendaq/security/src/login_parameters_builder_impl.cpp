#include <opendaq/login_parameters_builder_impl.h>
#include <opendaq/login_parameters_factory.h>

BEGIN_NAMESPACE_OPENDAQ

LoginParametersBuilderImpl::LoginParametersBuilderImpl()
{
}

ErrCode LoginParametersBuilderImpl::build(ILoginParameters** parameters)
{
    OPENDAQ_PARAM_NOT_NULL(parameters);

    const auto builderPtr = this->borrowPtr<LoginParametersBuilderPtr>();

    return daqTry(
        [&]()
        {
            *parameters = LoginParametersFromBuilder(builderPtr).detach();
            return OPENDAQ_SUCCESS;
        });
}

ErrCode LoginParametersBuilderImpl::setCredentials(IDict* credentials)
{
    this->credentials = credentials;
    return OPENDAQ_SUCCESS;
}

ErrCode LoginParametersBuilderImpl::getCredentials(IDict** credentials)
{
    OPENDAQ_PARAM_NOT_NULL(credentials);

    *credentials = this->credentials.addRefAndReturn();
    return OPENDAQ_SUCCESS;
}

ErrCode LoginParametersBuilderImpl::setAuthenticationMethod(IAuthenticationMethod* method)
{
    this->authenticationMethod = method;
    return OPENDAQ_SUCCESS;
}

ErrCode LoginParametersBuilderImpl::getAuthenticationMethod(IAuthenticationMethod** method)
{
    OPENDAQ_PARAM_NOT_NULL(method);

    *method = this->authenticationMethod.addRefAndReturn();
    return OPENDAQ_SUCCESS;
}

OPENDAQ_DEFINE_CLASS_FACTORY_WITH_INTERFACE(LIBRARY_FACTORY, LoginParametersBuilder, ILoginParametersBuilder)

END_NAMESPACE_OPENDAQ
