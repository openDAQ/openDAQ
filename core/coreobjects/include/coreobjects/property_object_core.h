

#pragma once
#include <coreobjects/mutex_ptr.h>
#include <coreobjects/property_object_ptr.h>
#include <coreobjects/object_lock_guard.h>
#include <map>
#include <coreobjects/permissions_builder_factory.h>
#include <coreobjects/permission_mask_builder_factory.h>

BEGIN_NAMESPACE_OPENDAQ

namespace object_utils
{
    struct NullMutex
    {
        void lock() {}
        void unlock() noexcept {}
        bool try_lock() { return true; }
    };

    inline const auto UnrestrictedPermissions = []() { 
        daqDisableObjectTracking();
        auto permissions = PermissionsBuilder().assign("everyone", PermissionMaskBuilder().read().write().execute()).build();
        daqEnableObjectTracking();
        return permissions;
    }();
}

class RecursiveConfigLockGuard : public std::enable_shared_from_this<RecursiveConfigLockGuard>
{
public:
    virtual ~RecursiveConfigLockGuard() = default;
};

template <typename TMutex>
class GenericRecursiveConfigLockGuard : public RecursiveConfigLockGuard
{
public:
    GenericRecursiveConfigLockGuard(const TMutex& lock, std::thread::id* threadId, int* depth);
    ~GenericRecursiveConfigLockGuard() override;

private:
    std::thread::id* id;
    int* depth;
    TMutex mutex;
    std::lock_guard<TMutex> lock;
};

struct LockGuardImpl : ImplementationOf<ILockGuard>
{
    LockGuardImpl(IPropertyObject* owner, MutexPtr lock);

private:
    // to ensure that owner is destroyed after lock
    PropertyObjectPtr owner;
    MutexPtr mutex;
    std::lock_guard<MutexPtr> lock;
};

template <typename TMutex>
class RecursiveLockGuardImpl : public ImplementationOf<ILockGuard>
{
public:
    RecursiveLockGuardImpl(IPropertyObject* owner, const TMutex& lock, std::thread::id* threadId, int* depth);
    ~RecursiveLockGuardImpl() override;

private:
    // to ensure that owner is destroyed after lock
    PropertyObjectPtr owner;
    std::thread::id* id;
    int* depth;
    TMutex mutex;
    std::lock_guard<TMutex> lockGuard;
};

class PropertyUpdateStack
{
    struct PropertyUpdateStackItem
    {
        PropertyUpdateStackItem(const BaseObjectPtr& value);
        bool setValue(const BaseObjectPtr& value);
        bool unregister();
        size_t getStackLevel() const;

        BaseObjectPtr value;
        size_t stackLevel;
        bool isPushed;
    };

public:
    PropertyUpdateStack() = default;

    // return true if property is registered
    bool registerPropertyUpdating(const std::string& name, const BaseObjectPtr& value);
    // returns true is object need to be written
    bool unregisterPropertyUpdating(const std::string& name);
    bool isBaseStackLevel(const std::string& name) const;
    bool getPropertyValue(const std::string& name, BaseObjectPtr& value) const;

private:
    std::map<std::string, PropertyUpdateStackItem> updatePropertyStack;
};

END_NAMESPACE_OPENDAQ
