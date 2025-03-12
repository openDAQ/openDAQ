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
#include <opendaq/device_ptr.h>
#include <opendaq/component_holder_ptr.h>
#include <opendaq/component_holder_factory.h>
#include <opendaq/search_filter_factory.h>
#include <opendaq/device_private_ptr.h>

namespace daq::config_protocol
{

class ConfigServerDevice
{
public:
    static BaseObjectPtr getInfo(const RpcContext& context, const DevicePtr& device, const ParamsDictPtr& params);
    static BaseObjectPtr getTicksSinceOrigin(const RpcContext& context, const DevicePtr& device, const ParamsDictPtr& params);
    static BaseObjectPtr lock(const RpcContext& context, const DevicePtr& device, const ParamsDictPtr& params);
    static BaseObjectPtr unlock(const RpcContext& context, const DevicePtr& device, const ParamsDictPtr& params);
    static BaseObjectPtr forceUnlock(const RpcContext& context, const DevicePtr& device, const ParamsDictPtr& params);
    static BaseObjectPtr getAvailableDevices(const RpcContext& context, const DevicePtr& device, const ParamsDictPtr& params);
    static BaseObjectPtr addDevice(const RpcContext& context, const DevicePtr& device, const ParamsDictPtr& params);
    static BaseObjectPtr removeDevice(const RpcContext& context, const DevicePtr& device, const ParamsDictPtr& params);
    static BaseObjectPtr getAvailableDeviceTypes(const RpcContext& context, const DevicePtr& device, const ParamsDictPtr& params);
    static BaseObjectPtr getLogFileInfos(const RpcContext& context, const DevicePtr& device, const ParamsDictPtr& params);
    static BaseObjectPtr getLog(const RpcContext& context, const DevicePtr& device, const ParamsDictPtr& params);

    static BaseObjectPtr setPropertyValue(const RpcContext& context, const ComponentPtr& component, const ParamsDictPtr& params);
    static BaseObjectPtr setProtectedPropertyValue(const RpcContext& context, const ComponentPtr& component, const ParamsDictPtr& params);

