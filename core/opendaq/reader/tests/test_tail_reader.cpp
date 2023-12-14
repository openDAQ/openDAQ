#include <opendaq/reader_factory.h>
#include <coreobjects/unit_factory.h>
#include <testutils/testutils.h>
#include <date/date.h>
#include "reader_common.h"
#include <opendaq/input_port_factory.h>
#include <future>

using namespace daq;

using TailReaderTest = ReaderTest<>;

TEST_F(TailReaderTest, Create)
{
    ASSERT_NO_THROW((TailReader<Int, ClockRange>(signal, 10)));
}

TEST_F(TailReaderTest, CreateNullThrows)
{
    ASSERT_THROW_MSG((StreamReader<Int, ClockRange>)(nullptr), ArgumentNullException, "Signal must not be null")
}

TEST_F(TailReaderTest, IsReader)
{
    auto reader = TailReader(this->signal, 10);
    ASSERT_NO_THROW(reader.template asPtr<IReader>());
}

TEST_F(TailReaderTest, IsSampleReader)
{
    auto reader = TailReader(this->signal, 10);
    ASSERT_NO_THROW(reader.template asPtr<ISampleReader>());
}

TEST_F(TailReaderTest, IsTailReader)
{
    auto reader = TailReader(this->signal, 10);
    ASSERT_NO_THROW(reader.template asPtr<ITailReader>());
}

TEST_F(TailReaderTest, GetDefaultReadType)
{
    auto reader = TailReader(this->signal, 10);
    ASSERT_EQ(reader.getValueReadType(), SampleType::Float64);
    ASSERT_EQ(reader.getDomainReadType(), SampleType::Int64);
}

TEST_F(TailReaderTest, GetSamplesAvailableEmpty)
{
    auto reader = TailReader(this->signal, 10);
    ASSERT_EQ(reader.getAvailableCount(), 0u);
}

TEST_F(TailReaderTest, GetSamplesAvailable)
{
    this->signal.setDescriptor(setupDescriptor(SampleType::Float64));

    auto reader = TailReader(this->signal, 10);
    ASSERT_EQ(reader.getAvailableCount(), 0u);

    this->sendPacket(DataPacket(this->signal.getDescriptor(), 1));

    ASSERT_EQ(reader.getAvailableCount(), 1u);
}

TEST_F(TailReaderTest, GetSamplesBelowHistorySize)
{
    using ValueType = std::int64_t;

    this->signal.setDescriptor(setupDescriptor((SampleTypeFromType<ValueType>::SampleType)));

    constexpr auto PACKET_SAMPLES = 5u;
    constexpr auto HISTORY_SIZE = 10u;

    auto reader = TailReader(this->signal, HISTORY_SIZE);
    ASSERT_EQ(reader.getAvailableCount(), 0u);

    auto dataPacket = DataPacket(this->signal.getDescriptor(), PACKET_SAMPLES);
    auto dataPtr = static_cast<ValueType*>(dataPacket.getData());
    dataPtr[0] = 11;
    dataPtr[1] = 22;
    dataPtr[2] = 33;
    dataPtr[3] = 44;
    dataPtr[4] = 55;

    this->sendPacket(dataPacket);

    ASSERT_EQ(reader.getAvailableCount(), PACKET_SAMPLES);

    SizeT count{HISTORY_SIZE};
    double values[HISTORY_SIZE]{};
    reader.read(&values, &count);

    ASSERT_EQ(count, PACKET_SAMPLES);
    ASSERT_EQ(reader.getAvailableCount(), PACKET_SAMPLES);

    ASSERT_EQ(values[0], dataPtr[0]);
    ASSERT_EQ(values[1], dataPtr[1]);
    ASSERT_EQ(values[2], dataPtr[2]);
    ASSERT_EQ(values[3], dataPtr[3]);
    ASSERT_EQ(values[4], dataPtr[4]);
}

TEST_F(TailReaderTest, GetSamplesBelowHistorySizeTwice)
{
    using ValueType = std::int64_t;

    this->signal.setDescriptor(setupDescriptor(SampleTypeFromType<ValueType>::SampleType));

    constexpr auto PACKET_SAMPLES = 5u;
    constexpr auto HISTORY_SIZE = 10u;

    auto reader = TailReader(this->signal, HISTORY_SIZE);
    ASSERT_EQ(reader.getAvailableCount(), 0u);

    auto dataPacket = DataPacket(this->signal.getDescriptor(), PACKET_SAMPLES);
    auto dataPtr = static_cast<ValueType*>(dataPacket.getData());
    dataPtr[0] = 11;
    dataPtr[1] = 22;
    dataPtr[2] = 33;
    dataPtr[3] = 44;
    dataPtr[4] = 55;

    this->sendPacket(dataPacket);

    ASSERT_EQ(reader.getAvailableCount(), PACKET_SAMPLES);

    SizeT count{HISTORY_SIZE};
    double values[HISTORY_SIZE];
    reader.read(&values, &count);

    ASSERT_EQ(count, PACKET_SAMPLES);

    ASSERT_EQ(values[0], dataPtr[0]);
    ASSERT_EQ(values[1], dataPtr[1]);
    ASSERT_EQ(values[2], dataPtr[2]);
    ASSERT_EQ(values[3], dataPtr[3]);
    ASSERT_EQ(values[4], dataPtr[4]);

    // Second
    SizeT count2{10};
    double values2[10];
    reader.read(&values2, &count2);

    ASSERT_EQ(count2, PACKET_SAMPLES);
    ASSERT_EQ(reader.getAvailableCount(), PACKET_SAMPLES);

    ASSERT_EQ(values2[0], dataPtr[0]);
    ASSERT_EQ(values2[1], dataPtr[1]);
    ASSERT_EQ(values2[2], dataPtr[2]);
    ASSERT_EQ(values2[3], dataPtr[3]);
    ASSERT_EQ(values2[4], dataPtr[4]);
}

