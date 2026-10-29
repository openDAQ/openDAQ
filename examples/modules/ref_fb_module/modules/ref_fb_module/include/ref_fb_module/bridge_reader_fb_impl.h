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
#include <ref_fb_module/common.h>
#include <ref_fb_module/reader_fb_base.h>
#include <opendaq/function_block_type_factory.h>
#include <opendaq/signal_config_ptr.h>

BEGIN_NAMESPACE_REF_FB_MODULE

namespace BridgeReader
{

enum class BridgeType : Int
{
    Quarter = 0,
    HalfBending,
    FullBending
};

enum class OutputQuantity : Int
{
    Strain = 0,
    Ratio
};

// A ratiometric strain-gauge bridge: the bridge output divided by the excitation measured with it, so excitation
// drift cancels, minus the unstrained ratio, converted to strain for the configured bridge. It needs both signals,
// so it waits out errors; the bridge output is the pinned main input. Both ports take scalar numeric signals in V,
// mV or uV, or without a unit, read as V. Samples whose excitation is below MinExcitation yield NaN
class BridgeReaderFbImpl final : public ReaderFbBase
{
public:
    explicit BridgeReaderFbImpl(const ModuleInfoPtr& moduleInfo, const ContextPtr& ctx, const ComponentPtr& parent, const StringPtr& localId);

    static FunctionBlockTypePtr CreateType(const ModuleInfoPtr& moduleInfo);

private:
    bool accepts(const ComponentPtr& input, const DataDescriptorPtr& descriptor, const ListPtr<IDataDescriptor>& values) override;
    void processAndSend(SizeT count, SizeT packetOffset) override;
    void rebuildOutputDescriptor() override;

    void propertyChanged();
    double strain(double ratio) const;  // ratio in V/V, after the zero offset

    InputPortConfigPtr signalPort;
    InputPortConfigPtr excitationPort;
    SignalConfigPtr outputSignal;
    SignalConfigPtr outputDomainSignal;

    BridgeType bridgeType = BridgeType::Quarter;
    OutputQuantity quantity = OutputQuantity::Strain;
    double gaugeFactor = 2;
    double zeroOffset = 0;  // V/V
    double minExcitation = 0.5;  // V
};

}

END_NAMESPACE_REF_FB_MODULE
