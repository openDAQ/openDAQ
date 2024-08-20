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

    explicit RefDeviceImpl();
    ~RefDeviceImpl() override;

protected:
    
    uint64_t getTicksSinceOrigin() override;
    void handleConfig(const PropertyObjectPtr& config) override;
    void handleOptions(const DictPtr<IString, IBaseObject>& options) override;
    void initProperties() override;
    BaseObjectPtr onPropertyWrite(const PropertyObjectPtr& owner, const StringPtr& propertyName, const PropertyPtr& property, const BaseObjectPtr& value) override;
    void initIOFolder(const IoFolderConfigPtr& ioFolder) override;
    DeviceDomainPtr initDeviceDomain() override;
    void start() override;
    
    bool allowAddDevicesFromModules() override;
    bool allowAddFunctionBlocksFromModules() override;

private:

    void acqLoop();
    std::chrono::microseconds getMicroSecondsSinceDeviceStart() const;

    void updateNumberOfChannels(size_t numberOfChannels);
    void enableCANChannel(bool enableCANChannel);
    void updateAcqLoopTime(size_t loopTime);
    void updateDeviceSampleRate(double sampleRate);

    std::thread acqThread;
    size_t acqLoopTime;
    std::condition_variable cv;
    bool stopAcq;

    std::chrono::steady_clock::time_point startTime;
    std::chrono::microseconds microSecondsFromEpochToDeviceStart;

    std::vector<ChannelPtr> channels;
    ChannelPtr canChannel;
    
    UnitPtr domainUnit;
    FolderConfigPtr aiFolder;
    FolderConfigPtr canFolder;
};

END_NAMESPACE_REF_DEVICE_MODULE
