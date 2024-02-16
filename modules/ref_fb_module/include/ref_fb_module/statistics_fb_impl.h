/*
 * Copyright 2022-2023 Blueberry d.o.o.
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
#include <opendaq/data_packet_ptr.h>
#include <opendaq/function_block_impl.h>
#include <opendaq/input_port_config_ptr.h>
#include <opendaq/sample_type_traits.h>
#include <ref_fb_module/common.h>

BEGIN_NAMESPACE_REF_FB_MODULE

namespace Statistics
{

// TODO
class TriggerHistory
{
public:
    void addElement(Bool value, Int domainValue)
    {
        values.push_back(value);
        domainValues.push_back(domainValue);
    }

    void dropHistory(Int toExcludingDomainValue)
    {
        auto whereTo = std::find(values.begin(), values.end(), toExcludingDomainValue);
        if (whereTo != values.end())
        {
            // int index = it - v.begin();

            // values.erase();
            // domainValues.erase(domainValues.begin(), whereTo);
        }
    }

private:
    std::vector<Bool> values;
    std::vector<Int> domainValues;
};

template <SampleType T>
struct AggSample
{
    static constexpr SampleType Type = T;
};

#define DEFINE_AGGREGATE_SAMPLE_TYPE(Input, Agg)            \
    template <>                                             \
    struct AggSample<SampleType::Input>                     \
    {                                                       \
        static constexpr SampleType Type = SampleType::Agg; \
    }

DEFINE_AGGREGATE_SAMPLE_TYPE(UInt8, UInt16);
DEFINE_AGGREGATE_SAMPLE_TYPE(Int8, Int16);
DEFINE_AGGREGATE_SAMPLE_TYPE(UInt16, UInt32);
DEFINE_AGGREGATE_SAMPLE_TYPE(Int16, Int32);
DEFINE_AGGREGATE_SAMPLE_TYPE(UInt32, UInt64);
DEFINE_AGGREGATE_SAMPLE_TYPE(Int32, Int64);
DEFINE_AGGREGATE_SAMPLE_TYPE(Float32, Float64);

enum class DomainSignalType
{
    implicit,
    explicit_,
    explicitRange
};

class StatisticsFbImpl final : public FunctionBlock
{
public:
    StatisticsFbImpl(const ContextPtr& ctx, const ComponentPtr& parent, const StringPtr& localId, const PropertyObjectPtr& config);
    static FunctionBlockTypePtr CreateType();

private:
    struct FreeDeleter
    {
        void operator()(uint8_t* p) const
        {
            free(p);
        }
    };

    bool triggerMode;
    bool doWork;
    FunctionBlockPtr nestedTriggerFunctionBlock;
    InputPortPtr triggerOutput;

    size_t blockSize;
    DomainSignalType domainSignalType;

    SignalConfigPtr avgSignal;
    SignalConfigPtr rmsSignal;
    SignalConfigPtr domainSignal;

    DataDescriptorPtr inputValueDataDescriptor;
    DataDescriptorPtr inputDomainDataDescriptor;
    DataDescriptorPtr outputAverageDataDescriptor;
    DataDescriptorPtr outputRmsDataDescriptor;
    DataDescriptorPtr outputDomainDataDescriptor;

    SampleType sampleType;
    std::unique_ptr<uint8_t, FreeDeleter> calcBuf;

    size_t calcBufSize;
    size_t calcBufAllocatedSize;
    size_t sampleSize;
    size_t domainSampleSize;
    Int start;
    Int inputDeltaTicks;
    Int outputDeltaTicks;
    Int nextExpectedDomainValue;
    bool valid;

    PacketReadyNotification packetReadyNotification;

    void initProperties();
    void propertyChanged();
    void triggerModeChanged();
    void configure();
    void readProperties();

    bool acceptSampleType(SampleType sampleType);
    void checkCalcBuf(size_t newSamples);
    void copyToCalcBuf(uint8_t* buf, size_t sampleCount);
    void copyRemainingCalcBuf(size_t calculatedSampleCount);
    void resetCalcBuf();
    void getNextOutputDomainValue(const DataPacketPtr& domainPacket, NumberPtr& outputPacketStartDomainValue, bool& haveGap);
    void processSignalDescriptorChanged(const DataDescriptorPtr& valueDataDescriptor, const DataDescriptorPtr& domainDataDescriptor);
    void processDataPacketTrigger(const DataPacketPtr& packet);
    void processDataPacketInput(const DataPacketPtr& packet);
    NumberPtr addNumbers(const NumberPtr a, const NumberPtr& b);

    template <SampleType ST,
              SampleType DST,
              SampleType AT = AggSample<ST>::Type,
              class SampleT = typename SampleTypeToType<ST>::Type,
              class AggT = typename SampleTypeToType<AT>::Type,
              class DomainSampleT = typename SampleTypeToType<DST>::Type>
    void calc(SampleT* data, int64_t firstTick, SampleT* outAvgData, SampleT* outRmsData, DomainSampleT* outDomainData, size_t avgCount);

    template <SampleType ST,
              SampleType DST,
              SampleType AT = AggSample<ST>::Type,
              class SampleT = typename SampleTypeToType<ST>::Type,
              class AggT = typename SampleTypeToType<AT>::Type,
              class DomainSampleT = typename SampleTypeToType<DST>::Type>
    void calcUntyped(uint8_t* data, int64_t firstTick, uint8_t* outAvgData, uint8_t* outRmsData, uint8_t* outDomainData, size_t avgCount);

    void calculate(uint8_t* data, int64_t firstTick, uint8_t* outAvgData, uint8_t* outRmsData, uint8_t* outDomainData, size_t avgCount);

    void onPacketReceived(const InputPortPtr& port) override;
    void processTriggerPackets(const InputPortPtr& port);
    void processInputPackets(const InputPortPtr& port);
};

}

END_NAMESPACE_REF_FB_MODULE