    static BaseObjectPtr getAvailableOperationModes(const RpcContext& context, const DevicePtr& device, const ParamsDictPtr& params);
    static BaseObjectPtr setOperationMode(const RpcContext& context, const DevicePtr& device, const ParamsDictPtr& params);
    static BaseObjectPtr setOperationModeRecursive(const RpcContext& context, const DevicePtr& device, const ParamsDictPtr& params);
    static BaseObjectPtr getOperationMode(const RpcContext& context, const DevicePtr& device, const ParamsDictPtr& params);
};

inline BaseObjectPtr ConfigServerDevice::getInfo(const RpcContext& context, const DevicePtr& device, const ParamsDictPtr& params)
{
    ConfigServerAccessControl::protectObject(device, context.user, Permission::Read);

    return device.getInfo();
}

inline BaseObjectPtr ConfigServerDevice::getTicksSinceOrigin(const RpcContext& context,
                                                             const DevicePtr& device,
                                                             const ParamsDictPtr& params)
{
    ConfigServerAccessControl::protectObject(device, context.user, Permission::Read);

    return device.getTicksSinceOrigin();
}

inline BaseObjectPtr ConfigServerDevice::lock(const RpcContext& context,
                                              const DevicePtr& device, 
                                              const ParamsDictPtr& params)
{
    ConfigServerAccessControl::protectObject(device, context.user, {Permission::Read, Permission::Write});
    ConfigServerAccessControl::protectViewOnlyConnection(context.connectionType);

    device.asPtr<IDevicePrivate>().lock(context.user);
    return nullptr;
}

inline BaseObjectPtr ConfigServerDevice::unlock(const RpcContext& context,
                                                const DevicePtr& device,
                                                const ParamsDictPtr& params)
{
    ConfigServerAccessControl::protectObject(device, context.user, {Permission::Read, Permission::Write});
    ConfigServerAccessControl::protectViewOnlyConnection(context.connectionType);

    device.asPtr<IDevicePrivate>().unlock(context.user);
    return nullptr;
}

inline BaseObjectPtr ConfigServerDevice::forceUnlock(const RpcContext& context,
                                                     const DevicePtr& device,
                                                     const ParamsDictPtr& params)
{
    ConfigServerAccessControl::protectObject(device, context.user, {Permission::Read, Permission::Write});
    ConfigServerAccessControl::protectViewOnlyConnection(context.connectionType);

    device.asPtr<IDevicePrivate>().forceUnlock();
    return nullptr;
}

inline BaseObjectPtr ConfigServerDevice::getLogFileInfos(const RpcContext& context,
                                                         const DevicePtr& device,
                                                         const ParamsDictPtr& params)
{
    ConfigServerAccessControl::protectObject(device, context.user, Permission::Read);
    return device.getLogFileInfos();
}

inline BaseObjectPtr ConfigServerDevice::getLog(const RpcContext& context,
                                                const DevicePtr& device,
                                                const ParamsDictPtr& params)
{
    ConfigServerAccessControl::protectObject(device, context.user, Permission::Read);

    const auto Id = params.get("Id");
    const auto size = params.get("Size");
    const auto offset = params.get("Offset");

    return device.getLog(Id, size, offset);
}

inline BaseObjectPtr ConfigServerDevice::getAvailableDevices(const RpcContext& context,
                                                             const DevicePtr& device,
                                                             const ParamsDictPtr& params)
{
    ConfigServerAccessControl::protectObject(device, context.user, Permission::Read);
    return device.getAvailableDevices();
}

inline BaseObjectPtr ConfigServerDevice::addDevice(const RpcContext& context,
                                                   const DevicePtr& device,
                                                   const ParamsDictPtr& params)
{
    ConfigServerAccessControl::protectLockedComponent(device);
    ConfigServerAccessControl::protectObject(device, context.user, {Permission::Read, Permission::Write});
    ConfigServerAccessControl::protectViewOnlyConnection(context.connectionType);

    const auto connectionString = params.get("ConnectionString");
    PropertyObjectPtr config = params.getOrDefault("Config");

    const auto dev = device.addDevice(connectionString, config);
    return ComponentHolder(dev);
}

inline BaseObjectPtr ConfigServerDevice::removeDevice(const RpcContext& context,
                                                      const DevicePtr& device,
                                                      const ParamsDictPtr& params)
{
    ConfigServerAccessControl::protectLockedComponent(device);
    ConfigServerAccessControl::protectObject(device, context.user, {Permission::Read, Permission::Write});
    ConfigServerAccessControl::protectViewOnlyConnection(context.connectionType);

    const auto localId = params.get("LocalId");

    const auto devs = device.getDevices(search::LocalId(localId));

    if (devs.getCount() == 0)
        DAQ_THROW_EXCEPTION(NotFoundException, "Device not found");

    if (devs.getCount() > 1)
        DAQ_THROW_EXCEPTION(InvalidStateException, "Duplicate device");

    device.removeDevice(devs[0]);

    return nullptr;
}

inline BaseObjectPtr ConfigServerDevice::getAvailableDeviceTypes(const RpcContext& context,
                                                                 const DevicePtr& device,
                                                                 const ParamsDictPtr& params)
{
    ConfigServerAccessControl::protectObject(device, context.user, Permission::Read);

    const auto devTypes = device.getAvailableDeviceTypes();
    return devTypes;
}

inline BaseObjectPtr ConfigServerDevice::setPropertyValue(const RpcContext& context,
                                                          const ComponentPtr& component,
                                                          const ParamsDictPtr& params)
{
    ConfigServerAccessControl::protectLockedComponent(component);
    ConfigServerAccessControl::protectViewOnlyConnection(context.connectionType);

    const auto propertyName = static_cast<std::string>(params["PropertyName"]);
    const auto propertyValue = params["PropertyValue"];
    const auto propertyParent = ConfigServerAccessControl::getFirstPropertyParent(component, propertyName);

    ConfigServerAccessControl::protectObject(propertyParent, context.user, {Permission::Read, Permission::Write});

    component.setPropertyValue(propertyName, propertyValue);

    return nullptr;
}

inline BaseObjectPtr ConfigServerDevice::setProtectedPropertyValue(const RpcContext& context,
                                                                   const ComponentPtr& component,
                                                                   const ParamsDictPtr& params)
{
    ConfigServerAccessControl::protectLockedComponent(component);
    ConfigServerAccessControl::protectViewOnlyConnection(context.connectionType);

    const auto propertyName = static_cast<std::string>(params["PropertyName"]);
    const auto propertyValue = static_cast<std::string>(params["PropertyValue"]);
    const auto propertyParent = ConfigServerAccessControl::getFirstPropertyParent(component, propertyName);

    ConfigServerAccessControl::protectObject(propertyParent, context.user, {Permission::Read, Permission::Write});

    component.asPtr<IPropertyObjectProtected>(true).setProtectedPropertyValue(propertyName, propertyValue);

    return nullptr;
}

inline BaseObjectPtr ConfigServerDevice::getAvailableOperationModes(const RpcContext& context,
                                                                    const DevicePtr& device,
                                                                    const ParamsDictPtr& params)
{
    ConfigServerAccessControl::protectObject(device, context.user, Permission::Read);
    return device.getAvailableOperationModes();
}

inline BaseObjectPtr ConfigServerDevice::setOperationMode(const RpcContext& context, 
                                                          const DevicePtr& device,
                                                          const ParamsDictPtr& params)
{
    ConfigServerAccessControl::protectObject(device, context.user, Permission::Read);
    const auto modeType = static_cast<std::string>(params["ModeType"]);
    device.setOperationMode(modeType);
    return nullptr;
}

inline BaseObjectPtr ConfigServerDevice::setOperationModeRecursive(const RpcContext& context,
                                                                   const DevicePtr& device,
                                                                   const ParamsDictPtr& params)
{
    ConfigServerAccessControl::protectObject(device, context.user, Permission::Read);
    const auto modeType = static_cast<std::string>(params["ModeType"]);
    device.setOperationModeRecursive(modeType);
    return nullptr;
}

inline BaseObjectPtr ConfigServerDevice::getOperationMode(const RpcContext& context,
                                                          const DevicePtr& device,
                                                          const ParamsDictPtr& params)
{
    ConfigServerAccessControl::protectObject(device, context.user, Permission::Read);
    return device.getOperationMode();
}

}
