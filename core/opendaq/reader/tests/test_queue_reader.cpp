#include <gtest/gtest.h>

#include <coreobjects/unit_factory.h>
#include <coretypes/exceptions.h>
#include <coretypes/ratio_factory.h>

#include <opendaq/data_descriptor_factory.h>
#include <opendaq/data_rule_factory.h>
#include <opendaq/domain_value.h>
#include <opendaq/event_packet_utils.h>
#include <opendaq/input_port_factory.h>
#include <opendaq/packet_factory.h>
#include <opendaq/queue_reader.h>
#include <opendaq/reader_utils.h>
#include <opendaq/typed_reading_utils.h>
#include "reader_common.h"

#include <chrono>
#include <thread>

class QueueReaderTest : public ReaderTest<>
{
public:
    using Super = ReaderTest<>;

protected:
    void SetUp() override
    {
        Super::SetUp();

        domainSignal = Signal(context, nullptr, "timeSig");
        signal.setDomainSignal(domainSignal);
    }

    void setDomainDescriptor(const DataDescriptorPtr& descriptor)
    {
        domainSignal.setDescriptor(descriptor);
    }

    void setValueDescriptor(const DataDescriptorPtr& descriptor)
    {
        signal.setDescriptor(descriptor);
    }

    void setPacketSize(SizeT size)
    {
        packetSize = size;
    }

    void setOffsetDelta(Int newOffset, Int newDelta)
    {
        offset = newOffset;
        delta = newDelta;
        samples = 0;
    }

    Int getOffset() const
    {
        return offset;
    }

    void sendNextPacket()
    {
        const auto domainPacket = DataPacket(domainSignal.getDescriptor(), packetSize, offset);
        const auto valuePacket = DataPacketWithDomain(domainPacket, signal.getDescriptor(), packetSize);
        auto* d = static_cast<float*>(valuePacket.getRawData());
        for (size_t i = 0; i < packetSize; ++i)
        {
            d[i] = static_cast<double>(samples + i);
        }

        domainSignal.sendPacket(domainPacket);
        signal.sendPacket(valuePacket);

        samples += packetSize;
        offset += packetSize * delta;
    }

protected:
    Int offset = 0;
    Int delta = 1;
    SizeT packetSize = 1;

    SizeT samples = 0;

    SignalConfigPtr domainSignal;
};

void assertReaderAtDomainValue(QueueReader& reader, Int tick)
{
    auto start = reader.getFirstSampleDomainValue();
    auto* startP = dynamic_cast<DomainValueImpl<Int>*>(start.get());
    ASSERT_EQ(startP->getValue(), tick);
}

