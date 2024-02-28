#include <testutils/testutils.h>
#include "reader_common.h"
#include <opendaq/stream_reader_ptr.h>
#include <opendaq/data_packet_ptr.h>
#include <opendaq/event_packet_ptr.h>
#include <opendaq/packet_factory.h>
#include <opendaq/data_rule_factory.h>
#include <opendaq/reader_errors.h>
#include <opendaq/reader_exceptions.h>
#include <opendaq/reader_factory.h>
#include <opendaq/input_port_factory.h>
#include <future>

using namespace daq;

template <typename T>
using BlockReaderTest = ReaderTest<T>;

using SampleTypes = ::testing::Types<OPENDAQ_VALUE_SAMPLE_TYPES>;

TYPED_TEST_SUITE(BlockReaderTest, SampleTypes);

static constexpr const SizeT BLOCK_SIZE = 2u;

TYPED_TEST(BlockReaderTest, Create)
{
    ASSERT_NO_THROW((BlockReader<TypeParam, ClockRange>)(this->signal, BLOCK_SIZE));
}

TYPED_TEST(BlockReaderTest, CreateNullThrows)
{
    ASSERT_THROW_MSG((BlockReader<TypeParam, ClockRange>)(nullptr, BLOCK_SIZE), ArgumentNullException, "Signal must not be null.")
}

TYPED_TEST(BlockReaderTest, IsReader)
{
    auto reader = daq::BlockReader<TypeParam, ClockRange>(this->signal, BLOCK_SIZE);
    ASSERT_NO_THROW(reader.template asPtr<IReader>());
}

TYPED_TEST(BlockReaderTest, IsSampleReader)
{
    auto reader = daq::BlockReader<TypeParam, ClockRange>(this->signal, BLOCK_SIZE);
    ASSERT_NO_THROW(reader.template asPtr<ISampleReader>());
}

TYPED_TEST(BlockReaderTest, IsBlockReader)
{
    auto reader = daq::BlockReader<TypeParam, ClockRange>(this->signal, BLOCK_SIZE);
    ASSERT_NO_THROW(reader.template asPtr<IBlockReader>());
}

TYPED_TEST(BlockReaderTest, GetDefaultReadType)
{
    auto reader = BlockReader<TypeParam>(this->signal, BLOCK_SIZE);
    ASSERT_EQ(reader.getValueReadType(), SampleTypeFromType<TypeParam>::SampleType);
    ASSERT_EQ(reader.getDomainReadType(), SampleType::Int64);
}

TYPED_TEST(BlockReaderTest, GetSamplesAvailableEmpty)
{
    auto reader = daq::BlockReader<TypeParam, ClockRange>(this->signal, BLOCK_SIZE);
    ASSERT_EQ(reader.getAvailableCount(), 0u);
}

TYPED_TEST(BlockReaderTest, GetSamplesAvailable)
{
    this->signal.setDescriptor(setupDescriptor(SampleType::Float64));

    auto reader = daq::BlockReader<TypeParam, ClockRange>(this->signal, BLOCK_SIZE);
    ASSERT_EQ(reader.getAvailableCount(), 0u);

    this->sendPacket(DataPacket(this->signal.getDescriptor(), 1));
    this->scheduler.waitAll();

    ASSERT_EQ(reader.getAvailableCount(), 0u);

    this->sendPacket(DataPacket(this->signal.getDescriptor(), 1));
    this->scheduler.waitAll();

    ASSERT_EQ(reader.getAvailableCount(), 1u);
}

TYPED_TEST(BlockReaderTest, ReadOneBlock)
{
    this->signal.setDescriptor(setupDescriptor(SampleType::Float64));

    auto reader = daq::BlockReader<TypeParam, ClockRange>(this->signal, BLOCK_SIZE);
    auto dataPacket = DataPacket(this->signal.getDescriptor(), 1 * BLOCK_SIZE);

    // Set the first sample to
    auto dataPtr = static_cast<double*>(dataPacket.getData());
    dataPtr[0] = 11.1;
    dataPtr[1] = 22.2;

    this->sendPacket(dataPacket);
    this->scheduler.waitAll();

    SizeT count{1};
    TypeParam samples[1 * BLOCK_SIZE]{};
    reader.read((TypeParam*) &samples, &count);

    ASSERT_EQ(count, 1u);
    ASSERT_EQ(reader.getAvailableCount(), 0u);

    if constexpr (IsTemplateOf<TypeParam, Complex_Number>::value || IsTemplateOf<TypeParam, RangeType>::value)
    {
        ASSERT_EQ(samples[0], TypeParam(typename TypeParam::Type(11.1)));
        ASSERT_EQ(samples[1], TypeParam(typename TypeParam::Type(22.2)));
    }
    else
    {
        ASSERT_EQ(samples[0], TypeParam(11.1));
        ASSERT_EQ(samples[1], TypeParam(22.2));
    }
}

