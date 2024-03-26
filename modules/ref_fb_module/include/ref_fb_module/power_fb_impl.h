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
#include <ref_fb_module/common.h>
#include <opendaq/function_block_ptr.h>
#include <opendaq/function_block_type_factory.h>
#include <opendaq/function_block_impl.h>
#include <opendaq/signal_config_ptr.h>

#include "opendaq/data_packet_ptr.h"

BEGIN_NAMESPACE_REF_FB_MODULE
    namespace Power
{

class PowerFbImpl final : public FunctionBlock
{
public:
    explicit PowerFbImpl(const ContextPtr& ctx, const ComponentPtr& parent, const StringPtr& localId);
    ~PowerFbImpl() override = default;

    void onConnected(const InputPortPtr& port) override;
    void onDisconnected(const InputPortPtr& port) override;

    static FunctionBlockTypePtr CreateType();

private:
    InputPortPtr voltageInputPort;
    InputPortPtr currentInputPort;

    DataDescriptorPtr voltageDataDescriptor;
    DataDescriptorPtr currentDataDescriptor;
    DataDescriptorPtr voltageDomainDataDescriptor;
    DataDescriptorPtr currentDomainDataDescriptor;

    DataDescriptorPtr powerDataDescriptor;
    DataDescriptorPtr powerDomainDataDescriptor;

    SampleType voltageSampleType;
    SampleType currentSampleType;

    SignalConfigPtr powerSignal;
    SignalConfigPtr powerDomainSignal;

    std::deque<DataPacketPtr> voltageQueue;
    std::deque<DataPacketPtr> currentQueue;
    size_t voltagePos;
    size_t currentPos;
    Int curDomainValue;

    bool synced { false };
    Int start;
    Int delta;

    Float voltageScale;
    Float voltageOffset;
    Float currentScale;
    Float currentOffset;
    Float powerHighValue;
    Float powerLowValue;
    Bool useCustomOutputRange;

    void createInputPorts();
    void createSignals();
    void processPackets();
    void checkPacketQueues();
    void processDataPackets();
    RangePtr getValueRange(DataDescriptorPtr voltageDataDescriptor, DataDescriptorPtr currentDataDescriptor);
    void onPacketReceived(const InputPortPtr& port) override;

    void processSignalDescriptorChanged(const DataDescriptorPtr& voltageDataDescriptor,
                                        const DataDescriptorPtr& voltageDomainDataDescriptor,
                                        const DataDescriptorPtr& currentDataDescriptor,
                                        const DataDescriptorPtr& currentDomainDataDescriptor);

    void configure(bool resync);

    template <SampleType VoltageSampleType, SampleType CurrentSampleType>
    void processPacketTemplated();

    void processPacket();
    void initProperties();
    void propertyChanged(bool configure);
    void readProperties();
};

}

END_NAMESPACE_REF_FB_MODULE