TEST_F(QueueReaderTest, AdvancePastEnd)
{
    // Domain (time) signal: Int64, linear rule.
    constexpr Int sampleRate = 10000;
    setDomainDescriptor(DataDescriptorBuilder()
                            .setSampleType(SampleType::Int64)
                            .setTickResolution(Ratio(1, sampleRate))
                            .setOrigin("2022-09-27T00:02:03+00:00")
                            .setRule(LinearDataRule(1, 0))
                            .setUnit(Unit("s", -1, "second", "time"))
                            .build());

    setValueDescriptor(DataDescriptorBuilder().setSampleType(SampleType::Float64).build());

    setOffsetDelta(500, 1);

    const size_t packetSize = 5;
    setPacketSize(packetSize);

    auto inputPort = InputPort(context, nullptr, "port", true);
    inputPort.connect(signal);

    QueueReader reader = QueueReader(inputPort, SampleType::Float64, SampleType::Int64, ReadMode::Scaled, loggerComponent, false);

    for (int c = 0; c < 3; ++c)
    {
        sendNextPacket();
    }

    ASSERT_TRUE(reader.hasPendingEvents());

    ASSERT_EQ(reader.getAvailableSamples(), 3u * packetSize);

    auto start = reader.getFirstSampleDomainValue();
    auto* startP = dynamic_cast<DomainValueImpl<Int>*>(start.get());

    ASSERT_TRUE(startP != nullptr);

    ASSERT_EQ(startP->getValue(), 500);

    std::unique_ptr<DomainValue> domainValue = std::make_unique<DomainValueImpl<Int>>(reader.getDomainInfo(), 512);
    AdvanceResult result = reader.advanceToDomainValue(domainValue.get());
    ASSERT_EQ(result, AdvanceResult::Success);

    auto start2 = reader.getFirstSampleDomainValue();
    ASSERT_EQ(*domainValue, *start2);

    ASSERT_TRUE(reader.hasPendingEvents());
    auto event = reader.popFrontEvent();
    ASSERT_EQ(event.getType(), PacketType::Event);
    auto params = event.getParameters();
    const DataDescriptorPtr domainFromEvent = params[event_packet_param::DOMAIN_DATA_DESCRIPTOR];
    ASSERT_EQ(domainFromEvent.getTickResolution().getDenominator(), sampleRate);
    ASSERT_FALSE(reader.hasPendingEvents());

    domainValue = std::make_unique<DomainValueImpl<Int>>(reader.getDomainInfo(), 100512);
    result = reader.advanceToDomainValue(domainValue.get());
    ASSERT_EQ(result, AdvanceResult::NeedMoreData);

    start2 = reader.getFirstSampleDomainValue();
    ASSERT_EQ(start2.get(), nullptr);
}

TEST_F(QueueReaderTest, DomainChangeDetection)
{
    // Domain (time) signal: Int64, linear rule.
    constexpr Int sampleRate = 10000;
    setDomainDescriptor(DataDescriptorBuilder()
                            .setSampleType(SampleType::Int64)
                            .setTickResolution(Ratio(1, sampleRate))
                            .setOrigin("1970-01-01T00:00:00+00:00")
                            .setRule(LinearDataRule(1, 0))
                            .setUnit(Unit("s", -1, "second", "time"))
                            .build());

    setValueDescriptor(DataDescriptorBuilder().setSampleType(SampleType::Float64).build());

    setOffsetDelta(500, 1);

    const size_t packetSize = 5;
    setPacketSize(packetSize);

    auto inputPort = InputPort(context, nullptr, "port", true);
    inputPort.connect(signal);

    QueueReader reader = QueueReader(inputPort, SampleType::Float64, SampleType::Int64, ReadMode::Scaled, loggerComponent, false);

    sendNextPacket();

    ASSERT_TRUE(reader.hasPendingEvents());

    ASSERT_EQ(reader.getAvailableSamples(), packetSize);

    auto start = reader.getFirstSampleDomainValue();
    auto* startP = dynamic_cast<DomainValueImpl<Int>*>(start.get());

    ASSERT_TRUE(startP != nullptr);

    ASSERT_EQ(startP->getValue(), 500);

    std::unique_ptr<DomainValue> domainValue = std::make_unique<DomainValueImpl<Int>>(reader.getDomainInfo(), 512);
    AdvanceResult result = reader.advanceToDomainValue(domainValue.get());
    ASSERT_EQ(result, AdvanceResult::NeedMoreData);

    auto start2 = reader.getFirstSampleDomainValue();
    ASSERT_TRUE(start2 == nullptr);

    sendNextPacket();
    sendNextPacket();

    domainValue = std::make_unique<DomainValueImpl<Int>>(reader.getDomainInfo(), 512);
    result = reader.advanceToDomainValue(domainValue.get());
    ASSERT_EQ(result, AdvanceResult::Success);

    // The descriptor change event when the sig was connected to port
    ASSERT_TRUE(reader.hasPendingEvents());
    auto eventPacket = reader.popFrontEvent();
    ASSERT_FALSE(reader.hasPendingEvents());
    ASSERT_EQ(reader.getSampleRate(), sampleRate);

    setDomainDescriptor(DataDescriptorBuilder()
                            .setSampleType(SampleType::Int64)
                            .setTickResolution(Ratio(1, sampleRate))
                            .setOrigin("1970-01-01T00:00:00+00:00")
                            .setRule(LinearDataRule(10, 0))
                            .setUnit(Unit("s", -1, "second", "time"))
                            .build());

    setOffsetDelta(getOffset(), 10);

    ASSERT_EQ(reader.getAvailableSamples(), 3);

    domainValue = std::make_unique<DomainValueImpl<Int>>(reader.getDomainInfo(), 525);
    result = reader.advanceToDomainValue(domainValue.get());
    ASSERT_EQ(result, AdvanceResult::DomainChanged);

    // Event from changing the descriptor mid operation
    ASSERT_TRUE(reader.hasPendingEvents());

    eventPacket = reader.popFrontEvent();
    ASSERT_FALSE(reader.hasPendingEvents());
    ASSERT_EQ(reader.getSampleRate(), sampleRate / 10);
    ASSERT_TRUE(reader.isValid());
}

