/*
 * Copyright 2022-2026 openDAQ d.o.o.
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
#include <ref_fb_module/reader_fb_base.h>
#include <opendaq/function_block_type_factory.h>
#include <opendaq/signal_config_ptr.h>

BEGIN_NAMESPACE_REF_FB_MODULE

namespace PowerReader
{

// Power from two fixed ports, voltage and current, with the voltage port pinned as the main input. Both inputs
// are needed, so an erroring port stops the block until it recovers.
class PowerReaderFbImpl final : public ReaderFbBase
{
public:
    explicit PowerReaderFbImpl(const ModuleInfoPtr& moduleInfo, const ContextPtr& ctx, const ComponentPtr& parent, const StringPtr& localId);
    ~PowerReaderFbImpl() override = default;

    static FunctionBlockTypePtr CreateType(const ModuleInfoPtr& moduleInfo);

private:
    void initProperties();
    void readProperties();
    RangePtr getValueRange(const DataDescriptorPtr& voltageDescriptor, const DataDescriptorPtr& currentDescriptor) const;

    bool accepts(const ComponentPtr& input, const DataDescriptorPtr& descriptor) override;
    void processAndSend(SizeT count, SizeT packetOffset) override;
    void rebuildOutputDescriptor() override;

    InputPortConfigPtr voltageInputPort;
    InputPortConfigPtr currentInputPort;
    SignalConfigPtr powerSignal;
    SignalConfigPtr powerDomainSignal;

    Float voltageScale = 1.0;
    Float voltageOffset = 0.0;
    Float currentScale = 1.0;
    Float currentOffset = 0.0;
    Float powerHighValue = 10.0;
    Float powerLowValue = -10.0;
    Bool useCustomOutputRange = False;
    RangePtr powerRange;
    bool outputValid = false;
};

}

END_NAMESPACE_REF_FB_MODULE
