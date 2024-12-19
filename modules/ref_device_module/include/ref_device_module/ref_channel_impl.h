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
#include <opendaq/channel_impl.h>
#include <opendaq/signal_config_ptr.h>
#include <optional>
#include <random>
#include <opendaq_module_template/channel_template.h>

BEGIN_NAMESPACE_REF_DEVICE_MODULE

struct RefChannelInit
{
    size_t index;
    double globalSampleRate;
    std::chrono::microseconds startTime;
    std::chrono::microseconds microSecondsFromEpochToStartTime;
    StringPtr referenceDomainId;
};

class RefChannelBase final : public templates::ChannelTemplateHooks
{
public:
    RefChannelBase(const templates::ChannelParams& params, const RefChannelInit& init);
};

enum class WaveformType { Sine, Rect, None, Counter, ConstantValue };

class RefChannelImpl final : public templates::ChannelTemplate
{
public:
    explicit RefChannelImpl(const RefChannelInit& init);

    void collectSamples(std::chrono::microseconds curTime);
    void globalSampleRateChanged(double newGlobalSampleRate);

    static std::string getEpoch();
    static RatioPtr getResolution();

private:
    void initProperties() override;
    BaseObjectPtr onPropertyWrite(const templates::PropertyEventArgs& args) override;
    BaseObjectPtr onPropertyRead(const templates::PropertyEventArgs& propArgs) override;
    void onEndUpdate(const templates::UpdateEndArgs& args) override;

    void initSignals(const FolderConfigPtr& signalsFolder) override;

    void packetSizeChanged();
    void waveformChanged();
    
    void updateSamplesGenerated();
    void buildSignalDescriptors();
    void setSignalDescriptors() const;
    void updateSignalParams();
    void signalTypeChanged();

    void resetCounter();
    void setCounter(uint64_t cnt, bool shouldLock = true);

    uint64_t getSamplesSinceStart(std::chrono::microseconds time) const;
    std::tuple<PacketPtr, PacketPtr> generateSamples(int64_t curTime, uint64_t samplesGenerated, uint64_t newSamples);

    [[nodiscard]] static Int getDeltaT(double sr);
    [[nodiscard]] static double coerceSampleRate(double wantedSampleRate);

    DataDescriptorPtr valueDescriptor;
    DataDescriptorPtr timeDescriptor;

    WaveformType waveformType;
    double freq;
    double ampl;
    double dc;
    double noiseAmpl;
    double constantValue;
    double sampleRate;
    StructPtr customRange;
    bool clientSideScaling;
    size_t index;
    double globalSampleRate;
    uint64_t counter;
    uint64_t deltaT;
    std::chrono::microseconds startTime;
    std::chrono::microseconds microSecondsFromEpochToStartTime;
    std::chrono::microseconds lastCollectTime;
    uint64_t samplesGenerated;
    std::default_random_engine re;
    std::normal_distribution<double> dist;
    SignalConfigPtr valueSignal;
    SignalConfigPtr timeSignal;
    bool fixedPacketSize;
    uint64_t packetSize;
    StringPtr referenceDomainId;

    std::unordered_set<std::string> signalTypeProps;
    std::unordered_set<std::string> waveformProps;
    std::unordered_set<std::string> packetSizeProps;
};

END_NAMESPACE_REF_DEVICE_MODULE
