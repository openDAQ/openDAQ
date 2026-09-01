#include <opendaq/reader_factory.h>
#include <opendaq/reader_factory.h>
#include "reader_common.h"

using namespace daq;

using DurationTailReaderTest = ReaderTest<>;

static DurationTailReaderPtr createReader(const SignalConfigPtr& signal, UInt historyDurationMs)
{
    return DurationTailReader(signal, historyDurationMs, SampleType::Float64, SampleType::Int64, ReadMode::Scaled);
}

TEST_F(DurationTailReaderTest, Create)
{
    ASSERT_NO_THROW(createReader(signal, 5000u));
}

TEST_F(DurationTailReaderTest, IsReader)
{
    auto reader = createReader(signal, 5000u);
    ASSERT_NO_THROW(reader.asPtr<IReader>());
}

TEST_F(DurationTailReaderTest, IsSampleReader)
{
    auto reader = createReader(signal, 5000u);
    ASSERT_NO_THROW(reader.asPtr<ISampleReader>());
}

TEST_F(DurationTailReaderTest, GetHistoryDurationMsMatchesConstructor)
{
    auto reader = createReader(signal, 5000u);
    ASSERT_EQ(reader.getHistoryDurationMs(), 5000u);
}

TEST_F(DurationTailReaderTest, SetHistoryDurationMs)
{
    auto reader = createReader(signal, 5000u);
    reader.setHistoryDurationMs(12000u);
    ASSERT_EQ(reader.getHistoryDurationMs(), 12000u);
}

TEST_F(DurationTailReaderTest, SetHistoryDurationMsZeroThrows)
{
    auto reader = createReader(signal, 5000u);
    ASSERT_THROW(reader.setHistoryDurationMs(0u), InvalidParameterException);
}

TEST_F(DurationTailReaderTest, GetSamplesAvailableEmpty)
{
    auto reader = createReader(signal, 5000u);
    ASSERT_EQ(reader.getAvailableCount(), 0u);
}

TEST_F(DurationTailReaderTest, PacketsWithinWindowAreAllKept)
{
    signal.setDescriptor(setupDescriptor(SampleType::Float64));

    // The domain descriptor built by createDataPacket() defaults to a 1/1 tick resolution, so
    // `offset` below is the packet's start time in seconds - a 5000ms history window is a
    // 5 second trailing window.
    auto reader = createReader(signal, 5000u);

    sendPacket(createDataPacket(3u, 0));
    sendPacket(createDataPacket(3u, 3));

    // Both packets start within the 5 second trailing window of the latest one (3s), so
    // nothing should have been trimmed.
    ASSERT_EQ(reader.getAvailableCount(), 6u);
}

TEST_F(DurationTailReaderTest, PacketsOutsideWindowAreTrimmed)
{
    signal.setDescriptor(setupDescriptor(SampleType::Float64));

    auto reader = createReader(signal, 5000u);

    sendPacket(createDataPacket(3u, 0));
    // 20s is well outside the 5s trailing window measured from the latest packet, so the
    // first packet (start=0s) must be dropped once the second one (start=20s) arrives.
    sendPacket(createDataPacket(4u, 20));

    ASSERT_EQ(reader.getAvailableCount(), 4u);
}

TEST_F(DurationTailReaderTest, ShrinkingHistoryDurationTrimsImmediately)
{
    signal.setDescriptor(setupDescriptor(SampleType::Float64));

    auto reader = createReader(signal, 100000u);

    sendPacket(createDataPacket(3u, 0));
    sendPacket(createDataPacket(4u, 20));
    ASSERT_EQ(reader.getAvailableCount(), 7u);

    // Shrinking the window below the 20s gap between the two packets should immediately drop
    // the older one, without waiting for a new packet to arrive.
    reader.setHistoryDurationMs(5000u);
    ASSERT_EQ(reader.getAvailableCount(), 4u);
}

TEST_F(DurationTailReaderTest, ReadReturnsBufferedValues)
{
    using ValueType = double;
    signal.setDescriptor(setupDescriptor(SampleTypeFromType<ValueType>::SampleType));

    constexpr auto PACKET_SAMPLES = 5u;
    auto reader = createReader(signal, 5000u);

    auto dataPacket = DataPacketWithDomain(
        DataPacket(setupDescriptor(SampleType::UInt64, LinearDataRule(1, 0)), PACKET_SAMPLES, 0),
        signal.getDescriptor(),
        PACKET_SAMPLES);
    auto dataPtr = static_cast<ValueType*>(dataPacket.getData());
    dataPtr[0] = 11;
    dataPtr[1] = 22;
    dataPtr[2] = 33;
    dataPtr[3] = 44;
    dataPtr[4] = 55;

    sendPacket(dataPacket);
    ASSERT_EQ(reader.getAvailableCount(), PACKET_SAMPLES);

    SizeT count{PACKET_SAMPLES};
    ValueType values[PACKET_SAMPLES]{};
    reader.read(&values, &count);

    ASSERT_EQ(count, PACKET_SAMPLES);
    ASSERT_EQ(values[0], dataPtr[0]);
    ASSERT_EQ(values[1], dataPtr[1]);
    ASSERT_EQ(values[2], dataPtr[2]);
    ASSERT_EQ(values[3], dataPtr[3]);
    ASSERT_EQ(values[4], dataPtr[4]);
}
