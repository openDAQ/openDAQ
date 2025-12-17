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
#include <simulator_device_module/signal_generator.h>
#include <random>

BEGIN_NAMESPACE_SIMULATOR_DEVICE_MODULE

class SimulatorChannelImpl final : public Channel
{
public:
    explicit SimulatorChannelImpl(const ContextPtr& context,
                                  const ComponentPtr& parent,
                                  const StringPtr& localId,
                                  const PropertyObjectPtr& ownerDevice);

protected:
    void onPacketReceived(const InputPortPtr& port) override;

private:
    // Initialization
    void initProperties();
    void createSignals();
    void createDomainSignalInputPort() const;

    // Domain setup
    void dividerChanged(const PropertyValueEventArgsPtr& args);
    void updateAvailableDividers(uint64_t deviceSampleRate) const;
    void configureDomainSettings();

    // Data generation
    void sendData(const DataPacketPtr& domainPacket) const;
    void processEventPacket(const EventPacketPtr& eventPacket);

    // Component references
    SignalConfigPtr valueSignal;
    SignalConfigPtr timeSignal;
    DataDescriptorPtr inputDomainDescriptor;
    WeakRefPtr<IPropertyObject> ownerDevice;

    // Domain setup
    uint16_t sampleRateDivider;
    uint64_t sampleRate;
    uint64_t deltaTicks;

    // Signal data generator
    std::unique_ptr<SignalGenerator> generator;

};

END_NAMESPACE_SIMULATOR_DEVICE_MODULE
