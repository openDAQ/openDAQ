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

namespace SumReader
{

class SumReaderFbImpl final : public FunctionBlock
{
public:
    explicit SumReaderFbImpl(const ContextPtr& ctx, const ComponentPtr& parent, const StringPtr& localId);
    ~SumReaderFbImpl() override = default;

    static FunctionBlockTypePtr CreateType();

private:
    std::string getNextPortID() const;
    
    void createSignals();
    void updateInputPorts();
    void updateReader();
    void configure(const DataDescriptorPtr& domainDescriptor, const ListPtr<IDataDescriptor>& valueDescriptors);

    void onConnected(const InputPortPtr& inputPort) override;
    void onDisconnected(const InputPortPtr& inputPort) override;
    void onDataReceived();

    std::vector<InputPortPtr> connectedPorts;
    InputPortPtr disconnectedPort;

    std::unordered_map<std::string, DataDescriptorPtr> cachedDescriptors;
    DataDescriptorPtr sumDataDescriptor;
    DataDescriptorPtr sumDomainDataDescriptor;

    SignalConfigPtr sumSignal;
    SignalConfigPtr sumDomainSignal;
    MultiReaderPtr reader;
};

}

END_NAMESPACE_REF_FB_MODULE
