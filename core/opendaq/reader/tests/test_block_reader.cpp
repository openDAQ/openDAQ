#include <gmock/gmock.h>
#include <opendaq/data_packet_ptr.h>
#include <opendaq/data_rule_factory.h>
#include <opendaq/event_packet_params.h>
#include <opendaq/event_packet_ptr.h>
#include <opendaq/input_port_factory.h>
#include <opendaq/packet_factory.h>
#include <opendaq/reader_errors.h>
#include <opendaq/reader_exceptions.h>
#include <opendaq/reader_factory.h>
#include <opendaq/stream_reader_ptr.h>
#include <testutils/testutils.h>
#include <future>
#include "reader_common.h"

using namespace daq;
using namespace testing;

template <typename T>
using BlockReaderTest = ReaderTest<T>;

using SampleTypes = ::testing::Types<OPENDAQ_VALUE_SAMPLE_TYPES>;

TYPED_TEST_SUITE(BlockReaderTest, SampleTypes);

static constexpr const SizeT BLOCK_SIZE = 2u;
static constexpr const SizeT OVERLAP = 50;  // %
static constexpr auto READ_MODE = ReadMode::Scaled;

TYPED_TEST(BlockReaderTest, Create)
{
    ASSERT_NO_THROW((BlockReader<TypeParam, ClockRange>) (this->signal, BLOCK_SIZE));
    ASSERT_NO_THROW(BlockReaderBuilder()
                        .setSignal(this->signal)
                        .setValueReadType(SampleTypeFromType<TypeParam>::SampleType)
                        .setDomainReadType(SampleTypeFromType<ClockRange>::SampleType)
                        .setBlockSize(BLOCK_SIZE)
                        .build());
}

TYPED_TEST(BlockReaderTest, CreateReuseBuilderThrows)
{
    auto builder = BlockReaderBuilder()
                       .setSignal(this->signal)
                       .setValueReadType(SampleTypeFromType<TypeParam>::SampleType)
                       .setDomainReadType(SampleTypeFromType<ClockRange>::SampleType)
                       .setBlockSize(BLOCK_SIZE);
    ASSERT_NO_THROW(builder.build());
    ASSERT_THROW(builder.build(), CreateFailedException);
}

TYPED_TEST(BlockReaderTest, CreateOverlapped)
{
    ASSERT_NO_THROW(BlockReaderBuilder()
                        .setSignal(this->signal)
                        .setValueReadType(SampleTypeFromType<TypeParam>::SampleType)
                        .setDomainReadType(SampleTypeFromType<ClockRange>::SampleType)
                        .setBlockSize(BLOCK_SIZE)
                        .setOverlap(OVERLAP)
                        .build());
}

TYPED_TEST(BlockReaderTest, CreateNullThrows)
{
    ASSERT_THROW_MSG((BlockReader<TypeParam, ClockRange>) (nullptr, BLOCK_SIZE), ArgumentNullException, "Signal must not be null.");
    ASSERT_THROW(BlockReaderBuilder()
                     .setValueReadType(SampleTypeFromType<TypeParam>::SampleType)
                     .setDomainReadType(SampleTypeFromType<ClockRange>::SampleType)
                     .build(),
                 CreateFailedException);
    ASSERT_THROW(BlockReaderBuilder()
                     .setValueReadType(SampleTypeFromType<TypeParam>::SampleType)
                     .setDomainReadType(SampleTypeFromType<ClockRange>::SampleType)
                     .setBlockSize(BLOCK_SIZE)
                     .build(),
                 CreateFailedException);
}

TYPED_TEST(BlockReaderTest, IsReader)
{
    auto reader = daq::BlockReader<TypeParam, ClockRange>(this->signal, BLOCK_SIZE);
    ASSERT_NO_THROW(reader.template asPtr<IReader>());

    reader = BlockReaderBuilder()
                 .setValueReadType(SampleTypeFromType<TypeParam>::SampleType)
                 .setDomainReadType(SampleTypeFromType<ClockRange>::SampleType)
                 .setSignal(this->signal)
                 .setBlockSize(BLOCK_SIZE)
                 .build();
    ASSERT_NO_THROW(reader.template asPtr<IReader>());
}

TYPED_TEST(BlockReaderTest, IsSampleReader)
{
    auto reader = daq::BlockReader<TypeParam, ClockRange>(this->signal, BLOCK_SIZE);
    ASSERT_NO_THROW(reader.template asPtr<ISampleReader>());

    reader = BlockReaderBuilder()
                 .setValueReadType(SampleTypeFromType<TypeParam>::SampleType)
                 .setDomainReadType(SampleTypeFromType<ClockRange>::SampleType)
                 .setSignal(this->signal)
                 .setBlockSize(BLOCK_SIZE)
                 .build();
    ASSERT_NO_THROW(reader.template asPtr<ISampleReader>());
}

TYPED_TEST(BlockReaderTest, IsBlockReader)
{
    auto reader = daq::BlockReader<TypeParam, ClockRange>(this->signal, BLOCK_SIZE);
    ASSERT_NO_THROW(reader.template asPtr<IBlockReader>());

    reader = BlockReaderBuilder()
                 .setValueReadType(SampleTypeFromType<TypeParam>::SampleType)
                 .setDomainReadType(SampleTypeFromType<ClockRange>::SampleType)
                 .setSignal(this->signal)
                 .setBlockSize(BLOCK_SIZE)
                 .build();
    ASSERT_NO_THROW(reader.template asPtr<IBlockReader>());
}

TYPED_TEST(BlockReaderTest, GetDefaultReadType)
{
    auto reader = BlockReader<TypeParam>(this->signal, BLOCK_SIZE);
    ASSERT_EQ(reader.getValueReadType(), SampleTypeFromType<TypeParam>::SampleType);
    ASSERT_EQ(reader.getDomainReadType(), SampleType::Int64);

    reader = BlockReaderBuilder()
                 .setValueReadType(SampleTypeFromType<TypeParam>::SampleType)
                 .setSignal(this->signal)
                 .setBlockSize(BLOCK_SIZE)
                 .build();
    ASSERT_EQ(reader.getValueReadType(), SampleTypeFromType<TypeParam>::SampleType);
    ASSERT_EQ(reader.getDomainReadType(), SampleType::Int64);
}

TYPED_TEST(BlockReaderTest, GetBlocksAvailableEmpty)
{
    auto reader = BlockReaderBuilder()
                      .setValueReadType(SampleTypeFromType<TypeParam>::SampleType)
                      .setSignal(this->signal)
                      .setBlockSize(BLOCK_SIZE)
                      .build();
    ASSERT_EQ(reader.getAvailableCount(), 0u);
}

TYPED_TEST(BlockReaderTest, GetOverlappedBlocksAvailableEmpty)
{
    auto reader = BlockReaderBuilder()
                      .setSignal(this->signal)
                      .setValueReadType(SampleTypeFromType<TypeParam>::SampleType)
                      .setDomainReadType(SampleTypeFromType<ClockRange>::SampleType)
                      .setBlockSize(BLOCK_SIZE)
                      .setOverlap(OVERLAP)
                      .build();
    ASSERT_EQ(reader.getAvailableCount(), 0u);
}

TYPED_TEST(BlockReaderTest, GetBlocksAvailable)
{
    this->signal.setDescriptor(setupDescriptor(SampleType::Float64));

    auto reader = BlockReaderBuilder()
                      .setSignal(this->signal)
                      .setValueReadType(SampleTypeFromType<TypeParam>::SampleType)
                      .setDomainReadType(SampleTypeFromType<ClockRange>::SampleType)
                      .setBlockSize(BLOCK_SIZE)
                      .setSkipEvents(true)
                      .build();

    ASSERT_EQ(reader.getAvailableCount(), 0u);

    this->sendPacket(DataPacket(this->signal.getDescriptor(), 1));
    this->scheduler.waitAll();

    ASSERT_EQ(reader.getAvailableCount(), 0u);

    this->sendPacket(DataPacket(this->signal.getDescriptor(), 1));
    this->scheduler.waitAll();

    ASSERT_EQ(reader.getAvailableCount(), 1u);
}

TYPED_TEST(BlockReaderTest, GetOverlappedBlocksAvailable)
{
    this->signal.setDescriptor(setupDescriptor(SampleType::Float64));

    auto reader = BlockReaderBuilder()
                      .setSignal(this->signal)
                      .setValueReadType(SampleTypeFromType<TypeParam>::SampleType)
                      .setDomainReadType(SampleTypeFromType<ClockRange>::SampleType)
                      .setBlockSize(BLOCK_SIZE)
                      .setOverlap(OVERLAP)
                      .setSkipEvents(true)
                      .build();

    ASSERT_EQ(reader.getAvailableCount(), 0u);

    this->sendPacket(DataPacket(this->signal.getDescriptor(), 1));
    this->scheduler.waitAll();

    ASSERT_EQ(reader.getAvailableCount(), 0u);

    this->sendPacket(DataPacket(this->signal.getDescriptor(), 1));
    this->scheduler.waitAll();

    ASSERT_EQ(reader.getAvailableCount(), 1u);

    this->sendPacket(DataPacket(this->signal.getDescriptor(), 1));
    this->scheduler.waitAll();

    ASSERT_EQ(reader.getAvailableCount(), 2u);
}

