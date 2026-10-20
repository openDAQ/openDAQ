#include <opendaq/authentication_config_impl.h>
#include <coreobjects/property_factory.h>
#include <coretypes/listobject_factory.h>
#include <coretypes/dictobject_factory.h>
#include <coretypes/stringobject_factory.h>
#include <coretypes/serialized_object_ptr.h>
#include <coretypes/serializer_ptr.h>
#include <coretypes/function_ptr.h>
#include <coretypes/ctutils.h>
#include <coretypes/formatter.h>
#include <fmt/format.h>
#include <opendaq/authentication_config_ptr.h>
#include <opendaq/authentication_method_internal_ptr.h>

BEGIN_NAMESPACE_OPENDAQ

AuthenticationConfigImpl::AuthenticationConfigImpl(const DictPtr<IString, IAuthenticationMethod>& authenticationMethods)
    : Super()
{
    initProperties(authenticationMethods);
}

void AuthenticationConfigImpl::initProperties(const DictPtr<IString, IAuthenticationMethod>& authenticationMethods)
{
    if (!authenticationMethods.assigned() || authenticationMethods.getCount() == 0)
        DAQ_THROW_EXCEPTION(InvalidParameterException, "At least one authentication method must be supplied when creating an authentication config");

    ListPtr<IString> authenticationMethodIds = List<IString>();
    for (const auto& [id, authenticationMethod] : authenticationMethods)
        authenticationMethodIds.pushBack(id);

    Super::addProperty(SelectionProperty(AuthenticationMethodPropertyName, authenticationMethodIds, 0));
    Super::addProperty(DictProperty(SuppliedCredentialPropertyName, Dict<IString, IString>()));
    this->supportedAuthenticationMethods = authenticationMethods;
}

AuthenticationMethodPtr AuthenticationConfigImpl::onGetSelectedAuthenticationMethod() const
{
    const StringPtr selectedId = objPtr.getPropertySelectionValue(AuthenticationMethodPropertyName);
    return supportedAuthenticationMethods.get(selectedId);
}

ErrCode AuthenticationConfigImpl::getSelectedAuthenticationMethod(IAuthenticationMethod** authenticationMethod)
{
    OPENDAQ_PARAM_NOT_NULL(authenticationMethod);

    return daqTry([&]
    {
        *authenticationMethod = onGetSelectedAuthenticationMethod().addRefAndReturn();
        return OPENDAQ_SUCCESS;
    });
}

ErrCode AuthenticationConfigImpl::setAuthenticationMethod(IString* authenticationMethodId)
{
    OPENDAQ_PARAM_NOT_NULL(authenticationMethodId);

    const StringPtr idPtr = StringPtr::Borrow(authenticationMethodId);

    if (!supportedAuthenticationMethods.hasKey(idPtr))
    {
        return DAQ_MAKE_ERROR_INFO(OPENDAQ_ERR_NOTFOUND,
                                   fmt::format("\"{}\" is not one of this config's supported authentication methods", idPtr));
    }
    return this->setPropertySelectionValue(String(AuthenticationMethodPropertyName), authenticationMethodId);
}

ErrCode AuthenticationConfigImpl::getAuthenticationMethods(IDict** authenticationMethods)
{
    OPENDAQ_PARAM_NOT_NULL(authenticationMethods);

    *authenticationMethods = this->supportedAuthenticationMethods.addRefAndReturn();
    return OPENDAQ_SUCCESS;
}

ErrCode AuthenticationConfigImpl::setSuppliedCredential(IDict* credential)
{
    OPENDAQ_PARAM_NOT_NULL(credential);

    return setPropertyValue(String(SuppliedCredentialPropertyName), credential);
}

ErrCode AuthenticationConfigImpl::getSuppliedCredential(IDict** credential)
{
    OPENDAQ_PARAM_NOT_NULL(credential);

    return daqTry([&]
    {
        DictPtr<IString, IString> value = objPtr.getPropertyValue(SuppliedCredentialPropertyName);
        *credential = value.detach();
        return OPENDAQ_SUCCESS;
    });
}

ErrCode AuthenticationConfigImpl::getInterfaceIds(SizeT* idCount, IntfID** ids)
{
    OPENDAQ_PARAM_NOT_NULL(idCount);

    *idCount = InterfaceIds::Count() + 1;
    if (ids == nullptr)
        return OPENDAQ_SUCCESS;

    **ids = IPropertyObject::Id;
    (*ids)++;

    InterfaceIds::AddInterfaceIds(*ids);
    return OPENDAQ_SUCCESS;
}

ErrCode AuthenticationConfigImpl::setPropertySelectionValue(IString* propertyName, IBaseObject* value)
{
    const ErrCode errCode = Super::setPropertySelectionValue(propertyName, value);
    OPENDAQ_RETURN_IF_FAILED(errCode);

    return onPropertyValueChanged(StringPtr::Borrow(propertyName));
}