TYPED_TEST(BlockReaderTest, ReadOneBlockWithTimeout)
{
    this->signal.setDescriptor(setupDescriptor(SampleType::Float64));

    auto reader = daq::BlockReader<TypeParam, ClockRange>(this->signal, BLOCK_SIZE);
    auto dataPacket = DataPacket(this->signal.getDescriptor(), 1 * BLOCK_SIZE);

    // Set the first sample to
    auto dataPtr = static_cast<double*>(dataPacket.getData());
    dataPtr[0] = 11.1;
    dataPtr[1] = 22.2;

    this->sendPacket(dataPacket);
    this->scheduler.waitAll();

    std::thread t([this, &dataPacket]
    {
        using namespace std::chrono_literals;

        std::this_thread::sleep_for(30ms);
        this->sendPacket(dataPacket);
        this->scheduler.waitAll();
    });

    SizeT count{2};
    TypeParam samples[2 * BLOCK_SIZE]{};
    reader.read((TypeParam*) &samples, &count, 1000u);

    if (t.joinable())
    {
        t.join();
    }

    ASSERT_EQ(count, 2u);
    ASSERT_EQ(reader.getAvailableCount(), 0u);

    if constexpr (IsTemplateOf<TypeParam, Complex_Number>::value || IsTemplateOf<TypeParam, RangeType>::value)
    {
        ASSERT_EQ(samples[0], TypeParam(typename TypeParam::Type(11.1)));
        ASSERT_EQ(samples[1], TypeParam(typename TypeParam::Type(22.2)));
        ASSERT_EQ(samples[2], TypeParam(typename TypeParam::Type(11.1)));
        ASSERT_EQ(samples[3], TypeParam(typename TypeParam::Type(22.2)));
    }
    else
    {
        ASSERT_EQ(samples[0], TypeParam(11.1));
        ASSERT_EQ(samples[1], TypeParam(22.2));
        ASSERT_EQ(samples[2], TypeParam(11.1));
        ASSERT_EQ(samples[3], TypeParam(22.2));
    }
}

TYPED_TEST(BlockReaderTest, ReadOneBlockWithClockTicks)
{
    this->signal.setDescriptor(setupDescriptor(SampleType::Float64));

    auto reader = daq::BlockReader<TypeParam, ClockTick>(this->signal, BLOCK_SIZE);

    auto domainPacket = DataPacket(setupDescriptor(SampleType::RangeInt64, LinearDataRule(1, 0), nullptr), 1 * BLOCK_SIZE, 1);
    auto dataPacket = DataPacketWithDomain(domainPacket, this->signal.getDescriptor(), 1 * BLOCK_SIZE);

    auto dataPtr = static_cast<double*>(dataPacket.getData());
    dataPtr[0] = 11.1;
    dataPtr[1] = 22.2;

    this->sendPacket(dataPacket);
    this->scheduler.waitAll();

    SizeT count{1};
    TypeParam samples[1 * BLOCK_SIZE]{};
    ClockTick ticks[1 * BLOCK_SIZE]{};
    reader.readWithDomain((TypeParam*) &samples, (ClockTick*) &ticks, &count);

    ASSERT_EQ(count, 1u);
    ASSERT_EQ(reader.getAvailableCount(), 0u);

    ASSERT_EQ(ticks[0], 1);
    ASSERT_EQ(ticks[1], 2);

    if constexpr (IsTemplateOf<TypeParam, Complex_Number>::value || IsTemplateOf<TypeParam, RangeType>::value)
    {
        ASSERT_EQ(samples[0], TypeParam(typename TypeParam::Type(11.1)));
        ASSERT_EQ(samples[1], TypeParam(typename TypeParam::Type(22.2)));
    }
    else
    {
        ASSERT_EQ(samples[0], TypeParam(11.1));
        ASSERT_EQ(samples[1], TypeParam(22.2));
    }
}

TYPED_TEST(BlockReaderTest, ReadOneBlockWithClockTicksTimeout)
{
    this->signal.setDescriptor(setupDescriptor(SampleType::Float64));

    auto reader = daq::BlockReader<TypeParam, ClockTick>(this->signal, BLOCK_SIZE);

    auto domainPacket = DataPacket(setupDescriptor(SampleType::RangeInt64, LinearDataRule(1, 0), nullptr), 1 * BLOCK_SIZE, 1);
    auto dataPacket = DataPacketWithDomain(domainPacket, this->signal.getDescriptor(), 1 * BLOCK_SIZE);

    // Set the first sample to
    auto dataPtr = static_cast<double*>(dataPacket.getData());
    dataPtr[0] = 11.1;
    dataPtr[1] = 22.2;

    this->sendPacket(dataPacket);
    this->scheduler.waitAll();

    std::thread t([this, &dataPacket] {
        using namespace std::chrono_literals;

        std::this_thread::sleep_for(30ms);
        this->sendPacket(dataPacket);
        this->scheduler.waitAll();
    });

    SizeT count{2};
    TypeParam samples[2 * BLOCK_SIZE]{};
    ClockTick ticks[2 * BLOCK_SIZE]{};
    reader.readWithDomain((TypeParam*) &samples, (ClockTick*) &ticks, &count, 1000u);

    if (t.joinable())
    {
        t.join();
    }

    ASSERT_EQ(count, 2u);
    ASSERT_EQ(reader.getAvailableCount(), 0u);

    ASSERT_EQ(ticks[0], 1);
    ASSERT_EQ(ticks[1], 2);
    ASSERT_EQ(ticks[2], 1);
    ASSERT_EQ(ticks[3], 2);

    if constexpr (IsTemplateOf<TypeParam, Complex_Number>::value || IsTemplateOf<TypeParam, RangeType>::value)
    {
        ASSERT_EQ(samples[0], TypeParam(typename TypeParam::Type(11.1)));
        ASSERT_EQ(samples[1], TypeParam(typename TypeParam::Type(22.2)));
        ASSERT_EQ(samples[2], TypeParam(typename TypeParam::Type(11.1)));
        ASSERT_EQ(samples[3], TypeParam(typename TypeParam::Type(22.2)));
    }
    else
    {
        ASSERT_EQ(samples[0], TypeParam(11.1));
        ASSERT_EQ(samples[1], TypeParam(22.2));
        ASSERT_EQ(samples[2], TypeParam(11.1));
        ASSERT_EQ(samples[3], TypeParam(22.2));
    }
}

