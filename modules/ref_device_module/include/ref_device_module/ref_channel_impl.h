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
#include <opendaq/channel_impl.h>
#include <opendaq/signal_config_ptr.h>
#include <optional>
#include <random>

BEGIN_NAMESPACE_REF_DEVICE_MODULE

enum class WaveformType { Sine, Rect, None, Counter, ConstantValue };

DECLARE_OPENDAQ_INTERFACE(IRefChannel, IBaseObject)
{
    virtual void collectSamples(std::chrono::microseconds curTime) = 0;
    virtual void globalSampleRateChanged(double globalSampleRate) = 0;
};

struct RefChannelInit
{
    size_t index;
    double globalSampleRate;
    std::chrono::microseconds startTime;
    std::chrono::microseconds microSecondsFromEpochToStartTime;
    StringPtr referenceDomainId;
};

class RefChannelImpl final : public ChannelImpl<IRefChannel>
{
public:
    explicit RefChannelImpl(const ContextPtr& context,
                            const ComponentPtr& parent,
                            const StringPtr& localId,
                            const RefChannelInit& init);

    // IRefChannel
    void collectSamples(std::chrono::microseconds curTime) override;
    void globalSampleRateChanged(double newGlobalSampleRate) override;
    static std::string getEpoch();
    static RatioPtr getResolution();
    static Int getDeltaT(double sr);
protected:
    void endApplyProperties(const UpdatingActions& propsAndValues, bool parentUpdating) override;

private:
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
    bool needsSignalTypeChanged;
    bool fixedPacketSize;
    uint64_t packetSize;
    StringPtr referenceDomainId;

    void initProperties();
    void packetSizeChangedInternal();
    void packetSizeChanged();
    void waveformChanged();
    void waveformChangedInternal();
    void updateSamplesGenerated();
    void signalTypeChanged();
    void signalTypeChangedInternal();
    void resetCounter();
    void setCounter(uint64_t cnt, bool shouldLock = true);
    uint64_t getSamplesSinceStart(std::chrono::microseconds time) const;
    void createSignals();
    std::tuple<PacketPtr, PacketPtr> generateSamples(int64_t curTime, uint64_t samplesGenerated, uint64_t newSamples);
    void buildSignalDescriptors();
    [[nodiscard]] double coerceSampleRate(const double wantedSampleRate) const;
    void signalTypeChangedIfNotUpdating(const PropertyValueEventArgsPtr& args);
};

END_NAMESPACE_REF_DEVICE_MODULE
