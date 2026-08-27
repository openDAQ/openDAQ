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
#include <cstdint>
#include <memory>
#include <vector>

#include <opendaq/function_block_impl.h>
#include <opendaq/sample_type.h>
#include <ref_fb_module/common.h>

BEGIN_NAMESPACE_REF_FB_MODULE

namespace ConstantValue
{

class ConstantValueFbImpl final : public FunctionBlock
{
public:
    explicit ConstantValueFbImpl(const ModuleInfoPtr& moduleInfo,
                                 const ContextPtr& ctx,
                                 const ComponentPtr& parent,
                                 const StringPtr& localId);
    ~ConstantValueFbImpl() override = default;

    static FunctionBlockTypePtr CreateType(const ModuleInfoPtr& moduleInfo);

    void onPacketReceived(const InputPortPtr& port) override;
    void onConnected(const InputPortPtr& port) override;
    void onDisconnected(const InputPortPtr& port) override;

private:
    void createSignals();
    void createInputPorts();
    void initProperties();

    void configure();
    DataDescriptorPtr buildDescriptor() const;

    template <typename T, typename Source>
    void storeValue(const std::vector<Source>& values);

    void emitValue();
    void processEventPacket(const EventPacketPtr& packet);
    void processDomainPacket(const DataPacketPtr& packet);

    InputPortConfigPtr inputPort;
    SignalConfigPtr outputSignal;
    SignalConfigPtr outputDomainSignal;
    DataDescriptorPtr dataDescriptor;
    DataDescriptorPtr domainDescriptor;
    SampleType sampleType;
    size_t elementCount;
    bool domainConnected;

    std::unique_ptr<uint8_t[]> rawValue;
    size_t rawValueSize;

    bool valueEmitted;
    SampleType emittedSampleType;
    std::unique_ptr<uint8_t[]> emittedRawValue;
    size_t emittedRawValueSize;
};
}

END_NAMESPACE_REF_FB_MODULE
