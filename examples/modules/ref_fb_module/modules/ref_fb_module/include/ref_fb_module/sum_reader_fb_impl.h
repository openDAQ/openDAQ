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

// Sums equal-rate inputs. The block always offers one free port outside the reader; when a signal connects to it
// the port joins the reader and the next free port is offered. The reader asks the block to judge every value
// descriptor: scalar numeric, and the unit of the inputs already summed. A port that loses its signal is removed.
// Inputs set aside by the reader, rejected ones included, are reported in the component status.
class SumReaderFbImpl final : public ReaderFbBase
{
public:
    explicit SumReaderFbImpl(const ContextPtr& ctx, const ComponentPtr& parent, const StringPtr& localId, const PropertyObjectPtr& config);
    ~SumReaderFbImpl() override = default;

    static FunctionBlockTypePtr CreateType();

private:
    UnitPtr referenceUnit(const ComponentPtr& judged) const;

    bool onChanges(const MultiReader2StatusPtr& status) override;
    bool accepts(const ComponentPtr& input, const DataDescriptorPtr& descriptor) override;
    void processAndSend(SizeT count, SizeT packetOffset) override;
    void rebuildOutputDescriptor() override;

    SignalConfigPtr sumSignal;
    SignalConfigPtr sumDomainSignal;
};

}

END_NAMESPACE_REF_FB_MODULE
