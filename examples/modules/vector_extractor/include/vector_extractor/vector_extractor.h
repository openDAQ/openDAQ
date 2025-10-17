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
#include <opendaq/function_block_impl.h>
#include <opendaq/event_packet_utils.h>
#include <opendaq/opendaq.h>
#include <vector_extractor/common.h>

BEGIN_NAMESPACE_VECTOR_EXTRACTOR_MODULE

static const char* InputDisconnected = "Disconnected";
static const char* InputConnected = "Connected";
static const char* InputInvalid = "Invalid";

class VectorExtractorImpl final : public daq::FunctionBlock
{
public:
    explicit VectorExtractorImpl(const daq::ContextPtr& ctx,
                                                const daq::ComponentPtr& parent,
                                                const daq::StringPtr& localId);

    ~VectorExtractorImpl() override = default;

    static daq::FunctionBlockTypePtr CreateType();

    static bool descriptorNotNull(const DataDescriptorPtr& descriptor);
    static bool getDataDescriptor(const EventPacketPtr& eventPacket, DataDescriptorPtr& valueDesc, DataDescriptorPtr& domainDesc);
    static bool getDomainDescriptor(const EventPacketPtr& eventPacket, DataDescriptorPtr& domainDesc);

private:


    void onDisconnected(const daq::InputPortPtr& port) override;

    daq::InputPortPtr inputPort;

    daq::DataDescriptorPtr inputDataDescriptor;
    daq::DataDescriptorPtr inputDomainDescriptor;

    daq::DataDescriptorPtr outputDataDescriptor;
    daq::DataDescriptorPtr outputDomainDescriptor;

    daq::SignalConfigPtr outputSignal;
    daq::SignalConfigPtr outputDomainSignal;

    size_t structSize;

    SampleType outputType;
    uint8_t beginningIndex;

    StreamReaderPtr reader;

    bool configured;

    void initProperties();

    void initStatues() const;

    void createInputPorts();

    void propertyChanged(bool configure);

    void readProperties();

    void createSignals();

    void createReader();

    void configure();

    void onDataReceived();

    void setInputStatus(const daq::StringPtr& value) const;

    template <typename T>
    static void copySample(uint8_t* dest, const uint8_t* source);

    void copySamples(uint8_t* dest, uint8_t* source, const size_t fieldSampleSize, size_t sampleCount) const;

    template<typename T>
    void castingFunction(const DataDescriptorPtr& dataDescriptor, void* bufferData, void* destinationData, SizeT count);
};

END_NAMESPACE_VECTOR_EXTRACTOR_MODULE

