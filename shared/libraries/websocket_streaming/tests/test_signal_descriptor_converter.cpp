#include <memory>

#include "gtest/gtest.h"

#include "nlohmann/json.hpp"

#include "streaming_protocol/Defines.h"
#include "streaming_protocol/SubscribedSignal.hpp"
#include "streaming_protocol/SynchronousSignal.hpp"
#include "streaming_protocol/LinearTimeSignal.hpp"
#include "streaming_protocol/iWriter.hpp"
#include <opendaq/data_rule.h>
#include <opendaq/data_rule_ptr.h>
#include <opendaq/sample_type.h>
#include <opendaq/data_descriptor_factory.h>
#include <opendaq/signal_factory.h>
#include <coreobjects/unit.h>
#include <opendaq/data_descriptor_ptr.h>
#include "websocket_streaming/signal_descriptor_converter.h"
#include <opendaq/range_factory.h>
#include <opendaq/data_rule_factory.h>
#include <opendaq/scaling_factory.h>
#include <coreobjects/unit_factory.h>
#include <opendaq/context_factory.h>
#include <opendaq/context_ptr.h>
#include <opendaq/signal_ptr.h>
#include "streaming_protocol/Logging.hpp"
#include <streaming_protocol/Defines.h>

namespace bsp = daq::streaming_protocol;

BEGIN_NAMESPACE_OPENDAQ_WEBSOCKET_STREAMING

static uint64_t TimeTicksPerSecond = 1000000000;

class DummayWriter : public bsp::iWriter
{
public:
    virtual int writeMetaInformation(unsigned int signalNumber, const nlohmann::json& data) override
    {
        metaInformations.emplace_back(signalNumber, data);
        return 0;
    }
    virtual int writeSignalData(unsigned int, const void*, size_t) override
    {
        return 0;
    }

    virtual std::string id() const override
    {
        static unsigned int id = 0;
        return std::to_string(id);
        ++id;
    }

    std::vector<std::pair<int, nlohmann::json>> metaInformations;
};

SignalPtr createTimeSignal(const ContextPtr& ctx, uint64_t timeTicksPerSecond = TimeTicksPerSecond)
{
    uint64_t timeTickStart = 10000000;
    uint64_t outputRateInTicks = bsp::BaseDomainSignal::timeTicksFromNanoseconds(std::chrono::milliseconds(1), timeTicksPerSecond);

    auto descriptor = DataDescriptorBuilder()
    .setSampleType(SampleType::UInt64)
    .setRule(LinearDataRule(outputRateInTicks, timeTickStart))
    .setTickResolution(Ratio(1, timeTicksPerSecond))
    .setName("Time")
    .build();

    return SignalWithDescriptor(ctx, descriptor, nullptr, descriptor.getName());
}

SignalPtr createSineSignal(const ContextPtr& ctx)
{
    std::string signalId = "Sine";
    std::string memberName = "member name";
    int32_t unitId = 42;
    std::string unitDisplayName = "some unit";

    UnitPtr unit = Unit("V", unitId, "voltage");
    auto descriptor =
        DataDescriptorBuilder().setSampleType(SampleType::Float64).setName(memberName).setName(memberName).setUnit(unit).build();
    
    auto timeSignal = createTimeSignal(ctx);
    auto signal = SignalWithDescriptor(ctx, descriptor, nullptr, signalId);
    signal.setDomainSignal(timeSignal);
    return signal;
}

SignalPtr createSineSignalWithPostScaling(const ContextPtr& ctx)
{
    std::string signalId = "Sine";
    std::string memberName = "member name";
    int32_t unitId = 42;
    std::string unitDisplayName = "some unit";

    double scale = 1;
    double offset = 0;
    UnitPtr unit = Unit("V", unitId, "voltage");

    auto descriptor = DataDescriptorBuilder()
                          .setSampleType(SampleType::Float64)
                          .setName(memberName)
                          .setPostScaling(LinearScaling(scale, offset, SampleType::Int16, ScaledSampleType::Float64))
                          .setUnit(unit)
                          .build();

    auto timeSignal = createTimeSignal(ctx);
    auto signal = SignalWithDescriptor(ctx, descriptor, nullptr, signalId);
    signal.setDomainSignal(timeSignal);
    return signal;
}