ErrCode AuthenticationConfigImpl::setProtectedPropertyValue(IString* propertyName, IBaseObject* value)
{
    const ErrCode errCode = Super::setProtectedPropertyValue(propertyName, value);
    OPENDAQ_RETURN_IF_FAILED(errCode);

    return onPropertyValueChanged(StringPtr::Borrow(propertyName));
}

ErrCode AuthenticationConfigImpl::onPropertyValueChanged(const StringPtr& name)
{
    if (name != AuthenticationMethodPropertyName)
        return OPENDAQ_SUCCESS;

    return Super::setProtectedPropertyValue(String(SuppliedCredentialPropertyName), Dict<IString, IString>());
}

ErrCode AuthenticationConfigImpl::setPropertyValue(IString* propertyName, IBaseObject* value)
{
    const StringPtr name = StringPtr::Borrow(propertyName);

    if (name == SuppliedCredentialPropertyName)
    {
        return daqTry([&]
        {
            const BaseObjectPtr valuePtr = BaseObjectPtr::Borrow(value);
            const DictPtr<IString, IString> credential = valuePtr.asPtr<IDict>();
            if (credential.getCount() > 0)
            {
                const AuthenticationMethodPtr selected = onGetSelectedAuthenticationMethod();
                if (!selected.asPtr<IAuthenticationMethodInternal>().acceptsCredentials(credential))
                {
                    DAQ_THROW_EXCEPTION(InvalidParameterException,
                                        "Supplied credential is missing one or more fields required by the currently selected "
                                        "authentication method \"{}\"",
                                        selected.getId());
                }
            }

            return Super::setPropertyValue(propertyName, value);
        });
    }

    return Super::setPropertyValue(propertyName, value);
}

// Serialization is entirely custom - replaces the inherited generic `IPropertyObject` one, mirroring
// `DeviceTypeImpl`'s own pattern. Writes only `supportedAuthenticationMethods` (the real `IAuthenticationMethod`
// objects this config was built from) and the currently selected id - `"SuppliedCredential"` is never written.
ErrCode AuthenticationConfigImpl::serialize(ISerializer* serializer)
{
    return daqTry([&]
    {
        serializer->startTaggedObject(this);

        serializer->key(SupportedAuthenticationMethodsSerializedKey);
        supportedAuthenticationMethods.serialize(SerializerPtr::Borrow(serializer));

        const StringPtr selectedId = objPtr.getPropertySelectionValue(AuthenticationMethodPropertyName);
        serializer->key("selectedAuthenticationMethodId");
        serializer->writeString(selectedId.getCharPtr(), selectedId.getLength());

        serializer->endObject();
        return OPENDAQ_SUCCESS;
    });
}

ErrCode AuthenticationConfigImpl::getSerializeId(ConstCharPtr* id) const
{
    *id = SerializeId();
    return OPENDAQ_SUCCESS;
}

ConstCharPtr AuthenticationConfigImpl::SerializeId()
{
    return "AuthenticationConfig";
}

// Rebuilds the config directly from its two serialized pieces, via the real constructor, then restores the
// saved selection - fully self-contained, no module/type registry re-consultation needed. Throws if the saved
// selected id is no longer among the (also saved) supported methods - consistent with "no compatible method"
// already being a hard failure elsewhere in this framework.
ErrCode AuthenticationConfigImpl::Deserialize(ISerializedObject* serialized, IBaseObject* context, IFunction* factoryCallback, IBaseObject** obj)
{
    OPENDAQ_PARAM_NOT_NULL(obj);

    return daqTry([&]
    {
        const auto serializedObj = SerializedObjectPtr::Borrow(serialized);

        const DictPtr<IString, IAuthenticationMethod> supportedAuthenticationMethods =
            serializedObj.readObject(SupportedAuthenticationMethodsSerializedKey, BaseObjectPtr::Borrow(context), FunctionPtr::Borrow(factoryCallback))
                .asPtr<IDict>();
        const StringPtr selectedId = serializedObj.readString("selectedAuthenticationMethodId");

        AuthenticationConfigPtr authConfig = createWithImplementation<IAuthenticationConfig, AuthenticationConfigImpl>(supportedAuthenticationMethods);
        authConfig.setAuthenticationMethod(selectedId);

        *obj = authConfig.detach();
        return OPENDAQ_SUCCESS;
    });
}

OPENDAQ_DEFINE_CLASS_FACTORY_WITH_INTERFACE(
    LIBRARY_FACTORY, AuthenticationConfig, IAuthenticationConfig,
    IDict*, authenticationMethods
)

OPENDAQ_REGISTER_DESERIALIZE_FACTORY(AuthenticationConfigImpl)

END_NAMESPACE_OPENDAQ
