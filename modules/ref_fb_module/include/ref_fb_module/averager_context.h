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
#include <ref_fb_module/common.h>
#include <opendaq/signal_config_ptr.h>
#include <opendaq/data_packet_ptr.h>
#include <opendaq/sample_type_traits.h>
#include <opendaq/function_block_impl.h>

BEGIN_NAMESPACE_REF_FB_MODULE

namespace Averager
{

struct FreeDeleter
{
    void operator()(uint8_t* p) const { free(p); }
};

template <SampleType T>
struct AggSample
{
    static constexpr SampleType Type = T;
};

#define DEFINE_AGGREGATE_SAMPLE_TYPE(Input, Agg) \
template<> struct AggSample<SampleType::Input> { static constexpr SampleType Type = SampleType::Agg; }

DEFINE_AGGREGATE_SAMPLE_TYPE(UInt8, UInt16);
DEFINE_AGGREGATE_SAMPLE_TYPE(Int8, Int16);
DEFINE_AGGREGATE_SAMPLE_TYPE(UInt16, UInt32);
DEFINE_AGGREGATE_SAMPLE_TYPE(Int16, Int32);
DEFINE_AGGREGATE_SAMPLE_TYPE(UInt32, UInt64);
DEFINE_AGGREGATE_SAMPLE_TYPE(Int32, Int64);
DEFINE_AGGREGATE_SAMPLE_TYPE(Float32, Float64);

enum class DomainSignalType { implicit, explicit_, explicitRange };

class AveragerContext;

DECLARE_OPENDAQ_INTERFACE(IAveragerContext, IFunctionBlock)
{
    virtual AveragerContext& getAveragerContext() = 0;
};

class AveragerContext : public FunctionBlockImpl<IAveragerContext>
{
public:
    size_t blockSize;
    DomainSignalType domainSignalType;
    InputPortConfigPtr inputPort;

    AveragerContext(const InputPortNotificationsPtr& inputPortNotifications, const ContextPtr& context, const ComponentPtr& parent, const StringPtr& localId);
    AveragerContext& getAveragerContext() override;

    void onConnected(const InputPortPtr& port) override;
    void onDisconnected(const InputPortPtr& port) override;

    std::mutex& getSync();
    void configure();

    WeakRefPtr<IInputPortNotifications> owner;

private:
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
    Int inputDeltaTicks;
    Int outputDeltaTicks;
    Int nextExpectedDomainValue;
    bool valid;

    bool acceptSampleType(SampleType sampleType);
    void checkCalcBuf(size_t newSamples);
    void copyToCalcBuf(uint8_t* buf, size_t sampleCount);
    void copyRemainingCalcBuf(size_t calculatedSampleCount);
    void resetCalcBuf();
    void getNextOutputDomainValue(const DataPacketPtr& domainPacket, NumberPtr& outputPacketStartDomainValue, bool& haveGap);
    void processSignalDescriptorChanged(const DataDescriptorPtr& valueDataDescriptor, const DataDescriptorPtr& domainDataDescriptor);
    void processDataPacket(const DataPacketPtr& packet);
    NumberPtr addNumbers(const NumberPtr a, const NumberPtr& b);

    template <SampleType ST, SampleType DST, SampleType AT = AggSample<ST>::Type, class SampleT = typename SampleTypeToType<ST>::Type, class AggT = typename SampleTypeToType<AT>::Type, class DomainSampleT = typename SampleTypeToType<DST>::Type>
    void calc(SampleT* data, int64_t firstTick, SampleT* outAvgData, SampleT* outRmsData, DomainSampleT* outDomainData, size_t avgCount);

    template <SampleType ST, SampleType DST, SampleType AT = AggSample<ST>::Type, class SampleT = typename SampleTypeToType<ST>::Type, class AggT = typename SampleTypeToType<AT>::Type, class DomainSampleT = typename SampleTypeToType<DST>::Type>
    void calcUntyped(uint8_t* data, int64_t firstTick, uint8_t* outAvgData, uint8_t* outRmsData, uint8_t* outDomainData, size_t avgCount);

    void calculate(uint8_t* data, int64_t firstTick, uint8_t* outAvgData, uint8_t* outRmsData, uint8_t* outDomainData, size_t avgCount);

    static FunctionBlockTypePtr CreateType();

    void onPacketReceived(const InputPortPtr& port) override;
    void processPackets();
};

}

END_NAMESPACE_REF_FB_MODULE