TEST(SignalConverter, synchronousSignal)
{
    auto signal = createSineSignal(NullContext());
    auto dataDescriptor = signal.getDescriptor();
    auto unit = dataDescriptor.getUnit();

    auto domainSignal = signal.getDomainSignal();
    auto domainDescriptor = domainSignal.getDescriptor();

    DummayWriter dummyWriter;
    // 1kHz
    uint64_t outputRateInTicks = bsp::BaseDomainSignal::timeTicksFromNanoseconds(std::chrono::milliseconds(1), TimeTicksPerSecond);
    auto outputRateInNs = std::chrono::milliseconds(1);
    auto syncSignal = std::make_shared<bsp::SynchronousSignal<double>>(signal.getGlobalId(), "table1", dummyWriter, bsp::Logging::logCallback());
    auto timeSignal = std::make_shared<bsp::LinearTimeSignal>(domainSignal.getGlobalId(), "table1", TimeTicksPerSecond, outputRateInNs, dummyWriter, bsp::Logging::logCallback());

    ASSERT_NO_THROW(SignalDescriptorConverter::ToStreamedValueSignal(signal, syncSignal, SignalProps{}));
    ASSERT_EQ(syncSignal->getUnitId(), unit.getId());
    ASSERT_EQ(syncSignal->getUnitDisplayName(), unit.getSymbol());
    ASSERT_EQ(syncSignal->getMemberName(), signal.getName());
    ASSERT_EQ(syncSignal->getTableId(), "table1");

    ASSERT_NO_THROW(SignalDescriptorConverter::ToStreamedLinearSignal(domainSignal, timeSignal, SignalProps{}));
    ASSERT_EQ(timeSignal->getTimeDelta(), outputRateInTicks);
    ASSERT_EQ(timeSignal->getTableId(), "table1");

    auto start = domainDescriptor.getRule().getParameters().get("start");
    timeSignal->setTimeStart(start);
    ASSERT_EQ(timeSignal->getTimeStart(), start);
}

TEST(SignalConverter, TickResolution)
{
    auto domainSignal = createTimeSignal(NullContext(), TimeTicksPerSecond);
    auto domainDescriptor = domainSignal.getDescriptor();

    DummayWriter dummyWriter;
    // 1kHz
    auto outputRateInNs = std::chrono::milliseconds(1);
    auto timeSignal = std::make_shared<bsp::LinearTimeSignal>(domainSignal.getGlobalId(), "table1", TimeTicksPerSecond, outputRateInNs, dummyWriter, bsp::Logging::logCallback());

    ASSERT_NO_THROW(SignalDescriptorConverter::ToStreamedLinearSignal(domainSignal, timeSignal, SignalProps{}));
    timeSignal->writeSignalMetaInformation();

    auto getTickResolution = [](const nlohmann::json& data) -> RatioPtr {
        using namespace daq::streaming_protocol;
        auto timeResolution = data[PARAMS][META_DEFINITION][META_RESOLUTION];
        return Ratio(timeResolution[META_NUMERATOR], timeResolution[META_DENOMINATOR]);
    };

    auto lastMetaInformation = *(--dummyWriter.metaInformations.end());
    ASSERT_EQ(getTickResolution(lastMetaInformation.second), Ratio(1, TimeTicksPerSecond));

    auto newDomainSignal = createTimeSignal(NullContext(), 1000);

    ASSERT_NO_THROW(SignalDescriptorConverter::ToStreamedLinearSignal(newDomainSignal, timeSignal, SignalProps{}));
    timeSignal->writeSignalMetaInformation();

    lastMetaInformation = *(--dummyWriter.metaInformations.end());
    ASSERT_EQ(getTickResolution(lastMetaInformation.second), Ratio(1, 1000));
}

