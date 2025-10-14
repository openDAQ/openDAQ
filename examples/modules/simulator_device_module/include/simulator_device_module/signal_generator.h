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
#include <coreobjects/property_object_ptr.h>
#include <random>
#include <opendaq/logger_component_ptr.h>
#include <opendaq/packet_factory.h>
#include <opendaq/signal_config_ptr.h>

BEGIN_NAMESPACE_OPENDAQ

enum class WaveformType { Sine, Rect, None, Counter, ConstantValue };

class SignalGenerator
{
public:
    explicit SignalGenerator();

    PropertyObjectPtr initProperties();
    PacketPtr generateData(const DataPacketPtr& domainPacket);
    DataDescriptorPtr buildDescriptor() const;

    uint64_t sampleRate;
    uint64_t samplesGenerated;
    SignalConfigPtr valueSignal;

private:
    void waveformChanged(PropertyObjectPtr&, PropertyValueEventArgsPtr& args);
    void resetCounter();

    LoggerComponentPtr loggerComponent;

    // User settings
    PropertyObjectPtr generatorSettings;

    // Waveform setup
    WaveformType waveformType;
    double freq;
    double ampl;
    double dc;
    double noiseAmpl;
    double constantValue;
    uint64_t counter;

    std::default_random_engine re;
    std::normal_distribution<double> dist;
};

END_NAMESPACE_OPENDAQ
