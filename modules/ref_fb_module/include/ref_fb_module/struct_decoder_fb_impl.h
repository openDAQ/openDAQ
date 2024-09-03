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
#include <ref_fb_module/common.h>
#include <opendaq/function_block_ptr.h>
#include <opendaq/function_block_type_factory.h>
#include <opendaq/function_block_impl.h>
#include <opendaq/signal_config_ptr.h>
#include <opendaq/data_packet_ptr.h>
#include <opendaq/event_packet_ptr.h>

BEGIN_NAMESPACE_REF_FB_MODULE
    
namespace StructDecoder
{

class StructDecoderFbImpl final : public FunctionBlock
{
public:
    explicit StructDecoderFbImpl(const ContextPtr& ctx, const ComponentPtr& parent, const StringPtr& localId);
    ~StructDecoderFbImpl() override = default;

    static FunctionBlockTypePtr CreateType();

    void onPacketReceived(const InputPortPtr& port) override;
    void onDisconnected(const InputPortPtr& port) override;

private:
    InputPortPtr inputPort;

    DataDescriptorPtr inputDataDescriptor;
    DataDescriptorPtr inputDomainDataDescriptor;

    size_t structSize;

    bool configured;

    void createInputPorts();

    void processDataPacket(const DataPacketPtr& packet) const;

    void processEventPacket(const EventPacketPtr& packet);

    void processSignalDescriptorChanged(const DataDescriptorPtr& inputDataDescriptor,
                                        const DataDescriptorPtr& inputDomainDataDescriptor);

    void configure();

    void initStatuses() const;
    void setInputStatus(const StringPtr& value) const;

    template <typename T>
    static void copySample(uint8_t* dest, const uint8_t* source);

    void copySamples(uint8_t* dest, uint8_t* source, const size_t fieldSampleSize, size_t sampleCount) const;
};

}

END_NAMESPACE_REF_FB_MODULE