TEST_F(QueueReaderTest, OriginParsing)
{
    std::string origin = "1970-01-01T00:01:00+0000";
    auto epoch = reader::tryParseEpoch(origin);
    ASSERT_TRUE(epoch.has_value());

    origin = "1970-01-01T00:01:00+00:00";
    epoch = reader::tryParseEpoch(origin);
    ASSERT_TRUE(epoch.has_value());

    origin = "abc";
    epoch = reader::tryParseEpoch(origin);
    ASSERT_FALSE(epoch.has_value());

    origin = "1970-01-01T00:01:00+00:00abc";
    epoch = reader::tryParseEpoch(origin);
    ASSERT_FALSE(epoch.has_value());
}

TEST_F(QueueReaderTest, CreateBeforeConnection)
{
    auto inputPort = InputPort(context, nullptr, "port", true);

    QueueReader reader = QueueReader(inputPort, SampleType::Float64, SampleType::Int64, ReadMode::Scaled, loggerComponent, false);
    std::unique_ptr<DomainValue> domainValue =
        std::make_unique<DomainValueImpl<Int>>(DomainInfo{std::chrono::system_clock::time_point{}, Ratio(1, 1000)}, 512);

    bool valid;
    ASSERT_NO_THROW(valid = reader.isValid());
    ASSERT_FALSE(valid);

    ASSERT_THROW(reader.getDomainInfo(), InvalidOperationException);
    ASSERT_THROW(reader.getFirstSampleDomainValue(), InvalidOperationException);
    ASSERT_THROW(reader.advanceToDomainValue(domainValue.get()), InvalidOperationException);
    ASSERT_THROW(reader.getSampleRate(), InvalidOperationException);
    ASSERT_THROW(reader.dropOutdatedDomainSegments(), InvalidOperationException);
    ASSERT_THROW(reader.hasPendingEvents(), InvalidOperationException);
    ASSERT_THROW(reader.popFrontEvent(), InvalidOperationException);
}

