#include <opendaq/user_lock_impl.h>
#include <coretypes/validation.h>
#include <coretypes/serialized_object_ptr.h>
#include <opendaq/user_lock_factory.h>
#include <coreobjects/user_internal_ptr.h>
#include <coreobjects/authentication_provider_ptr.h>
#include <opendaq/component_deserialize_context_ptr.h>

BEGIN_NAMESPACE_OPENDAQ

UserLockImpl::UserLockImpl()
{
}

ErrCode INTERFACE_FUNC UserLockImpl::lock(IUser* user)
{
    UserPtr userPtr = UserPtr::Borrow(user);

    if (userPtr.assigned() && userPtr.asPtr<IUserInternal>().isAnonymous())
        userPtr = nullptr;

    if (userLock.has_value() && userLock != userPtr)
        return OPENDAQ_ERR_DEVICE_LOCKED;

    this->userLock = userPtr;
    return OPENDAQ_SUCCESS;
}

ErrCode INTERFACE_FUNC UserLockImpl::unlock(IUser* user)
{
    if (userLock.has_value() && userLock != nullptr && userLock != user)
        return OPENDAQ_ERR_ACCESSDENIED;

    this->userLock.reset();
    return OPENDAQ_SUCCESS;
}

ErrCode INTERFACE_FUNC UserLockImpl::forceUnlock()
{
    this->userLock.reset();
    return OPENDAQ_SUCCESS;
}

ErrCode INTERFACE_FUNC UserLockImpl::isLocked(Bool* isLockedOut)
{
    OPENDAQ_PARAM_NOT_NULL(isLockedOut);

    *isLockedOut = userLock.has_value();
    return OPENDAQ_SUCCESS;
}

ErrCode INTERFACE_FUNC UserLockImpl::serialize(ISerializer* serializer)
{
    serializer->startTaggedObject(this);

    serializer->key("locked");
    serializer->writeBool(userLock.has_value());

    if (userLock.has_value() && userLock != nullptr)
    {
        const auto username = userLock->getUsername();

        serializer->key("username");
        serializer->writeString(username.getCharPtr(), username.getLength());
    }

    serializer->endObject();
    return OPENDAQ_SUCCESS;
}

ErrCode INTERFACE_FUNC UserLockImpl::getSerializeId(ConstCharPtr* id) const
{
    OPENDAQ_PARAM_NOT_NULL(id);

    *id = SerializeId();
    return OPENDAQ_SUCCESS;
}

ConstCharPtr UserLockImpl::SerializeId()
{
    return "UserLock";
}

ErrCode UserLockImpl::Deserialize(ISerializedObject* serialized, IBaseObject* context, IFunction*, IBaseObject** obj)
{
    OPENDAQ_PARAM_NOT_NULL(obj);
    SerializedObjectPtr serializedObj = SerializedObjectPtr::Borrow(serialized);

    Bool isLocked;
    ErrCode err = serializedObj->readBool(String("locked"), &isLocked);
    if (OPENDAQ_FAILED(err))
        return err;

    StringPtr username;
    err = serializedObj->readString(String("username"), &username);
    if (OPENDAQ_FAILED(err) && err != OPENDAQ_ERR_NOTFOUND)
        return err;

    UserPtr user = nullptr;
    const auto contextPtr = BaseObjectPtr::Borrow(context);
    const auto deserializerContext = contextPtr.assigned() ? contextPtr.asPtrOrNull<IComponentDeserializeContext>() : nullptr;

    if (deserializerContext.assigned() && username.assigned())
    {
        const auto authProvider = deserializerContext.getContext().getAuthenticationProvider();
        user = authProvider.findUser(username);
    }

    auto userLockOut = UserLock();

    if (isLocked)
        userLockOut.lock(user);

    *obj = userLockOut.addRefAndReturn();
    return OPENDAQ_SUCCESS;
}

// Factory

OPENDAQ_DEFINE_CLASS_FACTORY(LIBRARY_FACTORY, UserLock)

OPENDAQ_REGISTER_DESERIALIZE_FACTORY(UserLockImpl)

END_NAMESPACE_OPENDAQ
