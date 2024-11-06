#include <gmock/gmock.h>
#include <opendaq/data_packet_ptr.h>
#include <opendaq/data_rule_factory.h>
#include <opendaq/dimension_factory.h>
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
using StreamReaderTest = ReaderTest<T>;

using SampleTypes = Types<OPENDAQ_VALUE_SAMPLE_TYPES>;

TYPED_TEST_SUITE(StreamReaderTest, SampleTypes);

TYPED_TEST(StreamReaderTest, Create)
{
    ASSERT_NO_THROW((StreamReader<TypeParam, ClockRange>) (this->signal));
}

TYPED_TEST(StreamReaderTest, CreateNullThrows)
{
    ASSERT_THROW_MSG((StreamReader<TypeParam, ClockRange>) (nullptr), ArgumentNullException, "Signal must not be null")
}

TYPED_TEST(StreamReaderTest, IsReader)
{
    auto reader = daq::StreamReader<TypeParam, ClockRange>(this->signal);
    ASSERT_NO_THROW(reader.template asPtr<IReader>());
}

TYPED_TEST(StreamReaderTest, IsSampleReader)
{
    auto reader = daq::StreamReader<TypeParam, ClockRange>(this->signal);
    ASSERT_NO_THROW(reader.template asPtr<ISampleReader>());
}

TYPED_TEST(StreamReaderTest, IsStreamReader)
{
    auto reader = daq::StreamReader<TypeParam, ClockRange>(this->signal);
    ASSERT_NO_THROW(reader.template asPtr<IStreamReader>());
}

TYPED_TEST(StreamReaderTest, GetDefaultReadType)
{
    auto reader = StreamReader<TypeParam>(this->signal);
    ASSERT_EQ(reader.getValueReadType(), SampleTypeFromType<TypeParam>::SampleType);
    ASSERT_EQ(reader.getDomainReadType(), SampleType::Int64);
}

TYPED_TEST(StreamReaderTest, GetSamplesAvailableEmpty)
{
    auto reader = daq::StreamReader<TypeParam, ClockRange>(this->signal);
    ASSERT_EQ(reader.getAvailableCount(), 0u);
}

TYPED_TEST(StreamReaderTest, GetSamplesAvailable)
{
    this->signal.setDescriptor(setupDescriptor(SampleType::Float64));

    auto reader = daq::StreamReaderBuilder()
        .setSignal(this->signal)
        .setValueReadType(SampleTypeFromType<TypeParam>::SampleType)
        .setDomainReadType(SampleTypeFromType<ClockRange>::SampleType)
        .setSkipEvents(true)
        .build();

    ASSERT_EQ(reader.getAvailableCount(), 0u);

    this->sendPacket(DataPacket(this->signal.getDescriptor(), 1));

    ASSERT_EQ(reader.getAvailableCount(), 1u);
}

TYPED_TEST(StreamReaderTest, ReadOneSample)
{
    this->signal.setDescriptor(setupDescriptor(SampleType::Float64));

    auto reader = daq::StreamReaderBuilder()
        .setSignal(this->signal)
        .setValueReadType(SampleTypeFromType<TypeParam>::SampleType)
        .setDomainReadType(SampleTypeFromType<ClockRange>::SampleType)
        .setSkipEvents(true)
        .build();
    auto dataPacket = DataPacket(this->signal.getDescriptor(), 1);

    // Set the first sample to
    auto dataPtr = static_cast<double*>(dataPacket.getData());
    dataPtr[0] = 123.4;

    this->sendPacket(dataPacket);

    SizeT count{1};
    TypeParam samples[1]{};
    reader.read(&samples, &count);

    ASSERT_EQ(count, 1u);

    if constexpr (IsTemplateOf<TypeParam, Complex_Number>::value || IsTemplateOf<TypeParam, RangeType>::value)
    {
        ASSERT_EQ(samples[0], TypeParam(typename TypeParam::Type(123.4)));
    }
    else
    {
        ASSERT_EQ(samples[0], TypeParam(123.4));
    }

    ASSERT_EQ(reader.getAvailableCount(), 0u);
}

TYPED_TEST(StreamReaderTest, ReadOneSampleRawValue)
{
    this->signal.setDescriptor(setupDescriptor(SampleType::Float64,
                                               nullptr,
                                               LinearScaling(4, 15, SampleType::Int32, ScaledSampleType::Float64)));

    auto reader = daq::StreamReader<TypeParam, ClockRange>(this->signal, ReadMode::RawValue);
    auto dataPacket = DataPacket(this->signal.getDescriptor(), 1);

    // Set the first sample
    auto dataPtr = static_cast<int32_t*>(dataPacket.getRawData());
    dataPtr[0] = 123;

    this->sendPacket(dataPacket);

    {
        SizeT count{0};
        auto status = reader.read(nullptr, &count);
    }

    SizeT count{1};
    TypeParam samples[1]{};
    reader.read(&samples, &count);

    ASSERT_EQ(count, 1u);

    if constexpr (IsTemplateOf<TypeParam, Complex_Number>::value || IsTemplateOf<TypeParam, RangeType>::value)
    {
        ASSERT_EQ(samples[0], TypeParam(typename TypeParam::Type(123)));
    }
    else
    {
        ASSERT_EQ(samples[0], TypeParam(123));
    }

    ASSERT_EQ(reader.getAvailableCount(), 0u);
}

TYPED_TEST(StreamReaderTest, ReadOneSampleWithTimeout)
{
    this->signal.setDescriptor(setupDescriptor(SampleType::Float64));

    auto reader = daq::StreamReaderBuilder()
        .setSignal(this->signal)
        .setValueReadType(SampleTypeFromType<TypeParam>::SampleType)
        .setDomainReadType(SampleTypeFromType<ClockRange>::SampleType)
        .setSkipEvents(true)
        .build();
    auto dataPacket = DataPacket(this->signal.getDescriptor(), 1);

    // Set the first sample to
    auto dataPtr = static_cast<double*>(dataPacket.getData());
    dataPtr[0] = 123.4;

    this->sendPacket(dataPacket);

    std::thread t([this, &dataPacket]
    {
        using namespace std::chrono_literals;

        std::this_thread::sleep_for(30ms);
        this->sendPacket(dataPacket);
    });

    SizeT count{2};
    TypeParam samples[2]{};
    reader.read(&samples, &count, 1000u);

    if (t.joinable())
    {
        t.join();
    }

    ASSERT_EQ(count, 2u);

    if constexpr (IsTemplateOf<TypeParam, Complex_Number>::value || IsTemplateOf<TypeParam, RangeType>::value)
    {
        ASSERT_EQ(samples[0], TypeParam(typename TypeParam::Type(123.4)));
        ASSERT_EQ(samples[1], TypeParam(typename TypeParam::Type(123.4)));
    }
    else
    {
        ASSERT_EQ(samples[0], TypeParam(123.4));
        ASSERT_EQ(samples[1], TypeParam(123.4));
    }

    ASSERT_EQ(reader.getAvailableCount(), 0u);
}

TYPED_TEST(StreamReaderTest, ReadOneSampleWithClockTicks)
{
    this->signal.setDescriptor(setupDescriptor(SampleType::Float64));

    auto reader = daq::StreamReaderBuilder()
        .setSignal(this->signal)
        .setValueReadType(SampleTypeFromType<TypeParam>::SampleType)
        .setDomainReadType(SampleTypeFromType<ClockTick>::SampleType)
        .setSkipEvents(true)
        .build();

    auto domainPacket = DataPacket(setupDescriptor(SampleType::RangeInt64, LinearDataRule(1, 0), nullptr), 1, 1);
    auto dataPacket = DataPacketWithDomain(domainPacket, this->signal.getDescriptor(), 1);

    // Set the first sample to
    auto dataPtr = static_cast<double*>(dataPacket.getData());
    dataPtr[0] = 123.4;

    this->sendPacket(dataPacket);

    SizeT count{1};
    TypeParam samples[1]{};
    ClockTick ticks[1]{};
    reader.readWithDomain(&samples, &ticks, &count);

    ASSERT_EQ(count, 1u);
    ASSERT_EQ(ticks[0], 1);

    if constexpr (IsTemplateOf<TypeParam, Complex_Number>::value || IsTemplateOf<TypeParam, RangeType>::value)
    {
        ASSERT_EQ(samples[0], TypeParam(typename TypeParam::Type(123.4)));
    }
    else
    {
        ASSERT_EQ(samples[0], TypeParam(123.4));
    }

    ASSERT_EQ(reader.getAvailableCount(), 0u);
}