TEST_F(TailReaderTest, GetSamplesEqualHistory)
{
    using ValueType = std::int64_t;

    this->signal.setDescriptor(setupDescriptor((SampleTypeFromType<ValueType>::SampleType)));

    constexpr auto HISTORY_SIZE = 10u;

    auto reader = TailReader(this->signal, HISTORY_SIZE);
    ASSERT_EQ(reader.getAvailableCount(), 0u);

    auto dataPacket = DataPacket(this->signal.getDescriptor(), HISTORY_SIZE);
    auto dataPtr = static_cast<ValueType*>(dataPacket.getData());
    dataPtr[0] = 00;
    dataPtr[1] = 11;
    dataPtr[2] = 22;
    dataPtr[3] = 33;
    dataPtr[4] = 44;
    dataPtr[5] = 55;
    dataPtr[6] = 66;
    dataPtr[7] = 77;
    dataPtr[8] = 88;
    dataPtr[9] = 99;

    this->sendPacket(dataPacket);

    ASSERT_EQ(reader.getAvailableCount(), HISTORY_SIZE);

    SizeT count{HISTORY_SIZE};
    double values[HISTORY_SIZE]{};
    reader.read(&values, &count);

    ASSERT_EQ(count, HISTORY_SIZE);
    ASSERT_EQ(reader.getAvailableCount(), HISTORY_SIZE);

    ASSERT_EQ(values[0], dataPtr[0]);
    ASSERT_EQ(values[1], dataPtr[1]);
    ASSERT_EQ(values[2], dataPtr[2]);
    ASSERT_EQ(values[3], dataPtr[3]);
    ASSERT_EQ(values[4], dataPtr[4]);
    ASSERT_EQ(values[5], dataPtr[5]);
    ASSERT_EQ(values[6], dataPtr[6]);
    ASSERT_EQ(values[7], dataPtr[7]);
    ASSERT_EQ(values[8], dataPtr[8]);
    ASSERT_EQ(values[9], dataPtr[9]);
}

TEST_F(TailReaderTest, GetSamplesEqualHistoryTwice)
{
    using ValueType = std::int64_t;

    this->signal.setDescriptor(setupDescriptor((SampleTypeFromType<ValueType>::SampleType)));

    constexpr auto HISTORY_SIZE = 10u;

    auto reader = TailReader(this->signal, HISTORY_SIZE);
    ASSERT_EQ(reader.getAvailableCount(), 0u);

    auto dataPacket = DataPacket(this->signal.getDescriptor(), HISTORY_SIZE);
    auto dataPtr = static_cast<ValueType*>(dataPacket.getData());
    dataPtr[0] = 00;
    dataPtr[1] = 11;
    dataPtr[2] = 22;
    dataPtr[3] = 33;
    dataPtr[4] = 44;
    dataPtr[5] = 55;
    dataPtr[6] = 66;
    dataPtr[7] = 77;
    dataPtr[8] = 88;
    dataPtr[9] = 99;

    this->sendPacket(dataPacket);

    ASSERT_EQ(reader.getAvailableCount(), HISTORY_SIZE);

    SizeT count{HISTORY_SIZE};
    double values[HISTORY_SIZE]{};
    reader.read(&values, &count);

    ASSERT_EQ(count, HISTORY_SIZE);
    ASSERT_EQ(reader.getAvailableCount(), HISTORY_SIZE);

    ASSERT_EQ(values[0], dataPtr[0]);
    ASSERT_EQ(values[1], dataPtr[1]);
    ASSERT_EQ(values[2], dataPtr[2]);
    ASSERT_EQ(values[3], dataPtr[3]);
    ASSERT_EQ(values[4], dataPtr[4]);
    ASSERT_EQ(values[5], dataPtr[5]);
    ASSERT_EQ(values[6], dataPtr[6]);
    ASSERT_EQ(values[7], dataPtr[7]);
    ASSERT_EQ(values[8], dataPtr[8]);
    ASSERT_EQ(values[9], dataPtr[9]);

    // Second

    SizeT count2{HISTORY_SIZE};
    double values2[HISTORY_SIZE]{};
    reader.read(&values2, &count2);

    ASSERT_EQ(count, HISTORY_SIZE);
    ASSERT_EQ(reader.getAvailableCount(), HISTORY_SIZE);

    ASSERT_EQ(values2[0], dataPtr[0]);
    ASSERT_EQ(values2[1], dataPtr[1]);
    ASSERT_EQ(values2[2], dataPtr[2]);
    ASSERT_EQ(values2[3], dataPtr[3]);
    ASSERT_EQ(values2[4], dataPtr[4]);
    ASSERT_EQ(values2[5], dataPtr[5]);
    ASSERT_EQ(values2[6], dataPtr[6]);
    ASSERT_EQ(values2[7], dataPtr[7]);
    ASSERT_EQ(values2[8], dataPtr[8]);
    ASSERT_EQ(values2[9], dataPtr[9]);
}

