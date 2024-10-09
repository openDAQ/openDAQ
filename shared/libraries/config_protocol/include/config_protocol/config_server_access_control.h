/*
 * Copyright 2022-2024 openDAQ d.o.o.
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
#include <opendaq/component_ptr.h>
#include <coreobjects/user_ptr.h>

namespace daq::config_protocol
{

class ConfigServerAccessControl
{
public:
    static void protectObject(const PropertyObjectPtr& component, const UserPtr& user, Permission requiredPermission);
    static void protectObject(const PropertyObjectPtr& component, const UserPtr& user, const std::vector<Permission>& requiredPermissions);
    static PropertyObjectPtr getFirstPropertyParent(const ComponentPtr& component, const StringPtr& propertyName);
    static void protectLockedComponent(const ComponentPtr& component);

private:
    static DevicePtr getParentDevice(const ComponentPtr& component);
};


inline void ConfigServerAccessControl::protectObject(const PropertyObjectPtr& component,
                                                        const UserPtr& user,
                                                        Permission requiredPermission)
{
    const std::vector<Permission> requiredPermissions = {requiredPermission};
    protectObject(component, user, requiredPermissions);
}

inline void ConfigServerAccessControl::protectObject(const PropertyObjectPtr& component,
                                                        const UserPtr& user,
                                                        const std::vector<Permission>& requiredPermissions)
{
    auto permissionManager = component.getPermissionManager();

    for (const auto permission : requiredPermissions)
    {
        if (!permissionManager.isAuthorized(user, permission))
            throw AccessDeniedException();
    }
}

inline PropertyObjectPtr daq::config_protocol::ConfigServerAccessControl::getFirstPropertyParent(const ComponentPtr& component,
                                                                                                 const StringPtr& propertyName)
{
    std::string parentObjectName = propertyName;

    auto pos = parentObjectName.find_last_of('.');

    if (pos == std::string::npos)
        return component;

    parentObjectName = parentObjectName.substr(0, pos);

    auto parentObject = component.getPropertyValue(parentObjectName);
    return parentObject.asPtr<IPropertyObject>();
}

inline void ConfigServerAccessControl::protectLockedComponent(const ComponentPtr& component)
{
    const auto device = getParentDevice(component);

    if (device.assigned() && device.isLocked())
        throw DeviceLockedException();
}

inline DevicePtr ConfigServerAccessControl::getParentDevice(const ComponentPtr& component)
{
    ComponentPtr current = component;

    while (current.assigned())
    {
        if (current.asPtrOrNull<IDevice>().assigned())
            return current;

        current = current.getParent();
    }

    return nullptr;
}

};
