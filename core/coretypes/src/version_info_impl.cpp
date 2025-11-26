#include <coretypes/impl.h>
#include <coretypes/struct_impl.h>
#include <coretypes/struct_type_impl.h>
#include <coretypes/version_info_factory.h>
#include <coretypes/version_info_impl.h>

BEGIN_NAMESPACE_OPENDAQ

template <typename StructInterfaces, typename... Interfaces>
VersionInfoBase<StructInterfaces, Interfaces...>::VersionInfoBase(SizeT major, SizeT minor, SizeT patch)
    : GenericStructImpl<StructInterfaces, IStruct, Interfaces...>(
        VersionInfoStructType(),
        Dict<IString, IBaseObject>({
            {"Major", major},
            {"Minor", minor},
            {"Patch", patch}
        })
      )
{
}

template <typename StructInterfaces, typename... Interfaces>
VersionInfoBase<StructInterfaces, Interfaces...>::VersionInfoBase(StructTypePtr type, DictPtr<IString, IBaseObject> fields)
    : GenericStructImpl<StructInterfaces, IStruct, Interfaces...>(type, fields)
{
}

template <typename StructInterfaces, typename... Interfaces>
ErrCode VersionInfoBase<StructInterfaces, Interfaces...>::getMajor(SizeT* major)
{
    OPENDAQ_PARAM_NOT_NULL(major);

    *major = this->fields.get("Major");
    return OPENDAQ_SUCCESS;
}

template <typename StructInterfaces, typename... Interfaces>
ErrCode VersionInfoBase<StructInterfaces, Interfaces...>::getMinor(SizeT* minor)
{
    OPENDAQ_PARAM_NOT_NULL(minor);

    *minor = this->fields.get("Minor");
    return OPENDAQ_SUCCESS;
}

template <typename StructInterfaces, typename... Interfaces>
ErrCode VersionInfoBase<StructInterfaces, Interfaces...>::getPatch(SizeT* patch)
{
    OPENDAQ_PARAM_NOT_NULL(patch);

    *patch = this->fields.get("Patch");
    return OPENDAQ_SUCCESS;
}

template <typename StructInterfaces, typename ... Interfaces>
void VersionInfoBase<StructInterfaces, Interfaces...>::serializeVersionInfo(ISerializer* serializer)
{
    Int major = this->fields.get("Major");
    if (major != -1)
    {
        serializer->key("major");
        serializer->writeInt(major);
    }

    Int minor = this->fields.get("Minor");
    if (minor != -1)
    {
        serializer->key("minor");
        serializer->writeInt(minor);
    }

    Int patch = this->fields.get("Patch");
    if (patch != -1)
    {
        serializer->key("patch");
        serializer->writeInt(patch);
    }
}

template <typename StructInterfaces, typename... Interfaces>
ErrCode INTERFACE_FUNC VersionInfoBase<StructInterfaces, Interfaces...>::serialize(ISerializer* serializer)
{
    serializer->startTaggedObject(this);
    {
        serializeVersionInfo(serializer);
    }

    serializer->endObject();
    return OPENDAQ_SUCCESS;
}

template <typename StructInterfaces, typename... Interfaces>
ErrCode INTERFACE_FUNC VersionInfoBase<StructInterfaces, Interfaces...>::getSerializeId(ConstCharPtr* id) const
{
    *id = SerializeId();

    return OPENDAQ_SUCCESS;
}

template <typename StructInterfaces, typename... Interfaces>
ConstCharPtr VersionInfoBase<StructInterfaces, Interfaces...>::SerializeId()
{
    return "VersionInfo";
}

template <typename StructInterfaces, typename... Interfaces>
ErrCode VersionInfoBase<StructInterfaces, Interfaces...>::Deserialize(ISerializedObject* serialized, IBaseObject*, IFunction*, IBaseObject** obj)
{
    SerializedObjectPtr serializedObj = SerializedObjectPtr::Borrow(serialized);

    Int majorVer = -1;
    ErrCode err = serializedObj->readInt(String("major"), &majorVer);
    OPENDAQ_RETURN_IF_FAILED_EXCEPT(err, OPENDAQ_ERR_NOTFOUND);

    Int minorVer = -1;
    err = serializedObj->readInt(String("minor"), &minorVer);
    OPENDAQ_RETURN_IF_FAILED_EXCEPT(err, OPENDAQ_ERR_NOTFOUND);

    Int patchVer = -1;
    err = serializedObj->readInt(String("patch"), &patchVer);
    OPENDAQ_RETURN_IF_FAILED_EXCEPT(err, OPENDAQ_ERR_NOTFOUND);

    return createObject<IVersionInfo, VersionInfoImpl, SizeT, SizeT, SizeT>(
        reinterpret_cast<IVersionInfo**>(obj),
        static_cast<SizeT>(majorVer),
        static_cast<SizeT>(minorVer),
        static_cast<SizeT>(patchVer)
    );
}

OPENDAQ_DEFINE_CLASS_FACTORY(LIBRARY_FACTORY, VersionInfo, SizeT, major, SizeT, minor, SizeT, patch)