TEST_F(TailReaderTest, GetSamplesMoreSamplesThanHistoryEmpty)
{
    using ValueType = std::int64_t;

    this->signal.setDescriptor(setupDescriptor((SampleTypeFromType<ValueType>::SampleType)));

    constexpr auto HISTORY_SIZE = 10u;

    auto reader = TailReader(this->signal, HISTORY_SIZE);
    ASSERT_EQ(reader.getAvailableCount(), 0u);

    SizeT count{HISTORY_SIZE + 1};
    double values[HISTORY_SIZE + 1];
    ASSERT_THROW_MSG(reader.read(&values, &count), SizeTooLargeException, "The requested sample-count exceeds the reader history size.");
}

TEST_F(TailReaderTest, GetSamplesMoreSamplesThanHistoryFull)
{
    using ValueType = std::int64_t;

    constexpr auto HISTORY_SIZE = 10u;

    this->signal.setDescriptor(setupDescriptor((SampleTypeFromType<ValueType>::SampleType)));

    auto reader = TailReader(this->signal, HISTORY_SIZE);
    ASSERT_EQ(reader.getAvailableCount(), 0u);

    auto dataPacket = DataPacket(this->signal.getDescriptor(), HISTORY_SIZE);

    auto dataPtr = static_cast<ValueType*>(dataPacket.getData());
    dataPtr[0] = 00;
    dataPtr[1] = 11;
    dataPtr[2] = 22;
    dataPtr[3] = 33;
    dataPtr[4] = 44;
    dataPtr[5] = 55;
    dataPtr[6] = 66;
    dataPtr[7] = 77;
    dataPtr[8] = 88;
    dataPtr[9] = 99;

    this->sendPacket(dataPacket);

    ASSERT_EQ(reader.getAvailableCount(), HISTORY_SIZE);

    SizeT count{HISTORY_SIZE + 1};
    double values[HISTORY_SIZE + 1];
    ASSERT_THROW_MSG(reader.read(&values, &count), SizeTooLargeException, "The requested sample-count exceeds the reader history size.");
}

TEST_F(TailReaderTest, GetSamplesRolling)
{
    using ValueType = std::int64_t;

    this->signal.setDescriptor(setupDescriptor((SampleTypeFromType<ValueType>::SampleType)));

    constexpr auto HISTORY_SIZE = 10u;

    auto reader = TailReader(this->signal, HISTORY_SIZE);
    ASSERT_EQ(reader.getAvailableCount(), 0u);

    auto dataPacket = DataPacket(this->signal.getDescriptor(), HISTORY_SIZE);

    auto dataPtr = static_cast<ValueType*>(dataPacket.getData());
    dataPtr[0] = 0;
    dataPtr[1] = 1;
    dataPtr[2] = 2;
    dataPtr[3] = 3;
    dataPtr[4] = 4;
    dataPtr[5] = 5;
    dataPtr[6] = 6;
    dataPtr[7] = 7;
    dataPtr[8] = 8;
    dataPtr[9] = 9;

    this->sendPacket(dataPacket);

    ASSERT_EQ(reader.getAvailableCount(), HISTORY_SIZE);

    SizeT count{HISTORY_SIZE};
    double values[HISTORY_SIZE]{};
    reader.read(&values, &count);

    ASSERT_EQ(count, HISTORY_SIZE);
    ASSERT_EQ(reader.getAvailableCount(), HISTORY_SIZE);

    ASSERT_EQ(values[0], dataPtr[0]);
    ASSERT_EQ(values[1], dataPtr[1]);
    ASSERT_EQ(values[2], dataPtr[2]);
    ASSERT_EQ(values[3], dataPtr[3]);
    ASSERT_EQ(values[4], dataPtr[4]);
    ASSERT_EQ(values[5], dataPtr[5]);
    ASSERT_EQ(values[6], dataPtr[6]);
    ASSERT_EQ(values[7], dataPtr[7]);
    ASSERT_EQ(values[8], dataPtr[8]);
    ASSERT_EQ(values[9], dataPtr[9]);

    constexpr auto NEXT_PACKET_SAMPLES = 3u;
    auto nextPacket = DataPacket(this->signal.getDescriptor(), NEXT_PACKET_SAMPLES);

    auto nextPtr = static_cast<ValueType*>(nextPacket.getData());
    nextPtr[0] = 10;
    nextPtr[1] = 11;
    nextPtr[2] = 12;

    this->sendPacket(nextPacket);

    ASSERT_EQ(reader.getAvailableCount(), HISTORY_SIZE + NEXT_PACKET_SAMPLES);

    SizeT nextCount{HISTORY_SIZE};
    double nextValues[HISTORY_SIZE]{};
    reader.read(&nextValues, &nextCount);

    ASSERT_EQ(nextCount, HISTORY_SIZE);
    ASSERT_GE(reader.getAvailableCount(), HISTORY_SIZE + NEXT_PACKET_SAMPLES);

    ASSERT_EQ(nextValues[0], dataPtr[3]);
    ASSERT_EQ(nextValues[1], dataPtr[4]);
    ASSERT_EQ(nextValues[2], dataPtr[5]);
    ASSERT_EQ(nextValues[3], dataPtr[6]);
    ASSERT_EQ(nextValues[4], dataPtr[7]);
    ASSERT_EQ(nextValues[5], dataPtr[8]);
    ASSERT_EQ(nextValues[6], dataPtr[9]);
    ASSERT_EQ(nextValues[7], nextPtr[0]);
    ASSERT_EQ(nextValues[8], nextPtr[1]);
    ASSERT_EQ(nextValues[9], nextPtr[2]);
}

