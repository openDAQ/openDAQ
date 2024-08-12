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
#include <ref_device_module/common.h>
#include <opendaq/channel_ptr.h>
#include <opendaq_module_template/device_template.h>
#include <thread>
#include <condition_variable>

BEGIN_NAMESPACE_REF_DEVICE_MODULE

class RefDeviceBase final : public DeviceTemplateHooks
{
public:
    RefDeviceBase(const DeviceParams& params);
};

class RefDeviceImpl final : public DeviceTemplate
{
public:
    explicit RefDeviceImpl(const DeviceParams& config);
    ~RefDeviceImpl() override;

    // IDevice
    uint64_t getTicksSinceOrigin() override;

private:
    void initClock();
    void initIoFolder();
    void initSyncComponent();
    void initProperties(const PropertyObjectPtr& config);
    void acqLoop();
    void updateNumberOfChannels();
    void enableCANChannel();
    void updateAcqLoopTime();
    void updateGlobalSampleRate();
    std::chrono::microseconds getMicroSecondsSinceDeviceStart() const;

    std::thread acqThread;
    std::condition_variable cv;

    std::chrono::steady_clock::time_point startTime;
    std::chrono::microseconds microSecondsFromEpochToDeviceStart;

    std::vector<ChannelPtr> channels;
    ChannelPtr canChannel;
    size_t acqLoopTime;
    bool stopAcq;

    FolderConfigPtr aiFolder;
    FolderConfigPtr canFolder;
    ComponentPtr syncComponent;
};

END_NAMESPACE_REF_DEVICE_MODULE
