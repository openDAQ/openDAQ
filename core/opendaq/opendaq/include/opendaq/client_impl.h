/*
 * Copyright 2022-2023 Blueberry d.o.o.
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
#include <opendaq/client_private.h>
#include <opendaq/device_impl.h>
#include <opendaq/logger_component_ptr.h>
#include <opendaq/logger_ptr.h>
#include <opendaq/module_manager_ptr.h>
#include <opendaq/component_ptr.h>

BEGIN_NAMESPACE_OPENDAQ

class ClientImpl : public DeviceBase<IClientPrivate>
{
public:
    ClientImpl(ContextPtr ctx, const StringPtr& localId, const DeviceInfoPtr& deviceInfo);

    // Device

    DeviceInfoPtr onGetInfo() override;

    // FunctionBlockDevice

    DictPtr<IString, IFunctionBlockType> onGetAvailableFunctionBlockTypes() override;
    FunctionBlockPtr onAddFunctionBlock(const StringPtr& typeId, const PropertyObjectPtr& config) override;
    void onRemoveFunctionBlock(const FunctionBlockPtr& functionBlock) override;

    // ClientDevice

    ListPtr<IDeviceInfo> onGetAvailableDevices() override;
    DictPtr<IString, IDeviceType> onGetAvailableDeviceTypes() override;
    DevicePtr onAddDevice(const StringPtr& connectionString, const PropertyObjectPtr& config) override;
    void onRemoveDevice(const DevicePtr& device) override;

    // IClientPrivate
    ErrCode INTERFACE_FUNC setRootDevice(IComponent* rootDevice) override;

private:
    ModuleManagerPtr manager;
    LoggerPtr logger;
    LoggerComponentPtr loggerComponent;

    std::unordered_map<std::string, size_t> functionBlockCountMap;

    bool rootDeviceSet;

    WeakRefPtr<IDevice> rootDevice;

    ComponentPtr getFunctionBlocksFolder();
};

END_NAMESPACE_OPENDAQ
