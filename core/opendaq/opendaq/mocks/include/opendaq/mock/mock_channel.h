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
#include <opendaq/channel_impl.h>
#include "signal_generator/signal_generator.h"

BEGIN_NAMESPACE_OPENDAQ

DECLARE_OPENDAQ_INTERFACE(IMockChannel, IBaseObject)
{
    virtual void generateSamplesUntil(std::chrono::milliseconds currentTime) = 0;
    virtual uint64_t getOutputRate() = 0;
};

class MockChannelImpl : public ChannelImpl<IMockChannel>
{
public:
    explicit MockChannelImpl(daq::ContextPtr ctx, const ComponentPtr& parent, const StringPtr& localId);
protected:
    SignalPtr timeSignal;
    SignalConfigPtr changingTimeSignal;
    SignalConfigPtr changingSignal;
    ListPtr<ISignal> signals;
    daq::ListPtr<daq::IInputPort> inputPorts;
    std::vector<SignalGeneratorPtr> generatedSignals;
    Int intStepValue = -10;
    std::chrono::time_point<std::chrono::system_clock> sigGenAbsStartTime;

    void generateSamplesUntil(std::chrono::milliseconds currentTime) override;
    uint64_t getOutputRate() override;
    void addTimeSignal();
    void addByteStepSignal();
    void addIntStepSignal();
    void addSineSignal();
    void addChangingSignal();
    void addChangingTimeSignal();
    void createSignals();
    void createInputPorts();
};

OPENDAQ_DECLARE_CLASS_FACTORY_WITH_INTERFACE(INTERNAL_FACTORY,
    MockChannel, IChannel,
    IContext*, ctx,
    IComponent*, parent,
    IString*, localId)

END_NAMESPACE_OPENDAQ
