#include <coretypes/impl.h>
#include <coretypes/struct_impl.h>
#include <coretypes/struct_type_impl.h>
#include <coretypes/version_info_factory.h>
#include <coretypes/version_info_impl.h>

BEGIN_NAMESPACE_OPENDAQ

VersionInfoImpl::VersionInfoImpl(SizeT major, SizeT minor, SizeT patch)
    : GenericStructImpl<IVersionInfo, IStruct>(VersionInfoStructType(),
                                               Dict<IString, IBaseObject>({{"Major", major}, {"Minor", minor}, {"Patch", patch}}))
{
}

ErrCode VersionInfoImpl::getMajor(SizeT* major)
{
    OPENDAQ_PARAM_NOT_NULL(major);

    *major = this->fields.get("Major");
    return OPENDAQ_SUCCESS;
}

ErrCode VersionInfoImpl::getMinor(SizeT* minor)
{
    OPENDAQ_PARAM_NOT_NULL(minor);

    *minor = this->fields.get("Minor");
    return OPENDAQ_SUCCESS;
}

ErrCode VersionInfoImpl::getPatch(SizeT* patch)
{
    OPENDAQ_PARAM_NOT_NULL(patch);

    *patch = this->fields.get("Patch");
    return OPENDAQ_SUCCESS;
}

ErrCode INTERFACE_FUNC VersionInfoImpl::serialize(ISerializer* serializer)
{
    serializer->startTaggedObject(this);
    {
        Int major = -1;
        major = this->fields.get("Major");
        if (major != -1)
        {
            serializer->key("major");
            serializer->writeInt(major);
        }

        Int minor = -1;
        minor = this->fields.get("Minor");
        if (minor != -1)
        {
            serializer->key("minor");
            serializer->writeInt(minor);
        }

        Int patch = -1;
        patch = this->fields.get("Patch");
        if (patch != -1)
        {
            serializer->key("patch");
            serializer->writeInt(patch);
        }
    }

    serializer->endObject();
    return OPENDAQ_SUCCESS;
}

ErrCode INTERFACE_FUNC VersionInfoImpl::getSerializeId(ConstCharPtr* id) const
{
    *id = SerializeId();

    return OPENDAQ_SUCCESS;
}

ConstCharPtr VersionInfoImpl::SerializeId()
{
    return "VersionInfo";
}

ErrCode VersionInfoImpl::Deserialize(ISerializedObject* serialized, IBaseObject*, IFunction*, IBaseObject** obj)
{
    SerializedObjectPtr serializedObj = SerializedObjectPtr::Borrow(serialized);

    Int major = -1;
    ErrCode err = serializedObj->readInt(String("major"), &major);
    if (err == OPENDAQ_ERR_NOTFOUND)
        daqClearErrorInfo();
    else
        OPENDAQ_RETURN_IF_FAILED(err);

    Int minor = -1;
    err = serializedObj->readInt(String("minor"), &minor);
    if (err == OPENDAQ_ERR_NOTFOUND)
        daqClearErrorInfo();
    else
    OPENDAQ_RETURN_IF_FAILED(err);

    Int patch = -1;
    err = serializedObj->readInt(String("patch"), &patch);
    if (err == OPENDAQ_ERR_NOTFOUND)
        daqClearErrorInfo();
    else
        OPENDAQ_RETURN_IF_FAILED(err);

    return createObject<IVersionInfo, VersionInfoImpl, SizeT, SizeT, SizeT>(reinterpret_cast<IVersionInfo**>(obj), major, minor, patch);
}

OPENDAQ_DEFINE_CLASS_FACTORY(LIBRARY_FACTORY, VersionInfo, SizeT, major, SizeT, minor, SizeT, patch)

END_NAMESPACE_OPENDAQ
