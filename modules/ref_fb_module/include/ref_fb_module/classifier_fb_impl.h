/*
 * Copyright 2022-2023 Blueberry d.o.o.
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

    SampleType inputSampleType;

    SignalConfigPtr outputSignal;
    SignalConfigPtr outputDomainSignal;

    size_t blockSizeMs;
    size_t classCount;
    Float inputDeltaTicks;
    
    Float inputHighValue;
    Float inputLowValue;
    std::string outputName;

    bool useExplicitDomain;
    ListPtr<Float> explicitDimension;

    UInt packetStarted {};
    size_t sampleStarted {};
    std::list<DataPacketPtr> packets;

    void createInputPorts();
    void createSignals();

    template <SampleType InputSampleType>
    void processDataPacket(const DataPacketPtr& packet);

    void processEventPacket(const EventPacketPtr& packet);
    void onPacketReceived(const InputPortPtr& port) override;

    void processSignalDescriptorChanged(const DataDescriptorPtr& inputDataDescriptor,
                                        const DataDescriptorPtr& inputDomainDataDescriptor);

    void configure();

    void initProperties();
    void propertyChanged(bool configure);
    void readProperties();

    inline UInt timeMs(UInt time);
    inline bool timeInInterval(UInt startTime, UInt endTime);
};

}

END_NAMESPACE_REF_FB_MODULE