TEST_F(TailReaderTest, GetSamplesRollingDomain)
{
    using ValueType = std::int64_t;
    using DomainType = ClockTick;

    this->signal.setDescriptor(setupDescriptor((SampleTypeFromType<ValueType>::SampleType)));

    constexpr auto HISTORY_SIZE = 10u;

    auto reader = TailReader(this->signal, HISTORY_SIZE);
    ASSERT_EQ(reader.getAvailableCount(), 0u);

    ///////////////////////////
    //// FIRST PACKET
    ///////////////////////////

    auto domainPacket = DataPacket(
        setupDescriptor(SampleType::UInt64, LinearDataRule(1, 0), nullptr),
        HISTORY_SIZE,
        0
    );
    auto dataPacket = DataPacketWithDomain(
        domainPacket,
        this->signal.getDescriptor(),
        HISTORY_SIZE
    );

    auto dataPtr = static_cast<ValueType*>(dataPacket.getData());
    dataPtr[0] = 0;
    dataPtr[1] = 1;
    dataPtr[2] = 2;
    dataPtr[3] = 3;
    dataPtr[4] = 4;
    dataPtr[5] = 5;
    dataPtr[6] = 6;
    dataPtr[7] = 7;
    dataPtr[8] = 8;
    dataPtr[9] = 9;

    this->sendPacket(dataPacket);

    ///////////////////////////
    ///////////////////////////

    ASSERT_EQ(reader.getAvailableCount(), HISTORY_SIZE);

    SizeT count{HISTORY_SIZE};
    double values[HISTORY_SIZE]{};
    DomainType domain[HISTORY_SIZE]{};
    reader.readWithDomain(&values, &domain, &count);

    ASSERT_EQ(count, HISTORY_SIZE);
    ASSERT_EQ(reader.getAvailableCount(), HISTORY_SIZE);

    ASSERT_EQ(values[0], dataPtr[0]);
    ASSERT_EQ(values[1], dataPtr[1]);
    ASSERT_EQ(values[2], dataPtr[2]);
    ASSERT_EQ(values[3], dataPtr[3]);
    ASSERT_EQ(values[4], dataPtr[4]);
    ASSERT_EQ(values[5], dataPtr[5]);
    ASSERT_EQ(values[6], dataPtr[6]);
    ASSERT_EQ(values[7], dataPtr[7]);
    ASSERT_EQ(values[8], dataPtr[8]);
    ASSERT_EQ(values[9], dataPtr[9]);

    ///////////////////////////
    //// SECOND PACKET
    ///////////////////////////

    constexpr auto NEXT_PACKET_SAMPLES = 3u;
    auto nextDomainPacket = DataPacket(
        setupDescriptor(SampleType::UInt64, LinearDataRule(1, 0), nullptr),
        NEXT_PACKET_SAMPLES,
        HISTORY_SIZE
    );
    auto nextPacket = DataPacketWithDomain(
        nextDomainPacket,
        this->signal.getDescriptor(),
        NEXT_PACKET_SAMPLES
    );

    auto nextPtr = static_cast<ValueType*>(nextPacket.getData());
    nextPtr[0] = 10;
    nextPtr[1] = 11;
    nextPtr[2] = 12;

    // auto ticks = static_cast<ClockTick*>(nextDomainPacket.getData());

    this->sendPacket(nextPacket);

    ///////////////////////////
    ///////////////////////////

    ASSERT_EQ(reader.getAvailableCount(), HISTORY_SIZE + NEXT_PACKET_SAMPLES);

    SizeT nextCount{HISTORY_SIZE};
    double nextValues[HISTORY_SIZE]{};
    DomainType nextDomain[HISTORY_SIZE]{};
    reader.readWithDomain(&nextValues, &nextDomain, &nextCount);

    ASSERT_EQ(nextCount, HISTORY_SIZE);
    ASSERT_EQ(reader.getAvailableCount(), HISTORY_SIZE + NEXT_PACKET_SAMPLES);

    ASSERT_EQ(nextValues[0], dataPtr[3]);
    ASSERT_EQ(nextValues[1], dataPtr[4]);
    ASSERT_EQ(nextValues[2], dataPtr[5]);
    ASSERT_EQ(nextValues[3], dataPtr[6]);
    ASSERT_EQ(nextValues[4], dataPtr[7]);
    ASSERT_EQ(nextValues[5], dataPtr[8]);
    ASSERT_EQ(nextValues[6], dataPtr[9]);
    ASSERT_EQ(nextValues[7], nextPtr[0]);
    ASSERT_EQ(nextValues[8], nextPtr[1]);
    ASSERT_EQ(nextValues[9], nextPtr[2]);

    ///////////////////////////
    //// THIRD PACKET
    ///////////////////////////

    constexpr auto THIRD_PACKET_SAMPLES = 2u;
    auto thirdDomainPacket = DataPacket(
        setupDescriptor(SampleType::UInt64, LinearDataRule(1, 0), nullptr),
        THIRD_PACKET_SAMPLES,
        HISTORY_SIZE + NEXT_PACKET_SAMPLES
    );
    auto thirdPacket = DataPacketWithDomain(
        thirdDomainPacket,
        this->signal.getDescriptor(),
        THIRD_PACKET_SAMPLES
    );

    auto thirdPtr = static_cast<ValueType*>(thirdPacket.getData());
    thirdPtr[0] = 13;
    thirdPtr[1] = 14;

    this->sendPacket(thirdPacket);

    ///////////////////////////
    ///////////////////////////

    ASSERT_EQ(reader.getAvailableCount(), HISTORY_SIZE + NEXT_PACKET_SAMPLES + THIRD_PACKET_SAMPLES);

    SizeT thirdCount{HISTORY_SIZE};
    double thirdValues[HISTORY_SIZE]{};
    DomainType thirdDomain[HISTORY_SIZE]{};
    reader.readWithDomain(&thirdValues, &thirdDomain, &thirdCount);

    ASSERT_EQ(thirdCount, HISTORY_SIZE);
    ASSERT_EQ(reader.getAvailableCount(), HISTORY_SIZE + NEXT_PACKET_SAMPLES + THIRD_PACKET_SAMPLES);

    ASSERT_EQ(thirdValues[0], dataPtr[5]);
    ASSERT_EQ(thirdValues[1], dataPtr[6]);
    ASSERT_EQ(thirdValues[2], dataPtr[7]);
    ASSERT_EQ(thirdValues[3], dataPtr[8]);
    ASSERT_EQ(thirdValues[4], dataPtr[9]);
    ASSERT_EQ(thirdValues[5], nextPtr[0]);
    ASSERT_EQ(thirdValues[6], nextPtr[1]);
    ASSERT_EQ(thirdValues[7], nextPtr[2]);
    ASSERT_EQ(thirdValues[8], thirdPtr[0]);
    ASSERT_EQ(thirdValues[9], thirdPtr[1]);

    ///////////////////////////
    ///////////////////////////

    SizeT fourthCount{4};
    double fourthValues[4]{};
    DomainType fourthDomain[4]{};
    reader.readWithDomain(&fourthValues, &fourthDomain, &fourthCount);

    ASSERT_EQ(fourthValues[0], nextPtr[1]);
    ASSERT_EQ(fourthValues[1], nextPtr[2]);
    ASSERT_EQ(fourthValues[2], thirdPtr[0]);
    ASSERT_EQ(fourthValues[3], thirdPtr[1]);

    SizeT fifthCount{0};
    double fifthValues[1]{};
    DomainType fifthDomain[1]{};
    reader.readWithDomain(&fifthValues, &fifthDomain, &fifthCount);

    ASSERT_EQ(fifthCount, 0u);
}

