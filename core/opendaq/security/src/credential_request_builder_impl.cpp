#include <opendaq/credential_request_builder_impl.h>
#include <opendaq/credential_request_factory.h>

BEGIN_NAMESPACE_OPENDAQ
CredentialRequestBuilderImpl::CredentialRequestBuilderImpl()
    : componentType(nullptr)
    , connectionString(nullptr)
{
}

ErrCode CredentialRequestBuilderImpl::build(ICredentialRequest** request)
{
    OPENDAQ_PARAM_NOT_NULL(request);

    const auto builderPtr = this->borrowPtr<CredentialRequestBuilderPtr>();

    return daqTry(
        [&]()
        {
            *request = CredentialRequestFromBuilder(builderPtr).detach();
            return OPENDAQ_SUCCESS;
        });
}

ErrCode CredentialRequestBuilderImpl::setComponentType(IComponentType* componentType)
{
    this->componentType = componentType;
    return OPENDAQ_SUCCESS;
}

ErrCode CredentialRequestBuilderImpl::getComponentType(IComponentType** componentType)
{
    OPENDAQ_PARAM_NOT_NULL(componentType);

    *componentType = this->componentType.addRefAndReturn();
    return OPENDAQ_SUCCESS;
}

ErrCode CredentialRequestBuilderImpl::setConnectionString(IString* connectionString)
{
    this->connectionString = connectionString;
    return OPENDAQ_SUCCESS;
}

ErrCode CredentialRequestBuilderImpl::getConnectionString(IString** connectionString)
{
    OPENDAQ_PARAM_NOT_NULL(connectionString);

    *connectionString = this->connectionString.addRefAndReturn();
    return OPENDAQ_SUCCESS;
}

ErrCode CredentialRequestBuilderImpl::setManufacturer(IString* manufacturer)
{
    this->manufacturer = manufacturer;
    return OPENDAQ_SUCCESS;
}

ErrCode CredentialRequestBuilderImpl::getManufacturer(IString** manufacturer)
{
    OPENDAQ_PARAM_NOT_NULL(manufacturer);

    *manufacturer = this->manufacturer.addRefAndReturn();
    return OPENDAQ_SUCCESS;
}

ErrCode CredentialRequestBuilderImpl::setSerialNumber(IString* serialNumber)
{
    this->serialNumber = serialNumber;
    return OPENDAQ_SUCCESS;
}

ErrCode CredentialRequestBuilderImpl::getSerialNumber(IString** serialNumber)
{
    OPENDAQ_PARAM_NOT_NULL(serialNumber);

    *serialNumber = this->serialNumber.addRefAndReturn();
    return OPENDAQ_SUCCESS;
}

ErrCode CredentialRequestBuilderImpl::setModel(IString* model)
{
    this->model = model;
    return OPENDAQ_SUCCESS;
}

ErrCode CredentialRequestBuilderImpl::getModel(IString** model)
{
    OPENDAQ_PARAM_NOT_NULL(model);

    *model = this->model.addRefAndReturn();
    return OPENDAQ_SUCCESS;
}

ErrCode CredentialRequestBuilderImpl::setDisplayName(IString* displayName)
{
    this->displayName = displayName;
    return OPENDAQ_SUCCESS;
}

ErrCode CredentialRequestBuilderImpl::getDisplayName(IString** displayName)
{
    OPENDAQ_PARAM_NOT_NULL(displayName);

    *displayName = this->displayName.addRefAndReturn();
    return OPENDAQ_SUCCESS;
}

ErrCode CredentialRequestBuilderImpl::setAuthenticationMethod(IAuthenticationMethod* authenticationMethod)
{
    this->authenticationMethod = authenticationMethod;
    return OPENDAQ_SUCCESS;
}

ErrCode CredentialRequestBuilderImpl::getAuthenticationMethod(IAuthenticationMethod** authenticationMethod)
{
    OPENDAQ_PARAM_NOT_NULL(authenticationMethod);

    *authenticationMethod = this->authenticationMethod.addRefAndReturn();
    return OPENDAQ_SUCCESS;
}

OPENDAQ_DEFINE_CLASS_FACTORY_WITH_INTERFACE(LIBRARY_FACTORY, CredentialRequestBuilder, ICredentialRequestBuilder)

END_NAMESPACE_OPENDAQ
