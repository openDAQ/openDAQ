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
#include <ref_fb_module/common.h>
#include <opendaq/function_block_ptr.h>
#include <opendaq/function_block_type_factory.h>
#include <opendaq/function_block_impl.h>
#include <opendaq/signal_config_ptr.h>
#include <opendaq/data_packet_ptr.h>
#include <opendaq/multi_reader_ptr.h>

BEGIN_NAMESPACE_REF_FB_MODULE

namespace PowerReader
{

class PowerReaderFbImpl final : public FunctionBlock
{
public:
    explicit PowerReaderFbImpl(const ContextPtr& ctx, const ComponentPtr& parent, const StringPtr& localId);
    ~PowerReaderFbImpl() override = default;

    static FunctionBlockTypePtr CreateType();
    static bool getDataDescriptor(const EventPacketPtr& eventPacket, DataDescriptorPtr& valueDesc);
    static bool getDomainDescriptor(const EventPacketPtr& eventPacket, DataDescriptorPtr& domainDesc);

private:
    InputPortPtr voltageInputPort;
    InputPortPtr currentInputPort;

    DataDescriptorPtr voltageDescriptor;
    DataDescriptorPtr currentDescriptor;
    DataDescriptorPtr domainDescriptor;

    DataDescriptorPtr powerDataDescriptor;
    DataDescriptorPtr powerDomainDataDescriptor;

    SampleType voltageSampleType;
    SampleType currentSampleType;

    SignalConfigPtr powerSignal;
    SignalConfigPtr powerDomainSignal;

    Float voltageScale;
    Float voltageOffset;
    Float currentScale;
    Float currentOffset;
    Float powerHighValue;
    Float powerLowValue;
    Bool useCustomOutputRange;

    MultiReaderPtr reader;

    void createInputPorts();
    void createReader();
    void createSignals();
    static RangePtr getValueRange(DataDescriptorPtr voltageDataDescriptor, DataDescriptorPtr currentDataDescriptor);
    void onDataReceived();

    void configure(const DataDescriptorPtr& domainDescriptor,
                   const DataDescriptorPtr& voltageDescriptor,
                   const DataDescriptorPtr& currentDescriptor);

    void processPacket();
    void initProperties();
    void propertyChanged(bool configure);
    void readProperties();
};

}

END_NAMESPACE_REF_FB_MODULE
