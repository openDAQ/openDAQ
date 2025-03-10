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
#include <ref_device_module/common.h>
#include <opendaq/channel_ptr.h>
#include <opendaq/device_impl.h>
#include <opendaq/logger_ptr.h>
#include <opendaq/logger_component_ptr.h>
#include <chrono>
#include <thread>
#include <condition_variable>
#include <opendaq/log_file_info_ptr.h>

BEGIN_NAMESPACE_REF_DEVICE_MODULE

class RefDeviceImpl final : public Device
{
public:
    explicit RefDeviceImpl(size_t id, const PropertyObjectPtr& config, const ContextPtr& ctx, const ComponentPtr& parent, const StringPtr& localId, const StringPtr& name = nullptr);
    ~RefDeviceImpl() override;

    static DeviceInfoPtr CreateDeviceInfo(size_t id, const StringPtr& serialNumber = nullptr);
    static DeviceTypePtr CreateType();

    // IDevice
    DeviceInfoPtr onGetInfo() override;
    uint64_t onGetTicksSinceOrigin() override;

    bool allowAddDevicesFromModules() override;
    bool allowAddFunctionBlocksFromModules() override;

protected:
    ListPtr<ILogFileInfo> onGetLogFileInfos() override;
    StringPtr onGetLog(const StringPtr& id, Int size, Int offset) override;
    std::set<OperationModeType> onGetAvailableOperationModes() override;

#ifdef DAQMODULES_REF_DEVICE_MODULE_SIMULATOR_ENABLED
#ifdef __linux__
    void onSubmitNetworkConfiguration(const StringPtr& ifaceName, const PropertyObjectPtr& config) override;
    PropertyObjectPtr onRetrieveNetworkConfiguration(const StringPtr& ifaceName) override;
    Bool onGetNetworkConfigurationEnabled() override;
    ListPtr<IString> onGetNetworkInterfaceNames() override;
#endif
#endif

private:
    void createSignals();
    void initClock();
    void initIoFolder();
    void initSyncComponent();
    void initProperties(const PropertyObjectPtr& config);
    void collectTimeSignalSamples(std::chrono::microseconds curTim);
    uint64_t getSamplesSinceStart(std::chrono::microseconds time) const;
    void updateSamplesGenerated();
    void acqLoop();
    void updateNumberOfChannels();
    void enableCANChannel();
    void enableProtectedChannel();
    void updateAcqLoopTime();
    void configureTimeSignal();
    void updateGlobalSampleRate();
    void enableLogging();
    std::chrono::microseconds getMicroSecondsSinceDeviceStart() const;
    PropertyObjectPtr createProtectedObject() const;

    size_t id;
    StringPtr serialNumber;

    std::thread acqThread;
    std::condition_variable cv;

    std::chrono::steady_clock::time_point startTime;
    std::chrono::microseconds startTimeInMs;
    std::chrono::microseconds lastCollectTime;
    std::chrono::microseconds microSecondsFromEpochToDeviceStart;
    
    std::vector<ChannelPtr> channels;
    ChannelPtr canChannel;
    ChannelPtr protectedChannel;
    size_t acqLoopTime;
    bool stopAcq;

    FolderConfigPtr aiFolder;
    FolderConfigPtr canFolder;
    ComponentPtr syncComponent;

    LoggerPtr logger;
    LoggerComponentPtr loggerComponent;

    bool loggingEnabled;
    StringPtr loggingPath;
    SignalConfigPtr timeSignal;
    StringPtr refDomainId;
    Float globalSampleRate;
    uint64_t samplesGenerated;
    uint64_t deltaT;
};

END_NAMESPACE_REF_DEVICE_MODULE
