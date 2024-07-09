/*
 * Copyright 2022-2024 openDAQ d. o. o.
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
#include <opendaq/block_reader_ptr.h>
#include <opendaq/data_packet_ptr.h>
#include <opendaq/event_packet_ptr.h>
#include <list>

BEGIN_NAMESPACE_REF_FB_MODULE
    
namespace Classifier
{

class ClassifierFbImpl final : public FunctionBlock
{
public:
    explicit ClassifierFbImpl(const ContextPtr& ctx, const ComponentPtr& parent, const StringPtr& localId);
    ~ClassifierFbImpl() override = default;

    static FunctionBlockTypePtr CreateType();

private:
    InputPortPtr inputPort;

    DataDescriptorPtr inputDataDescriptor;
    DataDescriptorPtr inputDomainDataDescriptor;

    DataDescriptorPtr outputDataDescriptor;
    DataDescriptorPtr outputDomainDataDescriptor;

    SignalConfigPtr outputSignal;
    SignalConfigPtr outputDomainSignal;

    bool domainLinear {false};
    SizeT linearBlockCount{1};
    BlockReaderPtr linearReader;

    SizeT blockSize;
    SizeT classCount;
    Float inputDeltaTicks;
    Float inputResolution;

    bool useCustomInputRange;
    Float inputHighValue;
    Float inputLowValue;
    std::string outputName;

    bool useCustomClasses;
    ListPtr<Float> customClassList;

    bool packetGap {false};
    UInt packetStarted {};
    ListPtr<Float> cachedSamples;

    void createInputPorts();
    void createSignals();

    void processData();
    void processLinearData(const std::vector<Float>& inputData, const std::vector<UInt>& inputDomainData);
    void processExplicitData(Float inputData, UInt inputDomainData);

    void processEventPacket(const EventPacketPtr& packet);
    
    bool processSignalDescriptorChanged(const DataDescriptorPtr& inputDataDescriptor,
                                        const DataDescriptorPtr& inputDomainDataDescriptor);

    void configure();

    void initProperties();
    void propertyChanged(bool configure);
    void readProperties();

    inline UInt blockSizeToTimeDuration();

    Int binarySearch(float value, const ListPtr<IBaseObject>& labels);
};

}

END_NAMESPACE_REF_FB_MODULE