TEST(SignalConverter, synchronousSignalWithPostScaling)
{
    auto signal = createSineSignalWithPostScaling(NullContext());
    auto valueDescriptor = signal.getDescriptor();
    auto unit = valueDescriptor.getUnit();

    DummayWriter dummyWriter;
    // 1kHz
    auto syncSignal = std::make_shared<bsp::SynchronousSignal<int16_t>>(signal.getGlobalId(), "table1", dummyWriter, bsp::Logging::logCallback());

    ASSERT_NO_THROW(SignalDescriptorConverter::ToStreamedValueSignal(signal, syncSignal, SignalProps{}));
    ASSERT_EQ(syncSignal->getUnitId(), unit.getId());
    ASSERT_EQ(syncSignal->getUnitDisplayName(), unit.getSymbol());
    ASSERT_EQ(syncSignal->getMemberName(), signal.getName());
}

TEST(SignalConverter, subscribedDataSignal)
{
    std::string method;
    int result;
    unsigned int signalNumber = 3;
    std::string tableId = "table id";
    std::string signalId = "signal id";
    std::string memberName = "This is the measured value";

    nlohmann::json interpretationObject = "just a string";

    int32_t unitId = 42;
    std::string unitDisplayName = "some unit";

    bsp::SubscribedSignal subscribedSignal(signalNumber, bsp::Logging::logCallback());

    // some meta information is to be processed to have the signal described:
    // -subscribe
    // -signal
    nlohmann::json subscribeParams;
    method = bsp::META_METHOD_SUBSCRIBE;
    subscribeParams[bsp::META_SIGNALID] = signalId;
    result = subscribedSignal.processSignalMetaInformation(method, subscribeParams);
    ASSERT_EQ(result, 0);

    nlohmann::json signalParams;
    method = bsp::META_METHOD_SIGNAL;
    signalParams[bsp::META_TABLEID] = tableId;
    signalParams[bsp::META_DEFINITION][bsp::META_NAME] = memberName;
    signalParams[bsp::META_DEFINITION][bsp::META_DATATYPE] = bsp::DATA_TYPE_INT32;
    signalParams[bsp::META_DEFINITION][bsp::META_RULE] = bsp::META_RULETYPE_EXPLICIT;

    signalParams[bsp::META_DEFINITION][bsp::META_UNIT][bsp::META_UNIT_ID] = unitId;
    signalParams[bsp::META_DEFINITION][bsp::META_UNIT][bsp::META_DISPLAY_NAME] = unitDisplayName;
    signalParams[bsp::META_INTERPRETATION] = interpretationObject;
    result = subscribedSignal.processSignalMetaInformation(method, signalParams);
    ASSERT_EQ(result, 0);
    ASSERT_FALSE(subscribedSignal.isTimeSignal());

    auto subscribedSignalInfo = SignalDescriptorConverter::ToDataDescriptor(subscribedSignal);
    auto dataDescriptor = subscribedSignalInfo.dataDescriptor;
    ASSERT_EQ(subscribedSignalInfo.signalName, memberName);

    ASSERT_EQ(dataDescriptor.getSampleType(), daq::SampleType::Int32);

    auto unit = dataDescriptor.getUnit();
    ASSERT_TRUE(unit.assigned());
    ASSERT_EQ(unit.getId(), unitId);
    ASSERT_EQ(unit.getSymbol(), unitDisplayName);

    auto rule = dataDescriptor.getRule();
    ASSERT_TRUE(rule.assigned());
    ASSERT_EQ(daq::DataRuleType::Explicit, rule.getType());

    // test default post scaling
    auto postScaling = dataDescriptor.getPostScaling();
    ASSERT_FALSE(postScaling.assigned());

    // test default range
    auto range = dataDescriptor.getValueRange();
    ASSERT_TRUE(range.assigned());
    ASSERT_EQ(range.getLowValue(), std::numeric_limits<int32_t>::lowest());
    ASSERT_EQ(range.getHighValue(), std::numeric_limits<int32_t>::max());

    // test custom range and post scaling
    signalParams[bsp::META_DEFINITION][bsp::META_RANGE][bsp::META_LOW] = -100;
    signalParams[bsp::META_DEFINITION][bsp::META_RANGE][bsp::META_HIGH] = 100;

    signalParams[bsp::META_DEFINITION][bsp::META_POSTSCALING][bsp::META_SCALE] = 2.0;
    signalParams[bsp::META_DEFINITION][bsp::META_POSTSCALING][bsp::META_POFFSET] = 3.0;

    result = subscribedSignal.processSignalMetaInformation(method, signalParams);
    ASSERT_EQ(result, 0);

    subscribedSignalInfo = SignalDescriptorConverter::ToDataDescriptor(subscribedSignal);
    dataDescriptor = subscribedSignalInfo.dataDescriptor;

    range = dataDescriptor.getValueRange();
    ASSERT_TRUE(range.assigned());
    ASSERT_EQ(range.getLowValue(), -100);
    ASSERT_EQ(range.getHighValue(), 100);

    postScaling = dataDescriptor.getPostScaling();
    ASSERT_TRUE(postScaling.assigned());
    ASSERT_EQ(dataDescriptor.getSampleType(), daq::SampleType::Float64);
    ASSERT_EQ(postScaling.getParameters().get("Scale"), 2.0);
    ASSERT_EQ(postScaling.getParameters().get("Offset"), 3.0);
}

