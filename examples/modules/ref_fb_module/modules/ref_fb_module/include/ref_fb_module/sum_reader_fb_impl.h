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

namespace SumReader
{

// Sums equal-rate scalar inputs. It always offers one free port; an input whose descriptor is rejected or that the reader
// cannot align is set aside and the sum goes on without it, and a port that disconnects is removed
class SumReaderFbImpl final : public ReaderFbBase
{
public:
    explicit SumReaderFbImpl(const ContextPtr& ctx, const ComponentPtr& parent, const StringPtr& localId, const PropertyObjectPtr& config);

    static FunctionBlockTypePtr CreateType();

private:
    bool accepts(const ComponentPtr& input, const DataDescriptorPtr& descriptor, const ListPtr<IDataDescriptor>& values) override;
    void onChanges(const MultiReader2StatusPtr& status) override;
    void processAndSend(SizeT count, SizeT packetOffset) override;
    void rebuildOutputDescriptor() override;

    SignalConfigPtr sumSignal;
    SignalConfigPtr sumDomainSignal;
};

}

END_NAMESPACE_REF_FB_MODULE