TYPED_TEST(StreamReaderTest, ReadOneSampleWithClockTicksTimeout)
{
    this->signal.setDescriptor(setupDescriptor(SampleType::Float64));

    auto reader = daq::StreamReaderBuilder()
        .setSignal(this->signal)
        .setValueReadType(SampleTypeFromType<TypeParam>::SampleType)
        .setDomainReadType(SampleTypeFromType<ClockTick>::SampleType)
        .setSkipEvents(true)
        .build();

    auto domainPacket = DataPacket(setupDescriptor(SampleType::RangeInt64, LinearDataRule(1, 0), nullptr), 1, 1);
    auto dataPacket = DataPacketWithDomain(domainPacket, this->signal.getDescriptor(), 1);

    // Set the first sample to
    auto dataPtr = static_cast<double*>(dataPacket.getData());
    dataPtr[0] = 123.4;

    this->sendPacket(dataPacket);

    std::thread t([this, &dataPacket] {
        using namespace std::chrono_literals;

        std::this_thread::sleep_for(30ms);
        this->sendPacket(dataPacket);
    });

    SizeT count{2};
    TypeParam samples[2]{};
    ClockTick ticks[2]{};
    reader.readWithDomain(&samples, &ticks, &count, 1000u);

    if (t.joinable())
    {
        t.join();
    }

    ASSERT_EQ(count, 2u);
    ASSERT_EQ(ticks[0], 1);
    ASSERT_EQ(ticks[1], 1);

    if constexpr (IsTemplateOf<TypeParam, Complex_Number>::value || IsTemplateOf<TypeParam, RangeType>::value)
    {
        ASSERT_EQ(samples[0], TypeParam(typename TypeParam::Type(123.4)));
        ASSERT_EQ(samples[1], TypeParam(typename TypeParam::Type(123.4)));
    }
    else
    {
        ASSERT_EQ(samples[0], TypeParam(123.4));
        ASSERT_EQ(samples[1], TypeParam(123.4));
    }

    ASSERT_EQ(reader.getAvailableCount(), 0u);
}

TYPED_TEST(StreamReaderTest, ReadOneSampleWithRanges)
{
    this->signal.setDescriptor(setupDescriptor(SampleType::Float64));

    auto reader = daq::StreamReaderBuilder()
        .setSignal(this->signal)
        .setValueReadType(SampleTypeFromType<TypeParam>::SampleType)
        .setDomainReadType(SampleTypeFromType<ClockRange>::SampleType)
        .setSkipEvents(true)
        .build();

    auto domainPacket = DataPacket(
        setupDescriptor(SampleType::RangeInt64, LinearDataRule(1, 0), nullptr),
        1,
        1
        );

    auto dataPacket = DataPacketWithDomain(
        domainPacket,
        this->signal.getDescriptor(),
        1
        );

    // Set the first sample to
    auto dataPtr = static_cast<double*>(dataPacket.getData());
    dataPtr[0] = 123.4;

    this->sendPacket(dataPacket);

    SizeT count{1};
    TypeParam samples[1]{};
    ClockRange stamps[1]{};
    reader->readWithDomain(&samples, &stamps, &count);

    ASSERT_EQ(count, 1u);
    ASSERT_EQ(stamps[0].start, 1);
    ASSERT_EQ(stamps[0].end, (ClockTick) -1);

    if constexpr (IsTemplateOf<TypeParam, Complex_Number>::value || IsTemplateOf<TypeParam, RangeType>::value)
    {
        ASSERT_EQ(samples[0], TypeParam(typename TypeParam::Type(123.4)));
    }
    else
    {
        ASSERT_EQ(samples[0], TypeParam(123.4));
    }

    ASSERT_EQ(reader.getAvailableCount(), 0u);
}

TYPED_TEST(StreamReaderTest, ReadOneSampleWithRangesTimeout)
{
    this->signal.setDescriptor(setupDescriptor(SampleType::Float64));

    auto reader = daq::StreamReaderBuilder()
        .setSignal(this->signal)
        .setValueReadType(SampleTypeFromType<TypeParam>::SampleType)
        .setDomainReadType(SampleTypeFromType<ClockRange>::SampleType)
        .setSkipEvents(true)
        .build();

    auto domainPacket = DataPacket(setupDescriptor(SampleType::RangeInt64, LinearDataRule(1, 0), nullptr), 1, 1);
    auto dataPacket = DataPacketWithDomain(domainPacket, this->signal.getDescriptor(), 1);

    // Set the first sample to
    auto dataPtr = static_cast<double*>(dataPacket.getData());
    dataPtr[0] = 123.4;

    this->sendPacket(dataPacket);

    std::thread t([this, &dataPacket] {
        using namespace std::chrono_literals;

        std::this_thread::sleep_for(30ms);
        this->sendPacket(dataPacket);
    });

    SizeT count{2};
    TypeParam samples[2]{};
    ClockRange stamps[2]{};
    reader->readWithDomain(&samples, &stamps, &count, 1000u);

    if (t.joinable())
    {
        t.join();
    }

    ASSERT_EQ(count, 2u);

    ASSERT_EQ(stamps[0].start, 1);
    ASSERT_EQ(stamps[0].end, (ClockTick) -1);

    ASSERT_EQ(stamps[1].start, 1);
    ASSERT_EQ(stamps[1].end, (ClockTick) -1);

    if constexpr (IsTemplateOf<TypeParam, Complex_Number>::value || IsTemplateOf<TypeParam, RangeType>::value)
    {
        ASSERT_EQ(samples[0], TypeParam(typename TypeParam::Type(123.4)));
        ASSERT_EQ(samples[1], TypeParam(typename TypeParam::Type(123.4)));
    }
    else
    {
        ASSERT_EQ(samples[0], TypeParam(123.4));
        ASSERT_EQ(samples[1], TypeParam(123.4));
    }

    ASSERT_EQ(reader.getAvailableCount(), 0u);
}

TYPED_TEST(StreamReaderTest, ReadLessThanOnePacket)
{
    this->signal.setDescriptor(setupDescriptor(SampleType::Float64));

    auto reader = daq::StreamReaderBuilder()
        .setSignal(this->signal)
        .setValueReadType(SampleTypeFromType<TypeParam>::SampleType)
        .setDomainReadType(SampleTypeFromType<ClockRange>::SampleType)
        .setSkipEvents(true)
        .build();

    const SizeT NUM_SAMPLES = 2;
    auto dataPacket = DataPacket(this->signal.getDescriptor(), NUM_SAMPLES);

    // Set the first sample to
    auto dataPtr = static_cast<double*>(dataPacket.getData());
    dataPtr[0] = 123.4;
    dataPtr[1] = 432.1;

    this->sendPacket(dataPacket);

    SizeT count{1};
    TypeParam samples[1]{};
    reader.read(&samples, &count);

    ASSERT_EQ(count, 1u);

    if constexpr (IsTemplateOf<TypeParam, Complex_Number>::value || IsTemplateOf<TypeParam, RangeType>::value)
    {
        ASSERT_EQ(samples[0], TypeParam(typename TypeParam::Type(123.4)));
    }
    else
    {
        ASSERT_EQ(samples[0], TypeParam(123.4));
    }

    ASSERT_EQ(reader.getAvailableCount(), 1u);
}

TYPED_TEST(StreamReaderTest, ReadBetweenPackets)
{
    this->signal.setDescriptor(setupDescriptor(SampleType::Float64));

    auto reader = daq::StreamReaderBuilder()
        .setSignal(this->signal)
        .setValueReadType(SampleTypeFromType<TypeParam>::SampleType)
        .setDomainReadType(SampleTypeFromType<ClockRange>::SampleType)
        .setSkipEvents(true)
        .build();
    auto dataPacket = DataPacket(this->signal.getDescriptor(), 2);

    // Set the first sample to
    auto dataPtr = static_cast<double*>(dataPacket.getData());
    dataPtr[0] = 111.1;
    dataPtr[1] = 222.2;

    this->sendPacket(dataPacket);

    SizeT count{1};
    TypeParam samples[1]{};
    reader.read(&samples, &count);

    ASSERT_EQ(reader.getAvailableCount(), 1u);

    dataPtr[0] = 333.3;
    dataPtr[1] = 444.4;
    this->sendPacket(dataPacket);

    ASSERT_EQ(reader.getAvailableCount(), 3u);
}

TYPED_TEST(StreamReaderTest, ReadBetweenPacketsTimeout)
{
    this->signal.setDescriptor(setupDescriptor(SampleType::Float64));

    auto reader = daq::StreamReaderBuilder()
        .setSignal(this->signal)
        .setValueReadType(SampleTypeFromType<TypeParam>::SampleType)
        .setDomainReadType(SampleTypeFromType<ClockRange>::SampleType)
        .setSkipEvents(true)
        .build();
    auto dataPacket = DataPacket(this->signal.getDescriptor(), 2);

    // Set the first sample to
    auto dataPtr = static_cast<double*>(dataPacket.getData());
    dataPtr[0] = 11.1;
    dataPtr[1] = 22.2;

    this->sendPacket(dataPacket);

    SizeT count{1};
    TypeParam samples[1]{};
    reader.read(&samples, &count);

    ASSERT_EQ(reader.getAvailableCount(), 1u);

    std::thread t([this]
    {
        using namespace std::chrono_literals;

        auto data2Packet = DataPacket(this->signal.getDescriptor(), 2);

        // Set the first sample to
        auto data2Ptr = static_cast<double*>(data2Packet.getData());
        data2Ptr[0] = 33.3;
        data2Ptr[1] = 44.4;

        std::this_thread::sleep_for(30ms);
        this->sendPacket(data2Packet, false);
    });

    count = 4;
    TypeParam samples2[4]{};
    reader.read(&samples2, &count, 1000u);

    if (t.joinable())
        t.join();

    ASSERT_EQ(count, 3u);

    if constexpr (IsTemplateOf<TypeParam, Complex_Number>::value || IsTemplateOf<TypeParam, RangeType>::value)
    {
        ASSERT_EQ(samples2[0], TypeParam(typename TypeParam::Type(22.2)));
        ASSERT_EQ(samples2[1], TypeParam(typename TypeParam::Type(33.3)));
        ASSERT_EQ(samples2[2], TypeParam(typename TypeParam::Type(44.4)));
    }
    else
    {
        ASSERT_EQ(samples2[0], TypeParam(22.2));
        ASSERT_EQ(samples2[1], TypeParam(33.3));
        ASSERT_EQ(samples2[2], TypeParam(44.4));
    }

    ASSERT_EQ(reader.getAvailableCount(), 0u);
}

