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
#include <opendaq/function_block_type_factory.h>
#include <opendaq/signal_config_ptr.h>
#include <ref_fb_module/common.h>
#include <ref_fb_module/reader_fb_base.h>

BEGIN_NAMESPACE_REF_FB_MODULE

namespace PowerReader
{

// Multiplies a voltage and a current input. It needs both, so it waits out errors; the voltage port is the pinned main
// input, and a voltage signal in another unit than volts makes the reader invalid
class PowerReaderFbImpl final : public ReaderFbBase
{
public:
    explicit PowerReaderFbImpl(const ModuleInfoPtr& moduleInfo,
                               const ContextPtr& ctx,
                               const ComponentPtr& parent,
                               const StringPtr& localId);

    static FunctionBlockTypePtr CreateType(const ModuleInfoPtr& moduleInfo);

private:
    bool accepts(const ComponentPtr& input, const DataDescriptorPtr& descriptor, const ListPtr<IDataDescriptor>& values) override;
    void processAndSend(SizeT count, SizeT packetOffset) override;
    void rebuildOutputDescriptor() override;

    void initProperties();
    void propertyChanged(bool rebuild);
    void readProperties();
    RangePtr valueRange(const DataDescriptorPtr& voltage, const DataDescriptorPtr& current) const;

    InputPortConfigPtr voltageInputPort;
    InputPortConfigPtr currentInputPort;
    SignalConfigPtr powerSignal;
    SignalConfigPtr powerDomainSignal;

    Float voltageScale = 1;
    Float voltageOffset = 0;
    Float currentScale = 1;
    Float currentOffset = 0;
    Float powerHighValue = 10;
    Float powerLowValue = -10;
    Bool useCustomOutputRange = false;
};

}

END_NAMESPACE_REF_FB_MODULE