TYPED_TEST(BlockReaderTest, ReadOneBlockWithRanges)
{
    this->signal.setDescriptor(setupDescriptor(SampleType::Float64));

    auto reader = daq::BlockReader<TypeParam, ClockRange>(this->signal, BLOCK_SIZE);

    auto domainPacket = DataPacket(
        setupDescriptor(SampleType::RangeInt64, LinearDataRule(1, 0), nullptr),
        1 * BLOCK_SIZE,
        1
    );

    auto dataPacket = DataPacketWithDomain(
        domainPacket,
        this->signal.getDescriptor(),
        1 * BLOCK_SIZE
    );

    // Set the first sample to
    auto dataPtr = static_cast<double*>(dataPacket.getData());
    dataPtr[0] = 11.1;
    dataPtr[1] = 22.2;

    this->sendPacket(dataPacket);
    this->scheduler.waitAll();

    SizeT count{1};
    TypeParam samples[1 * BLOCK_SIZE]{};
    ClockRange stamps[1 * BLOCK_SIZE]{};
    reader->readWithDomain((TypeParam*) &samples, (ClockRange*) &stamps, &count);

    ASSERT_EQ(count, 1u);
    ASSERT_EQ(reader.getAvailableCount(), 0u);

    ASSERT_EQ(stamps[0].start, 1);
    ASSERT_EQ(stamps[0].end, (ClockTick) -1);

    ASSERT_EQ(stamps[1].start, 2);
    ASSERT_EQ(stamps[1].end, (ClockTick) -1);

    if constexpr (IsTemplateOf<TypeParam, Complex_Number>::value || IsTemplateOf<TypeParam, RangeType>::value)
    {
        ASSERT_EQ(samples[0], TypeParam(typename TypeParam::Type(11.1)));
        ASSERT_EQ(samples[1], TypeParam(typename TypeParam::Type(22.2)));
    }
    else
    {
        ASSERT_EQ(samples[0], TypeParam(11.1));
        ASSERT_EQ(samples[1], TypeParam(22.2));
    }
}

TYPED_TEST(BlockReaderTest, ReadOneBlockWithRangesTimeout)
{
    this->signal.setDescriptor(setupDescriptor(SampleType::Float64));

    auto reader = daq::BlockReader<TypeParam, ClockRange>(this->signal, BLOCK_SIZE);

    auto domainPacket = DataPacket(setupDescriptor(SampleType::RangeInt64, LinearDataRule(1, 0), nullptr), 1 * BLOCK_SIZE, 1);
    auto dataPacket = DataPacketWithDomain(domainPacket, this->signal.getDescriptor(), 1 * BLOCK_SIZE);

    // Set the first sample to
    auto dataPtr = static_cast<double*>(dataPacket.getData());
    dataPtr[0] = 11.1;
    dataPtr[1] = 22.2;

    this->sendPacket(dataPacket);
    this->scheduler.waitAll();

    std::thread t([this, &dataPacket] {
        using namespace std::chrono_literals;

        std::this_thread::sleep_for(30ms);
        this->sendPacket(dataPacket);
        this->scheduler.waitAll();
    });

    SizeT count{2};
    TypeParam samples[2 * BLOCK_SIZE]{};
    ClockRange stamps[2 * BLOCK_SIZE]{};
    reader->readWithDomain((void*) &samples, (void*) &stamps, &count, 1000u);

    if (t.joinable())
    {
        t.join();
    }

    ASSERT_EQ(count, 2u);

    ASSERT_EQ(stamps[0].start, 1);
    ASSERT_EQ(stamps[0].end, (ClockTick) -1);

    ASSERT_EQ(stamps[1].start, 2);
    ASSERT_EQ(stamps[1].end, (ClockTick) -1);

    if constexpr (IsTemplateOf<TypeParam, Complex_Number>::value || IsTemplateOf<TypeParam, RangeType>::value)
    {
        ASSERT_EQ(samples[0], TypeParam(typename TypeParam::Type(11.1)));
        ASSERT_EQ(samples[1], TypeParam(typename TypeParam::Type(22.2)));
    }
    else
    {
        ASSERT_EQ(samples[0], TypeParam(11.1));
        ASSERT_EQ(samples[1], TypeParam(22.2));
    }

    ASSERT_EQ(reader.getAvailableCount(), 0u);
}

TYPED_TEST(BlockReaderTest, ReadLessThanOnePacket)
{
    this->signal.setDescriptor(setupDescriptor(SampleType::Float64));

    auto reader = daq::BlockReader<TypeParam, ClockRange>(this->signal, BLOCK_SIZE);

    const SizeT NUM_SAMPLES = BLOCK_SIZE + 1;
    auto dataPacket = DataPacket(this->signal.getDescriptor(), NUM_SAMPLES);

    auto dataPtr = static_cast<double*>(dataPacket.getData());
    dataPtr[0] = 11.1;
    dataPtr[1] = 22.2;
    dataPtr[2] = 33.3;

    this->sendPacket(dataPacket);
    this->scheduler.waitAll();

    SizeT count{1};
    TypeParam samples[1 * BLOCK_SIZE]{};
    reader.read((void*) &samples, &count);

    ASSERT_EQ(count, 1u);
    ASSERT_EQ(reader.getAvailableCount(), 0u);

    if constexpr (IsTemplateOf<TypeParam, Complex_Number>::value || IsTemplateOf<TypeParam, RangeType>::value)
    {
        ASSERT_EQ(samples[0], TypeParam(typename TypeParam::Type(11.1)));
        ASSERT_EQ(samples[1], TypeParam(typename TypeParam::Type(22.2)));
    }
    else
    {
        ASSERT_EQ(samples[0], TypeParam(11.1));
        ASSERT_EQ(samples[1], TypeParam(22.2));
    }
}