TYPED_TEST(BlockReaderTest, ReadOneBlock)
{
    this->signal.setDescriptor(setupDescriptor(SampleType::Float64));

    auto reader = BlockReaderBuilder()
                      .setSignal(this->signal)
                      .setValueReadType(SampleTypeFromType<TypeParam>::SampleType)
                      .setDomainReadType(SampleTypeFromType<ClockRange>::SampleType)
                      .setBlockSize(BLOCK_SIZE)
                      .setSkipEvents(true)
                      .build();

    auto dataPacket = DataPacket(this->signal.getDescriptor(), 1 * BLOCK_SIZE);
    auto dataPtr = static_cast<double*>(dataPacket.getData());
    dataPtr[0] = 11.1;
    dataPtr[1] = 22.2;

    this->sendPacket(dataPacket);
    this->scheduler.waitAll();

    SizeT count{1};
    TypeParam samples[1 * BLOCK_SIZE]{};
    reader.read(&samples, &count);

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

TYPED_TEST(BlockReaderTest, ReadThreeBlocksOverlapped)
{
    this->signal.setDescriptor(setupDescriptor(SampleType::Float64));

    auto reader = BlockReaderBuilder()
                      .setSignal(this->signal)
                      .setValueReadType(SampleTypeFromType<TypeParam>::SampleType)
                      .setDomainReadType(SampleTypeFromType<ClockRange>::SampleType)
                      .setBlockSize(BLOCK_SIZE)
                      .setOverlap(OVERLAP)
                      .setSkipEvents(true)
                      .build();

    auto dataPacket = DataPacket(this->signal.getDescriptor(), 1 * BLOCK_SIZE);
    auto dataPtr = static_cast<double*>(dataPacket.getData());
    dataPtr[0] = 11.1;
    dataPtr[1] = 22.2;

    this->sendPacket(dataPacket);
    this->sendPacket(dataPacket);
    this->scheduler.waitAll();

    SizeT count{3};
    TypeParam samples[3 * BLOCK_SIZE]{};
    reader.read(&samples, &count);

    ASSERT_EQ(count, 3u);
    ASSERT_EQ(reader.getAvailableCount(), 0u);

    if constexpr (IsTemplateOf<TypeParam, Complex_Number>::value || IsTemplateOf<TypeParam, RangeType>::value)
    {
        ASSERT_EQ(samples[0], TypeParam(typename TypeParam::Type(11.1)));
        ASSERT_EQ(samples[1], TypeParam(typename TypeParam::Type(22.2)));

        ASSERT_EQ(samples[2], TypeParam(typename TypeParam::Type(22.2)));
        ASSERT_EQ(samples[3], TypeParam(typename TypeParam::Type(11.1)));

        ASSERT_EQ(samples[4], TypeParam(typename TypeParam::Type(11.1)));
        ASSERT_EQ(samples[5], TypeParam(typename TypeParam::Type(22.2)));
    }
    else
    {
        ASSERT_EQ(samples[0], TypeParam(11.1));
        ASSERT_EQ(samples[1], TypeParam(22.2));

        ASSERT_EQ(samples[2], TypeParam(22.2));
        ASSERT_EQ(samples[3], TypeParam(11.1));

        ASSERT_EQ(samples[4], TypeParam(11.1));
        ASSERT_EQ(samples[5], TypeParam(22.2));
    }
}

TYPED_TEST(BlockReaderTest, ReadBigBlocksOverlapped)
{
    this->signal.setDescriptor(setupDescriptor(SampleType::Float64));

    const auto BIG_BLOCK_SIZE = 10;  // samples
    const auto PACKET_SIZE = 2;      // sample

    auto reader = BlockReaderBuilder()
                      .setSignal(this->signal)
                      .setValueReadType(SampleTypeFromType<TypeParam>::SampleType)
                      .setDomainReadType(SampleTypeFromType<ClockRange>::SampleType)
                      .setBlockSize(BIG_BLOCK_SIZE)
                      .setOverlap(OVERLAP)
                      .setSkipEvents(true)
                      .build();

    double d[] = {0, 1.1, 2.2, 3.3, 4.4, 5.5, 6.6, 7.7, 8.8, 9.9, 0, 1.1, 2.2, 3.3, 4.4, 5.5, 6.6, 7.7, 8.8, 9.9};

    for (int i = 0; i < 2 * BIG_BLOCK_SIZE; i += 2)
    {
        auto dataPacket = DataPacket(this->signal.getDescriptor(), PACKET_SIZE);
        auto dataPtr = static_cast<double*>(dataPacket.getData());
        dataPtr[0] = d[i];
        dataPtr[1] = d[i + 1];
        this->sendPacket(dataPacket);
    }
    this->scheduler.waitAll();

    SizeT count{3};
    TypeParam samples[3 * BIG_BLOCK_SIZE]{};
    reader.read(&samples, &count);

    ASSERT_EQ(count, 3u);
    ASSERT_EQ(reader.getAvailableCount(), 0u);

    if constexpr (IsTemplateOf<TypeParam, Complex_Number>::value || IsTemplateOf<TypeParam, RangeType>::value)
    {
        ASSERT_EQ(samples[0], TypeParam(typename TypeParam::Type(d[0])));
        ASSERT_EQ(samples[1], TypeParam(typename TypeParam::Type(d[1])));
        ASSERT_EQ(samples[2], TypeParam(typename TypeParam::Type(d[2])));
        ASSERT_EQ(samples[3], TypeParam(typename TypeParam::Type(d[3])));
        ASSERT_EQ(samples[4], TypeParam(typename TypeParam::Type(d[4])));
        ASSERT_EQ(samples[5], TypeParam(typename TypeParam::Type(d[5])));
        ASSERT_EQ(samples[6], TypeParam(typename TypeParam::Type(d[6])));
        ASSERT_EQ(samples[7], TypeParam(typename TypeParam::Type(d[7])));
        ASSERT_EQ(samples[8], TypeParam(typename TypeParam::Type(d[8])));
        ASSERT_EQ(samples[9], TypeParam(typename TypeParam::Type(d[9])));

        ASSERT_EQ(samples[10], TypeParam(typename TypeParam::Type(d[5])));
        ASSERT_EQ(samples[11], TypeParam(typename TypeParam::Type(d[6])));
        ASSERT_EQ(samples[12], TypeParam(typename TypeParam::Type(d[7])));
        ASSERT_EQ(samples[13], TypeParam(typename TypeParam::Type(d[8])));
        ASSERT_EQ(samples[14], TypeParam(typename TypeParam::Type(d[9])));
        ASSERT_EQ(samples[15], TypeParam(typename TypeParam::Type(d[10])));
        ASSERT_EQ(samples[16], TypeParam(typename TypeParam::Type(d[11])));
        ASSERT_EQ(samples[17], TypeParam(typename TypeParam::Type(d[12])));
        ASSERT_EQ(samples[18], TypeParam(typename TypeParam::Type(d[13])));
        ASSERT_EQ(samples[19], TypeParam(typename TypeParam::Type(d[14])));

        ASSERT_EQ(samples[20], TypeParam(typename TypeParam::Type(d[10])));
        ASSERT_EQ(samples[21], TypeParam(typename TypeParam::Type(d[11])));
        ASSERT_EQ(samples[22], TypeParam(typename TypeParam::Type(d[12])));
        ASSERT_EQ(samples[23], TypeParam(typename TypeParam::Type(d[13])));
        ASSERT_EQ(samples[24], TypeParam(typename TypeParam::Type(d[14])));
        ASSERT_EQ(samples[25], TypeParam(typename TypeParam::Type(d[15])));
        ASSERT_EQ(samples[26], TypeParam(typename TypeParam::Type(d[16])));
        ASSERT_EQ(samples[27], TypeParam(typename TypeParam::Type(d[17])));
        ASSERT_EQ(samples[28], TypeParam(typename TypeParam::Type(d[18])));
        ASSERT_EQ(samples[29], TypeParam(typename TypeParam::Type(d[19])));
    }
    else
    {
        ASSERT_EQ(samples[0], TypeParam(d[0]));
        ASSERT_EQ(samples[1], TypeParam(d[1]));
        ASSERT_EQ(samples[2], TypeParam(d[2]));
        ASSERT_EQ(samples[3], TypeParam(d[3]));
        ASSERT_EQ(samples[4], TypeParam(d[4]));
        ASSERT_EQ(samples[5], TypeParam(d[5]));
        ASSERT_EQ(samples[6], TypeParam(d[6]));
        ASSERT_EQ(samples[7], TypeParam(d[7]));
        ASSERT_EQ(samples[8], TypeParam(d[8]));
        ASSERT_EQ(samples[9], TypeParam(d[9]));

        ASSERT_EQ(samples[10], TypeParam(d[5]));
        ASSERT_EQ(samples[11], TypeParam(d[6]));
        ASSERT_EQ(samples[12], TypeParam(d[7]));
        ASSERT_EQ(samples[13], TypeParam(d[8]));
        ASSERT_EQ(samples[14], TypeParam(d[9]));
        ASSERT_EQ(samples[15], TypeParam(d[10]));
        ASSERT_EQ(samples[16], TypeParam(d[11]));
        ASSERT_EQ(samples[17], TypeParam(d[12]));
        ASSERT_EQ(samples[18], TypeParam(d[13]));
        ASSERT_EQ(samples[19], TypeParam(d[14]));

        ASSERT_EQ(samples[20], TypeParam(d[10]));
        ASSERT_EQ(samples[21], TypeParam(d[11]));
        ASSERT_EQ(samples[22], TypeParam(d[12]));
        ASSERT_EQ(samples[23], TypeParam(d[13]));
        ASSERT_EQ(samples[24], TypeParam(d[14]));
        ASSERT_EQ(samples[25], TypeParam(d[15]));
        ASSERT_EQ(samples[26], TypeParam(d[16]));
        ASSERT_EQ(samples[27], TypeParam(d[17]));
        ASSERT_EQ(samples[28], TypeParam(d[18]));
        ASSERT_EQ(samples[29], TypeParam(d[19]));
    }
}

TYPED_TEST(BlockReaderTest, ReadSamplesCountBeforeEventOverlapped)
{
    this->signal.setDescriptor(setupDescriptor(SampleType::Float64));

    const auto BIG_BLOCK_SIZE = 10;  // samples
    const auto PACKET_SIZE = 2;      // sample

    auto reader = BlockReaderBuilder()
                      .setSignal(this->signal)
                      .setValueReadType(SampleTypeFromType<TypeParam>::SampleType)
                      .setDomainReadType(SampleTypeFromType<ClockRange>::SampleType)
                      .setBlockSize(BIG_BLOCK_SIZE)
                      .setOverlap(OVERLAP)
                      .build();

    {
        SizeT count{0};
        auto status = reader.read(nullptr, &count);
        ASSERT_EQ(status.getReadStatus(), ReadStatus::Event);
    }

    double d[] = {0.1, 1.1, 2.2, 3.3, 4.4, 5.5, 6.6, 7.7, 8.8, 9.9, 0.1, 1.1, 2.2, 3.3, 4.4, 5.5, 6.6, 7.7, 8.8, 9.9};

    {
        auto dataPacket = DataPacket(this->signal.getDescriptor(), PACKET_SIZE);
        auto dataPtr = static_cast<double*>(dataPacket.getData());
        dataPtr[0] = d[0];
        dataPtr[1] = d[1];
        this->sendPacket(dataPacket);
        // 1 packet has been sent. (BIG_BLOCK_SIZE / PACKET_SIZE - 1) left.
    }

    // generate EventPacket
    this->signal.setDescriptor(setupDescriptor(SampleType::Int64));

    for (auto i = 0, j = 2; i < (BIG_BLOCK_SIZE / PACKET_SIZE - 1); ++i, j += 2)
    {
        auto dataPacket = DataPacket(this->signal.getDescriptor(), PACKET_SIZE);
        auto dataPtr = static_cast<double*>(dataPacket.getData());
        dataPtr[0] = d[j];
        dataPtr[1] = d[j + 1];
        this->sendPacket(dataPacket);
    }

    SizeT count{1};
    TypeParam samples[BIG_BLOCK_SIZE]{};
    BlockReaderStatusPtr status = reader.read(&samples, &count, 1000u);

    ASSERT_EQ(count, 0u);
    ASSERT_TRUE(status.assigned());
    ASSERT_EQ(status.getReadStatus(), ReadStatus::Event);
    ASSERT_EQ(status.getReadSamples(), 2u);
    ASSERT_EQ(reader.getAvailableCount(), 0u);

    if constexpr (IsTemplateOf<TypeParam, Complex_Number>::value || IsTemplateOf<TypeParam, RangeType>::value)
    {
        ASSERT_EQ(samples[0], TypeParam(typename TypeParam::Type(d[0])));
        ASSERT_EQ(samples[1], TypeParam(typename TypeParam::Type(d[1])));
    }
    else
    {
        ASSERT_EQ(samples[0], TypeParam(d[0]));
        ASSERT_EQ(samples[1], TypeParam(d[1]));
    }
}

TYPED_TEST(BlockReaderTest, ReadSamplesCountBeforeTimeoutOverlapped)
{
    this->signal.setDescriptor(setupDescriptor(SampleType::Float64));

    const auto BIG_BLOCK_SIZE = 10;  // samples
    const auto PACKET_SIZE = 2;      // sample

    auto reader = BlockReaderBuilder()
                      .setSignal(this->signal)
                      .setValueReadType(SampleTypeFromType<TypeParam>::SampleType)
                      .setDomainReadType(SampleTypeFromType<ClockRange>::SampleType)
                      .setBlockSize(BIG_BLOCK_SIZE)
                      .setOverlap(OVERLAP)
                      .build();

    {
        SizeT count{0};
        auto status = reader.read(nullptr, &count);
        ASSERT_EQ(status.getReadStatus(), ReadStatus::Event);
    }
    
    double d[] = {0.1, 1.1, 2.2, 3.3, 4.4, 5.5, 6.6, 7.7, 8.8, 9.9, 0.1, 1.1, 2.2, 3.3, 4.4, 5.5, 6.6, 7.7, 8.8, 9.9};

    {
        auto dataPacket = DataPacket(this->signal.getDescriptor(), PACKET_SIZE);
        auto dataPtr = static_cast<double*>(dataPacket.getData());
        dataPtr[0] = d[0];
        dataPtr[1] = d[1];
        this->sendPacket(dataPacket);
        // 1 packet has been sent. (BIG_BLOCK_SIZE / PACKET_SIZE - 1) left.
    }

    // generate EventPacket
    this->signal.setDescriptor(setupDescriptor(SampleType::Int64));

    SizeT count{1};
    TypeParam samples[BIG_BLOCK_SIZE]{};
    BlockReaderStatusPtr status = reader.read(&samples, &count, 100u);

    ASSERT_TRUE(status.assigned());
    ASSERT_EQ(status.getReadStatus(), ReadStatus::Event);
    ASSERT_EQ(status.getReadSamples(), 2);

    if constexpr (IsTemplateOf<TypeParam, Complex_Number>::value || IsTemplateOf<TypeParam, RangeType>::value)
    {
        ASSERT_EQ(samples[0], TypeParam(typename TypeParam::Type(d[0])));
        ASSERT_EQ(samples[1], TypeParam(typename TypeParam::Type(d[1])));
    }
    else
    {
        ASSERT_EQ(samples[0], TypeParam(d[0]));
        ASSERT_EQ(samples[1], TypeParam(d[1]));
    }
}

TYPED_TEST(BlockReaderTest, ReadOneBlockWithTimeout)
{
    this->signal.setDescriptor(setupDescriptor(SampleType::Float64));

    auto reader = BlockReaderBuilder()
                      .setSignal(this->signal)
                      .setValueReadType(SampleTypeFromType<TypeParam>::SampleType)
                      .setDomainReadType(SampleTypeFromType<ClockRange>::SampleType)
                      .setBlockSize(BLOCK_SIZE)
                      .build();

    {
        SizeT count{0};
        auto status = reader.read(nullptr, &count);
        ASSERT_EQ(status.getReadStatus(), ReadStatus::Event);
    }

    auto dataPacket = DataPacket(this->signal.getDescriptor(), 1 * BLOCK_SIZE);
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
    reader.read(&samples, &count, 1000u);

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

    auto reader = BlockReaderBuilder()
                      .setSignal(this->signal)
                      .setValueReadType(SampleTypeFromType<TypeParam>::SampleType)
                      .setDomainReadType(SampleTypeFromType<ClockTick>::SampleType)
                      .setBlockSize(BLOCK_SIZE)
                      .setSkipEvents(true)
                      .build();

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

TYPED_TEST(BlockReaderTest, ReadThreeBlocksOverlappedWithClockTicks)
{
    this->signal.setDescriptor(setupDescriptor(SampleType::Float64));

    auto reader = BlockReaderBuilder()
                      .setSignal(this->signal)
                      .setValueReadType(SampleTypeFromType<TypeParam>::SampleType)
                      .setDomainReadType(SampleTypeFromType<ClockTick>::SampleType)
                      .setBlockSize(BLOCK_SIZE)
                      .setOverlap(OVERLAP)
                      .setSkipEvents(true)
                      .build();

    auto domainDescriptor = setupDescriptor(SampleType::RangeInt64, LinearDataRule(1, 0), nullptr);

    {
        auto domainPacket = DataPacket(domainDescriptor, 1 * BLOCK_SIZE, 1);
        auto dataPacket = DataPacketWithDomain(domainPacket, this->signal.getDescriptor(), 1 * BLOCK_SIZE);
        auto dataPtr = static_cast<double*>(dataPacket.getData());
        dataPtr[0] = 11.1;
        dataPtr[1] = 22.2;

        this->sendPacket(dataPacket);
        this->scheduler.waitAll();
    }
    {
        auto domainPacket = DataPacket(domainDescriptor, 1 * BLOCK_SIZE, 3);
        auto dataPacket = DataPacketWithDomain(domainPacket, this->signal.getDescriptor(), 1 * BLOCK_SIZE);
        auto dataPtr = static_cast<double*>(dataPacket.getData());
        dataPtr[0] = 11.1;
        dataPtr[1] = 22.2;

        this->sendPacket(dataPacket);
        this->scheduler.waitAll();
    }

    SizeT count{3};
    TypeParam samples[3 * BLOCK_SIZE]{};
    ClockTick ticks[3 * BLOCK_SIZE]{};
    reader.readWithDomain((TypeParam*) &samples, (ClockTick*) &ticks, &count);

    ASSERT_EQ(count, 3u);
    ASSERT_EQ(reader.getAvailableCount(), 0u);

    ASSERT_EQ(ticks[0], 1);
    ASSERT_EQ(ticks[1], 2);

    ASSERT_EQ(ticks[2], 2);
    ASSERT_EQ(ticks[3], 3);

    ASSERT_EQ(ticks[4], 3);
    ASSERT_EQ(ticks[5], 4);

    if constexpr (IsTemplateOf<TypeParam, Complex_Number>::value || IsTemplateOf<TypeParam, RangeType>::value)
    {
        ASSERT_EQ(samples[0], TypeParam(typename TypeParam::Type(11.1)));
        ASSERT_EQ(samples[1], TypeParam(typename TypeParam::Type(22.2)));

        ASSERT_EQ(samples[2], TypeParam(typename TypeParam::Type(22.2)));
        ASSERT_EQ(samples[3], TypeParam(typename TypeParam::Type(11.1)));

        ASSERT_EQ(samples[4], TypeParam(typename TypeParam::Type(11.1)));
        ASSERT_EQ(samples[5], TypeParam(typename TypeParam::Type(22.2)));
    }
    else
    {
        ASSERT_EQ(samples[0], TypeParam(11.1));
        ASSERT_EQ(samples[1], TypeParam(22.2));

        ASSERT_EQ(samples[2], TypeParam(22.2));
        ASSERT_EQ(samples[3], TypeParam(11.1));

        ASSERT_EQ(samples[4], TypeParam(11.1));
        ASSERT_EQ(samples[5], TypeParam(22.2));
    }
}

TYPED_TEST(BlockReaderTest, ReadOneBlockWithClockTicksTimeout)
{
    this->signal.setDescriptor(setupDescriptor(SampleType::Float64));

    auto reader = BlockReaderBuilder()
                      .setSignal(this->signal)
                      .setValueReadType(SampleTypeFromType<TypeParam>::SampleType)
                      .setDomainReadType(SampleTypeFromType<ClockTick>::SampleType)
                      .setBlockSize(BLOCK_SIZE)
                      .setSkipEvents(true)
                      .build();

    auto domainPacket = DataPacket(setupDescriptor(SampleType::RangeInt64, LinearDataRule(1, 0), nullptr), 1 * BLOCK_SIZE, 1);
    auto dataPacket = DataPacketWithDomain(domainPacket, this->signal.getDescriptor(), 1 * BLOCK_SIZE);

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

    auto reader = BlockReaderBuilder()
                      .setSignal(this->signal)
                      .setValueReadType(SampleTypeFromType<TypeParam>::SampleType)
                      .setDomainReadType(SampleTypeFromType<ClockRange>::SampleType)
                      .setBlockSize(BLOCK_SIZE)
                      .setSkipEvents(true)
                      .build();

    auto domainPacket = DataPacket(setupDescriptor(SampleType::RangeInt64, LinearDataRule(1, 0), nullptr), 1 * BLOCK_SIZE, 1);

    auto dataPacket = DataPacketWithDomain(domainPacket, this->signal.getDescriptor(), 1 * BLOCK_SIZE);

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

TYPED_TEST(BlockReaderTest, ReadThreeBlockOverlappedWithRanges)
{
    this->signal.setDescriptor(setupDescriptor(SampleType::Float64));

    auto reader = BlockReaderBuilder()
                      .setSignal(this->signal)
                      .setValueReadType(SampleTypeFromType<TypeParam>::SampleType)
                      .setDomainReadType(SampleTypeFromType<ClockRange>::SampleType)
                      .setBlockSize(BLOCK_SIZE)
                      .setOverlap(OVERLAP)
                      .setSkipEvents(true)
                      .build();

    auto domainDescriptor = setupDescriptor(SampleType::RangeInt64, LinearDataRule(1, 0), nullptr);

    {
        auto domainPacket = DataPacket(domainDescriptor, 1 * BLOCK_SIZE, 1);
        auto dataPacket = DataPacketWithDomain(domainPacket, this->signal.getDescriptor(), 1 * BLOCK_SIZE);
        auto dataPtr = static_cast<double*>(dataPacket.getData());
        dataPtr[0] = 11.1;
        dataPtr[1] = 22.2;

        this->sendPacket(dataPacket);
        this->scheduler.waitAll();
    }
    {
        auto domainPacket = DataPacket(domainDescriptor, 1 * BLOCK_SIZE, 3);
        auto dataPacket = DataPacketWithDomain(domainPacket, this->signal.getDescriptor(), 1 * BLOCK_SIZE);
        auto dataPtr = static_cast<double*>(dataPacket.getData());
        dataPtr[0] = 11.1;
        dataPtr[1] = 22.2;

        this->sendPacket(dataPacket);
        this->scheduler.waitAll();
    }

    SizeT count{3};
    TypeParam samples[3 * BLOCK_SIZE]{};
    ClockRange stamps[3 * BLOCK_SIZE]{};
    reader.readWithDomain((TypeParam*) &samples, (ClockRange*) &stamps, &count);

    ASSERT_EQ(count, 3u);
    ASSERT_EQ(reader.getAvailableCount(), 0u);

    ASSERT_EQ(stamps[0].start, 1);
    ASSERT_EQ(stamps[0].end, (ClockTick) -1);

    ASSERT_EQ(stamps[1].start, 2);
    ASSERT_EQ(stamps[1].end, (ClockTick) -1);

    ASSERT_EQ(stamps[2].start, 2);
    ASSERT_EQ(stamps[2].end, (ClockTick) -1);

    ASSERT_EQ(stamps[3].start, 3);
    ASSERT_EQ(stamps[3].end, (ClockTick) -1);

    ASSERT_EQ(stamps[4].start, 3);
    ASSERT_EQ(stamps[4].end, (ClockTick) -1);

    ASSERT_EQ(stamps[5].start, 4);
    ASSERT_EQ(stamps[5].end, (ClockTick) -1);

    if constexpr (IsTemplateOf<TypeParam, Complex_Number>::value || IsTemplateOf<TypeParam, RangeType>::value)
    {
        ASSERT_EQ(samples[0], TypeParam(typename TypeParam::Type(11.1)));
        ASSERT_EQ(samples[1], TypeParam(typename TypeParam::Type(22.2)));

        ASSERT_EQ(samples[2], TypeParam(typename TypeParam::Type(22.2)));
        ASSERT_EQ(samples[3], TypeParam(typename TypeParam::Type(11.1)));

        ASSERT_EQ(samples[4], TypeParam(typename TypeParam::Type(11.1)));
        ASSERT_EQ(samples[5], TypeParam(typename TypeParam::Type(22.2)));
    }
    else
    {
        ASSERT_EQ(samples[0], TypeParam(11.1));
        ASSERT_EQ(samples[1], TypeParam(22.2));

        ASSERT_EQ(samples[2], TypeParam(22.2));
        ASSERT_EQ(samples[3], TypeParam(11.1));

        ASSERT_EQ(samples[4], TypeParam(11.1));
        ASSERT_EQ(samples[5], TypeParam(22.2));
    }
}

TYPED_TEST(BlockReaderTest, ReadOneBlockWithRangesTimeout)
{
    this->signal.setDescriptor(setupDescriptor(SampleType::Float64));

    auto reader = BlockReaderBuilder()
                      .setSignal(this->signal)
                      .setValueReadType(SampleTypeFromType<TypeParam>::SampleType)
                      .setDomainReadType(SampleTypeFromType<ClockRange>::SampleType)
                      .setBlockSize(BLOCK_SIZE)
                      .setSkipEvents(true)
                      .build();

    auto domainPacket = DataPacket(setupDescriptor(SampleType::RangeInt64, LinearDataRule(1, 0), nullptr), 1 * BLOCK_SIZE, 1);
    auto dataPacket = DataPacketWithDomain(domainPacket, this->signal.getDescriptor(), 1 * BLOCK_SIZE);

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

    auto reader = BlockReaderBuilder()
                      .setSignal(this->signal)
                      .setValueReadType(SampleTypeFromType<TypeParam>::SampleType)
                      .setDomainReadType(SampleTypeFromType<ClockRange>::SampleType)
                      .setBlockSize(BLOCK_SIZE)
                      .setSkipEvents(true)
                      .build();

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

TYPED_TEST(BlockReaderTest, ReadLessThanTwoPacketOverlapped)
{
    this->signal.setDescriptor(setupDescriptor(SampleType::Float64));

    auto reader = BlockReaderBuilder()
                      .setSignal(this->signal)
                      .setValueReadType(SampleTypeFromType<TypeParam>::SampleType)
                      .setDomainReadType(SampleTypeFromType<ClockRange>::SampleType)
                      .setBlockSize(BLOCK_SIZE)
                      .setOverlap(OVERLAP)
                      .setSkipEvents(true)
                      .build();

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
    ASSERT_EQ(reader.getAvailableCount(), 1u);

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

    auto reader = BlockReaderBuilder()
                      .setSignal(this->signal)
                      .setValueReadType(SampleTypeFromType<TypeParam>::SampleType)
                      .setDomainReadType(SampleTypeFromType<ClockRange>::SampleType)
                      .setBlockSize(BLOCK_SIZE)
                      .setSkipEvents(true)
                      .build();
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

TYPED_TEST(BlockReaderTest, ReadBetweenOverlappedPackets)
{
    this->signal.setDescriptor(setupDescriptor(SampleType::Float64));

    auto reader = BlockReaderBuilder()
                      .setSignal(this->signal)
                      .setValueReadType(SampleTypeFromType<TypeParam>::SampleType)
                      .setDomainReadType(SampleTypeFromType<ClockRange>::SampleType)
                      .setBlockSize(BLOCK_SIZE)
                      .setOverlap(OVERLAP)
                      .setSkipEvents(true)
                      .build();

    auto dataPacket = DataPacket(this->signal.getDescriptor(), BLOCK_SIZE);

    // Set the first sample to
    auto dataPtr = static_cast<double*>(dataPacket.getData());
    dataPtr[0] = 11.1;
    dataPtr[1] = 22.2;

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

    auto reader = BlockReaderBuilder()
                      .setSignal(this->signal)
                      .setValueReadType(SampleTypeFromType<TypeParam>::SampleType)
                      .setDomainReadType(SampleTypeFromType<ClockRange>::SampleType)
                      .setBlockSize(BLOCK_SIZE)
                      .setSkipEvents(true)
                      .build();
    
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
    reader.read(&samples2, &count, 1000u);

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

    auto reader = BlockReaderBuilder()
                      .setSignal(this->signal)
                      .setValueReadType(SampleTypeFromType<TypeParam>::SampleType)
                      .setDomainReadType(SampleTypeFromType<ClockRange>::SampleType)
                      .setBlockSize(BLOCK_SIZE)
                      .setSkipEvents(true)
                      .build();

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
    reader.read(&nextSamples, &count);

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

TYPED_TEST(BlockReaderTest, ReadBetweenOverlappedPacketsAndCheckValues)
{
    this->signal.setDescriptor(setupDescriptor(SampleType::Float64));

    auto reader = BlockReaderBuilder()
                      .setSignal(this->signal)
                      .setValueReadType(SampleTypeFromType<TypeParam>::SampleType)
                      .setDomainReadType(SampleTypeFromType<ClockRange>::SampleType)
                      .setBlockSize(BLOCK_SIZE)
                      .setOverlap(OVERLAP)
                      .setSkipEvents(true)
                      .build();

    auto dataPacket = DataPacket(this->signal.getDescriptor(), BLOCK_SIZE);

    // Set the first sample to
    auto dataPtr = static_cast<double*>(dataPacket.getData());
    dataPtr[0] = 11.1;
    dataPtr[1] = 22.2;

    this->sendPacket(dataPacket);
    this->scheduler.waitAll();

    SizeT count{1};
    TypeParam samples[1 * BLOCK_SIZE]{};
    reader.read((void*) &samples, &count);

    ASSERT_EQ(count, 1u);
    ASSERT_EQ(reader.getAvailableCount(), 0u);

    auto nextDataPacket = DataPacket(this->signal.getDescriptor(), BLOCK_SIZE);
    auto nextDataPtr = static_cast<double*>(nextDataPacket.getData());
    nextDataPtr[0] = 33.3;
    nextDataPtr[1] = 44.4;
    this->sendPacket(nextDataPacket);
    this->scheduler.waitAll();

    ASSERT_EQ(reader.getAvailableCount(), 2u);

    count = 2;
    TypeParam nextSamples[2 * BLOCK_SIZE]{};
    reader.read(&nextSamples, &count);

    ASSERT_EQ(count, 2u);
    ASSERT_EQ(reader.getAvailableCount(), 0u);

    if constexpr (IsTemplateOf<TypeParam, Complex_Number>::value || IsTemplateOf<TypeParam, RangeType>::value)
    {
        ASSERT_EQ(samples[0], TypeParam(typename TypeParam::Type(11.1)));
        ASSERT_EQ(samples[1], TypeParam(typename TypeParam::Type(22.2)));

        ASSERT_EQ(nextSamples[0], TypeParam(typename TypeParam::Type(22.2)));
        ASSERT_EQ(nextSamples[1], TypeParam(typename TypeParam::Type(33.3)));

        ASSERT_EQ(nextSamples[2], TypeParam(typename TypeParam::Type(33.3)));
        ASSERT_EQ(nextSamples[3], TypeParam(typename TypeParam::Type(44.4)));
    }
    else
    {
        ASSERT_EQ(samples[0], (TypeParam) 11.1);
        ASSERT_EQ(samples[1], (TypeParam) 22.2);

        ASSERT_EQ(nextSamples[0], (TypeParam) 22.2);
        ASSERT_EQ(nextSamples[1], (TypeParam) 33.3);
        ASSERT_EQ(nextSamples[2], (TypeParam) 33.3);
        ASSERT_EQ(nextSamples[3], (TypeParam) 44.4);
    }
}

TYPED_TEST(BlockReaderTest, ReadValuesMoreThanAvailable)
{
    this->signal.setDescriptor(setupDescriptor(SampleType::Float64));

    auto reader = BlockReaderBuilder()
                      .setSignal(this->signal)
                      .setValueReadType(SampleTypeFromType<TypeParam>::SampleType)
                      .setDomainReadType(SampleTypeFromType<ClockRange>::SampleType)
                      .setBlockSize(BLOCK_SIZE)
                      .setSkipEvents(true)
                      .build();
    
    const SizeT NUM_SAMPLES = BLOCK_SIZE;
    this->sendPacket(DataPacket(this->signal.getDescriptor(), NUM_SAMPLES));
    this->scheduler.waitAll();

    SizeT count{2};
    TypeParam samples[2 * BLOCK_SIZE]{};
    reader.read((void*) &samples, &count);

    ASSERT_EQ(count, 1u);
    ASSERT_EQ(reader.getAvailableCount(), 0u);
}

TYPED_TEST(BlockReaderTest, ReadOverlappedValuesMoreThanAvailable)
{
    this->signal.setDescriptor(setupDescriptor(SampleType::Float64));

    auto reader = BlockReaderBuilder()
                      .setSignal(this->signal)
                      .setValueReadType(SampleTypeFromType<TypeParam>::SampleType)
                      .setDomainReadType(SampleTypeFromType<ClockRange>::SampleType)
                      .setBlockSize(BLOCK_SIZE)
                      .setOverlap(OVERLAP)
                      .setSkipEvents(true)
                      .build();

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

    auto reader = BlockReaderBuilder()
                      .setSignal(this->signal)
                      .setValueReadType(SampleTypeFromType<TypeParam>::SampleType)
                      .setDomainReadType(SampleTypeFromType<ClockRange>::SampleType)
                      .setBlockSize(BLOCK_SIZE)
                      .setSkipEvents(true)
                      .build();

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
    reader.read(&samplesDouble, &count);

    ASSERT_EQ(reader.getAvailableCount(), 0u);

    // Change signal sample-type
    this->signal.setDescriptor(setupDescriptor(SampleType::Int32));
    auto dataPacketInt32 = DataPacket(this->signal.getDescriptor(), NUM_SAMPLES);
    auto dataPtrInt32 = static_cast<int32_t*>(dataPacketInt32.getData());
    dataPtrInt32[0] = 3;
    dataPtrInt32[1] = 4;

    this->sendPacket(dataPacketInt32);
    this->scheduler.waitAll();

    count = 1;
    TypeParam sampleInt32[1 * BLOCK_SIZE]{};

    reader.read(&sampleInt32, &count);

    ASSERT_EQ(reader.getAvailableCount(), 0u);

    ASSERT_EQ(sampleInt32[0], TypeParam(3));
    ASSERT_EQ(sampleInt32[1], TypeParam(4));
}

TYPED_TEST(BlockReaderTest, DescriptorChangedConvertibleOverlapped)
{
    this->signal.setDescriptor(setupDescriptor(SampleType::Float64));

    auto reader = BlockReaderBuilder()
                      .setSignal(this->signal)
                      .setValueReadType(SampleTypeFromType<TypeParam>::SampleType)
                      .setDomainReadType(SampleTypeFromType<ClockRange>::SampleType)
                      .setBlockSize(BLOCK_SIZE)
                      .setOverlap(OVERLAP)
                      .build();
    
    {
        SizeT tmpCount{0};
        auto status = reader.read(nullptr, &tmpCount);
        ASSERT_EQ(status.getReadStatus(), ReadStatus::Event);
    }
    
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
    reader.read(&samplesDouble, &count);

    ASSERT_EQ(reader.getAvailableCount(), 0u);

    // Change signal sample-type
    this->signal.setDescriptor(setupDescriptor(SampleType::Int32));
    auto dataPacketInt32 = DataPacket(this->signal.getDescriptor(), NUM_SAMPLES);
    auto dataPtrInt32 = static_cast<int32_t*>(dataPacketInt32.getData());
    dataPtrInt32[0] = 3;
    dataPtrInt32[1] = 4;

    this->sendPacket(dataPacketInt32);
    this->scheduler.waitAll();

    ASSERT_EQ(reader.getAvailableCount(), 0u);

    {
        // read event packet
        size_t tmpCount = 1;
        BlockReaderStatusPtr status = reader.read(&samplesDouble, &tmpCount);
        ASSERT_TRUE(status.assigned());
        ASSERT_EQ(status.getReadStatus(), ReadStatus::Event);
        ASSERT_EQ(status.getReadSamples(), 1u);
        ASSERT_EQ(tmpCount, 0u);
    }

    ASSERT_EQ(reader.getAvailableCount(), 1u);

    count = 2u;
    TypeParam sampleInt32[2 * BLOCK_SIZE]{};
    reader.read(&sampleInt32, &count);

    ASSERT_EQ(reader.getAvailableCount(), 0u);

    ASSERT_EQ(count, 1u);
    ASSERT_EQ(sampleInt32[0], TypeParam(3));
    ASSERT_EQ(sampleInt32[1], TypeParam(4));
}

TYPED_TEST(BlockReaderTest, GapDetected)
{
    this->signal.setDescriptor(setupDescriptor(SampleType::Float64));
    const auto domainSignal = daq::Signal(this->context, nullptr, "domainSig");
    const auto domainDesc = setupDescriptor(SampleType::Int64, LinearDataRule(1, 0), nullptr);
    domainSignal.setDescriptor(domainDesc);
    this->signal.setDomainSignal(domainSignal);

    constexpr SizeT blockSize = 4;
    auto reader = BlockReaderBuilder()
                      .setSignal(this->signal)
                      .setValueReadType(SampleTypeFromType<TypeParam>::SampleType)
                      .setDomainReadType(SampleTypeFromType<int64_t>::SampleType)
                      .setBlockSize(blockSize)
                      .setSkipEvents(true)
                      .build();
    
    constexpr SizeT NUM_SAMPLES = 6;

    auto domainPacket = DataPacket(domainDesc, NUM_SAMPLES, 0);
    auto dataPacketDouble = DataPacketWithDomain(domainPacket, this->signal.getDescriptor(), NUM_SAMPLES);
    auto dataPtrDouble = static_cast<double*>(dataPacketDouble.getData());
    dataPtrDouble[0] = 1.0;
    dataPtrDouble[1] = 2.0;
    dataPtrDouble[2] = 3.0;
    dataPtrDouble[3] = 4.0;
    dataPtrDouble[4] = 5.0;
    dataPtrDouble[5] = 6.0;

    this->sendPacket(dataPacketDouble);

    domainPacket = DataPacket(domainDesc, NUM_SAMPLES, 8);
    dataPacketDouble = DataPacketWithDomain(domainPacket, this->signal.getDescriptor(), NUM_SAMPLES);
    dataPtrDouble = static_cast<double*>(dataPacketDouble.getData());
    dataPtrDouble[0] = 8.0;
    dataPtrDouble[1] = 9.0;
    dataPtrDouble[2] = 10.0;
    dataPtrDouble[3] = 11.0;
    dataPtrDouble[4] = 12.0;
    dataPtrDouble[5] = 13.0;

    this->sendPacket(dataPacketDouble);

    ASSERT_EQ(reader.getAvailableCount(), 1u);

    SizeT count{3};
    TypeParam samplesDouble[3 * blockSize]{};
    int64_t domainSamples[3 * blockSize]{};

    BlockReaderStatusPtr status =
        reader.readWithDomain(reinterpret_cast<TypeParam*>(&samplesDouble), reinterpret_cast<int64_t*>(&domainSamples), &count);
    ASSERT_EQ(count, 1u);
    ASSERT_THAT(samplesDouble,
                ElementsAre(static_cast<TypeParam>(1),
                            static_cast<TypeParam>(2),
                            static_cast<TypeParam>(3),
                            static_cast<TypeParam>(4),
                            _,
                            _,
                            _,
                            _,
                            _,
                            _,
                            _,
                            _));
    ASSERT_THAT(domainSamples, ElementsAre(0, 1, 2, 3, _, _, _, _, _, _, _, _));
    ASSERT_EQ(reader.getAvailableCount(), 1u);

    ASSERT_TRUE(status.getValid());
    ASSERT_EQ(status.getReadStatus(), ReadStatus::Event);
    ASSERT_EQ(status.getReadSamples(), 6u);
    ASSERT_TRUE(status.getEventPacket().assigned());

    ASSERT_EQ(status.getEventPacket().getEventId(), event_packet_id::IMPLICIT_DOMAIN_GAP_DETECTED);
    ASSERT_EQ(status.getEventPacket().getParameters().get(event_packet_param::GAP_DIFF), 2);

    count = 3;
    status = reader.readWithDomain(reinterpret_cast<TypeParam*>(&samplesDouble), reinterpret_cast<int64_t*>(&domainSamples), &count);
    ASSERT_EQ(count, 1u);
    ASSERT_THAT(samplesDouble,
                ElementsAre(static_cast<TypeParam>(8),
                            static_cast<TypeParam>(9),
                            static_cast<TypeParam>(10),
                            static_cast<TypeParam>(11),
                            _,
                            _,
                            _,
                            _,
                            _,
                            _,
                            _,
                            _));
    ASSERT_THAT(domainSamples, ElementsAre(8, 9, 10, 11, _, _, _, _, _, _, _, _));
    ASSERT_EQ(reader.getAvailableCount(), 0u);

    ASSERT_TRUE(status.getValid());
    ASSERT_EQ(status.getReadStatus(), ReadStatus::Ok);
    ASSERT_FALSE(status.getEventPacket().assigned());
}

TYPED_TEST(BlockReaderTest, DescriptorChangedNotConvertible)
{
    this->signal.setDescriptor(setupDescriptor(SampleType::Int32));

    auto reader = BlockReaderBuilder()
                      .setSignal(this->signal)
                      .setValueReadType(SampleTypeFromType<std::int32_t>::SampleType)
                      .setDomainReadType(SampleTypeFromType<ClockRange>::SampleType)
                      .setBlockSize(BLOCK_SIZE)
                      .setSkipEvents(true)
                      .build();
    
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
    auto status = reader.read(samples, &count);
    ASSERT_FALSE(status.getValid());
}

TYPED_TEST(BlockReaderTest, ReuseReader)
{
    this->signal.setDescriptor(setupDescriptor(SampleType::ComplexFloat32));

    auto reader = BlockReaderBuilder()
                      .setSignal(this->signal)
                      .setValueReadType(SampleTypeFromType<TypeParam>::SampleType)
                      .setDomainReadType(SampleTypeFromType<ClockRange>::SampleType)
                      .setBlockSize(BLOCK_SIZE)
                      .setSkipEvents(true)
                      .build();

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
    auto status = reader.read(&samples, &count);

    const daq::Bool convertable = IsTemplateOf<TypeParam, Complex_Number>::value;
    ASSERT_EQ(status.getValid(), convertable);

    auto newReader = BlockReaderBuilder()
                         .setOldBlockReader(reader)
                         .setValueReadType(SampleTypeFromType<ComplexFloat32>::SampleType)
                         .setDomainReadType(SampleTypeFromType<ClockRange>::SampleType)
                         .setBlockSize(reader.getBlockSize())
                         .build();

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

TYPED_TEST(BlockReaderTest, ReuseReaderOverlapped)
{
    this->signal.setDescriptor(setupDescriptor(SampleType::ComplexFloat32));

    auto reader = BlockReaderBuilder()
                      .setSignal(this->signal)
                      .setValueReadType(SampleTypeFromType<TypeParam>::SampleType)
                      .setDomainReadType(SampleTypeFromType<ClockRange>::SampleType)
                      .setBlockSize(BLOCK_SIZE)
                      .setOverlap(OVERLAP)
                      .setSkipEvents(true)
                      .build();
    
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
    auto status = reader.read(&samples, &count);

    const daq::Bool convertable = IsTemplateOf<TypeParam, Complex_Number>::value;
    ASSERT_EQ(status.getValid(), convertable);

    auto newReader = BlockReaderBuilder()
                         .setOldBlockReader(reader)
                         .setValueReadType(SampleTypeFromType<ComplexFloat32>::SampleType)
                         .setDomainReadType(SampleTypeFromType<ClockRange>::SampleType)
                         .setBlockSize(BLOCK_SIZE)
                         .setOverlap(OVERLAP)
                         .setSkipEvents(true)
                         .build();

    if (convertable)
    {
        SizeT complexCount{2};
        ComplexFloat32 complexSamples[2 * BLOCK_SIZE];

        status = newReader.read((ComplexFloat32*) &complexSamples, &complexCount);
        ASSERT_EQ(status.getReadStatus(), ReadStatus::Ok);

        ComplexFloat32 expectedSample[2 * BLOCK_SIZE]{};

        expectedSample[0] = 22;
        expectedSample[1] = 33;

        expectedSample[2] = 33;
        expectedSample[3] = 44;

        ASSERT_EQ(complexSamples[0], expectedSample[0]);
        ASSERT_EQ(complexSamples[1], expectedSample[1]);

        ASSERT_EQ(complexSamples[2], expectedSample[2]);
        ASSERT_EQ(complexSamples[3], expectedSample[3]);
    }
    else
    {
        SizeT complexCount{1};
        ComplexFloat32 complexSamples[1 * BLOCK_SIZE];
        status = newReader.read((ComplexFloat32*) &complexSamples, &complexCount);
        ASSERT_EQ(status.getReadStatus(), ReadStatus::Ok);

        ASSERT_EQ(complexCount, 1u);

        ComplexFloat32 expectedSample[BLOCK_SIZE]{};

        expectedSample[0] = 11;
        expectedSample[1] = 22;

        ASSERT_EQ(complexSamples[0], expectedSample[0]);
        ASSERT_EQ(complexSamples[1], expectedSample[1]);
    }
}

TYPED_TEST(BlockReaderTest, ReadUndefinedNoDomain)
{
    this->signal.setDescriptor(setupDescriptor(SampleType::Float64));

    auto reader = BlockReaderBuilder()
                      .setSignal(this->signal)
                      .setValueReadType(SampleTypeFromType<UndefinedType>::SampleType)
                      .setDomainReadType(SampleTypeFromType<UndefinedType>::SampleType)
                      .setBlockSize(BLOCK_SIZE)
                      .build();
    
    {
        SizeT tmpCount{0};
        auto status = reader.read(nullptr, &tmpCount);
        ASSERT_EQ(status.getReadStatus(), ReadStatus::Event);
    }
    
    ASSERT_EQ(reader.getValueReadType(), SampleType::Float64);
    ASSERT_EQ(reader.getDomainReadType(), SampleType::Invalid);
}

TYPED_TEST(BlockReaderTest, ReadUndefinedWithDomain)
{
    this->signal.setDescriptor(setupDescriptor(SampleType::Float64));

    auto reader = BlockReaderBuilder()
                      .setSignal(this->signal)
                      .setValueReadType(SampleTypeFromType<UndefinedType>::SampleType)
                      .setDomainReadType(SampleTypeFromType<UndefinedType>::SampleType)
                      .setBlockSize(BLOCK_SIZE)
                      .setSkipEvents(true)
                      .build();
    
    {
        SizeT tmpCount{0};
        reader.read(nullptr, &tmpCount);
    }
    
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
    reader.read(&samples, &count);

    ASSERT_EQ(count, 1u);

    ASSERT_EQ(reader.getValueReadType(), SampleType::Float64);
    // domain info available from descriptor change event
    ASSERT_EQ(reader.getDomainReadType(), SampleType::RangeInt64);
}

TYPED_TEST(BlockReaderTest, ReadUndefinedWithDomainOverlapped)
{
    this->signal.setDescriptor(setupDescriptor(SampleType::Float64));

    auto reader = BlockReaderBuilder()
                      .setSignal(this->signal)
                      .setValueReadType(SampleType::Undefined)
                      .setDomainReadType(SampleType::Undefined)
                      .setBlockSize(BLOCK_SIZE)
                      .setOverlap(OVERLAP)
                      .setSkipEvents(true)
                      .build();
    
    {
        SizeT tmpCount{0};
        reader.read(nullptr, &tmpCount);
    }
    
    ASSERT_EQ(reader.getValueReadType(), SampleType::Float64);  // read from signal descriptor
    ASSERT_EQ(reader.getDomainReadType(), SampleType::Invalid);

    auto domainDescriptor = setupDescriptor(SampleType::RangeInt64, LinearDataRule(1, 0), nullptr);
    auto eventPacket = DataDescriptorChangedEventPacket(nullptr, domainDescriptor);
    this->sendPacket(eventPacket);

    auto domainPacket = DataPacket(domainDescriptor, BLOCK_SIZE, 1);
    auto dataPacket = DataPacketWithDomain(domainPacket, this->signal.getDescriptor(), 2 * BLOCK_SIZE);
    auto dataPtr = static_cast<double*>(dataPacket.getData());
    dataPtr[0] = 11.1;
    dataPtr[1] = 22.2;
    dataPtr[2] = 33.3;
    dataPtr[3] = 44.4;

    this->sendPacket(dataPacket);

    SizeT count{3};
    double samples[3 * BLOCK_SIZE]{};
    reader.read(&samples, &count);

    ASSERT_EQ(count, 3u);

    ASSERT_EQ(reader.getValueReadType(), SampleType::Float64);
    // domain info available from descriptor change event
    ASSERT_EQ(reader.getDomainReadType(), SampleType::RangeInt64);
}

TYPED_TEST(BlockReaderTest, ReadUndefinedWithNoDomainFromPacket)
{
    this->signal.setDescriptor(setupDescriptor(SampleType::Float64));

    auto reader = BlockReaderBuilder()
                      .setSignal(this->signal)
                      .setValueReadType(SampleTypeFromType<UndefinedType>::SampleType)
                      .setDomainReadType(SampleTypeFromType<UndefinedType>::SampleType)
                      .setBlockSize(BLOCK_SIZE)
                      .setSkipEvents(true)
                      .build();

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

TYPED_TEST(BlockReaderTest, ReadUndefinedWithNoDomainFromPacketOverlapped)
{
    this->signal.setDescriptor(setupDescriptor(SampleType::Float64));

    auto reader = BlockReaderBuilder()
                      .setSignal(this->signal)
                      .setValueReadType(SampleType::Undefined)
                      .setDomainReadType(SampleType::Undefined)
                      .setBlockSize(BLOCK_SIZE)
                      .setOverlap(OVERLAP)
                      .setSkipEvents(true)
                      .build();

    auto domainPacket = DataPacket(setupDescriptor(SampleType::RangeInt64, LinearDataRule(1, 0), nullptr), BLOCK_SIZE, 1);
    auto dataPacket = DataPacketWithDomain(domainPacket, this->signal.getDescriptor(), 2 * BLOCK_SIZE);
    auto dataPtr = static_cast<double*>(dataPacket.getData());
    dataPtr[0] = 111.1;
    dataPtr[1] = 222.2;
    dataPtr[2] = 333.3;
    dataPtr[3] = 444.4;

    this->sendPacket(dataPacket);

    SizeT count{3};
    double samples[3 * BLOCK_SIZE]{};
    reader.read(&samples, &count);

    ASSERT_EQ(count, 3u);
    ASSERT_EQ(reader.getValueReadType(), SampleType::Float64);

    // domain was not read so its info was not updated
    ASSERT_EQ(reader.getDomainReadType(), SampleType::Invalid);
}

TYPED_TEST(BlockReaderTest, ReadUndefinedWithWithDomainFromPacket)
{
    this->signal.setDescriptor(setupDescriptor(SampleType::Float64));

    auto reader = BlockReaderBuilder()
                      .setSignal(this->signal)
                      .setValueReadType(SampleTypeFromType<UndefinedType>::SampleType)
                      .setDomainReadType(SampleTypeFromType<UndefinedType>::SampleType)
                      .setBlockSize(BLOCK_SIZE)
                      .setSkipEvents(true)
                      .build();

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

TYPED_TEST(BlockReaderTest, ReadUndefinedWithWithDomainFromPacketOverlapped)
{
    this->signal.setDescriptor(setupDescriptor(SampleType::Float64));

    auto reader = BlockReaderBuilder()
                      .setSignal(this->signal)
                      .setValueReadType(SampleType::Undefined)
                      .setDomainReadType(SampleType::Undefined)
                      .setBlockSize(BLOCK_SIZE)
                      .setOverlap(OVERLAP)
                      .setSkipEvents(true)
                      .build();

    auto domainPacket = DataPacket(setupDescriptor(SampleType::RangeInt64, LinearDataRule(1, 0), nullptr), BLOCK_SIZE, 1);
    auto dataPacket = DataPacketWithDomain(domainPacket, this->signal.getDescriptor(), 2 * BLOCK_SIZE);
    auto dataPtr = static_cast<double*>(dataPacket.getData());
    dataPtr[0] = 111.1;
    dataPtr[1] = 222.2;
    dataPtr[2] = 111.1;
    dataPtr[3] = 222.2;

    this->sendPacket(dataPacket);

    SizeT count{3};
    double samples[3 * BLOCK_SIZE]{};
    RangeType64 domain[3 * BLOCK_SIZE]{};
    reader.readWithDomain(&samples, &domain, &count);

    ASSERT_EQ(count, 3u);
    ASSERT_EQ(reader.getValueReadType(), SampleType::Float64);

    // domain was read and updated from packet info
    ASSERT_EQ(reader.getDomainReadType(), SampleType::RangeInt64);
}

TYPED_TEST(BlockReaderTest, ToString)
{
    auto reader = BlockReaderBuilder()
                      .setSignal(this->signal)
                      .setValueReadType(SampleTypeFromType<TypeParam>::SampleType)
                      .setDomainReadType(SampleTypeFromType<ClockRange>::SampleType)
                      .setBlockSize(BLOCK_SIZE)
                      .build();

    CharPtr str;
    ASSERT_EQ(reader->toString(&str), OPENDAQ_SUCCESS);

    ASSERT_STREQ(str, "daq::IBlockReader");
    daqFreeMemory(str);
}

TYPED_TEST(BlockReaderTest, BlockReaderWithInputPort)
{
    this->signal.setDescriptor(setupDescriptor(SampleType::Float64));

    auto port = InputPort(this->signal.getContext(), nullptr, "readsig");
    auto reader = BlockReaderBuilder()
                      .setInputPort(port)
                      .setValueReadType(SampleType::Undefined)
                      .setDomainReadType(SampleType::Undefined)
                      .setBlockSize(BLOCK_SIZE)
                      .setSkipEvents(true)
                      .build();
    port.connect(this->signal);

    auto domainPacket = DataPacket(setupDescriptor(SampleType::RangeInt64, LinearDataRule(1, 0), nullptr), BLOCK_SIZE, 1);
    auto dataPacket = DataPacketWithDomain(domainPacket, this->signal.getDescriptor(), BLOCK_SIZE);
    auto dataPtr = static_cast<double*>(dataPacket.getData());
    dataPtr[0] = 111.1;
    dataPtr[1] = 222.2;

    this->sendPacket(dataPacket);

    SizeT count{1};
    double samples[BLOCK_SIZE]{};
    reader.read(samples, &count, 1000u);

    ASSERT_EQ(count, 1u);
    ASSERT_EQ(samples[0], dataPtr[0]);
    ASSERT_EQ(samples[1], dataPtr[1]);
}

TYPED_TEST(BlockReaderTest, BlockReaderWithInputPortOverlapped)
{
    this->signal.setDescriptor(setupDescriptor(SampleType::Float64));

    auto port = InputPort(this->signal.getContext(), nullptr, "readsig");
    auto reader = BlockReaderBuilder()
                      .setInputPort(port)
                      .setValueReadType(SampleType::Undefined)
                      .setDomainReadType(SampleType::Undefined)
                      .setBlockSize(BLOCK_SIZE)
                      .setOverlap(OVERLAP)
                      .setSkipEvents(true)
                      .build();
    port.connect(this->signal);

    auto domainPacket = DataPacket(setupDescriptor(SampleType::RangeInt64, LinearDataRule(1, 0), nullptr), BLOCK_SIZE, 1);
    auto dataPacket = DataPacketWithDomain(domainPacket, this->signal.getDescriptor(), 2 * BLOCK_SIZE);
    auto dataPtr = static_cast<double*>(dataPacket.getData());
    dataPtr[0] = 111.1;
    dataPtr[1] = 222.2;
    dataPtr[2] = 333.3;
    dataPtr[3] = 444.4;

    this->sendPacket(dataPacket);

    SizeT count{3};
    double samples[3 * BLOCK_SIZE]{};
    reader.read(samples, &count);

    ASSERT_EQ(count, 3u);
    ASSERT_EQ(samples[0], dataPtr[0]);
    ASSERT_EQ(samples[1], dataPtr[1]);
    ASSERT_EQ(samples[2], dataPtr[1]);
    ASSERT_EQ(samples[3], dataPtr[2]);
    ASSERT_EQ(samples[4], dataPtr[2]);
    ASSERT_EQ(samples[5], dataPtr[3]);
}

TYPED_TEST(BlockReaderTest, BlockReaderWithNotConnectedInputPort)
{
    this->signal.setDescriptor(setupDescriptor(SampleType::Float64));

    auto port = InputPort(this->signal.getContext(), nullptr, "readsig");
    auto reader = BlockReaderBuilder()
                      .setInputPort(port)
                      .setValueReadType(SampleType::Undefined)
                      .setDomainReadType(SampleType::Undefined)
                      .setBlockSize(BLOCK_SIZE)
                      .setSkipEvents(true)
                      .build();

    auto domainPacket = DataPacket(setupDescriptor(SampleType::RangeInt64, LinearDataRule(1, 0), nullptr), BLOCK_SIZE, 1);
    auto dataPacket = DataPacketWithDomain(domainPacket, this->signal.getDescriptor(), BLOCK_SIZE);
    auto dataPtr = static_cast<double*>(dataPacket.getData());
    dataPtr[0] = 111.1;
    dataPtr[1] = 222.2;

    port.connect(this->signal);
    this->sendPacket(dataPacket);

    SizeT count{1};
    double samples[BLOCK_SIZE]{};
    reader.read(&samples, &count);

    ASSERT_EQ(count, 1u);
    ASSERT_EQ(samples[0], dataPtr[0]);
    ASSERT_EQ(samples[1], dataPtr[1]);
}

TYPED_TEST(BlockReaderTest, BlockReaderWithNotConnectedInputPortOverlapped)
{
    this->signal.setDescriptor(setupDescriptor(SampleType::Float64));

    auto port = InputPort(this->signal.getContext(), nullptr, "readsig");
    auto reader = BlockReaderBuilder()
                      .setInputPort(port)
                      .setValueReadType(SampleType::Undefined)
                      .setDomainReadType(SampleType::Undefined)
                      .setBlockSize(BLOCK_SIZE)
                      .setOverlap(OVERLAP)
                      .setSkipEvents(true)
                      .build();

    auto domainPacket = DataPacket(setupDescriptor(SampleType::RangeInt64, LinearDataRule(1, 0), nullptr), BLOCK_SIZE, 1);
    auto dataPacket = DataPacketWithDomain(domainPacket, this->signal.getDescriptor(), 2 * BLOCK_SIZE);
    auto dataPtr = static_cast<double*>(dataPacket.getData());
    dataPtr[0] = 111.1;
    dataPtr[1] = 222.2;
    dataPtr[2] = 333.3;
    dataPtr[3] = 444.4;

    port.connect(this->signal);
    this->sendPacket(dataPacket);

    SizeT count{3};
    double samples[3 * BLOCK_SIZE]{};
    reader.read(&samples, &count);

    ASSERT_EQ(count, 3u);
    ASSERT_EQ(samples[0], dataPtr[0]);
    ASSERT_EQ(samples[1], dataPtr[1]);
    ASSERT_EQ(samples[2], dataPtr[1]);
    ASSERT_EQ(samples[3], dataPtr[2]);
    ASSERT_EQ(samples[4], dataPtr[2]);
    ASSERT_EQ(samples[5], dataPtr[3]);
}

TYPED_TEST(BlockReaderTest, MultipleBlockReaderToInputPort)
{
    auto port = InputPort(this->signal.getContext(), nullptr, "readsig");
    auto reader = BlockReaderBuilder()
                      .setInputPort(port)
                      .setValueReadType(SampleType::Undefined)
                      .setDomainReadType(SampleType::Undefined)
                      .setBlockSize(BLOCK_SIZE)
                      .build();

    ASSERT_THROW(BlockReaderBuilder()
                     .setInputPort(port)
                     .setValueReadType(SampleType::Undefined)
                     .setDomainReadType(SampleType::Undefined)
                     .setBlockSize(BLOCK_SIZE)
                     .build(),
                 AlreadyExistsException);
}

TYPED_TEST(BlockReaderTest, BlockReaderReuseInputPort)
{
    auto port = InputPort(this->signal.getContext(), nullptr, "readsig");
    {
        auto reader = BlockReaderBuilder()
                          .setInputPort(port)
                          .setValueReadType(SampleType::Undefined)
                          .setDomainReadType(SampleType::Undefined)
                          .setBlockSize(BLOCK_SIZE)
                          .build();
    }
    ASSERT_NO_THROW(BlockReaderBuilder()
                        .setInputPort(port)
                        .setValueReadType(SampleType::Undefined)
                        .setDomainReadType(SampleType::Undefined)
                        .setBlockSize(BLOCK_SIZE)
                        .build(););
}

TYPED_TEST(BlockReaderTest, BlockReaderOnReadCallback)
{
    SizeT count{1};
    double samples[BLOCK_SIZE]{};

    std::promise<void> promise;
    std::future<void> future = promise.get_future();

    this->signal.setDescriptor(setupDescriptor(SampleType::Float64));

    auto reader = BlockReaderBuilder()
                      .setSignal(this->signal)
                      .setValueReadType(SampleType::Undefined)
                      .setDomainReadType(SampleType::Undefined)
                      .setBlockSize(BLOCK_SIZE)
                      .setSkipEvents(true)
                      .build();

    reader.setOnDataAvailable([&]
    {
        reader.read(&samples, &count);
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

TYPED_TEST(BlockReaderTest, BlockReaderOnReadCallbackOverlapped)
{
    SizeT count{3};
    double samples[3 * BLOCK_SIZE]{};

    std::promise<void> promise;
    std::future<void> future = promise.get_future();

    this->signal.setDescriptor(setupDescriptor(SampleType::Float64));

    auto reader = BlockReaderBuilder()
                      .setSignal(this->signal)
                      .setValueReadType(SampleType::Undefined)
                      .setDomainReadType(SampleType::Undefined)
                      .setBlockSize(BLOCK_SIZE)
                      .setOverlap(OVERLAP)
                      .setSkipEvents(true)
                      .build();
    
    reader.setOnDataAvailable(
        [&]
        {
            reader.read(&samples, &count);
            promise.set_value();
        });

    auto domainPacket = DataPacket(setupDescriptor(SampleType::RangeInt64, LinearDataRule(1, 0), nullptr), BLOCK_SIZE, 1);
    auto dataPacket = DataPacketWithDomain(domainPacket, this->signal.getDescriptor(), 2 * BLOCK_SIZE);
    auto dataPtr = static_cast<double*>(dataPacket.getData());
    dataPtr[0] = 111.1;
    dataPtr[1] = 222.2;
    dataPtr[2] = 333.3;
    dataPtr[3] = 444.4;

    this->sendPacket(dataPacket);

    auto promiseStatus = future.wait_for(std::chrono::seconds(1));
    ASSERT_EQ(promiseStatus, std::future_status::ready);

    ASSERT_EQ(count, 3u);
    ASSERT_EQ(samples[0], dataPtr[0]);
    ASSERT_EQ(samples[1], dataPtr[1]);
    ASSERT_EQ(samples[2], dataPtr[1]);
    ASSERT_EQ(samples[3], dataPtr[2]);
    ASSERT_EQ(samples[4], dataPtr[2]);
    ASSERT_EQ(samples[5], dataPtr[3]);
}

TYPED_TEST(BlockReaderTest, BlockReaderEventInMiddleOfBlock)
{
    SizeT count{1};
    double samples[BLOCK_SIZE]{};
    SizeT samplesRead {0};

    std::promise<void> promise;
    std::future<void> future = promise.get_future();

    this->signal.setDescriptor(setupDescriptor(SampleType::Float64));

    auto reader = BlockReaderBuilder()
                      .setSignal(this->signal)
                      .setValueReadType(SampleType::Undefined)
                      .setDomainReadType(SampleType::Undefined)
                      .setBlockSize(BLOCK_SIZE)
                      .build();
    
    {
        SizeT tmpCount{0};
        auto status = reader.read(nullptr, &tmpCount);
        ASSERT_EQ(status.getReadStatus(), ReadStatus::Event);
    }
    
    reader.setOnDataAvailable([&]
    {
        BlockReaderStatusPtr status = reader.read(&samples[samplesRead], &count);
        samplesRead = status.getReadSamples();
        promise.set_value();
    });

    auto domainPacket1 = DataPacket(setupDescriptor(SampleType::RangeInt64, LinearDataRule(1, 0), nullptr), BLOCK_SIZE - 1, 1);
    auto dataPacket1 = DataPacketWithDomain(domainPacket1, this->signal.getDescriptor(), BLOCK_SIZE - 1);
    auto dataPtr1 = static_cast<double*>(dataPacket1.getData());
    dataPtr1[0] = 111.1;

    this->sendPacket(dataPacket1);

    // change descriptor in the middle of packet
    this->signal.setDescriptor(setupDescriptor(SampleType::Float32));

    auto promiseStatus = future.wait_for(std::chrono::seconds(1));
    ASSERT_EQ(promiseStatus, std::future_status::ready);

    ASSERT_EQ(count, 0u);
    ASSERT_EQ(samplesRead, 1u);
    ASSERT_EQ(samples[0], dataPtr1[0]);
}

TYPED_TEST(BlockReaderTest, BlockReaderEventInMiddleOfBlockOverlapped)
{
    SizeT count{1};
    double samples[BLOCK_SIZE]{};
    SizeT samplesRead {0};

    std::promise<void> promise;
    std::future<void> future = promise.get_future();

    this->signal.setDescriptor(setupDescriptor(SampleType::Float64));

    auto reader = BlockReaderBuilder()
                      .setSignal(this->signal)
                      .setBlockSize(BLOCK_SIZE)
                      .setValueReadType(SampleType::Undefined)
                      .setDomainReadType(SampleType::Undefined)
                      .setOverlap(OVERLAP)
                      .build();

    reader.setOnDataAvailable([&]
    {
        BlockReaderStatusPtr status = reader.read(&samples[samplesRead], &count);
        samplesRead = status.getReadSamples();
        promise.set_value();
    });

    auto domainPacket1 = DataPacket(setupDescriptor(SampleType::RangeInt64, LinearDataRule(1, 0), nullptr), BLOCK_SIZE - 1, 1);
    auto dataPacket1 = DataPacketWithDomain(domainPacket1, this->signal.getDescriptor(), BLOCK_SIZE - 1);
    auto dataPtr1 = static_cast<double*>(dataPacket1.getData());
    dataPtr1[0] = 111.1;

    this->sendPacket(dataPacket1);

    // change descriptor in the middle of packet
    this->signal.setDescriptor(setupDescriptor(SampleType::Float32));

    auto promiseStatus = future.wait_for(std::chrono::seconds(1));
    ASSERT_EQ(promiseStatus, std::future_status::ready);

    ASSERT_EQ(count, 0u);
    ASSERT_EQ(samplesRead, 1u);
    ASSERT_EQ(samples[0], dataPtr1[0]);
}

TYPED_TEST(BlockReaderTest, BlockReaderFromPortOnReadCallback)
{
    SizeT count{1};
    double samples[BLOCK_SIZE]{};

    std::promise<void> promise;
    std::future<void> future = promise.get_future();

    this->signal.setDescriptor(setupDescriptor(SampleType::Float64));
    auto port = InputPort(this->signal.getContext(), nullptr, "readsig");

    auto reader = BlockReaderBuilder()
                      .setInputPort(port)
                      .setValueReadType(SampleType::Undefined)
                      .setDomainReadType(SampleType::Undefined)
                      .setBlockSize(BLOCK_SIZE)
                      .build();
    port.connect(this->signal);

    reader.setOnDataAvailable([&]
    {
        SizeT tmpCnt = 0;
        auto status = reader.read(nullptr, &tmpCnt);
        if (status.getReadStatus() != ReadStatus::Event)
        {
            count = 0u;
        }

        reader.read(&samples, &count);
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

    std::promise<void> promise;
    std::future<void> future = promise.get_future();

    this->signal.setDescriptor(setupDescriptor(SampleType::Int64));

    auto reader = BlockReaderBuilder()
                      .setSignal(this->signal)
                      .setValueReadType(SampleType::Float64)
                      .setDomainReadType(SampleType::RangeInt64)
                      .setBlockSize(1)
                      .build();
    
    {
        SizeT tmpCount{0};
        auto status = reader.read(nullptr, &tmpCount);
        ASSERT_EQ(status.getReadStatus(), ReadStatus::Event);
    }
    
    BlockReaderPtr newReader;

    reader.setOnDataAvailable([&]
    {
        if (!newReader.assigned())
        {
            SizeT tmpCount = 0;
            auto status = reader.read(nullptr, &tmpCount);
            if (status.getReadStatus() == ReadStatus::Event)
            {
                newReader = BlockReaderBuilder()
                                .setOldBlockReader(reader)
                                .setValueReadType(SampleType::Float64)
                                .setDomainReadType(SampleType::RangeInt64)
                                .setBlockSize(BLOCK_SIZE)
                                .build();
            }
        }
        else
        {
            newReader.read(&samples, &count);
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

TYPED_TEST(BlockReaderTest, ReadingNotConnectedPort)
{
    this->signal.setDescriptor(setupDescriptor(SampleType::Float64));

    auto port = InputPort(this->signal.getContext(), nullptr, "readsig");

    auto reader = BlockReaderBuilder()
                      .setInputPort(port)
                      .setValueReadType(SampleType::Float64)
                      .setDomainReadType(SampleType::RangeInt64)
                      .setBlockSize(BLOCK_SIZE)
                      .build();

    auto avaialble = reader.getAvailableCount();
    ASSERT_EQ(avaialble, 0u);

    SizeT count{1};
    double samples[BLOCK_SIZE]{};
    auto status = reader.read(&samples, &count);
    ASSERT_EQ(count, 0u);
    ASSERT_EQ(status.getReadStatus(), ReadStatus::Ok);

    // read with timeout
    count = 1u;
    status = reader.read(&samples, &count, 100u);
    ASSERT_EQ(count, 0u);
    ASSERT_EQ(status.getReadStatus(), ReadStatus::Ok);

    // connecting port
    port.connect(this->signal);

    // check that event is encountered
    count = 1u;
    status = reader.read(&samples, &count);
    ASSERT_EQ(count, 0u);
    ASSERT_EQ(status.getReadStatus(), ReadStatus::Event);
}

TYPED_TEST(BlockReaderTest, NotifyPortIsConnected)
{
    this->signal.setDescriptor(setupDescriptor(SampleType::Float64));

    auto port = InputPort(this->signal.getContext(), nullptr, "readsig");

    auto reader = BlockReaderBuilder()
                      .setInputPort(port)
                      .setValueReadType(SampleType::Float64)
                      .setDomainReadType(SampleType::RangeInt64)
                      .setBlockSize(BLOCK_SIZE)
                      .build();
    
    BlockReaderStatusPtr status;
    std::promise<void> promise;
    std::future<void> future = promise.get_future();

    reader.setOnDataAvailable([&] {
        SizeT count{0};
        status = reader.read(nullptr, &count);
        promise.set_value();
    });

    port.connect(this->signal);

    auto promiseStatus = future.wait_for(std::chrono::seconds(1));
    ASSERT_EQ(promiseStatus, std::future_status::ready);
    ASSERT_EQ(status.getReadStatus(), ReadStatus::Event);
}

TYPED_TEST(BlockReaderTest, ReadWhilePortIsNotConnected)
{
    this->signal.setDescriptor(setupDescriptor(SampleType::Float64));

    auto port = InputPort(this->signal.getContext(), nullptr, "readsig");

    auto reader = BlockReaderBuilder()
                      .setInputPort(port)
                      .setValueReadType(SampleType::Float64)
                      .setDomainReadType(SampleType::RangeInt64)
                      .setBlockSize(BLOCK_SIZE)
                      .build();

    BlockReaderStatusPtr status;
    auto future = std::async(std::launch::async, [&] {
        SizeT count{0};
        status = reader.read(nullptr, &count, 1000u);
    });

    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    port.connect(this->signal);

    future.wait();
    ASSERT_EQ(status.getReadStatus(), ReadStatus::Event);
}

TYPED_TEST(BlockReaderTest, ReconnectWhileReading)
{
    this->signal.setDescriptor(setupDescriptor(SampleType::Float64));

    auto port = InputPort(this->signal.getContext(), nullptr, "readsig");

    auto reader = BlockReaderBuilder()
                      .setInputPort(port)
                      .setValueReadType(SampleType::Float64)
                      .setDomainReadType(SampleType::RangeInt64)
                      .setBlockSize(BLOCK_SIZE)
                      .build();
    port.connect(this->signal);

    SizeT count{0};
    BlockReaderStatusPtr status = reader.read(nullptr, &count);
    ASSERT_EQ(status.getReadStatus(), ReadStatus::Event);

    status = nullptr;
    auto future = std::async(std::launch::async, [&] {
        // the timeout is ignored for count 0
        SizeT count{1};
        double samples[BLOCK_SIZE];
        status = reader.read(samples, &count, 1000u);
    });

    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    port.disconnect();
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    port.connect(this->signal);
    
    auto domainPacket = DataPacket(setupDescriptor(SampleType::RangeInt64, LinearDataRule(1, 0), nullptr), BLOCK_SIZE, 1);
    auto dataPacket = DataPacketWithDomain(domainPacket, this->signal.getDescriptor(), BLOCK_SIZE);
    this->sendPacket(dataPacket);

    future.wait();
    ASSERT_EQ(status.getReadStatus(), ReadStatus::Event);
}

TYPED_TEST(BlockReaderTest, DeltaCheck)
{
    this->signal.setDescriptor(setupDescriptor(SampleType::Float64));

    auto port = InputPort(this->signal.getContext(), nullptr, "readsig");

    auto reader = BlockReaderBuilder()
                      .setInputPort(port)
                      .setValueReadType(SampleType::Float64)
                      .setDomainReadType(SampleType::RangeInt64)
                      .setBlockSize(1)
                      .build();
    port.connect(this->signal);

    {
        SizeT count{0};
        auto status = reader.read(nullptr, &count);
        ASSERT_EQ(status.getReadStatus(), ReadStatus::Event);
    }

    for (int i = 0; i < 5; i++)
    {
        auto domainPacket = DataPacket(setupDescriptor(SampleType::RangeInt64, LinearDataRule(1, 0), nullptr), BLOCK_SIZE, i*2);
        auto dataPacket = DataPacketWithDomain(domainPacket, this->signal.getDescriptor(), BLOCK_SIZE);
        this->sendPacket(dataPacket);
    }

    {
        SizeT count{7};
        double samples[7];
        auto status = reader.read(samples, &count, 1000u);
        ASSERT_EQ(count, 7u);
        ASSERT_EQ(status.getOffset(), 0);
    }

    {
        SizeT count{1};
        double samples[1];
        auto status = reader.read(samples, &count);
        ASSERT_EQ(count, 1u);
        ASSERT_EQ(status.getOffset(), 7);
    }
}
