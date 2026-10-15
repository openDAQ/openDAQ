#include <opendaq/credential_field_impl.h>
#include <opendaq/credential_field_factory.h>
#include <coretypes/serialized_object_ptr.h>
#include <coretypes/serializer_ptr.h>
#include <coretypes/function_ptr.h>
#include <coretypes/ctutils.h>

BEGIN_NAMESPACE_OPENDAQ

namespace detail
{
    inline ConstCharPtr CredentialFieldKindToString(CredentialFieldKind kind)
    {
        switch (kind)
        {
            case CredentialFieldKind::Secret:
                return "Secret";
            case CredentialFieldKind::FilePath:
                return "FilePath";
            case CredentialFieldKind::Text:
            default:
                return "Text";
        }
    }

    inline CredentialFieldKind CredentialFieldKindFromString(const StringPtr& kind)
    {
        if (kind == "Secret")
            return CredentialFieldKind::Secret;
        if (kind == "FilePath")
            return CredentialFieldKind::FilePath;
        if (kind == "Text")
            return CredentialFieldKind::Text;

        DAQ_THROW_EXCEPTION(InvalidParameterException, "Unknown credential field kind \"{}\"", kind);
    }
}

CredentialFieldImpl::CredentialFieldImpl(IString* id, CredentialFieldKind kind, IString* name, IDict* metadata, Bool required)
    : id(id)
    , kind(kind)
    , name(name)
    , metadata(metadata)
    , required(required)
{
}

ErrCode CredentialFieldImpl::getId(IString** idOut)
{
    OPENDAQ_PARAM_NOT_NULL(idOut);

    *idOut = id.addRefAndReturn();
    return OPENDAQ_SUCCESS;
}

ErrCode CredentialFieldImpl::getKind(CredentialFieldKind* kindOut)
{
    OPENDAQ_PARAM_NOT_NULL(kindOut);

    *kindOut = kind;
    return OPENDAQ_SUCCESS;
}

ErrCode CredentialFieldImpl::getName(IString** nameOut)
{
    OPENDAQ_PARAM_NOT_NULL(nameOut);

    *nameOut = name.addRefAndReturn();
    return OPENDAQ_SUCCESS;
}

ErrCode CredentialFieldImpl::getMetadata(IDict** metadataOut)
{
    OPENDAQ_PARAM_NOT_NULL(metadataOut);

    *metadataOut = metadata.addRefAndReturn();
    return OPENDAQ_SUCCESS;
}

ErrCode CredentialFieldImpl::isRequired(Bool* requiredOut)
{
    OPENDAQ_PARAM_NOT_NULL(requiredOut);

    *requiredOut = required;
    return OPENDAQ_SUCCESS;
}

ErrCode CredentialFieldImpl::serialize(ISerializer* serializer)
{
    return daqTry([&]
    {
        serializer->startTaggedObject(this);

        serializer->key("id");
        serializer->writeString(id.getCharPtr(), id.getLength());

        serializer->key("kind");
        const StringPtr kindStr = detail::CredentialFieldKindToString(kind);
        serializer->writeString(kindStr.getCharPtr(), kindStr.getLength());

        serializer->key("name");
        serializer->writeString(name.getCharPtr(), name.getLength());

        serializer->key("metadata");
        metadata.serialize(SerializerPtr::Borrow(serializer));

        serializer->key("required");
        serializer->writeBool(required);

        serializer->endObject();
        return OPENDAQ_SUCCESS;
    });
}

ErrCode CredentialFieldImpl::getSerializeId(ConstCharPtr* id) const
{
    *id = SerializeId();
    return OPENDAQ_SUCCESS;
}

ConstCharPtr CredentialFieldImpl::SerializeId()
{
    return "CredentialField";
}

ErrCode CredentialFieldImpl::Deserialize(ISerializedObject* serialized, IBaseObject* context, IFunction* factoryCallback, IBaseObject** obj)
{
    OPENDAQ_PARAM_NOT_NULL(obj);

    return daqTry([&]
    {
        const auto serializedObj = SerializedObjectPtr::Borrow(serialized);
        const StringPtr id = serializedObj.readString("id");
        const CredentialFieldKind kind = detail::CredentialFieldKindFromString(serializedObj.readString("kind"));
        const StringPtr name = serializedObj.readString("name");
        const DictPtr<IString, IString> metadata = serializedObj.readObject("metadata", BaseObjectPtr::Borrow(context), FunctionPtr::Borrow(factoryCallback)).asPtr<IDict>();
        const Bool required = serializedObj.readBool("required");

        *obj = CredentialField(id, kind, name, metadata, required).detach();
        return OPENDAQ_SUCCESS;
    });
}

OPENDAQ_DEFINE_CLASS_FACTORY_WITH_INTERFACE(LIBRARY_FACTORY, CredentialField, ICredentialField,
                                             IString*, id, CredentialFieldKind, kind, IString*, name, IDict*, metadata, Bool, required)

OPENDAQ_REGISTER_DESERIALIZE_FACTORY(CredentialFieldImpl)

END_NAMESPACE_OPENDAQ