TYPED_TEST(BlockReaderTest, ReadBetweenPackets)
{
    this->signal.setDescriptor(setupDescriptor(SampleType::Float64));

    auto reader = daq::BlockReader<TypeParam, ClockRange>(this->signal, BLOCK_SIZE);
    auto dataPacket = DataPacket(this->signal.getDescriptor(), BLOCK_SIZE + 1);

    // Set the first sample to
    auto dataPtr = static_cast<double*>(dataPacket.getData());
    dataPtr[0] = 11.1;
    dataPtr[1] = 22.2;
    dataPtr[2] = 33.3;

    this->sendPacket(dataPacket);
    this->scheduler.waitAll();

    SizeT count{1};
    TypeParam samples[1 * BLOCK_SIZE]{};
    reader.read((void*) &samples, &count);

    ASSERT_EQ(reader.getAvailableCount(), 0u);

    this->sendPacket(dataPacket);
    ASSERT_EQ(reader.getAvailableCount(), 2u);
}

TYPED_TEST(BlockReaderTest, ReadBetweenPacketsTimeout)
{
    this->signal.setDescriptor(setupDescriptor(SampleType::Float64));

    auto reader = daq::BlockReader<TypeParam, ClockRange>(this->signal, BLOCK_SIZE);
    auto dataPacket = DataPacket(this->signal.getDescriptor(), BLOCK_SIZE + 1);

    // Set the first sample to
    auto dataPtr = static_cast<double*>(dataPacket.getData());
    dataPtr[0] = 11.1;
    dataPtr[1] = 22.2;
    dataPtr[2] = 33.3;

    this->sendPacket(dataPacket);
    this->scheduler.waitAll();

    SizeT count{1};
    TypeParam samples[1 * BLOCK_SIZE]{};
    reader.read((void*) &samples, &count);

    ASSERT_EQ(reader.getAvailableCount(), 0u);

    std::thread t([this]
    {
        using namespace std::chrono_literals;

        auto data2Packet = DataPacket(this->signal.getDescriptor(), BLOCK_SIZE + 1);

        // Set the first sample to
        auto data2Ptr = static_cast<double*>(data2Packet.getData());
        data2Ptr[0] = 44.4;
        data2Ptr[1] = 55.5;
        data2Ptr[2] = 66.6;

        std::this_thread::sleep_for(30ms);
        this->sendPacket(data2Packet);
    });

    count = 2;
    TypeParam samples2[2 * BLOCK_SIZE]{};
    reader.read((TypeParam*) &samples2, &count, 1000u);

    if (t.joinable())
        t.join();

    ASSERT_EQ(count, 2u);
    ASSERT_EQ(reader.getAvailableCount(), 0u);

    if constexpr (IsTemplateOf<TypeParam, Complex_Number>::value || IsTemplateOf<TypeParam, RangeType>::value)
    {
        ASSERT_EQ(samples2[0], TypeParam(typename TypeParam::Type(33.3)));
        ASSERT_EQ(samples2[1], TypeParam(typename TypeParam::Type(44.4)));
        ASSERT_EQ(samples2[2], TypeParam(typename TypeParam::Type(55.5)));
        ASSERT_EQ(samples2[3], TypeParam(typename TypeParam::Type(66.6)));
    }
    else
    {
        ASSERT_EQ(samples2[0], TypeParam(33.3));
        ASSERT_EQ(samples2[1], TypeParam(44.4));
        ASSERT_EQ(samples2[2], TypeParam(55.5));
        ASSERT_EQ(samples2[3], TypeParam(66.6));
    }
}