TEST_F(TailReaderTest, SmallPackets)
{
    using ValueType = std::int64_t;
    using DomainType = ClockTick;

    this->signal.setDescriptor(setupDescriptor((SampleTypeFromType<ValueType>::SampleType)));

    constexpr auto HISTORY_SIZE = 10u;
    auto implicitValue = 0;

    auto reader = TailReader(this->signal, HISTORY_SIZE);
    ASSERT_EQ(reader.getAvailableCount(), 0u);

    ///////////////////////////
    //// FIRST PACKET
    ///////////////////////////

    constexpr auto FIRST_PACKET_SAMPLES = 4u;
    auto firstPacket = createDataPacket(FIRST_PACKET_SAMPLES, implicitValue);

    auto firstPtr = static_cast<ValueType*>(firstPacket.getData());
    firstPtr[0] = 0;
    firstPtr[1] = 1;
    firstPtr[2] = 2;
    firstPtr[3] = 3;

    this->sendPacket(firstPacket);

    implicitValue += FIRST_PACKET_SAMPLES;

    ///////////////////////////
    //// SECOND PACKET
    ///////////////////////////

    constexpr auto SECOND_PACKET_SAMPLES = 3u;
    auto secondPacket = createDataPacket(SECOND_PACKET_SAMPLES, implicitValue);

    auto secondPtr = static_cast<ValueType*>(secondPacket.getData());
    secondPtr[0] = 4;
    secondPtr[1] = 5;
    secondPtr[2] = 6;

    this->sendPacket(secondPacket);

    implicitValue += SECOND_PACKET_SAMPLES;

    ///////////////////////////
    //// THIRD PACKET
    ///////////////////////////

    constexpr auto THIRD_PACKET_SAMPLES = 1u;
    auto thirdPacket = createDataPacket(THIRD_PACKET_SAMPLES, implicitValue);

    auto thirdPtr = static_cast<ValueType*>(thirdPacket.getData());
    thirdPtr[0] = 7;

    this->sendPacket(thirdPacket);

    implicitValue += THIRD_PACKET_SAMPLES;

    ///////////////////////////
    //// FOURTH PACKET
    ///////////////////////////

    constexpr auto FOURTH_PACKET_SAMPLES = 2u;
    auto fourthPacket = createDataPacket(FOURTH_PACKET_SAMPLES, implicitValue);

    auto fourthPtr = static_cast<ValueType*>(fourthPacket.getData());
    fourthPtr[0] = 8;
    fourthPtr[1] = 9;

    this->sendPacket(fourthPacket);

    implicitValue += FOURTH_PACKET_SAMPLES;

    ///////////////////////////
    //// FIFTH PACKET
    ///////////////////////////

    constexpr auto FIFTH_PACKET_SAMPLES = 2u;
    auto fifthPacket = createDataPacket(FIFTH_PACKET_SAMPLES, implicitValue);

    auto fifthPtr = static_cast<ValueType*>(fifthPacket.getData());
    fifthPtr[0] = 10;
    fifthPtr[1] = 11;

    this->sendPacket(fifthPacket);

    // Will just add the packet to cache because removing the oldest packet would get us below `historySize`
    ASSERT_EQ(reader.getAvailableCount(), HISTORY_SIZE + FIFTH_PACKET_SAMPLES);

    implicitValue += FIFTH_PACKET_SAMPLES;

    ///////////////////////////
    //// SIXTH PACKET
    ///////////////////////////

    constexpr auto SIXTH_PACKET_SAMPLES = 7u;
    auto sixthPacket = createDataPacket(SIXTH_PACKET_SAMPLES, implicitValue);

    auto sixthPtr = static_cast<ValueType*>(sixthPacket.getData());
    sixthPtr[0] = 12;
    sixthPtr[1] = 13;
    sixthPtr[2] = 14;
    sixthPtr[3] = 15;
    sixthPtr[4] = 16;
    sixthPtr[5] = 17;
    sixthPtr[6] = 18;

    this->sendPacket(sixthPacket);

    // Will remove everything but the previous two packets exceeding the history by 1
    ASSERT_EQ(
        reader.getAvailableCount(),
        FOURTH_PACKET_SAMPLES +
        FIFTH_PACKET_SAMPLES +
        SIXTH_PACKET_SAMPLES
    );

    implicitValue += SIXTH_PACKET_SAMPLES;

    ///////////////////////////

    auto last = sixthPtr[6];

    for (SizeT i = 1; i <= HISTORY_SIZE; ++i)
    {
        auto values = std::make_unique<double[]>(i);
        auto domain = std::make_unique<DomainType[]>(i);

        memset(values.get(), 0, sizeof(double) * i);
        memset(domain.get(), 0, sizeof(DomainType) * i);

        SizeT size = i;
        reader.readWithDomain(values.get(), domain.get(), &size);
        ASSERT_EQ(size, i);

        for (SizeT j = 0; j < size; ++j)
        {
            ASSERT_EQ(values[size - j - 1], last - j);
            ASSERT_EQ(domain[size - j - 1], static_cast<DomainType>(last - j));
        }
    }
}

