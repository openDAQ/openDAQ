#include <opendaq/user_impl.h>
#include <coretypes/impl.h>
#include <coretypes/validation.h>
#include <coretypes/serialized_object_ptr.h>
#include <opendaq/user_factory.h>
#include <opendaq/user_internal_ptr.h>
#include <set>

BEGIN_NAMESPACE_OPENDAQ

UserImpl::UserImpl(const StringPtr& username, const StringPtr& passwordHash, const ListPtr<IString> groups)
    : username(username)
    , passwordHash(passwordHash)
{
    this->groups = sanitizeGroupList(groups);
}

ErrCode INTERFACE_FUNC UserImpl::getUsername(IString** username)
{
    OPENDAQ_PARAM_NOT_NULL(username);

    *username = this->username.addRefAndReturn();
    return OPENDAQ_SUCCESS;
}

ErrCode INTERFACE_FUNC UserImpl::getPasswordHash(IString** passwordHash)
{
    OPENDAQ_PARAM_NOT_NULL(passwordHash);

    *passwordHash = this->passwordHash.addRefAndReturn();
    return OPENDAQ_SUCCESS;
}

ErrCode INTERFACE_FUNC UserImpl::getGroups(IList** groups)
{
    OPENDAQ_PARAM_NOT_NULL(groups);

    *groups = this->groups.addRefAndReturn();
    return OPENDAQ_SUCCESS;
}

ErrCode INTERFACE_FUNC UserImpl::equals(IBaseObject* other, Bool* equal) const
{
    OPENDAQ_PARAM_NOT_NULL(equal);

    *equal = false;
    if (!other)
        return OPENDAQ_SUCCESS;

    UserPtr userOther = BaseObjectPtr::Borrow(other).asPtrOrNull<IUser>();
    if (userOther == nullptr)
        return OPENDAQ_SUCCESS;

    if (username != userOther.getUsername())
        return OPENDAQ_SUCCESS;

    if (passwordHash != userOther.asPtr<IUserInternal>(true).getPasswordHash())
        return OPENDAQ_SUCCESS;

    if (!BaseObjectPtr::Equals(groups, userOther.getGroups()))
        return OPENDAQ_SUCCESS;

    *equal = true;
    return OPENDAQ_SUCCESS;
}

ErrCode INTERFACE_FUNC UserImpl::serialize(ISerializer* serializer)
{
    serializer->startTaggedObject(this);

    if (username.assigned())
    {
        serializer->key("username");
        serializer->writeString(username.getCharPtr(), username.getLength());
    }

    if (passwordHash.assigned())
    {
        serializer->key("passwordHash");
        serializer->writeString(passwordHash.getCharPtr(), passwordHash.getLength());
    }

    if (groups.assigned())
    {
        serializer->key("groups");
        groups.serialize(serializer);
    }

    serializer->endObject();
    return OPENDAQ_SUCCESS;
}

ErrCode INTERFACE_FUNC UserImpl::getSerializeId(ConstCharPtr* id) const
{
    OPENDAQ_PARAM_NOT_NULL(id);

    *id = SerializeId();
    return OPENDAQ_SUCCESS;
}

ConstCharPtr UserImpl::SerializeId()
{
    return "User";
}

ErrCode UserImpl::Deserialize(ISerializedObject* serialized, IBaseObject*, IFunction*, IBaseObject** obj)
{
    OPENDAQ_PARAM_NOT_NULL(obj);
    SerializedObjectPtr serializedObj = SerializedObjectPtr::Borrow(serialized);

    StringPtr username;
    ErrCode err = serializedObj->readString(String("username"), &username);
    if (OPENDAQ_FAILED(err) && err != OPENDAQ_ERR_NOTFOUND)
        return err;

    StringPtr passwordHash;
    err = serializedObj->readString(String("passwordHash"), &passwordHash);
    if (OPENDAQ_FAILED(err) && err != OPENDAQ_ERR_NOTFOUND)
        return err;

    ListPtr<IUser> groups;
    err = serializedObj->readList(String("groups"), nullptr, nullptr, &groups);
    if (OPENDAQ_FAILED(err) && err != OPENDAQ_ERR_NOTFOUND)
        return err;

    auto user = User(username, passwordHash, groups);
    *obj = user.addRefAndReturn();
    return OPENDAQ_SUCCESS;
}

ListPtr<IString> UserImpl::sanitizeGroupList(const ListPtr<IString> groups)
{
    std::set<StringPtr> orderedUnique;
    orderedUnique.insert("everyone");

    if (groups.assigned())
    {
        for (const auto& group : groups)
            orderedUnique.insert(group);
    }

    auto list = List<IString>();
    for (const auto& group : orderedUnique)
        list.pushBack(group);

    return list;
}

// Factory

OPENDAQ_DEFINE_CLASS_FACTORY(LIBRARY_FACTORY, User, IString*, username, IString*, passwordHash, IList*, groups)

OPENDAQ_REGISTER_DESERIALIZE_FACTORY(UserImpl)

END_NAMESPACE_OPENDAQ