TYPED_TEST(BlockReaderTest, ReadBetweenPacketsAndCheckValues)
{
    this->signal.setDescriptor(setupDescriptor(SampleType::Float64));

    auto reader = daq::BlockReader<TypeParam, ClockRange>(this->signal, BLOCK_SIZE);
    auto dataPacket = DataPacket(this->signal.getDescriptor(), BLOCK_SIZE + 1);

    // Set the first sample to
    auto dataPtr = static_cast<double*>(dataPacket.getData());
    dataPtr[0] = 11.1;
    dataPtr[1] = 22.2;
    dataPtr[2] = 33.3;

    this->sendPacket(dataPacket);
    this->scheduler.waitAll();

    SizeT count{1};
    TypeParam samples[1 * BLOCK_SIZE]{};
    reader.read((void*) &samples, &count);

    ASSERT_EQ(reader.getAvailableCount(), 0u);

    auto nextDataPacket = DataPacket(this->signal.getDescriptor(), BLOCK_SIZE + 1);
    auto nextDataPtr = static_cast<double*>(nextDataPacket.getData());
    nextDataPtr[0] = 44.4;
    nextDataPtr[1] = 55.5;
    nextDataPtr[2] = 66.6;
    this->sendPacket(nextDataPacket);
    this->scheduler.waitAll();

    ASSERT_EQ(reader.getAvailableCount(), 2u);

    count = 2;
    TypeParam nextSamples[2 * BLOCK_SIZE]{};
    reader.read((TypeParam*) &nextSamples, &count);

    ASSERT_EQ(count, 2u);
    ASSERT_EQ(reader.getAvailableCount(), 0u);

    if constexpr (IsTemplateOf<TypeParam, Complex_Number>::value || IsTemplateOf<TypeParam, RangeType>::value)
    {
        ASSERT_EQ(nextSamples[0], TypeParam(typename TypeParam::Type(33.3)));
        ASSERT_EQ(nextSamples[1], TypeParam(typename TypeParam::Type(44.4)));
        ASSERT_EQ(nextSamples[2], TypeParam(typename TypeParam::Type(55.5)));
    }
    else
    {
        ASSERT_EQ(nextSamples[0], (TypeParam) 33.3);
        ASSERT_EQ(nextSamples[1], (TypeParam) 44.4);
        ASSERT_EQ(nextSamples[2], (TypeParam) 55.5);
    }
}

TYPED_TEST(BlockReaderTest, ReadValuesMoreThanAvailable)
{
    this->signal.setDescriptor(setupDescriptor(SampleType::Float64));

    auto reader = daq::BlockReader<TypeParam, ClockRange>(this->signal, BLOCK_SIZE);

    const SizeT NUM_SAMPLES = BLOCK_SIZE;
    this->sendPacket(DataPacket(this->signal.getDescriptor(), NUM_SAMPLES));
    this->scheduler.waitAll();

    SizeT count{2};
    TypeParam samples[2 * BLOCK_SIZE]{};
    reader.read((void*) &samples, &count);

    ASSERT_EQ(count, 1u);
    ASSERT_EQ(reader.getAvailableCount(), 0u);
}

TYPED_TEST(BlockReaderTest, DescriptorChangedConvertible)
{
    this->signal.setDescriptor(setupDescriptor(SampleType::Float64));

    auto reader = daq::BlockReader<TypeParam, ClockRange>(this->signal, BLOCK_SIZE);

    const SizeT NUM_SAMPLES = 2;
    auto dataPacketDouble = DataPacket(this->signal.getDescriptor(), NUM_SAMPLES);
    auto dataPtrDouble = static_cast<double*>(dataPacketDouble.getData());
    dataPtrDouble[0] = 11.1;
    dataPtrDouble[1] = 22.2;

    this->sendPacket(dataPacketDouble);
    this->scheduler.waitAll();

    ASSERT_EQ(reader.getAvailableCount(), 1u);

    SizeT count{1};
    TypeParam samplesDouble[1 * BLOCK_SIZE]{};
    reader.read((TypeParam*) &samplesDouble, &count);

    ASSERT_EQ(reader.getAvailableCount(), 0u);

    // Change signal sample-type
    this->signal.setDescriptor(setupDescriptor(SampleType::Int32));
    auto dataPacketInt32 = DataPacket(this->signal.getDescriptor(), NUM_SAMPLES);
    auto dataPtrInt32 = static_cast<int32_t*>(dataPacketInt32.getData());
    dataPtrInt32[0] = 3;
    dataPtrInt32[1] = 4;

    this->sendPacket(dataPacketInt32);
    this->scheduler.waitAll();

    ASSERT_EQ(reader.getAvailableCount(), 1u);

    count = 1;
    TypeParam sampleInt32[1 * BLOCK_SIZE]{};
    
    {    
        // read event packet
        size_t tmpCount = 1;
        auto status = reader.read((TypeParam*) &sampleInt32, &tmpCount);
        ASSERT_EQ(status.getReadStatus(), ReadStatus::Event);
    }

    reader.read((TypeParam*) &sampleInt32, &count);

    ASSERT_EQ(reader.getAvailableCount(), 0u);

    ASSERT_EQ(sampleInt32[0], TypeParam(3));
    ASSERT_EQ(sampleInt32[1], TypeParam(4));
}

TYPED_TEST(BlockReaderTest, DescriptorChangedNotConvertible)
{
    this->signal.setDescriptor(setupDescriptor(SampleType::Int32));

    auto reader = daq::BlockReader<std::int32_t, ClockRange>(this->signal, BLOCK_SIZE);
    this->signal.setDescriptor(setupDescriptor(SampleType::ComplexFloat32));

    const SizeT NUM_SAMPLES = 2;
    auto dataPacket = DataPacket(this->signal.getDescriptor(), NUM_SAMPLES);
    auto dataPtr = static_cast<ComplexFloat32*>(dataPacket.getData());
    dataPtr[0] = 111;
    dataPtr[1] = 222;

    this->sendPacket(dataPacket);
    this->scheduler.waitAll();

    SizeT count{1};
    std::int32_t samples[1 * BLOCK_SIZE];
    auto status = reader.read((std::int32_t*) &samples, &count);
    ASSERT_FALSE(status.getValid());
}

