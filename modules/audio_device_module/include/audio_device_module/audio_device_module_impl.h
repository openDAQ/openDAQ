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
#include <audio_device_module/common.h>
#include <audio_device_module/miniaudio_utils.h>
#include <opendaq/module_impl.h>
#include <miniaudio/miniaudio.h>

BEGIN_NAMESPACE_AUDIO_DEVICE_MODULE

class AudioDeviceModule final : public Module
{
public:
    explicit AudioDeviceModule(const ContextPtr& context);

    ListPtr<IDeviceInfo> onGetAvailableDevices() override;
    DictPtr<IString, IDeviceType> onGetAvailableDeviceTypes() override;
    DevicePtr onCreateDevice(const StringPtr& connectionString, const ComponentPtr& parent, const PropertyObjectPtr& config) override;

    DictPtr<IString, IFunctionBlockType> onGetAvailableFunctionBlockTypes() override;
    FunctionBlockPtr onCreateFunctionBlock(const StringPtr& id, const ComponentPtr& parent, const StringPtr& localId, const PropertyObjectPtr& config) override;

private:
    std::mutex sync;
    std::shared_ptr<MiniaudioContext> maContext;
    size_t deviceIndex;
};

END_NAMESPACE_AUDIO_DEVICE_MODULE
