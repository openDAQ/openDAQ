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

namespace SumReader
{

// Sums equal-rate inputs. The block always offers one free port, kept unused so its missing signal touches
// nothing. When a signal connects to it the block judges the descriptor, offers the next port, and configures
// once. A used port that loses its signal is removed. Inputs whose descriptor the block does not accept are set
// unused and reported in the component status.
class SumReaderFbImpl final : public ReaderFbBase
{
public:
    explicit SumReaderFbImpl(const ContextPtr& ctx, const ComponentPtr& parent, const StringPtr& localId, const PropertyObjectPtr& config);
    ~SumReaderFbImpl() override = default;

    static FunctionBlockTypePtr CreateType();

private:
    void addFreePort();
    ListPtr<IComponent> portList() const;
    UnitPtr referenceUnit() const;

    bool onChanges(const MultiReader2StatusPtr& status) override;
    bool accepts(const DataDescriptorPtr& descriptor) override;
    void processAndSend(SizeT count, SizeT packetOffset) override;
    void rebuildOutputDescriptor() override;

    InputPortConfigPtr freePort;
    int nextPortId = 1;
    SignalConfigPtr sumSignal;
    SignalConfigPtr sumDomainSignal;
};

}

END_NAMESPACE_REF_FB_MODULE