TYPED_TEST(BlockReaderTest, ReuseReader)
{
    this->signal.setDescriptor(setupDescriptor(SampleType::Int32));

    auto reader = daq::BlockReader<TypeParam, ClockRange>(this->signal, BLOCK_SIZE);
    this->signal.setDescriptor(setupDescriptor(SampleType::ComplexFloat32));

    const SizeT NUM_SAMPLES = 2 * BLOCK_SIZE;
    auto dataPacket = DataPacket(this->signal.getDescriptor(), NUM_SAMPLES);
    auto dataPtr = static_cast<ComplexFloat32*>(dataPacket.getData());
    dataPtr[0] = 11;
    dataPtr[1] = 22;
    dataPtr[2] = 33;
    dataPtr[3] = 44;

    this->sendPacket(dataPacket);
    this->scheduler.waitAll();

    SizeT count{1};
    TypeParam samples[1 * BLOCK_SIZE];
    {    
        // read event packet
        size_t tmpCount = 1;
        auto status = reader.read((TypeParam*) &samples, &tmpCount);
        ASSERT_EQ(status.getReadStatus(), ReadStatus::Event);
    }

    auto status = reader.read((TypeParam*) &samples, &count);

    Bool convertable = IsTemplateOf<TypeParam, Complex_Number>::value;
    ASSERT_EQ(status.getValid(), convertable);

    auto newReader = daq::BlockReaderFromExisting<ComplexFloat32, ClockRange>(reader, reader.getBlockSize());

    SizeT complexCount{1};
    ComplexFloat32 complexSamples[1 * BLOCK_SIZE];
    status = newReader.read((ComplexFloat32*) &complexSamples, &complexCount);
    ASSERT_EQ(status.getReadStatus(), ReadStatus::Ok);

    ASSERT_EQ(complexCount, 1u);

    ComplexFloat32 expectedSample[BLOCK_SIZE]{};
    if (!convertable)
    {
        expectedSample[0] = 11;
        expectedSample[1] = 22;
    }
    else
    {
        expectedSample[0] = 33;
        expectedSample[1] = 44;
    }
    ASSERT_EQ(complexSamples[0], expectedSample[0]);
    ASSERT_EQ(complexSamples[1], expectedSample[1]);
}

TYPED_TEST(BlockReaderTest, ReadUndefinedNoDomain)
{
    this->signal.setDescriptor(setupDescriptor(SampleType::Float64));

    auto reader = daq::BlockReader(this->signal, BLOCK_SIZE, SampleType::Undefined, SampleType::Undefined);

    ASSERT_EQ(reader.getValueReadType(), SampleType::Float64);
    ASSERT_EQ(reader.getDomainReadType(), SampleType::Invalid);
}

TYPED_TEST(BlockReaderTest, ReadUndefinedWithDomain)
{
    this->signal.setDescriptor(setupDescriptor(SampleType::Float64));

    auto reader = daq::BlockReader(this->signal, BLOCK_SIZE, SampleType::Undefined, SampleType::Undefined);

    ASSERT_EQ(reader.getValueReadType(), SampleType::Float64);  // read from signal descriptor
    ASSERT_EQ(reader.getDomainReadType(), SampleType::Invalid);

    auto domainDescriptor = setupDescriptor(SampleType::RangeInt64, LinearDataRule(1, 0), nullptr);
    auto eventPacket = DataDescriptorChangedEventPacket(nullptr, domainDescriptor);
    this->sendPacket(eventPacket);

    auto domainPacket = DataPacket(domainDescriptor, BLOCK_SIZE, 1);
    auto dataPacket = DataPacketWithDomain(domainPacket, this->signal.getDescriptor(), BLOCK_SIZE);
    auto dataPtr = static_cast<double*>(dataPacket.getData());
    dataPtr[0] = 11.1;
    dataPtr[1] = 22.2;

    this->sendPacket(dataPacket);

    SizeT count{1};
    double samples[BLOCK_SIZE]{};

    {    
        // read event packet
        size_t tmpCount = 1;
        auto status = reader.read(&samples, &tmpCount);
        ASSERT_EQ(status.getReadStatus(), ReadStatus::Event);
    }

    reader.read(&samples, &count);

    ASSERT_EQ(count, 1u);

    ASSERT_EQ(reader.getValueReadType(), SampleType::Float64);
    // domain info available from descriptor change event
    ASSERT_EQ(reader.getDomainReadType(), SampleType::RangeInt64);
}

TYPED_TEST(BlockReaderTest, ReadUndefinedWithNoDomainFromPacket)
{
    this->signal.setDescriptor(setupDescriptor(SampleType::Float64));

    auto reader = daq::BlockReader(this->signal, BLOCK_SIZE, SampleType::Undefined, SampleType::Undefined);

    ASSERT_EQ(reader.getValueReadType(), SampleType::Float64);  // read from signal descriptor
    ASSERT_EQ(reader.getDomainReadType(), SampleType::Invalid);

    auto domainPacket = DataPacket(setupDescriptor(SampleType::RangeInt64, LinearDataRule(1, 0), nullptr), BLOCK_SIZE, 1);
    auto dataPacket = DataPacketWithDomain(domainPacket, this->signal.getDescriptor(), BLOCK_SIZE);
    auto dataPtr = static_cast<double*>(dataPacket.getData());
    dataPtr[0] = 111.1;
    dataPtr[1] = 222.2;

    this->sendPacket(dataPacket);

    SizeT count{1};
    double samples[BLOCK_SIZE]{};
    reader.read(&samples, &count);

    ASSERT_EQ(count, 1u);
    ASSERT_EQ(reader.getValueReadType(), SampleType::Float64);

    // domain was not read so its info was not updated
    ASSERT_EQ(reader.getDomainReadType(), SampleType::Invalid);
}