TEST_F(TailReaderTest, ReadUndefinedNoDomain)
{
    const SizeT HISTORY_SIZE = 2u;
    this->signal.setDescriptor(setupDescriptor(SampleType::Float64));

    auto reader = daq::BlockReader(this->signal, HISTORY_SIZE, SampleType::Undefined, SampleType::Undefined);

    ASSERT_EQ(reader.getValueReadType(), SampleType::Float64);
    ASSERT_EQ(reader.getDomainReadType(), SampleType::Invalid);
}

TEST_F(TailReaderTest, ReadUndefinedWithDomain)
{
    const SizeT HISTORY_SIZE = 2u;
    this->signal.setDescriptor(setupDescriptor(SampleType::Float64));

    auto reader = daq::TailReader(this->signal, HISTORY_SIZE, SampleType::Undefined, SampleType::Undefined);

    ASSERT_EQ(reader.getValueReadType(), SampleType::Float64);  // read from signal descriptor
    ASSERT_EQ(reader.getDomainReadType(), SampleType::Invalid);

    auto domainDescriptor = setupDescriptor(SampleType::RangeInt64, LinearDataRule(1, 0), nullptr);
    auto eventPacket = DataDescriptorChangedEventPacket(nullptr, domainDescriptor);
    this->sendPacket(eventPacket);

    auto domainPacket = DataPacket(domainDescriptor, HISTORY_SIZE, 1);
    auto dataPacket = DataPacketWithDomain(domainPacket, this->signal.getDescriptor(), HISTORY_SIZE);
    auto dataPtr = static_cast<double*>(dataPacket.getData());
    dataPtr[0] = 11.1;
    dataPtr[1] = 22.2;

    this->sendPacket(dataPacket);

    SizeT count{HISTORY_SIZE};
    double samples[HISTORY_SIZE]{};
    reader.read(&samples, &count);

    ASSERT_EQ(count, HISTORY_SIZE);

    ASSERT_EQ(reader.getValueReadType(), SampleType::Float64);
    // domain info available from descriptor change event
    ASSERT_EQ(reader.getDomainReadType(), SampleType::RangeInt64);
}

