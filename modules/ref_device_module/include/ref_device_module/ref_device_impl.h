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
#include <ref_device_module/common.h>
#include <opendaq/channel_ptr.h>
#include <opendaq/device_impl.h>
#include <opendaq/logger_ptr.h>
#include <opendaq/logger_component_ptr.h>

#include <thread>
#include <condition_variable>

BEGIN_NAMESPACE_REF_DEVICE_MODULE

class RefDeviceImpl final : public Device
{
public:
    explicit RefDeviceImpl(size_t id, const ContextPtr& ctx, const ComponentPtr& parent, const StringPtr& localId);
    ~RefDeviceImpl() override;

    static DeviceInfoPtr CreateDeviceInfo(size_t id);
    static DeviceTypePtr createType();

    // IDevice
    DeviceInfoPtr onGetInfo() override;

    // IDeviceDomain
    RatioPtr onGetResolution() override;
    uint64_t onGetTicksSinceOrigin() override;
    std::string onGetOrigin() override;
    UnitPtr onGetDomainUnit() override;
private:
    size_t id;

    std::thread acqThread;
    std::condition_variable cv;

    std::chrono::steady_clock::time_point startTime;
    std::chrono::microseconds microSecondsFromEpochToDeviceStart;

    std::vector<ChannelPtr> channels;
    size_t acqLoopTime;

    FolderConfigPtr aiFolder;
    ComponentPtr syncComponent;

    void initClock();
    void initIoFolder();
    void initSyncComponent();
    void initProperties();
    bool stopAcq;
    void acqLoop();
    void updateNumberOfChannels();
    void updateAcqLoopTime();
    void updateGlobalSampleRate();
    std::chrono::microseconds getMicroSecondsSinceDeviceStart() const;

    LoggerPtr logger;
    LoggerComponentPtr loggerComponent;
};

END_NAMESPACE_REF_DEVICE_MODULE