///////////////////////////////////
/// Development VersionInfo
///////////////////////////////////

DevelopmentVersionInfoImpl::DevelopmentVersionInfoImpl(
    SizeT major,
    SizeT minor,
    SizeT patch,
    SizeT tweak,
    StringPtr branch,
    StringPtr hash
) try
    : VersionInfoBase(
        DevelopmentVersionInfoStructType(),
        Dict<IString, IBaseObject>({
            {"Major", major},
            {"Minor", minor},
            {"Patch", patch},
            {"Tweak", tweak},
            {"Branch", branch},
            {"Hash", hash}
        })
    )
{
}
catch (const std::exception& e)
{
    throw e;
}

ErrCode DevelopmentVersionInfoImpl::getTweak(SizeT* tweak)
{
    OPENDAQ_PARAM_NOT_NULL(tweak);

    *tweak = this->fields.get("Tweak");
    return OPENDAQ_SUCCESS;
}

ErrCode DevelopmentVersionInfoImpl::getBranchName(IString** branchName)
{
    OPENDAQ_PARAM_NOT_NULL(branchName);

    *branchName = this->fields.get("Branch").asOrNull<IString>();
    return OPENDAQ_SUCCESS;
}

ErrCode DevelopmentVersionInfoImpl::getHashDigest(IString** hash)
{
    OPENDAQ_PARAM_NOT_NULL(hash);

    *hash = this->fields.get("Hash").asOrNull<IString>();
    return OPENDAQ_SUCCESS;
}

ErrCode DevelopmentVersionInfoImpl::getSerializeId(ConstCharPtr* id) const
{
    *id = SerializeId();

    return OPENDAQ_SUCCESS;
}

ConstCharPtr DevelopmentVersionInfoImpl::SerializeId()
{
    return "DevelopmentVersionInfo";
}

ErrCode INTERFACE_FUNC DevelopmentVersionInfoImpl::serialize(ISerializer* serializer)
{
    serializer->startTaggedObject(this);
    {
        serializeVersionInfo(serializer);

        Int tweak = this->fields.get("Tweak");
        if (tweak != -1)
        {
            serializer->key("tweak");
            serializer->writeInt(tweak);
        }

        StringPtr branchName;
        getBranchName(&branchName);

        serializer->key("branch");
        if (branchName.assigned())
        {
            serializer->writeString(branchName.getCharPtr(), branchName.getLength());
        }
        else
        {
            serializer->writeNull();
        }

        StringPtr hashDigest;
        getHashDigest(&hashDigest);

        serializer->key("hash");
        if (branchName.assigned())
        {
            serializer->writeString(hashDigest.getCharPtr(), hashDigest.getLength());
        }
        else
        {
            serializer->writeNull();
        }
    }

    serializer->endObject();
    return OPENDAQ_SUCCESS;
}

ErrCode DevelopmentVersionInfoImpl::Deserialize(ISerializedObject* serialized, IBaseObject*, IFunction*, IBaseObject** obj)
{
    SerializedObjectPtr serializedObj = SerializedObjectPtr::Borrow(serialized);

    Int majorVer = -1;
    ErrCode err = serializedObj->readInt(String("major"), &majorVer);
    OPENDAQ_RETURN_IF_FAILED_EXCEPT(err, OPENDAQ_ERR_NOTFOUND);

    Int minorVer = -1;
    err = serializedObj->readInt(String("minor"), &minorVer);
    OPENDAQ_RETURN_IF_FAILED_EXCEPT(err, OPENDAQ_ERR_NOTFOUND);

    Int patchVer = -1;
    err = serializedObj->readInt(String("patch"), &patchVer);
    OPENDAQ_RETURN_IF_FAILED_EXCEPT(err, OPENDAQ_ERR_NOTFOUND);

    Int tweakVer = -1;
    err = serializedObj->readInt(String("tweak"), &tweakVer);
    OPENDAQ_RETURN_IF_FAILED_EXCEPT(err, OPENDAQ_ERR_NOTFOUND);

    StringPtr branchName;
    err = serializedObj->readString(String("branch"), &branchName);
    OPENDAQ_RETURN_IF_FAILED_EXCEPT(err, OPENDAQ_ERR_NOTFOUND);

    StringPtr hashDigest;
    err = serializedObj->readString(String("hash"), &hashDigest);
    OPENDAQ_RETURN_IF_FAILED_EXCEPT(err, OPENDAQ_ERR_NOTFOUND);


    return createObject<IDevelopmentVersionInfo, DevelopmentVersionInfoImpl, SizeT, SizeT, SizeT>(
        reinterpret_cast<IDevelopmentVersionInfo**>(obj),
        static_cast<SizeT>(majorVer),
        static_cast<SizeT>(minorVer),
        static_cast<SizeT>(patchVer),
        static_cast<SizeT>(tweakVer),
        branchName,
        hashDigest
    );
}


OPENDAQ_DEFINE_CLASS_FACTORY(LIBRARY_FACTORY, DevelopmentVersionInfo, SizeT, major, SizeT, minor, SizeT, patch, SizeT, tweak, IString*, branch, IString*, hash)

END_NAMESPACE_OPENDAQ
