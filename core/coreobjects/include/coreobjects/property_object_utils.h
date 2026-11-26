/*
 * Copyright 2022-2026 openDAQ d.o.o.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#pragma once
#include <coreobjects/mutex_ptr.h>
#include <coreobjects/property_object_ptr.h>
#include <coreobjects/object_lock_guard.h>
#include <map>
#include <unordered_map>
#include <coreobjects/permissions_builder_factory.h>
#include <coreobjects/permissions_internal_ptr.h>
#include <coreobjects/permission_mask_builder_factory.h>
#include <coreobjects/object_lock_guard_ptr.h>
#include <coretypes/coretypes.h>
#include <coretypes/impl.h>
#include <cassert>
#include <mutex>

BEGIN_NAMESPACE_OPENDAQ

namespace details
{
    static const std::unordered_map<IntfID, CoreType> intfIdToCoreTypeMap = {
        {IBoolean::Id, ctBool},
        {IInteger::Id, ctInt},
        {IFloat::Id, ctFloat},
        {IString::Id, ctString},
        {IList::Id, ctList},
        {IDict::Id, ctDict},
        {IRatio::Id, ctRatio},
        {IProcedure::Id, ctProc},
        {IFunction::Id, ctFunc},
        {IBinaryData::Id, ctBinaryData},
        {IComplexNumber::Id, ctComplexNumber},
        {IPropertyObject::Id, ctObject}
    };

    static CoreType intfIdToCoreType(IntfID intfID)
    {
        if (intfIdToCoreTypeMap.find(intfID) == intfIdToCoreTypeMap.end())
        {
            return ctUndefined;
        }

        return intfIdToCoreTypeMap.at(intfID);
    }
}

namespace object_utils
{
    inline const auto UnrestrictedPermissions = []() { 
        static PermissionsPtr permissions;
        if (!permissions.assigned())
        {
            permissions = PermissionsBuilder().assign("everyone", PermissionMaskBuilder().read().write().execute()).build();
            daqUntrackObject(permissions);
            daqUntrackObject(permissions.getAllowed());
            daqUntrackObject(permissions.getDenied());
            daqUntrackObject(permissions.asPtr<IPermissionsInternal>(true).getAssigned());
            for (const auto& [key, _] : permissions.getAllowed())
                daqUntrackObject(key);
            for (const auto& [key, _] : permissions.getDenied())
                daqUntrackObject(key);
            for (const auto& [key, _] : permissions.asPtr<IPermissionsInternal>(true).getAssigned())
                daqUntrackObject(key);
        }
        return permissions;
    }();

    // The type of the items in a list or dictionary; the first item's type when the container does not declare one
    inline CoreType getItemCoreType(const BaseObjectPtr& container)
    {
        IntfID intfID = IUnknown::Id;
        if (const auto dictElementType = container.asPtrOrNull<IDictElementType>(true); dictElementType.assigned())
            checkErrorInfo(dictElementType->getValueInterfaceId(&intfID));
        else if (const auto listElementType = container.asPtrOrNull<IListElementType>(true); listElementType.assigned())
            checkErrorInfo(listElementType->getElementInterfaceId(&intfID));

        if (const auto coreType = details::intfIdToCoreType(intfID); coreType != ctUndefined)
            return coreType;
        if (const auto list = container.asPtrOrNull<IList>(); list.assigned() && list.getCount() > 0)
            return list[0].getCoreType();
        if (const auto dict = container.asPtrOrNull<IDict>(); dict.assigned() && dict.getCount() > 0)
            return dict.getValueList()[0].getCoreType();
        return ctUndefined;
    }
}

// RecursiveConfigLockGuard

class RecursiveConfigLockGuard : public std::enable_shared_from_this<RecursiveConfigLockGuard>
{
public:
    RecursiveConfigLockGuard(const LockGuardPtr& lg);

private:
    LockGuardPtr lg;
};

inline RecursiveConfigLockGuard::RecursiveConfigLockGuard(const LockGuardPtr& lg)
    : lg(lg)
{
}

// LockGuardImpl

// Holds the mutex locked without recursion until the guard is released
class LockGuardImpl : public ImplementationOf<ILockGuard>
{
public:
    explicit LockGuardImpl(MutexPtr lock);

private:
    MutexPtr mutex;
    std::lock_guard<MutexPtr> lockGuard;
};

inline LockGuardImpl::LockGuardImpl(MutexPtr lock)
    : mutex(std::move(lock))
    , lockGuard(mutex)
{
}

// RecursiveLockGuardImpl

// Holds the mutex through lockRecursive, so its thread can take further recursive guards of any object that shares it
class RecursiveLockGuardImpl : public ImplementationOf<ILockGuard>
{
public:
    explicit RecursiveLockGuardImpl(MutexPtr lock);
    ~RecursiveLockGuardImpl() override;

private:
    MutexPtr mutex;
};

inline RecursiveLockGuardImpl::RecursiveLockGuardImpl(MutexPtr lock)
    : mutex(std::move(lock))
{
    checkErrorInfo(mutex->lockRecursive());
}

inline RecursiveLockGuardImpl::~RecursiveLockGuardImpl()
{
    // Fails only for a guard released on a thread other than the one that took it; a destructor cannot report it
    const ErrCode errCode = mutex->unlockRecursive();
    assert(OPENDAQ_SUCCEEDED(errCode));
    if (OPENDAQ_FAILED(errCode))
        daqClearErrorInfo();
}

// PropertyUpdateStack

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

inline PropertyUpdateStack::PropertyUpdateStackItem::PropertyUpdateStackItem(const BaseObjectPtr& value)
    : value(value)
    , stackLevel(1)
    , isPushed(true)
{
}

inline bool PropertyUpdateStack::PropertyUpdateStackItem::setValue(const BaseObjectPtr& value)
{
    if (this->value == value)
        return false;

    this->value = value;
    this->isPushed = true;
    this->stackLevel++;
    return true;
}

inline bool PropertyUpdateStack::PropertyUpdateStackItem::unregister()
{
    bool result = this->isPushed;
    this->isPushed = false;
    this->stackLevel--;
    return result;
}

inline size_t PropertyUpdateStack::PropertyUpdateStackItem::getStackLevel() const
{
    return this->stackLevel;
}

inline bool PropertyUpdateStack::registerPropertyUpdating(const std::string& name, const BaseObjectPtr& value)
{
    auto [it, inserted] = updatePropertyStack.try_emplace(name, value);

    if (inserted)
        return true;

    auto& propertItem = it->second;
    return propertItem.setValue(value);
}

inline bool PropertyUpdateStack::unregisterPropertyUpdating(const std::string& name)
{
    auto it = updatePropertyStack.find(name);
    if (it == updatePropertyStack.end())
        return false;

    auto& propertItem = it->second;
    bool result = propertItem.unregister();
    if (propertItem.getStackLevel() == 0)
        updatePropertyStack.erase(it);
    return result;
}

inline bool PropertyUpdateStack::isBaseStackLevel(const std::string& name) const
{
    auto it = updatePropertyStack.find(name);
    if (it == updatePropertyStack.end())
        return false;

    return it->second.getStackLevel() == 1;
}

inline bool PropertyUpdateStack::getPropertyValue(const std::string& name, BaseObjectPtr& value) const
{
    auto it = updatePropertyStack.find(name);
    if (it == updatePropertyStack.end())
        return false;

    // if value is not assigned, it means, that property is on clearing stage (clearPropertyValue)
    value = it->second.value;
    return true;
}

END_NAMESPACE_OPENDAQ
