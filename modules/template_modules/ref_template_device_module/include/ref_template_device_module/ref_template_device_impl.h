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
#include <ref_template_device_module/common.h>
#include <opendaq/channel_ptr.h>
#include <opendaq_module_template/device_template.h>
#include <condition_variable>
#include <ref_template_device_module/ref_template_channel_impl.h>
#include <ref_template_device_module/ref_template_can_channel_impl.h>
#include <opendaq/log_file_info_ptr.h>

BEGIN_NAMESPACE_REF_TEMPLATE_DEVICE_MODULE

class RefDeviceBase final : public templates::DeviceTemplateHooks
{
public:
    RefDeviceBase(const templates::DeviceParams& params);
};

class RefTemplateDevice final : public templates::DeviceTemplate
{
public:
    explicit RefTemplateDevice();

protected:
    void initProperties() override;
    void applyConfig(const PropertyObjectPtr& config) override;

    DeviceDomainPtr initDeviceDomain() override;
    void initIOFolder(const IoFolderConfigPtr& ioFolder) override;
    void start() override;

    templates::AcquisitionLoopParams getAcquisitionLoopParameters() override;
    void onAcquisitionLoop() override;

    uint64_t getTicksSinceOrigin() override;
    BaseObjectPtr onPropertyWrite(const templates::PropertyEventArgs& args) override;

    ListPtr<ILogFileInfo> getLogFileInfos() override;
    StringPtr getLog(const StringPtr& id, Int size, Int offset) override;

private:
    std::chrono::microseconds getMicroSecondsSinceDeviceStart() const;
    static PropertyObjectPtr createProtectedObject();

    void removeRedundantChannels(size_t numberOfChannels);
    void addMissingChannels(size_t numberOfChannels);
    void updateNumberOfChannels(size_t numberOfChannels);
    void enableCANChannel(bool enableCANChannel);
    void enableProtectedChannel();
    void updateAcqLoopTime(size_t loopTime) const;
    void updateDeviceSampleRate(double sampleRate) const;

    StringPtr loggingPath;

    std::chrono::steady_clock::time_point startTime;
    std::chrono::microseconds microSecondsFromEpochToDeviceStart;

    std::vector<std::shared_ptr<RefTemplateChannelImpl>> channels;
    //ChannelPtr protectedChannel;
    std::shared_ptr<RefTemplateCANChannelImpl> canChannel;
    
    UnitPtr domainUnit;
    FolderConfigPtr aiFolder;
    FolderConfigPtr canFolder;
};

END_NAMESPACE_REF_TEMPLATE_DEVICE_MODULE