TEST_F(QueueReaderTest, CreateBeforeConnectionRecovery)
{
    // Domain (time) signal: Int64, linear rule.
    constexpr Int sampleRate = 10000;
    setDomainDescriptor(DataDescriptorBuilder()
                            .setSampleType(SampleType::Int64)
                            .setTickResolution(Ratio(1, sampleRate))
                            .setOrigin("1970-01-01T00:00:00+00:00")
                            .setRule(LinearDataRule(1, 0))
                            .setUnit(Unit("s", -1, "second", "time"))
                            .build());

    setValueDescriptor(DataDescriptorBuilder().setSampleType(SampleType::Float64).build());

    setOffsetDelta(500, 1);

    const size_t packetSize = 5;
    setPacketSize(packetSize);

    auto inputPort = InputPort(context, nullptr, "port", true);

    QueueReader reader = QueueReader(inputPort, SampleType::Float64, SampleType::Int64, ReadMode::Scaled, loggerComponent, false);

    ASSERT_FALSE(reader.isValid());
    ASSERT_THROW(reader.getDomainInfo(), InvalidOperationException);

    inputPort.connect(signal);
    reader.updateConnection();

    std::unique_ptr<DomainValue> domainValue =
        std::make_unique<DomainValueImpl<Int>>(DomainInfo{std::chrono::system_clock::time_point{}, Ratio(1, 1000)}, 512);

    bool valid;
    ASSERT_NO_THROW(valid = reader.isValid());
    ASSERT_TRUE(valid);

    ASSERT_NO_THROW(reader.getDomainInfo());
    ASSERT_NO_THROW(reader.getFirstSampleDomainValue());
    ASSERT_NO_THROW(reader.advanceToDomainValue(domainValue.get()));
    Int sr;
    ASSERT_NO_THROW(sr = reader.getSampleRate());
    ASSERT_EQ(sr, sampleRate);
    ASSERT_NO_THROW(reader.dropOutdatedDomainSegments());
    ASSERT_NO_THROW(reader.hasPendingEvents());
    ASSERT_NO_THROW(reader.popFrontEvent());
}

TEST_F(QueueReaderTest, InvalidDomainAndBack)
{
    // Domain (time) signal: Int64, linear rule.
    constexpr Int sampleRate = 10000;
    setDomainDescriptor(DataDescriptorBuilder()
                            .setSampleType(SampleType::Int64)
                            .setTickResolution(Ratio(1, sampleRate))
                            .setOrigin("abc")  // Invalid origin
                            .setRule(LinearDataRule(1, 0))
                            .setUnit(Unit("s", -1, "second", "time"))
                            .build());

    setValueDescriptor(DataDescriptorBuilder().setSampleType(SampleType::Float64).build());

    setOffsetDelta(500, 1);

    const size_t packetSize = 5;
    setPacketSize(packetSize);

    auto inputPort = InputPort(context, nullptr, "port", true);
    inputPort.setNotificationMethod(PacketReadyNotification::SameThread);

    QueueReader reader = QueueReader(inputPort, SampleType::Float64, SampleType::Int64, ReadMode::Scaled, loggerComponent, false);

    ASSERT_FALSE(reader.isValid());
    ASSERT_THROW(reader.getDomainInfo(), InvalidOperationException);

    inputPort.connect(signal);
    reader.updateConnection();

    bool valid;
    ASSERT_NO_THROW(valid = reader.isValid());
    ASSERT_FALSE(valid);

    setDomainDescriptor(DataDescriptorBuilder()
                            .setSampleType(SampleType::Int64)
                            .setTickResolution(Ratio(1, sampleRate))
                            .setOrigin("1970-01-01T00:00:00+00:00")
                            .setRule(LinearDataRule(1, 0))
                            .setUnit(Unit("s", -1, "second", "time"))
                            .build());

    ASSERT_TRUE(reader.isValid());

    setDomainDescriptor(DataDescriptorBuilder()
                            .setSampleType(SampleType::Int64)
                            .setTickResolution(Ratio(1, sampleRate))
                            .setOrigin("1970-01-01T00:00:00+00:00")
                            .setRule(LinearDataRule(3.5, 0))  // Non-integer delta
                            .setUnit(Unit("s", -1, "second", "time"))
                            .build());

    ASSERT_FALSE(reader.isValid());

    setDomainDescriptor(DataDescriptorBuilder()
                            .setSampleType(SampleType::Int64)
                            .setTickResolution(Ratio(1, sampleRate))
                            .setOrigin("1970-01-01T00:00:00+00:00")
                            .setRule(LinearDataRule(1, 0))
                            .setUnit(Unit("s", -1, "second", "time"))
                            .build());

    ASSERT_TRUE(reader.isValid());

    setDomainDescriptor(DataDescriptorBuilder()
                            .setSampleType(SampleType::Int64)
                            .setTickResolution(Ratio(1, sampleRate))
                            .setOrigin("1970-01-01T00:00:00+00:00")
                            .setRule(LinearDataRule(3, 0))  // Non-integer sample rate
                            .setUnit(Unit("s", -1, "second", "time"))
                            .build());

    ASSERT_FALSE(reader.isValid());

    setDomainDescriptor(DataDescriptorBuilder()
                            .setSampleType(SampleType::Int64)
                            .setTickResolution(Ratio(1, sampleRate))
                            .setOrigin("1970-01-01T00:00:00+00:00")
                            .setRule(LinearDataRule(1, 0))
                            .setUnit(Unit("s", -1, "second", "time"))
                            .build());

    ASSERT_TRUE(reader.isValid());

    setDomainDescriptor(DataDescriptorBuilder()
                            .setSampleType(SampleType::Int64)
                            .setTickResolution(Ratio(1, sampleRate))
                            .setOrigin("1970-01-01T00:00:00+00:00")
                            .setRule(ExplicitDataRule())  // Explicit data rule - shall be removed when resampling is added
                            .setUnit(Unit("s", -1, "second", "time"))
                            .build());

    ASSERT_FALSE(reader.isValid());
}

