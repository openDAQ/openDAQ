#include <opendaq/authentication_method_impl.h>
#include <opendaq/authentication_method_factory.h>
#include <coretypes/dictobject_factory.h>
#include <coretypes/serialized_object_ptr.h>
#include <coretypes/serializer_ptr.h>
#include <coretypes/function_ptr.h>
#include <coretypes/ctutils.h>

BEGIN_NAMESPACE_OPENDAQ

AuthenticationMethodImpl::AuthenticationMethodImpl(const StringPtr& id, const DictPtr<IString, ICredentialField>& fields, const StringPtr& description)
    : id(id)
    , fields(fields)
    , description(description)
{
}

ErrCode AuthenticationMethodImpl::getId(IString** id)
{
    OPENDAQ_PARAM_NOT_NULL(id);

    *id = this->id.addRefAndReturn();
    return OPENDAQ_SUCCESS;
}

ErrCode AuthenticationMethodImpl::getFields(IDict** fields)
{
    OPENDAQ_PARAM_NOT_NULL(fields);

    *fields = this->fields.addRefAndReturn();
    return OPENDAQ_SUCCESS;
}

ErrCode AuthenticationMethodImpl::getDescription(IString** description)
{
    OPENDAQ_PARAM_NOT_NULL(description);

    *description = this->description.addRefAndReturn();
    return OPENDAQ_SUCCESS;
}

ErrCode AuthenticationMethodImpl::serialize(ISerializer* serializer)
{
    return daqTry([&]
    {
        serializer->startTaggedObject(this);

        serializer->key("id");
        serializer->writeString(id.getCharPtr(), id.getLength());

        serializer->key("description");
        serializer->writeString(description.getCharPtr(), description.getLength());

        serializer->key("fields");
        fields.serialize(SerializerPtr::Borrow(serializer));

        serializer->endObject();
        return OPENDAQ_SUCCESS;
    });
}

ErrCode AuthenticationMethodImpl::getSerializeId(ConstCharPtr* id) const
{
    *id = SerializeId();
    return OPENDAQ_SUCCESS;
}

ConstCharPtr AuthenticationMethodImpl::SerializeId()
{
    return "AuthenticationMethod";
}

ErrCode AuthenticationMethodImpl::Deserialize(ISerializedObject* serialized, IBaseObject* context, IFunction* factoryCallback, IBaseObject** obj)
{
    OPENDAQ_PARAM_NOT_NULL(obj);

    return daqTry([&]
    {
        const auto serializedObj = SerializedObjectPtr::Borrow(serialized);
        const StringPtr id = serializedObj.readString("id");
        const StringPtr description = serializedObj.readString("description");
        const DictPtr<IString, ICredentialField> fields =
            serializedObj.readObject("fields", BaseObjectPtr::Borrow(context), FunctionPtr::Borrow(factoryCallback)).asPtr<IDict>();

        *obj = AuthenticationMethod(id, fields, description).detach();
        return OPENDAQ_SUCCESS;
    });
}

OPENDAQ_DEFINE_CLASS_FACTORY_WITH_INTERFACE(LIBRARY_FACTORY, AuthenticationMethod, IAuthenticationMethod, IString*, id, IDict*, fields, IString*, description)

OPENDAQ_REGISTER_DESERIALIZE_FACTORY(AuthenticationMethodImpl)

END_NAMESPACE_OPENDAQ
