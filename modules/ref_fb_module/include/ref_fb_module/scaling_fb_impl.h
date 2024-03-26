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
#include "opendaq/event_packet_ptr.h"

BEGIN_NAMESPACE_REF_FB_MODULE
    
namespace Scaling
{

class ScalingFbImpl final : public FunctionBlock
{
public:
    explicit ScalingFbImpl(const ContextPtr& ctx, const ComponentPtr& parent, const StringPtr& localId);
    ~ScalingFbImpl() override = default;

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

    Float scale;
    Float offset;
    Float outputHighValue;
    Float outputLowValue;
    Bool useCustomOutputRange;
    std::string outputUnit;
    std::string outputName;

    void createInputPorts();
    void createSignals();

    template <SampleType InputSampleType>
    void processDataPacket(const DataPacketPtr& packet);

    void processEventPacket(const EventPacketPtr& packet);
    void onPacketReceived(const InputPortPtr& port) override;
    void onDisconnected(const InputPortPtr& port) override;

    void processSignalDescriptorChanged(const DataDescriptorPtr& inputDataDescriptor,
                                        const DataDescriptorPtr& inputDomainDataDescriptor);

    void configure();

    void initProperties();
    void propertyChanged(bool configure);
    void readProperties();

    void initStatuses();
    void setInputStatus(const StringPtr& value);
};

}

END_NAMESPACE_REF_FB_MODULE