TEST_F(QueueReaderTest, MergeDomainAndValueChange)
{
    constexpr Int sampleRate = 10000;
    auto domainDescriptor = DataDescriptorBuilder()
                                .setSampleType(SampleType::Int64)
                                .setTickResolution(Ratio(1, sampleRate))
                                .setOrigin("1970-01-01T00:00:00+00:00")
                                .setRule(LinearDataRule(1, 0))
                                .setUnit(Unit("s", -1, "second", "time"))
                                .build();

    auto valueDescriptor = DataDescriptorBuilder().setSampleType(SampleType::Float64).setUnit(Unit("V", -1, "volt", "voltage")).build();

    auto domainChangePacket =
        DataDescriptorChangedEventPacket(descriptorToEventPacketParam(nullptr), descriptorToEventPacketParam(domainDescriptor));
    SignalEvent domainChange(domainChangePacket);

    ASSERT_EQ(domainChange.getType(), SignalEventType::DomainChanged);

    auto valueChangePacket =
        DataDescriptorChangedEventPacket(descriptorToEventPacketParam(valueDescriptor), descriptorToEventPacketParam(nullptr));
    SignalEvent valueChange(valueChangePacket);

    ASSERT_EQ(valueChange.getType(), SignalEventType::ValueChanged);

    auto merged = domainChange.merge(valueChange);
    ASSERT_TRUE(merged);

    ASSERT_EQ(domainChange.getType(), SignalEventType::DomainAndValueChanged);

    ASSERT_EQ(domainChange.getDomainDescriptor().getTickResolution().getDenominator(), sampleRate);
    ASSERT_EQ(domainChange.getValueDescriptor().getUnit().getQuantity(), String("voltage"));
}

TEST_F(QueueReaderTest, LatestDescriptorPreserved)
{
    constexpr Int sampleRate = 10000;
    auto domainDescriptor = DataDescriptorBuilder()
                                .setSampleType(SampleType::Int64)
                                .setTickResolution(Ratio(1, sampleRate))
                                .setOrigin("1970-01-01T00:00:00+00:00")
                                .setRule(LinearDataRule(1, 0))
                                .setUnit(Unit("s", -1, "second", "time"))
                                .build();
    auto domainDescriptor2 = DataDescriptorBuilder()
                                 .setSampleType(SampleType::Int64)
                                 .setTickResolution(Ratio(1, sampleRate))
                                 .setOrigin("1970-01-01T00:00:00+00:00")
                                 .setRule(LinearDataRule(10, 0))
                                 .setUnit(Unit("s", -1, "second", "time"))
                                 .build();

    auto domainChangePacket =
        DataDescriptorChangedEventPacket(descriptorToEventPacketParam(nullptr), descriptorToEventPacketParam(domainDescriptor));
    SignalEvent domainChange(domainChangePacket);
    ASSERT_EQ(domainChange.getType(), SignalEventType::DomainChanged);
    NumberPtr delta = domainChange.getDomainDescriptor().getRule().getParameters()["delta"];
    ASSERT_EQ(delta.getIntValue(), 1u);

    auto domainChangePacket2 =
        DataDescriptorChangedEventPacket(descriptorToEventPacketParam(nullptr), descriptorToEventPacketParam(domainDescriptor2));
    SignalEvent domainChange2(domainChangePacket2);
    ASSERT_EQ(domainChange2.getType(), SignalEventType::DomainChanged);
    delta = domainChange2.getDomainDescriptor().getRule().getParameters()["delta"];
    ASSERT_EQ(delta.getIntValue(), 10u);

    bool merged = domainChange.merge(domainChange2);
    ASSERT_TRUE(merged);

    ASSERT_EQ(domainChange.getType(), SignalEventType::DomainChanged);
    delta = domainChange.getDomainDescriptor().getRule().getParameters()["delta"];
    ASSERT_EQ(delta.getIntValue(), 10u);
}

