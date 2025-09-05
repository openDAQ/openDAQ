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
#include <simulator_device_module/common.h>
#include <opendaq/module_impl.h>

/*
 * Simulator module that mimics the behaviour of an Analog Input test and measurement device
 *
 * Remaining tasks for module finalization:
 *  - Add logging to module
 *  - Document module
 *  - Add device and channel statuses
 *  - Set up property descriptions
 *  - Support the begin/end update mechanism
 *  - Add option to set up custom device tick resolution
 *  - Support client-side scaling (post scaling)
 *  - Enable domain signal sharing across channels (requires `setDomainSignal` bugfix)
 *  - Allow for fixed packet sizes
 *  - Implement log retrieval callbacks
 *  - Enable user-configurable device information fields
 *  - Test save/load mechanism
 */

BEGIN_NAMESPACE_SIMULATOR_DEVICE_MODULE

class SimulatorDeviceModule final : public Module
{
public:
    explicit SimulatorDeviceModule(const ContextPtr& context);

    ListPtr<IDeviceInfo> onGetAvailableDevices() override;
    DictPtr<IString, IDeviceType> onGetAvailableDeviceTypes() override;
    DevicePtr onCreateDevice(const StringPtr& connectionString, const ComponentPtr& parent, const PropertyObjectPtr& config) override;

private:
    WeakRefPtr<IDevice> device;
    std::mutex sync;

    static DictPtr<IString, IBaseObject> populateDefaultModuleOptions(const DictPtr<IString, IBaseObject>& inputOptions);
    void clearRemovedDevice();
};

END_NAMESPACE_SIMULATOR_DEVICE_MODULE
