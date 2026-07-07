#include <gtest/gtest.h>

#include <coreobjects/unit_factory.h>
#include <coretypes/exceptions.h>
#include <coretypes/ratio_factory.h>

#include <opendaq/data_descriptor_factory.h>
#include <opendaq/data_rule_factory.h>
#include <opendaq/domain_value.h>
#include <opendaq/input_port_factory.h>
#include <opendaq/packet_factory.h>
#include <opendaq/reader_utils.h>
#include <opendaq/typed_reading_utils.h>
#include <opendaq/queue_reader.h>
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

    void setDomainDescriptor(const DataDescriptorPtr &descriptor)
    {
        domainSignal.setDescriptor(descriptor);
    }

    void setValueDescriptor(const DataDescriptorPtr &descriptor)
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

    Int getOffset() const { return offset; }

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
        offset += packetSize*delta;
    }

protected:
    Int offset = 0;
    Int delta = 1;
    SizeT packetSize = 1;

    SizeT samples = 0;

    SignalConfigPtr domainSignal;
};

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
    
    setValueDescriptor(DataDescriptorBuilder()
        .setSampleType(SampleType::Float64)
        .build());

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
    
    setValueDescriptor(DataDescriptorBuilder()
        .setSampleType(SampleType::Float64)
        .build());

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