TEST_F(TailReaderTest, ReadUndefinedWithNoDomainFromPacket)
{
    const SizeT HISTORY_SIZE = 2u;

    this->signal.setDescriptor(setupDescriptor(SampleType::Float64));

    auto reader = daq::TailReader(this->signal, HISTORY_SIZE, SampleType::Undefined, SampleType::Undefined);

    ASSERT_EQ(reader.getValueReadType(), SampleType::Float64);  // read from signal descriptor
    ASSERT_EQ(reader.getDomainReadType(), SampleType::Invalid);

    auto domainPacket = DataPacket(setupDescriptor(SampleType::RangeInt64, LinearDataRule(1, 0), nullptr), HISTORY_SIZE, 1);
    auto dataPacket = DataPacketWithDomain(domainPacket, this->signal.getDescriptor(), HISTORY_SIZE);
    auto dataPtr = static_cast<double*>(dataPacket.getData());
    dataPtr[0] = 111.1;
    dataPtr[1] = 222.2;

    this->sendPacket(dataPacket);

    SizeT count{HISTORY_SIZE};
    double samples[HISTORY_SIZE]{};
    reader.read(&samples, &count);

    ASSERT_EQ(count, HISTORY_SIZE);
    ASSERT_EQ(reader.getValueReadType(), SampleType::Float64);

    // domain was not read so its info was not updated
    ASSERT_EQ(reader.getDomainReadType(), SampleType::Invalid);
}

TEST_F(TailReaderTest, ReadUndefinedWithWithDomainFromPacket)
{
    const SizeT HISTORY_SIZE = 2u;

    this->signal.setDescriptor(setupDescriptor(SampleType::Float64));

    auto reader = daq::TailReader(this->signal, HISTORY_SIZE, SampleType::Undefined, SampleType::Undefined);

    ASSERT_EQ(reader.getValueReadType(), SampleType::Float64);  // read from signal descriptor
    ASSERT_EQ(reader.getDomainReadType(), SampleType::Invalid);

    auto domainPacket = DataPacket(setupDescriptor(SampleType::RangeInt64, LinearDataRule(1, 0), nullptr), HISTORY_SIZE, 1);
    auto dataPacket = DataPacketWithDomain(domainPacket, this->signal.getDescriptor(), HISTORY_SIZE);
    auto dataPtr = static_cast<double*>(dataPacket.getData());
    dataPtr[0] = 111.1;
    dataPtr[1] = 222.2;

    this->sendPacket(dataPacket);

    SizeT count{HISTORY_SIZE};
    double samples[HISTORY_SIZE]{};
    RangeType64 domain[HISTORY_SIZE]{};
    reader.readWithDomain(&samples, &domain, &count);

    ASSERT_EQ(count, HISTORY_SIZE);
    ASSERT_EQ(reader.getValueReadType(), SampleType::Float64);

    // domain was read and updated from packet info
    ASSERT_EQ(reader.getDomainReadType(), SampleType::RangeInt64);
}