TEST(SignalConverter, subscribedBitfieldSignal)
{
    std::string method;
    int result;
    unsigned int signalNumber = 3;
    std::string tableId = "table id";
    std::string signalId = "signal id";
    std::string memberName = "This is the measured value";

    nlohmann::json bitsInterpretationObject =
        R"([{"Description": "Data overrun","index": 0,"uuid": "c214c128-2447-4cee-ba39-6227aed2eff4"}])"_json;

    bsp::SubscribedSignal subscribedSignal(signalNumber, bsp::Logging::logCallback());

    // some meta information is to be processed to have the signal described:
    // -subscribe
    // -signal
    nlohmann::json subscribeParams;
    method = bsp::META_METHOD_SUBSCRIBE;
    subscribeParams[bsp::META_SIGNALID] = signalId;
    result = subscribedSignal.processSignalMetaInformation(method, subscribeParams);
    ASSERT_EQ(result, 0);

    nlohmann::json signalParams;
    method = bsp::META_METHOD_SIGNAL;
    signalParams[bsp::META_TABLEID] = tableId;
    signalParams[bsp::META_DEFINITION][bsp::META_NAME] = memberName;
    signalParams[bsp::META_DEFINITION][bsp::META_DATATYPE] = bsp::DATA_TYPE_BITFIELD;
    signalParams[bsp::META_DEFINITION][bsp::DATA_TYPE_BITFIELD]["bits"] =
        bitsInterpretationObject;
    signalParams[bsp::META_DEFINITION][bsp::DATA_TYPE_BITFIELD][bsp::META_DATATYPE] =
        bsp::DATA_TYPE_UINT64;
    signalParams[bsp::META_DEFINITION][bsp::META_RULE] = bsp::META_RULETYPE_CONSTANT;

    result = subscribedSignal.processSignalMetaInformation(method, signalParams);
    ASSERT_EQ(result, 0);
    ASSERT_FALSE(subscribedSignal.isTimeSignal());
    auto subscribedSignalInfo = SignalDescriptorConverter::ToDataDescriptor(subscribedSignal);
    auto dataDescriptor = subscribedSignalInfo.dataDescriptor;
    ASSERT_EQ(subscribedSignalInfo.signalName, memberName);

    ASSERT_EQ(dataDescriptor.getSampleType(), daq::SampleType::UInt64);

    auto rule = dataDescriptor.getRule();
    ASSERT_TRUE(rule.assigned());
    ASSERT_EQ(daq::DataRuleType::Constant, rule.getType());

    ASSERT_TRUE(dataDescriptor.getMetadata().hasKey("bits"));
    ASSERT_EQ(dataDescriptor.getMetadata().get("bits"), bitsInterpretationObject.dump());
}