TYPED_TEST(StreamReaderTest, ReadBetweenPacketsAndCheckValues)
{
    this->signal.setDescriptor(setupDescriptor(SampleType::Float64));

    auto reader = daq::StreamReaderBuilder()
        .setSignal(this->signal)
        .setValueReadType(SampleTypeFromType<TypeParam>::SampleType)
        .setDomainReadType(SampleTypeFromType<ClockRange>::SampleType)
        .setSkipEvents(true)
        .build();
    auto dataPacket = DataPacket(this->signal.getDescriptor(), 2);

    // Set the first sample to
    auto dataPtr = static_cast<double*>(dataPacket.getData());
    dataPtr[0] = 11.1;
    dataPtr[1] = 22.2;

    this->sendPacket(dataPacket);

    SizeT count{1};
    TypeParam samples[1]{};
    reader.read(&samples, &count);

    ASSERT_EQ(reader.getAvailableCount(), 1u);

    auto nextDataPacket = DataPacket(this->signal.getDescriptor(), 2);
    auto nextDataPtr = static_cast<double*>(nextDataPacket.getData());
    nextDataPtr[0] = 33.3;
    nextDataPtr[1] = 44.4;
    this->sendPacket(nextDataPacket);

    ASSERT_EQ(reader.getAvailableCount(), 3u);

    count = 3;
    TypeParam nextSamples[3]{};
    reader.read(&nextSamples, &count);

    ASSERT_EQ(count, 3u);
    ASSERT_EQ(reader.getAvailableCount(), 0u);

    if constexpr (IsTemplateOf<TypeParam, Complex_Number>::value || IsTemplateOf<TypeParam, RangeType>::value)
    {
        ASSERT_EQ(nextSamples[0], TypeParam(typename TypeParam::Type(22.2)));
        ASSERT_EQ(nextSamples[1], TypeParam(typename TypeParam::Type(33.3)));
        ASSERT_EQ(nextSamples[2], TypeParam(typename TypeParam::Type(44.4)));
    }
    else
    {
        ASSERT_EQ(nextSamples[0], (TypeParam) 22.2);
        ASSERT_EQ(nextSamples[1], (TypeParam) 33.3);
        ASSERT_EQ(nextSamples[2], (TypeParam) 44.4);
    }
}

TYPED_TEST(StreamReaderTest, ReadValuesMoreThanAvailable)
{
    this->signal.setDescriptor(setupDescriptor(SampleType::Float64));

    auto reader = daq::StreamReaderBuilder()
            .setSignal(this->signal)
            .setValueReadType(SampleTypeFromType<TypeParam>::SampleType)
            .setDomainReadType(SampleTypeFromType<ClockRange>::SampleType)
            .setSkipEvents(true)
            .build();

    const SizeT NUM_SAMPLES = 2;
    this->sendPacket(DataPacket(this->signal.getDescriptor(), NUM_SAMPLES));

    SizeT count{3};
    TypeParam samples[3]{};
    reader.read(&samples, &count);

    ASSERT_EQ(count, 2u);
    ASSERT_EQ(reader.getAvailableCount(), 0u);
}

TYPED_TEST(StreamReaderTest, ReadConstantRule)
{
    if constexpr (!std::is_same_v<TypeParam, Complex_Number<float>> && !std::is_same_v<TypeParam, Complex_Number<double>>)
    {
        constexpr size_t samplesInPacket = 2;

        const auto domainDesc = setupDescriptor(SampleType::Int64, LinearDataRule(1, 0), nullptr);

        this->signal.setDescriptor(setupDescriptor(SampleTypeFromType<TypeParam>::SampleType, ConstantDataRule()));
        auto reader = daq::StreamReaderBuilder()
            .setSignal(this->signal)
            .setValueReadType(SampleTypeFromType<TypeParam>::SampleType)
            .setDomainReadType(SampleTypeFromType<ClockTick>::SampleType)
            .setSkipEvents(true)
            .build();

        auto domainPacket = DataPacket(domainDesc, samplesInPacket, 0);
        auto dataPacket = ConstantDataPacketWithDomain<TypeParam>(domainPacket, this->signal.getDescriptor(), samplesInPacket, 12);
        this->sendPacket(dataPacket);

        domainPacket = DataPacket(domainDesc, samplesInPacket, 2);
        dataPacket = ConstantDataPacketWithDomain<TypeParam>(domainPacket, this->signal.getDescriptor(), samplesInPacket, 24);
        this->sendPacket(dataPacket);

        SizeT count{samplesInPacket * 2};
        TypeParam samples[samplesInPacket * 2]{};
        ClockTick ticks[samplesInPacket * 2]{};
        reader.readWithDomain(&samples, &ticks, &count);
        ASSERT_EQ(count, samplesInPacket * 2);

        ASSERT_THAT(ticks, ElementsAre(0, 1, 2, 3));
        ASSERT_THAT(samples, ElementsAre(12, 12, 24, 24));
    }
}

TYPED_TEST(StreamReaderTest, DescriptorChangedConvertible)
{
    this->signal.setDescriptor(setupDescriptor(SampleType::Float64));

    auto reader = daq::StreamReaderBuilder()
        .setSignal(this->signal)
        .setValueReadType(SampleTypeFromType<TypeParam>::SampleType)
        .setDomainReadType(SampleTypeFromType<ClockRange>::SampleType)
        .setSkipEvents(true)
        .build();

    const SizeT NUM_SAMPLES = 2;
    auto dataPacketDouble = DataPacket(this->signal.getDescriptor(), NUM_SAMPLES);
    auto dataPtrDouble = static_cast<double*>(dataPacketDouble.getData());
    dataPtrDouble[0] = 111.1;
    dataPtrDouble[1] = 222.2;

    this->sendPacket(dataPacketDouble);

    ASSERT_EQ(reader.getAvailableCount(), 2u);

    SizeT count{2};
    TypeParam samplesDouble[2]{};
    reader.read(&samplesDouble, &count);

    ASSERT_EQ(reader.getAvailableCount(), 0u);

    // Change signal sample-type
    this->signal.setDescriptor(setupDescriptor(SampleType::Int32));
    auto dataPacketInt32 = DataPacket(this->signal.getDescriptor(), NUM_SAMPLES);
    auto dataPtrInt32 = static_cast<int32_t*>(dataPacketInt32.getData());
    dataPtrInt32[0] = 3;
    dataPtrInt32[1] = 4;

    this->sendPacket(dataPacketInt32);

    ASSERT_EQ(reader.getAvailableCount(), 2u);

    count = 2;
    TypeParam sampleInt32[2]{};
    reader.read(&sampleInt32, &count);

    ASSERT_EQ(reader.getAvailableCount(), 0u);

    ASSERT_EQ(sampleInt32[0], TypeParam(3));
    ASSERT_EQ(sampleInt32[1], TypeParam(4));
}

TYPED_TEST(StreamReaderTest, GapDetected)
{
    this->signal.setDescriptor(setupDescriptor(SampleType::Float64));
    const auto domainSignal = daq::Signal(this->context, nullptr, "domainSig");
    const auto domainDesc = setupDescriptor(SampleType::Int64, LinearDataRule(1, 0), nullptr);
    domainSignal.setDescriptor(domainDesc);
    this->signal.setDomainSignal(domainSignal);

    auto reader = daq::StreamReaderBuilder()
        .setSignal(this->signal)
        .setValueReadType(SampleTypeFromType<TypeParam>::SampleType)
        .setDomainReadType(SampleTypeFromType<int64_t>::SampleType)
        .setSkipEvents(true)
        .build();

    constexpr SizeT NUM_SAMPLES = 2;

    auto domainPacket = DataPacket(domainDesc, NUM_SAMPLES, 0);
    auto dataPacketDouble = DataPacketWithDomain(domainPacket, this->signal.getDescriptor(), NUM_SAMPLES);
    auto dataPtrDouble = static_cast<double*>(dataPacketDouble.getData());
    dataPtrDouble[0] = 1.0;
    dataPtrDouble[1] = 2.0;

    this->sendPacket(dataPacketDouble);

    domainPacket = DataPacket(domainDesc, NUM_SAMPLES, 8);
    dataPacketDouble = DataPacketWithDomain(domainPacket, this->signal.getDescriptor(), NUM_SAMPLES);
    dataPtrDouble = static_cast<double*>(dataPacketDouble.getData());
    dataPtrDouble[0] = 8.0;
    dataPtrDouble[1] = 9.0;

    this->sendPacket(dataPacketDouble);

    ASSERT_EQ(reader.getAvailableCount(), 2u);

    SizeT count{4};
    TypeParam samplesDouble[4]{};
    int64_t domainSamples[4]{};

    auto status = reader.readWithDomain(&samplesDouble, &domainSamples, &count);
    ASSERT_EQ(count, 2u);
    ASSERT_THAT(samplesDouble, ElementsAre(static_cast<TypeParam>(1), static_cast<TypeParam>(2), _, _));
    ASSERT_THAT(domainSamples, ElementsAre(0, 1, _, _));
    ASSERT_EQ(reader.getAvailableCount(), 2u);

    ASSERT_TRUE(status.getValid());
    ASSERT_EQ(status.getReadStatus(), ReadStatus::Event);
    ASSERT_TRUE(status.getEventPacket().assigned());

    ASSERT_EQ(status.getEventPacket().getEventId(), event_packet_id::IMPLICIT_DOMAIN_GAP_DETECTED);
    ASSERT_EQ(status.getEventPacket().getParameters().get(event_packet_param::GAP_DIFF), 6);

    count = 4;
    status = reader.readWithDomain(reinterpret_cast<TypeParam*>(&samplesDouble), reinterpret_cast<int64_t*>(&domainSamples), &count);
    ASSERT_EQ(count, 2u);
    ASSERT_THAT(samplesDouble, ElementsAre(static_cast<TypeParam>(8), static_cast<TypeParam>(9), _, _));
    ASSERT_THAT(domainSamples, ElementsAre(8, 9, _, _));
    ASSERT_EQ(reader.getAvailableCount(), 0u);

    ASSERT_TRUE(status.getValid());
    ASSERT_EQ(status.getReadStatus(), ReadStatus::Ok);
    ASSERT_FALSE(status.getEventPacket().assigned());
}