TYPED_TEST(BlockReaderTest, ReadUndefinedWithWithDomainFromPacket)
{
    this->signal.setDescriptor(setupDescriptor(SampleType::Float64));

    auto reader = daq::BlockReader(this->signal, BLOCK_SIZE, SampleType::Undefined, SampleType::Undefined);

    ASSERT_EQ(reader.getValueReadType(), SampleType::Float64);  // read from signal descriptor
    ASSERT_EQ(reader.getDomainReadType(), SampleType::Invalid);

    auto domainPacket = DataPacket(setupDescriptor(SampleType::RangeInt64, LinearDataRule(1, 0), nullptr), BLOCK_SIZE, 1);
    auto dataPacket = DataPacketWithDomain(domainPacket, this->signal.getDescriptor(), BLOCK_SIZE);
    auto dataPtr = static_cast<double*>(dataPacket.getData());
    dataPtr[0] = 111.1;
    dataPtr[1] = 222.2;

    this->sendPacket(dataPacket);

    SizeT count{1};
    double samples[BLOCK_SIZE]{};
    RangeType64 domain[BLOCK_SIZE]{};
    reader.readWithDomain(&samples, &domain, &count);

    ASSERT_EQ(count, 1u);
    ASSERT_EQ(reader.getValueReadType(), SampleType::Float64);

    // domain was read and updated from packet info
    ASSERT_EQ(reader.getDomainReadType(), SampleType::RangeInt64);
}

TYPED_TEST(BlockReaderTest, ToString)
{
    auto reader = daq::BlockReader<TypeParam, ClockRange>(this->signal, BLOCK_SIZE);

    CharPtr str;
    ASSERT_EQ(reader->toString(&str), OPENDAQ_SUCCESS);

    ASSERT_STREQ(str, "daq::IBlockReader");
    daqFreeMemory(str);
}

TYPED_TEST(BlockReaderTest, BlockReaderWithInputPort)
{
    this->signal.setDescriptor(setupDescriptor(SampleType::Float64));
    
    auto port = InputPort(this->signal.getContext(), nullptr, "readsig");
    port.connect(this->signal);
    auto reader = daq::BlockReaderFromPort(port, BLOCK_SIZE, SampleType::Undefined, SampleType::Undefined);
    
    auto domainPacket = DataPacket(setupDescriptor(SampleType::RangeInt64, LinearDataRule(1, 0), nullptr), BLOCK_SIZE, 1);
    auto dataPacket = DataPacketWithDomain(domainPacket, this->signal.getDescriptor(), BLOCK_SIZE);
    auto dataPtr = static_cast<double*>(dataPacket.getData());
    dataPtr[0] = 111.1;
    dataPtr[1] = 222.2;

    this->sendPacket(dataPacket);

    SizeT count{1};
    double samples[BLOCK_SIZE]{};
    RangeType64 domain[BLOCK_SIZE]{};
    reader.readWithDomain(&samples, &domain, &count);

    ASSERT_EQ(count, 1u);
    ASSERT_EQ(samples[0], dataPtr[0]);
    ASSERT_EQ(samples[1], dataPtr[1]);
}

TYPED_TEST(BlockReaderTest, BlockReaderWithNotConnectedInputPort)
{
    this->signal.setDescriptor(setupDescriptor(SampleType::Float64));
    
    auto port = InputPort(this->signal.getContext(), nullptr, "readsig");
    auto reader = daq::BlockReaderFromPort(port, BLOCK_SIZE, SampleType::Undefined, SampleType::Undefined);
    
    auto domainPacket = DataPacket(setupDescriptor(SampleType::RangeInt64, LinearDataRule(1, 0), nullptr), BLOCK_SIZE, 1);
    auto dataPacket = DataPacketWithDomain(domainPacket, this->signal.getDescriptor(), BLOCK_SIZE);
    auto dataPtr = static_cast<double*>(dataPacket.getData());
    dataPtr[0] = 111.1;
    dataPtr[1] = 222.2;

    port.connect(this->signal);
    this->sendPacket(dataPacket);

    SizeT count{1};
    double samples[BLOCK_SIZE]{};
    RangeType64 domain[BLOCK_SIZE]{};
    {    
        // read event packet
        size_t tmpCount = 1;
        auto status = reader.readWithDomain(&samples, &domain, &tmpCount);
        ASSERT_EQ(status.getReadStatus(), ReadStatus::Event);
    }
    reader.readWithDomain(&samples, &domain, &count);

    ASSERT_EQ(count, 1u);
    ASSERT_EQ(samples[0], dataPtr[0]);
    ASSERT_EQ(samples[1], dataPtr[1]);
}

TYPED_TEST(BlockReaderTest, MultipleBlockReaderToInputPort)
{
    this->signal.setDescriptor(setupDescriptor(SampleType::Float64));
    
    auto port = InputPort(this->signal.getContext(), nullptr, "readsig");
    port.connect(this->signal);
    auto reader1 = daq::BlockReaderFromPort(port, BLOCK_SIZE, SampleType::Undefined, SampleType::Undefined);
    ASSERT_THROW(daq::BlockReaderFromPort(port, BLOCK_SIZE, SampleType::Undefined, SampleType::Undefined), AlreadyExistsException);
}