TEST_F(QueueReaderTest, GapEventsRefuseMerge)
{
    constexpr Int sampleRate = 10000;
    auto domainDescriptor = DataDescriptorBuilder()
                                .setSampleType(SampleType::Int64)
                                .setTickResolution(Ratio(1, sampleRate))
                                .setOrigin("1970-01-01T00:00:00+00:00")
                                .setRule(LinearDataRule(1, 0))
                                .setUnit(Unit("s", -1, "second", "time"))
                                .build();

    auto domainChangePacket =
        DataDescriptorChangedEventPacket(descriptorToEventPacketParam(nullptr), descriptorToEventPacketParam(domainDescriptor));
    SignalEvent domainChange(domainChangePacket);
    ASSERT_EQ(domainChange.getType(), SignalEventType::DomainChanged);

    auto gapPacket = ImplicitDomainGapDetectedEventPacket(0);
    SignalEvent gapChange(gapPacket);
    ASSERT_EQ(gapChange.getType(), SignalEventType::Gap);

    auto merged = domainChange.merge(gapChange);
    ASSERT_FALSE(merged);
}

TEST_F(QueueReaderTest, CheckAdvanceDomainEdgeCases)
{
    // Domain (time) signal: Int64, linear rule.
    constexpr Int sampleRate = 10000;
    setDomainDescriptor(DataDescriptorBuilder()
                            .setSampleType(SampleType::Int64)
                            .setTickResolution(Ratio(1, sampleRate))
                            .setOrigin("1970-01-01T00:00:00+00:00")
                            .setRule(LinearDataRule(1, 0))
                            .setUnit(Unit("s", -1, "second", "time"))
                            .build());

    setValueDescriptor(DataDescriptorBuilder().setSampleType(SampleType::Float64).build());

    setOffsetDelta(500, 1);

    const size_t packetSize = 5;
    setPacketSize(packetSize);

    auto inputPort = InputPort(context, nullptr, "port", true);
    inputPort.connect(signal);
    QueueReader reader = QueueReader(inputPort, SampleType::Float64, SampleType::Int64, ReadMode::Scaled, loggerComponent, false);

    sendNextPacket(); // [500 - 504]
    sendNextPacket();
    sendNextPacket(); // [510 - 514]
    setDomainDescriptor(DataDescriptorBuilder()
                            .setSampleType(SampleType::Int64)
                            .setTickResolution(Ratio(1, sampleRate))
                            .setOrigin("1970-01-01T00:00:00+00:00")
                            .setRule(LinearDataRule(10, 0))
                            .setUnit(Unit("s", -1, "second", "time"))
                            .build());

    setOffsetDelta(getOffset(), 10);
    sendNextPacket(); // [515 - 555]
    sendNextPacket(); // [565 - 605]

    setDomainDescriptor(DataDescriptorBuilder()
                            .setSampleType(SampleType::Int64)
                            .setTickResolution(Ratio(1, sampleRate))
                            .setOrigin("1970-01-01T00:00:00+00:00")
                            .setRule(LinearDataRule(2, 0))
                            .setUnit(Unit("s", -1, "second", "time"))
                            .build());

    setOffsetDelta(getOffset(), 2);

    sendNextPacket(); // [615 - 623]
    sendNextPacket(); // [625 - 633]
    // Establish a queue, now test queue handling
    
    // Initial data segment
    ASSERT_TRUE(reader.hasPendingEvents());
    ASSERT_EQ(reader.getAvailableSamples(), 3 * packetSize); // First three packets worth of samples
    
    auto event = reader.popFrontEvent();
    ASSERT_FALSE(reader.hasPendingEvents());

    DataDescriptorPtr descriptor = event.getParameters()[event_packet_param::DOMAIN_DATA_DESCRIPTOR];
    NumberPtr delta = descriptor.getRule().getParameters()["delta"];
    ASSERT_EQ(delta.getIntValue(), 1);

    auto start = reader.getFirstSampleDomainValue();
    auto* startP = dynamic_cast<DomainValueImpl<Int>*>(start.get());
    ASSERT_EQ(startP->getValue(), 500);
    // End Initial data segment

    // Second data segment
    std::unique_ptr<DomainValue> domainValue = std::make_unique<DomainValueImpl<Int>>(reader.getDomainInfo(), 515);
    AdvanceResult result = reader.advanceToDomainValue(domainValue.get());
    ASSERT_EQ(result, AdvanceResult::DomainChanged);
    
    ASSERT_TRUE(reader.hasPendingEvents());

    event = reader.popFrontEvent();
    ASSERT_FALSE(reader.hasPendingEvents());

    descriptor = event.getParameters()[event_packet_param::DOMAIN_DATA_DESCRIPTOR];
    delta = descriptor.getRule().getParameters()["delta"];
    ASSERT_EQ(delta.getIntValue(), 10); // Check that we got the second descriptor

    assertReaderAtDomainValue(reader, 515); // Advance succeeded, but domain change reporting takes priority

    result = reader.advanceToDomainValue(domainValue.get());
    ASSERT_EQ(result, AdvanceResult::Success); // Advancing for the second time will result in a success
    assertReaderAtDomainValue(reader, 515);

    domainValue = std::make_unique<DomainValueImpl<Int>>(reader.getDomainInfo(), 605);
    result = reader.advanceToDomainValue(domainValue.get());
    ASSERT_EQ(result, AdvanceResult::Success); // Can advance to the last sample in the data segment
    assertReaderAtDomainValue(reader, 605);
    // End Second data segment

    // Third data segment
    domainValue = std::make_unique<DomainValueImpl<Int>>(reader.getDomainInfo(), 615);
    result = reader.advanceToDomainValue(domainValue.get());
    ASSERT_EQ(result, AdvanceResult::DomainChanged); // The first sample in the next sample only through domain change
    assertReaderAtDomainValue(reader, 615);

    ASSERT_TRUE(reader.hasPendingEvents());

    event = reader.popFrontEvent();
    ASSERT_FALSE(reader.hasPendingEvents());

    descriptor = event.getParameters()[event_packet_param::DOMAIN_DATA_DESCRIPTOR];
    delta = descriptor.getRule().getParameters()["delta"];
    ASSERT_EQ(delta.getIntValue(), 2); // Check that we got the second descriptor

    result = reader.advanceToDomainValue(domainValue.get());
    ASSERT_EQ(result, AdvanceResult::Success); // Advancing for the second time will result in a success
    assertReaderAtDomainValue(reader, 615);

    domainValue = std::make_unique<DomainValueImpl<Int>>(reader.getDomainInfo(), 633);
    result = reader.advanceToDomainValue(domainValue.get());
    ASSERT_EQ(result, AdvanceResult::Success); // Can advance to the last sample in the data segment
    assertReaderAtDomainValue(reader, 633);
}