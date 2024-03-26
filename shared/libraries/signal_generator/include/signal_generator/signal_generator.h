/*
 * Copyright 2022-2024 Blueberry d.o.o.
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
#include <coretypes/common.h>
#include <opendaq/signal_factory.h>

BEGIN_NAMESPACE_OPENDAQ

class SignalGenerator;
using SignalGeneratorPtr = std::shared_ptr<SignalGenerator>;

class SignalGenerator
{
public:
    using GenerateSampleFunc = std::function<void(uint64_t tick, void* valueOut)>;
    using UpdateGeneratorFunc = std::function<void(SignalGenerator& generator, uint64_t packetOffset)>;

    SignalGenerator(const SignalConfigPtr& signal);

    void setFunction(GenerateSampleFunc function);
    void setUpdateFunction(UpdateGeneratorFunc function);
    void generateSamplesTo(std::chrono::milliseconds currentTime);
    SignalConfigPtr getSignal();

protected:
    void generatePacket(uint64_t startTick, size_t sampleCount);
    void calculateSampleSize();
    void calculateResolutionAndOutputRate();
    void calculateAbsStartTick();
    uint64_t getAbsTick(uint64_t currentTick);

    SignalConfigPtr signal;
    GenerateSampleFunc generateFunc;
    UpdateGeneratorFunc updateFunc;
    uint64_t tick;
    size_t sampleSize{};
    double outputRate = 1000;
    size_t maxPacketSize{};
    RatioPtr resolution;
    uint64_t absStartTick = 0;
};

END_NAMESPACE_OPENDAQ