TEST_F(TailReaderTest, TailReaderWithInputPort)
{
    const SizeT HISTORY_SIZE = 2u;

    this->signal.setDescriptor(setupDescriptor(SampleType::Float64));

    auto port = InputPort(this->signal.getContext(), nullptr, "readsig");
    port.connect(this->signal);
    auto reader = daq::TailReaderFromPort(port, HISTORY_SIZE, SampleType::Undefined, SampleType::Undefined);

    ASSERT_EQ(reader.getValueReadType(), SampleType::Float64);  // read from signal descriptor
    ASSERT_EQ(reader.getDomainReadType(), SampleType::Invalid);

    auto domainPacket = DataPacket(setupDescriptor(SampleType::RangeInt64, LinearDataRule(1, 0), nullptr), HISTORY_SIZE, 1);
    auto dataPacket = DataPacketWithDomain(domainPacket, this->signal.getDescriptor(), HISTORY_SIZE);
    auto dataPtr = static_cast<double*>(dataPacket.getData());
    dataPtr[0] = 111.1;
    dataPtr[1] = 222.2;

    this->sendPacket(dataPacket);

    SizeT count{HISTORY_SIZE};
    double samples[HISTORY_SIZE]{};
    RangeType64 domain[HISTORY_SIZE]{};
    reader.readWithDomain(&samples, &domain, &count);

    ASSERT_EQ(count, HISTORY_SIZE);
    ASSERT_EQ(reader.getValueReadType(), SampleType::Float64);

    // domain was read and updated from packet info
    ASSERT_EQ(reader.getDomainReadType(), SampleType::RangeInt64);
}

TEST_F(TailReaderTest, MultipleTailReaderToInputPort)
{
    const SizeT HISTORY_SIZE = 2u;

    this->signal.setDescriptor(setupDescriptor(SampleType::Float64));

    auto port = InputPort(this->signal.getContext(), nullptr, "readsig");
    port.connect(this->signal);

    auto reader1 = TailReaderFromPort(port, HISTORY_SIZE, SampleType::Undefined, SampleType::Undefined);
    ASSERT_THROW(TailReaderFromPort(port, HISTORY_SIZE, SampleType::Undefined, SampleType::Undefined), AlreadyExistsException);
}

TEST_F(TailReaderTest, TailReaderReuseInputPort)
{
    const SizeT HISTORY_SIZE = 2u;

    this->signal.setDescriptor(setupDescriptor(SampleType::Float64));

    auto port = InputPort(this->signal.getContext(), nullptr, "readsig");
    port.connect(this->signal);

    {
        auto reader1 = TailReaderFromPort(port, HISTORY_SIZE, SampleType::Undefined, SampleType::Undefined);
        printf("check\n");
    }
    ASSERT_NO_THROW(TailReaderFromPort(port, HISTORY_SIZE, SampleType::Undefined, SampleType::Undefined));
}

TEST_F(TailReaderTest, TailReaderWithNotConnectedInputPort)
{
    const SizeT HISTORY_SIZE = 2u;

    this->signal.setDescriptor(setupDescriptor(SampleType::Float64));

    auto port = InputPort(this->signal.getContext(), nullptr, "readsig");
    ASSERT_THROW(TailReaderFromPort(port, HISTORY_SIZE, SampleType::Undefined, SampleType::Undefined), ArgumentNullException);
}

TEST_F(TailReaderTest, TailReaderOnReadCallback)
{
    const SizeT HISTORY_SIZE = 2u;
    SizeT count{HISTORY_SIZE};
    double samples[HISTORY_SIZE]{};
    RangeType64 domain[HISTORY_SIZE]{};

    std::promise<void> promise;
    std::future<void> future = promise.get_future();

    this->signal.setDescriptor(setupDescriptor(SampleType::Float64));

    auto reader = daq::TailReader(this->signal, HISTORY_SIZE, SampleType::Undefined, SampleType::Undefined);
    reader.setOnAvailablePackets([&, promise = std::move(promise)] () mutable  {
        reader.readWithDomain(&samples, &domain, &count);
        promise.set_value();
        return nullptr;
    });

    ASSERT_EQ(reader.getValueReadType(), SampleType::Float64);  // read from signal descriptor
    ASSERT_EQ(reader.getDomainReadType(), SampleType::Invalid);

    auto domainPacket = DataPacket(setupDescriptor(SampleType::RangeInt64, LinearDataRule(1, 0), nullptr), HISTORY_SIZE, 1);
    auto dataPacket = DataPacketWithDomain(domainPacket, this->signal.getDescriptor(), HISTORY_SIZE);
    auto dataPtr = static_cast<double*>(dataPacket.getData());
    dataPtr[0] = 111.1;
    dataPtr[1] = 222.2;

    this->sendPacket(dataPacket);
    
    auto promiseStatus = future.wait_for(std::chrono::seconds(1));
    ASSERT_EQ(promiseStatus, std::future_status::ready);
   
    ASSERT_EQ(count, HISTORY_SIZE);
    ASSERT_EQ(reader.getValueReadType(), SampleType::Float64);

    // domain was read and updated from packet info
    ASSERT_EQ(reader.getDomainReadType(), SampleType::RangeInt64);
}