#include <opendaq/credential_request_impl.h>
#include <opendaq/credential_request_builder_ptr.h>
#include <opendaq/credential_request_factory.h>

BEGIN_NAMESPACE_OPENDAQ

CredentialRequestImpl::CredentialRequestImpl(ICredentialRequestBuilder* credentialRequestBuilder)
{
    const auto builderPtr = CredentialRequestBuilderPtr::Borrow(credentialRequestBuilder);
    componentType = builderPtr.getComponentType();
    connectionString = builderPtr.getConnectionString();
    displayName = builderPtr.getDisplayName();
    manufacturer = builderPtr.getManufacturer();
    serialNumber = builderPtr.getSerialNumber();
    model = builderPtr.getModel();
    authenticationMethod = builderPtr.getAuthenticationMethod();

    if (!componentType.assigned())
        DAQ_THROW_EXCEPTION(InvalidParameterException, "Component type must be assigned when creating a credential request");

    if (!connectionString.assigned() || connectionString.getLength() == 0)
        DAQ_THROW_EXCEPTION(InvalidParameterException, "Connection string must be assigned when creating a credential request");

    if (!authenticationMethod.assigned())
        DAQ_THROW_EXCEPTION(InvalidParameterException, "Authentication method must be assigned when creating a credential request");
}

ErrCode CredentialRequestImpl::getComponentType(IComponentType** componentType)
{
    OPENDAQ_PARAM_NOT_NULL(componentType);

    *componentType = this->componentType.addRefAndReturn();
    return OPENDAQ_SUCCESS;
}

ErrCode CredentialRequestImpl::getConnectionString(IString** connectionString)
{
    OPENDAQ_PARAM_NOT_NULL(connectionString);

    *connectionString = this->connectionString.addRefAndReturn();
    return OPENDAQ_SUCCESS;
}

ErrCode CredentialRequestImpl::getDisplayName(IString** displayName)
{
    OPENDAQ_PARAM_NOT_NULL(displayName);

    *displayName = this->displayName.addRefAndReturn();
    return OPENDAQ_SUCCESS;
}

ErrCode CredentialRequestImpl::getManufacturer(IString** manufacturer)
{
    OPENDAQ_PARAM_NOT_NULL(manufacturer);

    *manufacturer = this->manufacturer.addRefAndReturn();
    return OPENDAQ_SUCCESS;
}

ErrCode CredentialRequestImpl::getSerialNumber(IString** serialNumber)
{
    OPENDAQ_PARAM_NOT_NULL(serialNumber);

    *serialNumber = this->serialNumber.addRefAndReturn();
    return OPENDAQ_SUCCESS;
}

ErrCode CredentialRequestImpl::getModel(IString** model)
{
    OPENDAQ_PARAM_NOT_NULL(model);

    *model = this->model.addRefAndReturn();
    return OPENDAQ_SUCCESS;
}

ErrCode CredentialRequestImpl::getAuthenticationMethod(IAuthenticationMethod** authenticationMethod)
{
    OPENDAQ_PARAM_NOT_NULL(authenticationMethod);

    *authenticationMethod = this->authenticationMethod.addRefAndReturn();
    return OPENDAQ_SUCCESS;
}

OPENDAQ_DEFINE_CLASS_FACTORY_WITH_INTERFACE_AND_CREATEFUNC(
    LIBRARY_FACTORY, CredentialRequest,
    ICredentialRequest, createCredentialRequestFromBuilder,
    ICredentialRequestBuilder*, builder
)

END_NAMESPACE_OPENDAQ
