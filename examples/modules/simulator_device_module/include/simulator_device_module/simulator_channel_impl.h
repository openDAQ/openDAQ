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
#include <opendaq/channel_impl.h>
#include <opendaq/signal_config_ptr.h>
#include <opendaq/packet_buffer_ptr.h>
#include <random>

BEGIN_NAMESPACE_SIMULATOR_DEVICE_MODULE

// TODO: Don't change things during update

enum class WaveformType { Sine, Rect, None, Counter, ConstantValue };

class SimulatorChannelImpl final : public Channel
{
public:
    explicit SimulatorChannelImpl(const ContextPtr& context,
                                  const ComponentPtr& parent,
                                  const StringPtr& localId);

protected:
    void endApplyProperties(const UpdatingActions& propsAndValues, bool parentUpdating) override;
    void onPacketReceived(const InputPortPtr& port) override;
    void onConnected(const InputPortPtr& port) override;

private:
    SignalConfigPtr valueSignal; // TODO: SRDiv

    WaveformType waveformType;
    double sampleRate;
    uint64_t deltaT;
    uint64_t samplesGenerated;
    double freq;
    double ampl;
    double dc;
    double noiseAmpl;
    double constantValue;
    StructPtr customRange;
    bool clientSideScaling;
    uint64_t counter;
    SizeT offset;
    std::default_random_engine re;
    std::normal_distribution<double> dist;

    bool needsSignalTypeChanged;

    void sendData(const DataPacketPtr& domainPacket);
    void processEventPacket(const EventPacketPtr& eventPacket);

    void initProperties();
    void createValueSignal();
    void createDomainSignalInputPort() const;

    void waveformChanged();
    void signalTypeChanged();

    void resetCounter();

    PacketPtr generateSamples(uint64_t newSampleCount, const PacketPtr& domainPacket);
    void buildSignalDescriptors() const;
};

END_NAMESPACE_SIMULATOR_DEVICE_MODULE
