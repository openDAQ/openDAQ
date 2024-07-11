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
#include <ref_device_module/ref_channel_impl.h>
#include <opendaq/channel_impl.h>
#include <opendaq/signal_config_ptr.h>
#include <optional>
#include <random>

BEGIN_NAMESPACE_REF_DEVICE_MODULE

struct RefCANChannelInit
{
    std::chrono::microseconds startTime;
    std::chrono::microseconds microSecondsFromEpochToStartTime;
};

#pragma pack(push, 1)
struct CANData
{
    uint32_t arbId;
    uint8_t length;
    uint8_t data[64];
};
#pragma pack(pop)

static_assert(sizeof(CANData) == 69);

class RefCANChannelImpl final : public ChannelImpl<IRefChannel>
{
public:
    explicit RefCANChannelImpl(const ContextPtr& context,
                               const ComponentPtr& parent,
                               const StringPtr& localId,
                               const RefCANChannelInit& init);

    // IRefChannel
    void collectSamples(std::chrono::microseconds curTime) override;
    void globalSampleRateChanged(double globalSampleRate) override;

    static std::string getEpoch();
    static RatioPtr getResolution();
private:
    int32_t lowerLimit;
    int32_t upperLimit;
    int32_t counter1;
    int32_t counter2;
    size_t index;
    double globalSampleRate;
    uint64_t deltaT;
    std::chrono::microseconds startTime;
    std::chrono::microseconds microSecondsFromEpochToStartTime;
    std::chrono::microseconds lastCollectTime;
    uint64_t samplesGenerated;
    SignalConfigPtr valueSignal;
    SignalConfigPtr timeSignal;

    void initProperties();
    void propChangedInternal();
    void propChanged();
    void createSignals();
    void generateSamples(int64_t curTime, uint64_t duration, size_t newSamples);
    void buildSignalDescriptors();
};

END_NAMESPACE_REF_DEVICE_MODULE
