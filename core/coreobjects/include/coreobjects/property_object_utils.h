/*
 * Copyright 2022-2025 openDAQ d.o.o.
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
#include <coreobjects/permissions_builder_factory.h>
#include <coreobjects/permission_mask_builder_factory.h>
#include <coreobjects/object_lock_guard_ptr.h>

BEGIN_NAMESPACE_OPENDAQ

namespace object_utils
{
    inline const auto UnrestrictedPermissions = []() { 
        daqDisableObjectTracking();
        auto permissions = PermissionsBuilder().assign("everyone", PermissionMaskBuilder().read().write().execute()).build();
        daqEnableObjectTracking();
        return permissions;
    }();
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