TEST(SignalConverter, subscribedTimeSignal)
{
    std::string method;
    int result;
    unsigned int signalNumber = 3;
    std::string tableId = "table id";
    std::string signalId = "signal id";
    std::string memberName = "This is the time";

    uint64_t ticksPerSecond = 10000000;
    uint64_t linearDelta = 1000;
    int32_t unitId = bsp::Unit::UNIT_ID_SECONDS;
    std::string unitDisplayName = "s";

    bsp::SubscribedSignal subscribedSignal(signalNumber, bsp::Logging::logCallback());

    // some meta information is to be processed to have the signal described:
    // -subscribe
    // -signal
    // -set the time
    nlohmann::json subscribeParams;
    method = bsp::META_METHOD_SUBSCRIBE;

    subscribeParams[bsp::META_SIGNALID] = signalId;
    result = subscribedSignal.processSignalMetaInformation(method, subscribeParams);
    ASSERT_EQ(result, 0);

    nlohmann::json timeSignalParams;
    method = bsp::META_METHOD_SIGNAL;

    timeSignalParams[bsp::META_TABLEID] = tableId;
    timeSignalParams[bsp::META_DEFINITION][bsp::META_NAME] = memberName;

    timeSignalParams[bsp::META_DEFINITION][bsp::META_RULE] = bsp::META_RULETYPE_LINEAR;

    timeSignalParams[bsp::META_DEFINITION][bsp::META_RULETYPE_LINEAR][bsp::META_DELTA] = linearDelta;
    timeSignalParams[bsp::META_DEFINITION][bsp::META_DATATYPE] = bsp::DATA_TYPE_UINT64;

    timeSignalParams[bsp::META_DEFINITION][bsp::META_UNIT][bsp::META_UNIT_ID] = unitId;
    timeSignalParams[bsp::META_DEFINITION][bsp::META_UNIT][bsp::META_DISPLAY_NAME] = unitDisplayName;
    timeSignalParams[bsp::META_DEFINITION][bsp::META_UNIT][bsp::META_QUANTITY] = bsp::META_TIME;

    timeSignalParams[bsp::META_DEFINITION][bsp::META_ABSOLUTE_REFERENCE] = bsp::UNIX_EPOCH;
    timeSignalParams[bsp::META_DEFINITION][bsp::META_RESOLUTION][bsp::META_NUMERATOR] = 1;
    timeSignalParams[bsp::META_DEFINITION][bsp::META_RESOLUTION][bsp::META_DENOMINATOR] = ticksPerSecond;
    result = subscribedSignal.processSignalMetaInformation(method, timeSignalParams);
    ASSERT_EQ(result, 0);
    ASSERT_TRUE(subscribedSignal.isTimeSignal());

    auto subscribedSignalInfo = SignalDescriptorConverter::ToDataDescriptor(subscribedSignal);
    auto dataDescriptor = subscribedSignalInfo.dataDescriptor;
    ASSERT_EQ(subscribedSignalInfo.signalName, memberName);

    ASSERT_EQ(dataDescriptor.getSampleType(), daq::SampleType::UInt64);

    auto unit = dataDescriptor.getUnit();
    ASSERT_TRUE(unit.assigned());
    ASSERT_EQ(unit.getId(), unitId);
    ASSERT_EQ(unit.getSymbol(), unitDisplayName);

    auto rule = dataDescriptor.getRule();
    ASSERT_TRUE(rule.assigned());
    ASSERT_EQ(daq::DataRuleType::Linear, rule.getType());
    DictPtr<IString, IBaseObject> params = rule.getParameters();
    ASSERT_EQ(params.getCount(), 2u);
    uint64_t resultDelta = params.get("delta");
    uint64_t resultStart = params.get("start");
    ASSERT_EQ(resultDelta, linearDelta);
    ASSERT_EQ(resultStart, 0);
}

END_NAMESPACE_OPENDAQ_WEBSOCKET_STREAMING