TYPED_TEST(StreamReaderTest, DescriptorChangedNotConvertible)
{
    this->signal.setDescriptor(setupDescriptor(SampleType::Int32));

    auto reader = daq::StreamReaderBuilder()
        .setSignal(this->signal)
        .setValueReadType(SampleTypeFromType<int32_t>::SampleType)
        .setDomainReadType(SampleTypeFromType<ClockRange>::SampleType)
        .setSkipEvents(true)
        .build();


    this->signal.setDescriptor(setupDescriptor(SampleType::ComplexFloat32));

    const SizeT NUM_SAMPLES = 2;
    auto dataPacket = DataPacket(this->signal.getDescriptor(), NUM_SAMPLES);
    auto dataPtr = static_cast<ComplexFloat32*>(dataPacket.getData());
    dataPtr[0] = 111;
    dataPtr[1] = 222;

    this->sendPacket(dataPacket);

    SizeT count{0};
    auto status = reader.read(nullptr, &count);
    ASSERT_EQ(status.getReadStatus(), ReadStatus::Event);
    ASSERT_FALSE(status.getValid());
}

TYPED_TEST(StreamReaderTest, ReadWithZeroAvailableAndTimeoutAny)
{
    const uint64_t FIRST_PACKET_SIZE = 2u;
    const uint64_t SECOND_PACKET_SIZE = 4u;

    this->signal.setDescriptor(setupDescriptor(SampleType::Float64));

    auto reader = daq::StreamReaderBuilder()
        .setSignal(this->signal)
        .setValueReadType(SampleTypeFromType<TypeParam>::SampleType)
        .setDomainReadType(SampleTypeFromType<ClockRange>::SampleType)
        .setReadTimeoutType(ReadTimeoutType::Any)
        .setSkipEvents(true)
        .build();


    ASSERT_EQ(reader.getAvailableCount(), 0u);

    std::thread t([this, &FIRST_PACKET_SIZE, &SECOND_PACKET_SIZE] {
        using namespace std::chrono_literals;

        auto packet = DataPacket(this->signal.getDescriptor(), FIRST_PACKET_SIZE);

        // Set the first sample to
        auto dataPtr = static_cast<double*>(packet.getData());
        dataPtr[0] = 11.1;
        dataPtr[1] = 22.2;

        std::this_thread::sleep_for(30ms);
        this->sendPacket(packet, false);

        // Packet 2

        packet = DataPacket(this->signal.getDescriptor(), SECOND_PACKET_SIZE);

        // Set the first sample to
        dataPtr = static_cast<double*>(packet.getData());
        dataPtr[0] = 33.3;
        dataPtr[1] = 44.4;
        dataPtr[2] = 55.5;
        dataPtr[3] = 66.6;

        std::this_thread::sleep_for(10ms);
        this->sendPacket(packet, false);
    });

    SizeT count{6};
    TypeParam samples[6]{};
    reader.read(&samples, &count, 1000u);
    
    if (t.joinable())
        t.join();

    ASSERT_EQ(count, FIRST_PACKET_SIZE);
    ASSERT_EQ(reader.getAvailableCount(), SECOND_PACKET_SIZE);

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

TYPED_TEST(StreamReaderTest, ReuseReader)
{
    this->signal.setDescriptor(setupDescriptor(SampleType::Int32));

    auto reader = daq::StreamReaderBuilder()
        .setSignal(this->signal)
        .setValueReadType(SampleTypeFromType<TypeParam>::SampleType)
        .setDomainReadType(SampleTypeFromType<ClockRange>::SampleType)
        .setSkipEvents(true)
        .build();
    this->signal.setDescriptor(setupDescriptor(SampleType::ComplexFloat32));

    const SizeT NUM_SAMPLES = 2;
    auto dataPacket = DataPacket(this->signal.getDescriptor(), NUM_SAMPLES);
    auto dataPtr = static_cast<ComplexFloat32*>(dataPacket.getData());
    dataPtr[0] = 111;
    dataPtr[1] = 222;

    this->sendPacket(dataPacket);

    SizeT count{0};
    auto status = reader.read(nullptr, &count);

    bool convertable = IsTemplateOf<TypeParam, Complex_Number>::value;
    ASSERT_EQ(status.getValid(), convertable);

    auto newReader = daq::StreamReaderFromExisting<ComplexFloat32, ClockRange>(reader);

    SizeT complexCount{1};
    ComplexFloat32 complexSamples[1];
    status = newReader.read(&complexSamples, &complexCount);
    ASSERT_EQ(status.getReadStatus(), ReadStatus::Ok);

    ASSERT_EQ(complexCount, 1u);

    ComplexFloat32 expectedSample = 111;
    ASSERT_EQ(complexSamples[0], expectedSample);
}

TYPED_TEST(StreamReaderTest, ToString)
{
    auto reader = daq::StreamReader<TypeParam, ClockRange>(this->signal);

    CharPtr str;
    ASSERT_EQ(reader->toString(&str), OPENDAQ_SUCCESS);

    ASSERT_STREQ(str, "daq::IStreamReader");
    daqFreeMemory(str);
}

TYPED_TEST(StreamReaderTest, ReadUndefinedNoDomain)
{
    this->signal.setDescriptor(setupDescriptor(SampleType::Float64));

    auto reader = daq::StreamReader(this->signal, SampleType::Undefined, SampleType::Undefined);

    {
        SizeT tmpCount = 0u;
        reader.read(nullptr, &tmpCount);
    }

    ASSERT_EQ(reader.getValueReadType(), SampleType::Float64);
    ASSERT_EQ(reader.getDomainReadType(), SampleType::Invalid);
}

TYPED_TEST(StreamReaderTest, ReadUndefinedWithDomain)
{
    this->signal.setDescriptor(setupDescriptor(SampleType::Float64));

    auto reader = daq::StreamReaderBuilder()
        .setSignal(this->signal)
        .setValueReadType(SampleType::Undefined)
        .setDomainReadType(SampleType::Undefined)
        .build();

    {
        SizeT tmpCount{0};
        auto status = reader.read(nullptr, &tmpCount);
        ASSERT_EQ(status.getReadStatus(), ReadStatus::Event);
    }

    ASSERT_EQ(reader.getValueReadType(), SampleType::Float64); // read from signal descriptor
    ASSERT_EQ(reader.getDomainReadType(), SampleType::Invalid);

    auto domainDescriptor = setupDescriptor(SampleType::RangeInt64, LinearDataRule(1, 0), nullptr);
    auto eventPacket = DataDescriptorChangedEventPacket(nullptr, domainDescriptor);
    this->sendPacket(eventPacket);

    auto domainPacket = DataPacket(domainDescriptor, 1, 1);
    auto dataPacket = DataPacketWithDomain(domainPacket, this->signal.getDescriptor(), 1);
    auto dataPtr = static_cast<double*>(dataPacket.getData());
    dataPtr[0] = 123.4;

    this->sendPacket(dataPacket);

    {
        SizeT tmpCount{0};
        auto status = reader.read(nullptr, &tmpCount);
        ASSERT_EQ(status.getReadStatus(), ReadStatus::Event);
    }

    SizeT count{1};
    double samples[1]{};

    reader.read(&samples, &count);

    ASSERT_EQ(count, 1u);

    ASSERT_EQ(reader.getValueReadType(), SampleType::Float64);
    // domain info available from descriptor change event
    ASSERT_EQ(reader.getDomainReadType(), SampleType::RangeInt64);
}

TYPED_TEST(StreamReaderTest, ReadUndefinedWithNoDomainFromPacket)
{
    this->signal.setDescriptor(setupDescriptor(SampleType::Float64));

    auto reader = daq::StreamReader(this->signal, SampleType::Undefined, SampleType::Undefined);

    {
        SizeT tmpCount = 0u;
        reader.read(nullptr, &tmpCount);
    }

    ASSERT_EQ(reader.getValueReadType(), SampleType::Float64); // read from signal descriptor
    ASSERT_EQ(reader.getDomainReadType(), SampleType::Invalid);

    auto domainPacket = DataPacket(setupDescriptor(SampleType::RangeInt64, LinearDataRule(1, 0), nullptr), 1, 1);
    auto dataPacket = DataPacketWithDomain(domainPacket, this->signal.getDescriptor(), 1);
    auto dataPtr = static_cast<double*>(dataPacket.getData());
    dataPtr[0] = 123.4;

    this->sendPacket(dataPacket);

    SizeT count{1};
    double samples[1]{};
    reader.read(&samples, &count);

    ASSERT_EQ(count, 1u);
    ASSERT_EQ(reader.getValueReadType(), SampleType::Float64);

    // domain was not read so its info was not updated
    ASSERT_EQ(reader.getDomainReadType(), SampleType::Invalid);
}

TYPED_TEST(StreamReaderTest, ReadUndefinedWithWithDomainFromPacket)
{
    this->signal.setDescriptor(setupDescriptor(SampleType::Float64));

    auto reader = daq::StreamReaderBuilder()
        .setSignal(this->signal)
        .setValueReadType(SampleType::Invalid)
        .setDomainReadType(SampleType::Invalid)
        .build();

    {
        SizeT tmpCount{0};
        auto status = reader.read(nullptr, &tmpCount);
        ASSERT_EQ(status.getReadStatus(), ReadStatus::Event);
    }

    auto domainPacket = DataPacket(setupDescriptor(SampleType::RangeInt64, LinearDataRule(1, 0), nullptr), 1, 1);
    auto dataPacket = DataPacketWithDomain(domainPacket, this->signal.getDescriptor(), 1);
    auto dataPtr = static_cast<double*>(dataPacket.getData());
    dataPtr[0] = 123.4;

    this->sendPacket(dataPacket);

    SizeT count{1};
    double samples[1]{};
    RangeType64 domain[1]{};
    reader.readWithDomain(&samples, &domain, &count);

    ASSERT_EQ(count, 1u);
    ASSERT_EQ(reader.getValueReadType(), SampleType::Float64);

    // domain was read and updated from packet info
    ASSERT_EQ(reader.getDomainReadType(), SampleType::RangeInt64);
}

TYPED_TEST(StreamReaderTest, ReadVoid)
{
    this->signal.setDescriptor(setupDescriptor(SampleTypeFromType<TypeParam>::SampleType));

    auto reader = daq::StreamReaderBuilder()
        .setSignal(this->signal)
        .setValueReadType(SampleTypeFromType<void*>::SampleType)
        .setDomainReadType(SampleTypeFromType<ClockRange>::SampleType)
        .setSkipEvents(true)
        .build();

    ASSERT_EQ(reader.getValueReadType(), SampleType::Struct);

    auto dataPacket = DataPacket(this->signal.getDescriptor(), 1);

    // Set the first sample to
    const auto dataPtr = static_cast<TypeParam*>(dataPacket.getData());
    dataPtr[0] = static_cast<TypeParam>(123);

    this->sendPacket(dataPacket);

    SizeT count{1};
    TypeParam samples[1]{};
    reader.read(&samples, &count);

    ASSERT_EQ(count, 1u);

    if constexpr (IsTemplateOf<TypeParam, Complex_Number>::value || IsTemplateOf<TypeParam, RangeType>::value)
    {
        ASSERT_EQ(samples[0], TypeParam(typename TypeParam::Type(123)));
    }
    else
    {
        ASSERT_EQ(samples[0], TypeParam(123));
    }

    ASSERT_EQ(reader.getAvailableCount(), 0u);
}

class StructStreamReaderTest : public testing::Test
{
protected:
    SignalConfigPtr valueSignal;
    SignalConfigPtr domainSignal;
    DataDescriptorPtr domainDescriptor;
    DataDescriptorPtr canMsgDescriptor;

    void SetUp() override
    {
        const auto context = NullContext();
        valueSignal = daq::Signal(context, nullptr, "valuesig");
        domainSignal = daq::Signal(context, nullptr, "domainsig");
        valueSignal.setDomainSignal(domainSignal);

        const auto arbIdDescriptor =
            DataDescriptorBuilder().setName("ArbId").setSampleType(SampleType::UInt32).setRule(ExplicitDataRule()).build();

        const auto lengthDescriptor =
            DataDescriptorBuilder().setName("Length").setSampleType(SampleType::UInt8).setRule(ExplicitDataRule()).build();

        const auto dataDescriptor = DataDescriptorBuilder()
                                    .setName("Data")
                                    .setSampleType(SampleType::UInt8)
                                    .setDimensions(List<IDimension>(DimensionBuilder().setRule(LinearDimensionRule(0, 1, 64)).build()))
                                    .setRule(ExplicitDataRule())
                                    .build();

        canMsgDescriptor = DataDescriptorBuilder()
                           .setName("CAN")
                           .setSampleType(SampleType::Struct)
                           .setStructFields(List<IDataDescriptor>(arbIdDescriptor, lengthDescriptor, dataDescriptor))
                           .build();

        domainDescriptor = DataDescriptorBuilder()
                           .setSampleType(SampleType::UInt64)
                           .setRule(LinearDataRule(1, 0))
                           .setTickResolution(Ratio(1, 1000))
                           .build();

        valueSignal.setDescriptor(canMsgDescriptor);
        domainSignal.setDescriptor(domainDescriptor);
    }
};

#pragma pack(push, 1)
struct CANData
{
    uint32_t arbId;
    uint8_t length;
    uint8_t data[64];
};
#pragma pack(pop)

TEST_F(StructStreamReaderTest, ReadStructData)
{
    const auto streamReader = StreamReader<void*>(valueSignal);

    const auto domainPacket1 = DataPacket(domainDescriptor, 2, 0);
    const auto valuePacket1 = DataPacketWithDomain(domainPacket1, canMsgDescriptor, 2);
    auto* canData = static_cast<CANData*>(valuePacket1.getRawData());

    canData->arbId = 12;
    canData->length = 2;
    canData->data[0] = 1;
    canData->data[1] = 2;
    canData++;
    canData->arbId = 15;
    canData->length = 4;
    canData->data[0] = 5;
    canData->data[1] = 6;
    canData->data[2] = 7;
    canData->data[3] = 8;

    valueSignal.sendPacket(valuePacket1);
    domainSignal.sendPacket(domainPacket1);

    const auto domainPacket2 = DataPacket(domainDescriptor, 1, 2);
    const auto valuePacket2 = DataPacketWithDomain(domainPacket2, canMsgDescriptor, 1);
    canData = static_cast<CANData*>(valuePacket2.getRawData());

    canData->arbId = 14;
    canData->length = 3;
    canData->data[0] = 10;
    canData->data[1] = 11;
    canData->data[2] = 12;

    valueSignal.sendPacket(valuePacket2);
    domainSignal.sendPacket(domainPacket2);

    CANData dataRead[3];

    SizeT count = 0;
    do
    {
        SizeT toRead = 3;
        streamReader.read(&dataRead[count], &toRead, 10);
        count += toRead;
    } while (count < 3);

    ASSERT_EQ(count, 3u);

    ASSERT_EQ(dataRead[0].arbId, (uint32_t) 12);
    ASSERT_EQ(dataRead[0].length, (uint8_t) 2);
    ASSERT_EQ(dataRead[0].data[0], (uint8_t) 1);
    ASSERT_EQ(dataRead[0].data[1], (uint8_t) 2);
    ASSERT_EQ(dataRead[1].arbId, (uint32_t) 15);
    ASSERT_EQ(dataRead[1].length, (uint8_t) 4);
    ASSERT_EQ(dataRead[1].data[0], (uint8_t) 5);
    ASSERT_EQ(dataRead[1].data[1], (uint8_t) 6);
    ASSERT_EQ(dataRead[1].data[2], (uint8_t) 7);
    ASSERT_EQ(dataRead[1].data[3], (uint8_t) 8);
    ASSERT_EQ(dataRead[2].arbId, (uint32_t) 14);
    ASSERT_EQ(dataRead[2].length, (uint8_t) 3);
    ASSERT_EQ(dataRead[2].data[0], (uint8_t) 10);
    ASSERT_EQ(dataRead[2].data[1], (uint8_t) 11);
    ASSERT_EQ(dataRead[2].data[2], (uint8_t) 12);
}

TEST_F(StructStreamReaderTest, ReadStructDataInvalid)
{
    const auto streamReader = StreamReader<double>(valueSignal);

    double dataRead[1];
    SizeT count = 1;
    auto status = streamReader.read(&dataRead[0], &count, 0);
    ASSERT_FALSE(status.getValid());
}

TEST_F(StructStreamReaderTest, ReadStructDataWithDomain)
{
    const auto streamReader = StreamReader<void*>(valueSignal);
    const auto domainDescriptor = DataDescriptorBuilder().setSampleType(SampleType::UInt64).setTickResolution(Ratio(1, 1000)).build();
    domainSignal.setDescriptor(domainDescriptor);

    const auto domainPacket1 = DataPacket(domainDescriptor, 2, 0);
    const auto valuePacket1 = DataPacketWithDomain(domainPacket1, canMsgDescriptor, 2);
    auto* canData = static_cast<CANData*>(valuePacket1.getRawData());
    auto* timeData = static_cast<uint64_t*>(domainPacket1.getRawData());

    canData->arbId = 12;
    canData->length = 2;
    canData->data[0] = 1;
    canData++->data[1] = 2;
    *timeData++ = 0;
    canData->arbId = 15;
    canData->length = 4;
    canData->data[0] = 5;
    canData->data[1] = 6;
    canData->data[2] = 7;
    canData->data[3] = 8;
    *timeData++ = 3;

    valueSignal.sendPacket(valuePacket1);
    domainSignal.sendPacket(domainPacket1);

    const auto domainPacket2 = DataPacket(domainDescriptor, 1, 5);
    const auto valuePacket2 = DataPacketWithDomain(domainPacket2, canMsgDescriptor, 1);
    canData = static_cast<CANData*>(valuePacket2.getRawData());
    timeData = static_cast<uint64_t*>(domainPacket2.getRawData());

    canData->arbId = 14;
    canData->length = 3;
    canData->data[0] = 10;
    canData->data[1] = 11;
    canData->data[2] = 12;
    *timeData = 5;

    valueSignal.sendPacket(valuePacket2);
    domainSignal.sendPacket(domainPacket2);

    CANData dataRead[3];
    uint64_t timeRead[3];

    SizeT count = 0;
    do
    {
        SizeT toRead = 3;
        streamReader.readWithDomain(&dataRead[count], &timeRead[count], &toRead, 10);
        count += toRead;
    } while (count < 3);

    ASSERT_EQ(count, 3u);

    ASSERT_EQ(dataRead[0].arbId, (uint32_t)12);
    ASSERT_EQ(dataRead[0].length, (uint8_t)2);
    ASSERT_EQ(dataRead[0].data[0], (uint8_t)1);
    ASSERT_EQ(dataRead[0].data[1], (uint8_t)2);
    ASSERT_EQ(timeRead[0], (uint64_t)0);
    ASSERT_EQ(dataRead[1].arbId, (uint32_t)15);
    ASSERT_EQ(dataRead[1].length, (uint8_t)4);
    ASSERT_EQ(dataRead[1].data[0], (uint8_t)5);
    ASSERT_EQ(dataRead[1].data[1], (uint8_t)6);
    ASSERT_EQ(dataRead[1].data[2], (uint8_t)7);
    ASSERT_EQ(dataRead[1].data[3], (uint8_t)8);
    ASSERT_EQ(timeRead[1], (uint64_t)3);
    ASSERT_EQ(dataRead[2].arbId, (uint32_t)14);
    ASSERT_EQ(dataRead[2].length, (uint8_t)3);
    ASSERT_EQ(dataRead[2].data[0], (uint8_t)10);
    ASSERT_EQ(dataRead[2].data[1], (uint8_t)11);
    ASSERT_EQ(dataRead[2].data[2], (uint8_t)12);
    ASSERT_EQ(timeRead[2], (uint64_t)5);
}

TYPED_TEST(StreamReaderTest, StreamReaderWithInputPort)
{
    this->signal.setDescriptor(setupDescriptor(SampleType::Float64));
    auto port = InputPort(this->signal.getContext(), nullptr, "readsig");
   
    auto reader = daq::StreamReaderFromPort(port, SampleType::Undefined, SampleType::Undefined);
    port.connect(this->signal);

    auto domainPacket = DataPacket(setupDescriptor(SampleType::RangeInt64, LinearDataRule(1, 0), nullptr), 1, 1);
    auto dataPacket = DataPacketWithDomain(domainPacket, this->signal.getDescriptor(), 1);
    auto dataPtr = static_cast<double*>(dataPacket.getData());
    dataPtr[0] = 123.4;

    this->sendPacket(dataPacket);

    SizeT count{0};
    auto status = reader.read(nullptr, &count);
    ASSERT_EQ(status.getReadStatus(), ReadStatus::Event);

    count = 1;
    double samples[1]{};
    RangeType64 domain[1]{};
    reader.readWithDomain(&samples, &domain, &count);

    ASSERT_EQ(count, 1u);
    ASSERT_EQ(reader.getValueReadType(), SampleType::Float64);

    // domain was read and updated from packet info
    ASSERT_EQ(reader.getDomainReadType(), SampleType::RangeInt64);
}

TYPED_TEST(StreamReaderTest, StreamReaderWithNotConnectedInputPort)
{
    this->signal.setDescriptor(setupDescriptor(SampleType::Float64));
    auto port = InputPort(this->signal.getContext(), nullptr, "readsig");

    auto reader = daq::StreamReaderFromPort(port, SampleType::Undefined, SampleType::Undefined);

    auto domainPacket = DataPacket(setupDescriptor(SampleType::RangeInt64, LinearDataRule(1, 0), nullptr), 1, 1);
    auto dataPacket = DataPacketWithDomain(domainPacket, this->signal.getDescriptor(), 1);
    auto dataPtr = static_cast<double*>(dataPacket.getData());
    dataPtr[0] = 123.4;

    port.connect(this->signal);

    this->sendPacket(dataPacket);

    SizeT count{1};
    double samples[1]{};
    RangeType64 domain[1]{};

    {
        size_t tempCnt = 0u;
        auto status = reader.read(nullptr, &tempCnt);
        ASSERT_EQ(status.getReadStatus(), ReadStatus::Event);
    }

    reader.readWithDomain(&samples, &domain, &count);

    ASSERT_EQ(count, 1u);
    ASSERT_EQ(reader.getValueReadType(), SampleType::Float64);

    // domain was read and updated from packet info
    ASSERT_EQ(reader.getDomainReadType(), SampleType::RangeInt64);
}

TYPED_TEST(StreamReaderTest, MultipleStreamReaderToInputPort)
{
    this->signal.setDescriptor(setupDescriptor(SampleType::Float64));
    auto port = InputPort(this->signal.getContext(), nullptr, "readsig");

    auto reader1 = daq::StreamReaderFromPort(port, SampleType::Undefined, SampleType::Undefined);
    ASSERT_THROW(daq::StreamReaderFromPort(port, SampleType::Undefined, SampleType::Undefined), AlreadyExistsException);
}

TYPED_TEST(StreamReaderTest, StreamReaderReuseInputPort)
{
    this->signal.setDescriptor(setupDescriptor(SampleType::Float64));
    auto port = InputPort(this->signal.getContext(), nullptr, "readsig");

    {
        auto reader1 = daq::StreamReaderFromPort(port, SampleType::Undefined, SampleType::Undefined);
    }
    ASSERT_NO_THROW(daq::StreamReaderFromPort(port, SampleType::Undefined, SampleType::Undefined));
}

TYPED_TEST(StreamReaderTest, StreamReaderOnReadCallback)
{
    SizeT count{1};
    double samples[1]{};
    RangeType64 domain[1]{};

    std::promise<void> promise;
    std::future<void> future = promise.get_future();

    this->signal.setDescriptor(setupDescriptor(SampleType::Float64));

    auto reader = daq::StreamReader(this->signal, SampleType::Undefined, SampleType::Undefined);
    
    {
        SizeT tmpCount = 0u;
        auto status = reader.read(nullptr, &tmpCount);
        ASSERT_EQ(status.getReadStatus(), ReadStatus::Event);
    }

    reader.setOnDataAvailable([&] {
        reader.readWithDomain(&samples, &domain, &count);
        promise.set_value();
    });

    ASSERT_EQ(reader.getValueReadType(), SampleType::Float64); // read from signal descriptor
    ASSERT_EQ(reader.getDomainReadType(), SampleType::Invalid);

    auto domainPacket = DataPacket(setupDescriptor(SampleType::RangeInt64, LinearDataRule(1, 0), nullptr), 1, 1);
    auto dataPacket = DataPacketWithDomain(domainPacket, this->signal.getDescriptor(), 1);
    auto dataPtr = static_cast<double*>(dataPacket.getData());
    dataPtr[0] = 123.4;

    this->sendPacket(dataPacket);

    auto promiseStatus = future.wait_for(std::chrono::seconds(1));
    ASSERT_EQ(promiseStatus, std::future_status::ready);

    ASSERT_EQ(count, 1u);
    ASSERT_EQ(reader.getValueReadType(), SampleType::Float64);

    // domain was read and updated from packet info
    ASSERT_EQ(reader.getDomainReadType(), SampleType::RangeInt64);
}

TYPED_TEST(StreamReaderTest, StreamReaderFromPortOnReadCallback)
{
    SizeT count{1};
    double samples[1]{};
    RangeType64 domain[1]{};

    std::promise<void> promise;
    std::future<void> future = promise.get_future();

    this->signal.setDescriptor(setupDescriptor(SampleType::Float64));
    auto port = InputPort(this->signal.getContext(), nullptr, "readsig");

    auto reader = daq::StreamReaderFromPort(port, SampleType::Undefined, SampleType::Undefined);
    port.connect(this->signal);
    reader.setOnDataAvailable([&] {
        auto tmpCount = count;
        while (reader.readWithDomain(&samples, &domain, &count).getReadStatus() == ReadStatus::Event)
        {
            count = tmpCount;
        }
        promise.set_value();
    });

    auto domainPacket = DataPacket(setupDescriptor(SampleType::RangeInt64, LinearDataRule(1, 0), nullptr), 1, 1);
    auto dataPacket = DataPacketWithDomain(domainPacket, this->signal.getDescriptor(), 1);
    auto dataPtr = static_cast<double*>(dataPacket.getData());
    dataPtr[0] = 123.4;

    this->sendPacket(dataPacket);

    auto promiseStatus = future.wait_for(std::chrono::seconds(1));
    ASSERT_EQ(promiseStatus, std::future_status::ready);

    ASSERT_EQ(count, 1u);
    ASSERT_EQ(reader.getValueReadType(), SampleType::Float64);

    // domain was read and updated from packet info
    ASSERT_EQ(reader.getDomainReadType(), SampleType::RangeInt64);
}

TYPED_TEST(StreamReaderTest, StreamReaderFromExistingOnReadCallback)
{
    SizeT count{1};
    double samples[1]{};
    RangeType64 domain[1]{};

    std::promise<void> promise;
    std::future<void> future = promise.get_future();

    this->signal.setDescriptor(setupDescriptor(SampleType::Int64));

    StreamReaderPtr reader = daq::StreamReader(this->signal, SampleType::Undefined, SampleType::Undefined);
    StreamReaderPtr newReader;
    {
        SizeT tmpCount = 0u;
        auto status = reader.read(nullptr, &tmpCount);
        ASSERT_EQ(status.getReadStatus(), ReadStatus::Event);
    }
    
    reader.setOnDataAvailable([&] {
        if (!newReader.assigned())
        {
            SizeT tmpCount = 0u;
            auto status = reader.read(nullptr, &tmpCount);
            if (status.getReadStatus() == ReadStatus::Event)
            {
                newReader = daq::StreamReaderFromExisting(reader, SampleType::Undefined, SampleType::Undefined);
            }
        }
        else
        {
            newReader.readWithDomain(&samples, &domain, &count);
            promise.set_value();
        }
    });

    this->signal.setDescriptor(setupDescriptor(SampleType::Float64));

    auto domainPacket = DataPacket(setupDescriptor(SampleType::RangeInt64, LinearDataRule(1, 0), nullptr), 1, 1);
    auto dataPacket = DataPacketWithDomain(domainPacket, this->signal.getDescriptor(), 1);
    auto dataPtr = static_cast<double*>(dataPacket.getData());
    dataPtr[0] = 123.4;

    this->sendPacket(dataPacket);

    auto promiseStatus = future.wait_for(std::chrono::seconds(1));
    ASSERT_EQ(promiseStatus, std::future_status::ready);

    ASSERT_EQ(count, 1u);
}

TYPED_TEST(StreamReaderTest, ReadingNotConnectedPort)
{
    auto port = InputPort(this->signal.getContext(), nullptr, "readsig");
    auto reader = daq::StreamReaderFromPort(port, SampleType::Float64, SampleType::RangeInt64);

    auto availableCount = reader.getAvailableCount();
    ASSERT_EQ(availableCount, 0u);

    SizeT count{1};
    double samples[1]{};
    auto status = reader.read(&samples, &count);
    ASSERT_EQ(count, 0u);
    ASSERT_EQ(status.getReadStatus(), ReadStatus::Ok);

    // read with timeout
    count = 1;
    status = reader.read(&samples, &count, 100u);
    ASSERT_EQ(count, 0u);
    ASSERT_EQ(status.getReadStatus(), ReadStatus::Ok);

    // connecting port
    port.connect(this->signal);

    // check that event is encountered
    count = 1;
    status = reader.read(&samples, &count);
    ASSERT_EQ(count, 0u);
    ASSERT_EQ(status.getReadStatus(), ReadStatus::Event);
}

TYPED_TEST(StreamReaderTest, NotifyPortIsConnected)
{
    auto port = InputPort(this->signal.getContext(), nullptr, "readsig");
    auto reader = daq::StreamReaderFromPort(port, SampleType::Float64, SampleType::RangeInt64);

    ReaderStatusPtr status;
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

TYPED_TEST(StreamReaderTest, ReadWhilePortIsNotConnected)
{
    auto port = InputPort(this->signal.getContext(), nullptr, "readsig");
    auto reader = daq::StreamReaderFromPort(port, SampleType::Float64, SampleType::RangeInt64);

    ReaderStatusPtr status;
    auto future = std::async(std::launch::async, [&] {
        SizeT count{0};
        status = reader.read(nullptr, &count, 1000u);
    });

    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    port.connect(this->signal);

    future.wait();
    ASSERT_EQ(status.getReadStatus(), ReadStatus::Event);
}

TYPED_TEST(StreamReaderTest, ReconnectWhileReading)
{
    this->signal.setDescriptor(setupDescriptor(SampleType::Float64));
    auto port = InputPort(this->signal.getContext(), nullptr, "readsig");

    auto reader = daq::StreamReaderFromPort(port, SampleType::Float64, SampleType::RangeInt64);
    port.connect(this->signal);

    SizeT count{0};
    ReaderStatusPtr status = reader.read(nullptr, &count);
    ASSERT_EQ(status.getReadStatus(), ReadStatus::Event);

    status = nullptr;
    auto future = std::async(std::launch::async, [&] {
        // the timeout is ignored for count 0
        SizeT count{1};
        double samples;
        status = reader.read(&samples, &count, 1000u);
    });

    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    port.disconnect();
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    port.connect(this->signal);

    auto domainPacket = DataPacket(setupDescriptor(SampleType::RangeInt64, LinearDataRule(1, 0), nullptr), 1, 1);
    auto dataPacket = DataPacketWithDomain(domainPacket, this->signal.getDescriptor(), 1);
    this->sendPacket(dataPacket);

    future.wait();
    ASSERT_EQ(status.getReadStatus(), ReadStatus::Event);
}

TYPED_TEST(StreamReaderTest, DeltaCheck)
{
    this->signal.setDescriptor(setupDescriptor(SampleType::Float64));

    auto reader = daq::StreamReaderBuilder()
        .setSignal(this->signal)
        .setValueReadType(SampleType::Float64)
        .setDomainReadType(SampleType::RangeInt64)
        .setSkipEvents(true)
        .build();

    for (int i = 0; i < 5; i++)
    {
        auto domainPacket = DataPacket(setupDescriptor(SampleType::RangeInt64, LinearDataRule(1, 0), nullptr), 1, i);
        auto dataPacket = DataPacketWithDomain(domainPacket, this->signal.getDescriptor(), 1);
        this->sendPacket(dataPacket);
    }

    {
        SizeT count{3};
        double samples[3];
        auto status = reader.read(samples, &count, 1000u);
        ASSERT_EQ(count, 3u);
        ASSERT_EQ(status.getOffset(), 0);
    }

    {
        SizeT count{1};
        double samples[1];
        auto status = reader.read(samples, &count);
        ASSERT_EQ(count, 1u);
        ASSERT_EQ(status.getOffset(), 3);
    }
}

TYPED_TEST(StreamReaderTest, SkipSamplesNullParam)
{
    this->signal.setDescriptor(setupDescriptor(SampleType::Float64));
    auto reader = daq::StreamReader<TypeParam, ClockRange>(this->signal);

    ASSERT_THROW(reader.skipSamples(nullptr), daq::ArgumentNullException);
}

TYPED_TEST(StreamReaderTest, SkipSamplesStatus)
{
    this->signal.setDescriptor(setupDescriptor(SampleType::Float64));
    auto reader = daq::StreamReader<TypeParam, ClockRange>(this->signal);
    this->signal.setDescriptor(setupDescriptor(SampleType::Invalid));

    // read first descriptor event
    {
        SizeT count{0};
        auto status = reader.read(nullptr, &count);
    }

    // First skip reads the descriptor event and puts reader to invalid state
    SizeT count = 10;
    auto status = reader.skipSamples(&count);
    ASSERT_EQ(count, 0u);
    ASSERT_TRUE(status.assigned());
    ASSERT_EQ(status.getReadStatus(), ReadStatus::Event);
    ASSERT_EQ(status.getValid(), false);
    ASSERT_TRUE(status.getEventPacket().assigned());

    // Second time reader is already in invalid state
    count = 10;
    status = reader.skipSamples(&count);
    ASSERT_EQ(count, 0u);
    ASSERT_TRUE(status.assigned());
    ASSERT_EQ(status.getReadStatus(), ReadStatus::Fail);
    ASSERT_EQ(status.getValid(), false);
    ASSERT_FALSE(status.getEventPacket().assigned());
}

TYPED_TEST(StreamReaderTest, SkipSamplesNoData)
{
    this->signal.setDescriptor(setupDescriptor(SampleType::Float64));
    auto reader = daq::StreamReader<TypeParam, ClockRange>(this->signal);
    
    SizeT count{10};
    reader.skipSamples(&count);
    ASSERT_EQ(count, 0u);
    ASSERT_EQ(reader.getAvailableCount(), 0u);
}

TYPED_TEST(StreamReaderTest, SkipSamplesOnePacket)
{
    this->signal.setDescriptor(setupDescriptor(SampleType::Float64));

    auto reader = daq::StreamReader<TypeParam, ClockRange>(this->signal);
    auto dataPacket = DataPacket(this->signal.getDescriptor(), 50);

    // read first descriptor event
    {
        SizeT count{0};
        auto status = reader.read(nullptr, &count);
    }

    // Fill data packet
    auto dataPtr = static_cast<double*>(dataPacket.getData());
    for (size_t i = 0; i < 50; ++i)
        dataPtr[i] = i * 11.1;

    this->sendPacket(dataPacket);

    SizeT count{10};
    TypeParam firstBatchSamples[10]{};
    reader.read((void*) &firstBatchSamples, &count);

    ASSERT_EQ(count, 10u);
    ASSERT_EQ(reader.getAvailableCount(), 40u);

    count = 30;
    reader.skipSamples(&count);
    ASSERT_EQ(count, 30u);
    ASSERT_EQ(reader.getAvailableCount(), 10u);

    count = 10;
    TypeParam secondBatchSamples[10]{};
    reader.read((void*) &secondBatchSamples, &count);

    ASSERT_EQ(count, 10u);
    ASSERT_EQ(reader.getAvailableCount(), 0u);

    if constexpr (IsTemplateOf<TypeParam, Complex_Number>::value || IsTemplateOf<TypeParam, RangeType>::value)
    {
        for (size_t i = 0; i < 10; ++i)
        {
            ASSERT_EQ(firstBatchSamples[i], TypeParam(typename TypeParam::Type(i * 11.1)));
            ASSERT_EQ(secondBatchSamples[i], TypeParam(typename TypeParam::Type((40 + i) * 11.1)));
        }
    }
    else
    {
        for (size_t i = 0; i < 10; ++i)
        {
            ASSERT_EQ(firstBatchSamples[i], (TypeParam)(i * 11.1));
            ASSERT_EQ(secondBatchSamples[i], (TypeParam) ((40 + i) * 11.1));
        }
    }
}


TYPED_TEST(StreamReaderTest, SkipSamplesBetweenPackets)
{
    this->signal.setDescriptor(setupDescriptor(SampleType::Float64));

    auto reader = daq::StreamReader<TypeParam, ClockRange>(this->signal);

    // read first descriptor event
    {
        SizeT count{0};
        auto status = reader.read(nullptr, &count);
    }

    // Send first data packet
    auto dataPacket = DataPacket(this->signal.getDescriptor(), 25);
    auto dataPtr = static_cast<double*>(dataPacket.getData());
    for (size_t i = 0; i < 25; ++i)
        dataPtr[i] = i * 11.1;

    this->sendPacket(dataPacket);

    SizeT count{10};
    TypeParam firstBatchSamples[10]{};
    reader.read((void*) &firstBatchSamples, &count);

    ASSERT_EQ(count, 10u);
    ASSERT_EQ(reader.getAvailableCount(), 15u);

    count = 20;
    reader.skipSamples(&count);
    ASSERT_EQ(count, 15u);
    ASSERT_EQ(reader.getAvailableCount(), 0u);

    // Send second data packet
    dataPacket = DataPacket(this->signal.getDescriptor(), 25);
    dataPtr = static_cast<double*>(dataPacket.getData());
    for (size_t i = 0; i < 25; ++i)
        dataPtr[i] = (i + 25) * 11.1;

    this->sendPacket(dataPacket);

    count = 15;
    reader.skipSamples(&count);
    ASSERT_EQ(count, 15u);
    ASSERT_EQ(reader.getAvailableCount(), 10u);

    count = 10;
    TypeParam secondBatchSamples[10]{};
    reader.read((void*) &secondBatchSamples, &count);

    ASSERT_EQ(count, 10u);
    ASSERT_EQ(reader.getAvailableCount(), 0u);

    if constexpr (IsTemplateOf<TypeParam, Complex_Number>::value || IsTemplateOf<TypeParam, RangeType>::value)
    {
        for (size_t i = 0; i < 10; ++i)
        {
            ASSERT_EQ(firstBatchSamples[i], TypeParam(typename TypeParam::Type(i * 11.1)));
            ASSERT_EQ(secondBatchSamples[i], TypeParam(typename TypeParam::Type((40 + i) * 11.1)));
        }
    }
    else
    {
        for (size_t i = 0; i < 10; ++i)
        {
            ASSERT_EQ(firstBatchSamples[i], (TypeParam) (i * 11.1));
            ASSERT_EQ(secondBatchSamples[i], (TypeParam) ((40 + i) * 11.1));
        }
    }
}

TYPED_TEST(StreamReaderTest, SkipSamplesNotEnoughData)
{
    this->signal.setDescriptor(setupDescriptor(SampleType::Float64));

    auto reader = daq::StreamReader<TypeParam, ClockRange>(this->signal);
    auto dataPacket = DataPacket(this->signal.getDescriptor(), 50);

    // read first descriptor event
    {
        SizeT count{0};
        auto status = reader.read(nullptr, &count);
    }

    // Fill data packet
    auto dataPtr = static_cast<double*>(dataPacket.getData());
    for (size_t i = 0; i < 50; ++i)
        dataPtr[i] = i * 11.1;

    this->sendPacket(dataPacket);

    SizeT count = 100;
    reader.skipSamples(&count);
    ASSERT_EQ(count, 50u);
    ASSERT_EQ(reader.getAvailableCount(), 0u);
}

TYPED_TEST(StreamReaderTest, TestReaderWithConnectedPortConnectionEmpty)
{
    this->signal.setDescriptor(setupDescriptor(SampleType::Float64));
    auto port = InputPort(this->signal.getContext(), nullptr, "readsig");
    port.connect(this->signal);

    {
        auto connection = port.getConnection();
        ASSERT_TRUE(connection.assigned());
        
        SizeT packetInConnection = 0;
        while (true)
        {
            if (connection.dequeue().assigned())
                packetInConnection++;
            else
                break;
        }

        // 1 event packet
        ASSERT_EQ(packetInConnection, 1u);
    }

    auto reader = daq::StreamReaderFromPort(port, SampleType::Float64, SampleType::RangeInt64);
    
    {
        SizeT count{0};
        auto status = reader.read(nullptr, &count);
        ASSERT_EQ(status.getReadStatus(), ReadStatus::Event);
    }

    {
        auto domainPacket = DataPacket(setupDescriptor(SampleType::RangeInt64, LinearDataRule(1, 0), nullptr), 4, 1);
        auto dataPacket = DataPacketWithDomain(domainPacket, this->signal.getDescriptor(), 4);
        auto dataPtr = static_cast<double*>(dataPacket.getData());
        dataPtr[0] = 111.1;
        dataPtr[1] = 222.2;
        dataPtr[2] = 333.3;
        dataPtr[3] = 444.4;

        this->sendPacket(dataPacket);
    }

    {
        SizeT count{4};
        double samples[4]{};
        auto status = reader.read(&samples, &count, 500);
        ASSERT_EQ(status.getReadStatus(), ReadStatus::Ok);

        ASSERT_EQ(count, 4u);
        ASSERT_EQ(samples[0], 111.1);
        ASSERT_EQ(samples[1], 222.2);
        ASSERT_EQ(samples[2], 333.3);
        ASSERT_EQ(samples[3], 444.4);
    }
}

TYPED_TEST(StreamReaderTest, TestReaderWithConnectedPortConnectionNotEmpty)
{
    this->signal.setDescriptor(setupDescriptor(SampleType::Float64));
    auto port = InputPort(this->signal.getContext(), nullptr, "readsig");
    port.connect(this->signal);

    {
        auto connection = port.getConnection();
        ASSERT_TRUE(connection.assigned());
        
        SizeT packetInConnection = 0;
        while (true)
        {
            if (connection.dequeue().assigned())
                packetInConnection++;
            else
                break;
        }

        // 1 event packet
        ASSERT_EQ(packetInConnection, 1u);
    }

    {
        auto domainPacket = DataPacket(setupDescriptor(SampleType::RangeInt64, LinearDataRule(1, 0), nullptr), 4, 1);
        auto dataPacket = DataPacketWithDomain(domainPacket, this->signal.getDescriptor(), 4);
        auto dataPtr = static_cast<double*>(dataPacket.getData());
        dataPtr[0] = 111.1;
        dataPtr[1] = 222.2;
        dataPtr[2] = 333.3;
        dataPtr[3] = 444.4;

        this->sendPacket(dataPacket);
    }

    auto reader = daq::StreamReaderFromPort(port, SampleType::Float64, SampleType::RangeInt64);
    
    {
        SizeT count{0};
        auto status = reader.read(nullptr, &count);
        ASSERT_EQ(status.getReadStatus(), ReadStatus::Event);
    }

    {
        SizeT count{4};
        double samples[4]{};
        auto status = reader.read(&samples, &count, 500);
        ASSERT_EQ(status.getReadStatus(), ReadStatus::Ok);

        ASSERT_EQ(count, 4u);
        ASSERT_EQ(samples[0], 111.1);
        ASSERT_EQ(samples[1], 222.2);
        ASSERT_EQ(samples[2], 333.3);
        ASSERT_EQ(samples[3], 444.4);
    }
}
