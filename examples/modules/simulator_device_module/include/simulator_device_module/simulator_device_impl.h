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
#include <opendaq/device_impl.h>
#include <chrono>
#include <thread>
#include <condition_variable>

BEGIN_NAMESPACE_SIMULATOR_DEVICE_MODULE



class SimulatorDeviceImpl final : public Device
{
public:
    explicit SimulatorDeviceImpl(const PropertyObjectPtr& config, const ContextPtr& ctx, const ComponentPtr& parent, const DeviceInfoPtr& info);
    ~SimulatorDeviceImpl() override;

    static DeviceInfoPtr CreateDeviceInfo(const DictPtr<IString, IBaseObject>& moduleOptions);
    static DeviceTypePtr CreateType();

    void onSRDivChanged(const ChannelPtr& channel, Int newDivider);

protected:
    // IDevice
    uint64_t onGetTicksSinceOrigin() override;
    bool allowAddDevicesFromModules() override;
    bool allowAddFunctionBlocksFromModules() override;
    std::set<OperationModeType> onGetAvailableOperationModes() override;
    void onOperationModeChanged(OperationModeType modeType) override;

private:
    void initProperties();
    void initClock();
    void resetClock();
    void createTimeSignal();
    void updateTimeSignal() const;
    void createChannels(const PropertyObjectPtr& config);
    void updateAcqLoopTime();
    void updateSampleRate(uint64_t newSampleRate);
    void updateDividers();
    void calculateDividerLcm();
    void acqLoop();
    void signalTypeChanged();

    uint64_t sendTimePackets(uint64_t newSampleCount);
    uint64_t getNewSampleCount(const std::chrono::steady_clock::time_point& curLoopTime,
                               const std::chrono::steady_clock::time_point& prevLoopTime,
                               uint64_t remainingSamples) const;

    FolderConfigPtr aiFolder;
    SignalConfigPtr timeSignal;

    size_t acqLoopTime;
    bool stopAcq;

    std::thread acqThread;
    std::condition_variable cv;

    uint64_t ticksSinceEpochToDeviceStart;
    uint64_t samplesGenerated;
    
    uint64_t sampleRate;
    uint64_t deltaTicks;
    uint64_t offset;
    uint64_t dividerLcm;

    RatioPtr resolution;
    UnitPtr domainUnit;
    StringPtr epoch;
};

END_NAMESPACE_SIMULATOR_DEVICE_MODULE
