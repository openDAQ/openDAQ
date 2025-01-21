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
#include <opendaq/device_impl.h>
#include <opendaq/logger_component_ptr.h>
#include <opendaq/logger_ptr.h>
#include <opendaq/module_manager_ptr.h>

BEGIN_NAMESPACE_OPENDAQ

class ClientImpl : public Device
{
public:
    ClientImpl(ContextPtr ctx, const StringPtr& localId, const DeviceInfoPtr& deviceInfo, const ComponentPtr& parent = nullptr);

    // Device

    DeviceInfoPtr onGetInfo() override;
    bool allowAddDevicesFromModules() override;
    bool allowAddFunctionBlocksFromModules() override;

private:
    ModuleManagerPtr manager;
    LoggerPtr logger;
    LoggerComponentPtr loggerComponent;
    DeviceInfoPtr customDeviceInfo;

    std::unordered_map<std::string, size_t> functionBlockCountMap;
};

END_NAMESPACE_OPENDAQ