TYPED_TEST(BlockReaderTest, BlockReaderReuseInputPort)
{
    this->signal.setDescriptor(setupDescriptor(SampleType::Float64));
    
    auto port = InputPort(this->signal.getContext(), nullptr, "readsig");
    port.connect(this->signal);
    {
        auto reader1 = daq::BlockReaderFromPort(port, BLOCK_SIZE, SampleType::Undefined, SampleType::Undefined);
    }
    ASSERT_NO_THROW(daq::BlockReaderFromPort(port, BLOCK_SIZE, SampleType::Undefined, SampleType::Undefined));
}

TYPED_TEST(BlockReaderTest, BlockReaderOnReadCallback)
{
    SizeT count{1};
    double samples[BLOCK_SIZE]{};
    RangeType64 domain[BLOCK_SIZE]{};

    std::promise<void> promise;
    std::future<void> future = promise.get_future();

    this->signal.setDescriptor(setupDescriptor(SampleType::Float64));

    auto reader = daq::BlockReader(this->signal, BLOCK_SIZE, SampleType::Undefined, SampleType::Undefined);
    reader.setOnDataAvailable([&, promise = std::move(promise)] () mutable {
        reader.readWithDomain(&samples, &domain, &count);
        promise.set_value();
    });

    auto domainPacket = DataPacket(setupDescriptor(SampleType::RangeInt64, LinearDataRule(1, 0), nullptr), BLOCK_SIZE, 1);
    auto dataPacket = DataPacketWithDomain(domainPacket, this->signal.getDescriptor(), BLOCK_SIZE);
    auto dataPtr = static_cast<double*>(dataPacket.getData());
    dataPtr[0] = 111.1;
    dataPtr[1] = 222.2;

    this->sendPacket(dataPacket);

    auto promiseStatus = future.wait_for(std::chrono::seconds(1));
    ASSERT_EQ(promiseStatus, std::future_status::ready);

    ASSERT_EQ(count, 1u);
    ASSERT_EQ(samples[0], dataPtr[0]);
    ASSERT_EQ(samples[1], dataPtr[1]);
}

TYPED_TEST(BlockReaderTest, BlockReaderFromPortOnReadCallback)
{
    SizeT count{1};
    double samples[BLOCK_SIZE]{};
    RangeType64 domain[BLOCK_SIZE]{};

    std::promise<void> promise;
    std::future<void> future = promise.get_future();

    this->signal.setDescriptor(setupDescriptor(SampleType::Float64));
    auto port = InputPort(this->signal.getContext(), nullptr, "readsig");
    port.connect(this->signal);

    auto reader = daq::BlockReaderFromPort(port, BLOCK_SIZE, SampleType::Undefined, SampleType::Undefined);
    reader.setOnDataAvailable([&, promise = std::move(promise)] () mutable {
        reader.readWithDomain(&samples, &domain, &count);
        promise.set_value();
    });

    auto domainPacket = DataPacket(setupDescriptor(SampleType::RangeInt64, LinearDataRule(1, 0), nullptr), BLOCK_SIZE, 1);
    auto dataPacket = DataPacketWithDomain(domainPacket, this->signal.getDescriptor(), BLOCK_SIZE);
    auto dataPtr = static_cast<double*>(dataPacket.getData());
    dataPtr[0] = 111.1;
    dataPtr[1] = 222.2;

    this->sendPacket(dataPacket);

    auto promiseStatus = future.wait_for(std::chrono::seconds(1));
    ASSERT_EQ(promiseStatus, std::future_status::ready);

    ASSERT_EQ(count, 1u);
    ASSERT_EQ(samples[0], dataPtr[0]);
    ASSERT_EQ(samples[1], dataPtr[1]);
}

TYPED_TEST(BlockReaderTest, BlockReaderFromExistingOnReadCallback)
{
    SizeT count{1};
    double samples[BLOCK_SIZE]{};
    RangeType64 domain[BLOCK_SIZE]{};

    std::promise<void> promise;
    std::future<void> future = promise.get_future();

    this->signal.setDescriptor(setupDescriptor(SampleType::Int64));

    BlockReaderPtr reader = daq::BlockReader(this->signal, 1, SampleType::Float64, SampleType::RangeInt64);
    BlockReaderPtr newReader;
    
    reader.setOnDataAvailable([&, promise = std::move(promise)] () mutable {
        if (!newReader.assigned())
        {
            SizeT tmpCount = 1;
            auto status = reader.readWithDomain(&samples, &domain, &tmpCount);
            if (status.getReadStatus() == ReadStatus::Event)
            {
                newReader = daq::BlockReaderFromExisting(reader, BLOCK_SIZE, SampleType::Float64, SampleType::RangeInt64);
            }
        }
        else
        {
            newReader.readWithDomain(&samples, &domain, &count);
            promise.set_value();
        }
    });

    this->signal.setDescriptor(setupDescriptor(SampleType::Float64));
    auto domainPacket = DataPacket(setupDescriptor(SampleType::RangeInt64, LinearDataRule(1, 0), nullptr), BLOCK_SIZE, 1);
    auto dataPacket = DataPacketWithDomain(domainPacket, this->signal.getDescriptor(), BLOCK_SIZE);
    auto dataPtr = static_cast<double*>(dataPacket.getData());
    dataPtr[0] = 111.1;
    dataPtr[1] = 222.2;

    this->sendPacket(dataPacket);

    auto promiseStatus = future.wait_for(std::chrono::seconds(1));
    ASSERT_EQ(promiseStatus, std::future_status::ready);

    ASSERT_EQ(count, 1u);
    ASSERT_EQ(samples[0], dataPtr[0]);
    ASSERT_EQ(samples[1], dataPtr[1]);
}