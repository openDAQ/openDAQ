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
#include <ref_template_device_module/common.h>
#include <ref_template_device_module/ref_template_channel_impl.h>
#include <opendaq/channel_impl.h>
#include <opendaq/signal_config_ptr.h>
#include <random>

BEGIN_NAMESPACE_REF_TEMPLATE_DEVICE_MODULE

/*
 * Template CAN channel TODO:
 *  - Refactor code and match it to the template reference channel
 *  - 
 */

struct RefCANChannelInit
{
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

class RefTemplateCANChannelBase final : public templates::ChannelTemplateHooks
{
public:
    RefTemplateCANChannelBase(const templates::ChannelParams& params, const RefCANChannelInit& init);
};

class RefTemplateCANChannelImpl final : public templates::ChannelTemplate
{
public:
    explicit RefTemplateCANChannelImpl(const RefCANChannelInit& init);

    void collectSamples(std::chrono::microseconds curTime);

    static std::string getEpoch();
    static RatioPtr getResolution();

private:
    
    void initProperties() override;
    BaseObjectPtr onPropertyWrite(const templates::PropertyEventArgs& args) override;
    void propertyChanged();
    void onEndUpdate(const templates::UpdateEndArgs& args) override;
    
    void initSignals(const FolderConfigPtr& signalsFolder) override;

    SignalConfigPtr valueSignal;
    SignalConfigPtr timeSignal;

    int32_t lowerLimit;
    int32_t upperLimit;
    int32_t counter1;
    int32_t counter2;
    std::chrono::microseconds microSecondsFromEpochToStartTime;
    std::chrono::microseconds lastCollectTime;

    void generateSamples(int64_t curTime, uint64_t duration, size_t newSamples);
};

END_NAMESPACE_REF_TEMPLATE_DEVICE_MODULE
