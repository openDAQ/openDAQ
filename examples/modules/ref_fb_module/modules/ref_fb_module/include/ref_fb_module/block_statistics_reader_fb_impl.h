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

namespace BlockStatisticsReader
{

enum class Statistic : Int
{
    Mean = 0,
    Rms,
    AcRms,
    PeakToPeak,
    CrestFactor
};

// One statistic per block of every connected equal-rate scalar input, each on its own output signal, all on one domain
// signal at the block rate. Blocks start on the samples whose index from the domain origin is a multiple of BlockSize,
// so two blocks reading the same signals agree on their boundaries; each value is stamped with its block's first
// sample. A block never spans a resynchronization or a descriptor change: the partial block is dropped and the next
// one starts on the next boundary. An input that did not contribute to the whole block yields NaN for it. Like the sum
// block it offers a free port, sets an erroring input aside, and removes a port that disconnects, with its output
class BlockStatisticsReaderFbImpl final : public ReaderFbBase
{
public:
    explicit BlockStatisticsReaderFbImpl(const ModuleInfoPtr& moduleInfo, const ContextPtr& ctx, const ComponentPtr& parent, const StringPtr& localId);

    static FunctionBlockTypePtr CreateType(const ModuleInfoPtr& moduleInfo);

private:
    struct Accumulator
    {
        SizeT n = 0;
        double mean = 0;
        double m2 = 0;  // Welford's sum of squared deviations, so a large DC part does not cancel the AC part
        double sumSq = 0;
        double min = 0;
        double max = 0;
        bool spoiled = false;  // the input missed part of the block

        void add(const double* x, SizeT count);
        double value(Statistic statistic) const;
    };

    bool accepts(const ComponentPtr& input, const DataDescriptorPtr& descriptor, const ListPtr<IDataDescriptor>& values) override;
    void onChanges(const MultiReader2StatusPtr& status) override;
    void processAndSend(SizeT count, SizeT packetOffset) override;
    void rebuildOutputDescriptor() override;

    void propertyChanged();
    DataDescriptorPtr statisticDescriptor(const DataDescriptorPtr& input) const;
    SignalConfigPtr outputFor(const ComponentPtr& input);

    SignalConfigPtr domainSignal;
    std::unordered_map<std::string, SignalConfigPtr> outputs;  // by input global ID
    std::unordered_map<std::string, Accumulator> accumulators;  // by input global ID

    Int blockSize = 100;
    Statistic statistic = Statistic::Rms;
    SizeT filled = 0;  // samples of the current block read so far; 0 between blocks
    SizeT blockOffset = 0;  // packetOffset of the current block's first sample
    Int domainDelta = 1;    // the main domain rule's, kept by rebuildOutputDescriptor
    Int domainStart = 0;
};

}

END_NAMESPACE_REF_FB_MODULE
