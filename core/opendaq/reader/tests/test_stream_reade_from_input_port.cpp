#include <coretypes/validation.h>
#include <opendaq/input_port_factory.h>
#include <opendaq/input_port_notifications_ptr.h>
#include <opendaq/reader_exceptions.h>
#include <opendaq/reader_factory.h>
#include <testutils/testutils.h>

#include "reader_common.h"

using namespace daq;

class InputPortNotifications : public ImplementationOfWeak<IInputPortNotifications>
{
public:

    ErrCode INTERFACE_FUNC acceptsSignal(IInputPort* port, ISignal* signal, Bool* accept) override
    {
        OPENDAQ_PARAM_NOT_NULL(port);
        OPENDAQ_PARAM_NOT_NULL(signal);
        OPENDAQ_PARAM_NOT_NULL(accept);

        *accept = true;
        return OPENDAQ_SUCCESS;
    }

    ErrCode INTERFACE_FUNC connected(IInputPort* port) override
    {
        OPENDAQ_PARAM_NOT_NULL(port);
        return OPENDAQ_SUCCESS;
    }

    ErrCode INTERFACE_FUNC disconnected(IInputPort* port) override
    {
        OPENDAQ_PARAM_NOT_NULL(port);
        return OPENDAQ_SUCCESS;
    }

    ErrCode INTERFACE_FUNC packetReceived(IInputPort* port) override
    {
        OPENDAQ_PARAM_NOT_NULL(port);
        return OPENDAQ_SUCCESS;
    }
};

template <typename T>
class StreamReaderFromExistingTest : public ReaderTest<T>
{
public:
    using Super = ReaderTest<T>;

    void SetUp() override
    {
        Super::SetUp();

        notifications = createWithImplementation<IInputPortNotifications, InputPortNotifications>();
        port = InputPort(this->context, nullptr, "Test stream reader");

        port.setListener(notifications);
        port.setNotificationMethod(PacketReadyNotification::SameThread);
    }

protected:
    InputPortNotificationsPtr notifications;
    InputPortConfigPtr port;
};

using SampleTypes = ::testing::Types<OPENDAQ_VALUE_SAMPLE_TYPES>;

TYPED_TEST_SUITE(StreamReaderFromExistingTest, SampleTypes);

TYPED_TEST(StreamReaderFromExistingTest, Create)
{
    this->signal.setDescriptor(setupDescriptor(SampleType::Float64));

    auto oldReader = daq::StreamReader<TypeParam, ClockRange>(this->signal);
    ASSERT_NO_THROW((StreamReaderFromExisting<TypeParam, ClockRange>)(oldReader));
}

TYPED_TEST(StreamReaderFromExistingTest, CreateNullThrows)
{
    ASSERT_THROW_MSG(
        (StreamReaderFromExisting<TypeParam, ClockRange>(nullptr)),
        ArgumentNullException,
        "Existing reader must not be null"
    )
}

TYPED_TEST(StreamReaderFromExistingTest, IsReader)
{
    this->signal.setDescriptor(setupDescriptor(SampleType::Float64));
    auto oldReader = daq::StreamReader<TypeParam, ClockRange>(this->signal);

    auto reader = daq::StreamReaderFromExisting<TypeParam, ClockRange>(oldReader);
    ASSERT_NO_THROW(reader.template asPtr<IReader>());
}

TYPED_TEST(StreamReaderFromExistingTest, IsStreamReader)
{
    this->signal.setDescriptor(setupDescriptor(SampleType::Float64));
    auto oldReader = daq::StreamReader<TypeParam, ClockRange>(this->signal);

    auto reader = daq::StreamReaderFromExisting<TypeParam, ClockRange>(oldReader);
    ASSERT_NO_THROW(reader.template asPtr<IStreamReader>());
}

TYPED_TEST(StreamReaderFromExistingTest, GetSamplesAvailableEmpty)
{
    this->signal.setDescriptor(setupDescriptor(SampleType::Float64));
    auto oldReader = daq::StreamReader<TypeParam, ClockRange>(this->signal);

    auto reader = daq::StreamReaderFromExisting<TypeParam, ClockRange>(oldReader);
    ASSERT_EQ(reader.getAvailableCount(), 0u);
}

TYPED_TEST(StreamReaderFromExistingTest, GetSamplesAvailable)
{
    this->signal.setDescriptor(setupDescriptor(SampleType::Float64));
    auto oldReader = daq::StreamReader<TypeParam, ClockRange>(this->signal);

    auto reader = daq::StreamReaderFromExisting<TypeParam, ClockRange>(oldReader);
    ASSERT_EQ(reader.getAvailableCount(), 0u);

    this->sendPacket(DataPacket(this->signal.getDescriptor(), 1));

    ASSERT_EQ(reader.getAvailableCount(), 1u);
}

TYPED_TEST(StreamReaderFromExistingTest, ReadOneSample)
{
    this->signal.setDescriptor(setupDescriptor(SampleType::Float64));
    auto oldReader = daq::StreamReader<TypeParam, ClockRange>(this->signal);

    auto reader = daq::StreamReaderFromExisting<TypeParam, ClockRange>(oldReader);
    auto dataPacket = DataPacket(this->signal.getDescriptor(), 1);

    // Set the first sample to
    auto dataPtr = static_cast<double*>(dataPacket.getData());
    dataPtr[0] = 123.4;

    this->sendPacket(dataPacket);

    SizeT count{1};
    TypeParam samples[1]{};
    reader.read((TypeParam*) &samples, &count);

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

TYPED_TEST(StreamReaderFromExistingTest, ReadOneSampleTimeout)
{
    this->signal.setDescriptor(setupDescriptor(SampleType::Float64));
    auto oldReader = daq::StreamReader<TypeParam, ClockRange>(this->signal);

    auto reader = daq::StreamReaderFromExisting<TypeParam, ClockRange>(oldReader);
    auto dataPacket = DataPacket(this->signal.getDescriptor(), 1);

    // Set the first sample to
    auto dataPtr = static_cast<double*>(dataPacket.getData());
    dataPtr[0] = 123.4;

    this->sendPacket(dataPacket);

    std::thread t([this, &dataPacket] {
        using namespace std::chrono_literals;

        std::this_thread::sleep_for(30ms);
        this->sendPacket(dataPacket);

        fmt::print("Sent packets!\n");
    });


    fmt::print("Reading!\n");

    SizeT count{2};
    TypeParam samples[2]{};
    reader.read((TypeParam*) &samples, &count, 1000u);

    if (t.joinable())
    {
        fmt::print("Joining thread!\n");
        t.join();
        fmt::print("JoinED thread!\n");
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

TYPED_TEST(StreamReaderFromExistingTest, ReadOneSampleWithClockTicks)
{
    this->signal.setDescriptor(setupDescriptor(SampleType::Float64));
    auto oldReader = daq::StreamReader<TypeParam, ClockTick>(this->signal);

    auto reader = daq::StreamReaderFromExisting<TypeParam, ClockTick>(oldReader);

    auto domainPacket = DataPacket(setupDescriptor(SampleType::UInt64, LinearDataRule(1, 0), nullptr), 1, 1);
    auto dataPacket = DataPacketWithDomain(domainPacket, this->signal.getDescriptor(), 1);

    // Set the first sample to
    auto dataPtr = static_cast<double*>(dataPacket.getData());
    dataPtr[0] = 123.4;

    this->sendPacket(dataPacket);

    SizeT count{1};
    TypeParam samples[1]{};
    ClockTick ticks[1]{};
    reader.readWithDomain((TypeParam*) &samples, (ClockTick*) &ticks, &count);

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

TYPED_TEST(StreamReaderFromExistingTest, ReadOneSampleWithClockTicksTimeout)
{
    this->signal.setDescriptor(setupDescriptor(SampleType::Float64));
    auto oldReader = daq::StreamReader<TypeParam, ClockTick>(this->signal);

    auto reader = daq::StreamReaderFromExisting<TypeParam, ClockTick>(oldReader);

    auto domainPacket = DataPacket(setupDescriptor(SampleType::UInt64, LinearDataRule(1, 0), nullptr), 1, 1);
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
    reader.readWithDomain((TypeParam*) &samples, (ClockTick*) &ticks, &count, 1000u);

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

TYPED_TEST(StreamReaderFromExistingTest, ReadOneSampleWithRanges)
{
    this->signal.setDescriptor(setupDescriptor(SampleType::Float64));
    auto oldReader = daq::StreamReader<TypeParam, ClockRange>(this->signal);

    auto reader = daq::StreamReaderFromExisting<TypeParam, ClockRange>(oldReader);

    auto domainPacket = DataPacket(setupDescriptor(SampleType::RangeInt64, LinearDataRule(1, 0), nullptr), 1, 1);
    auto dataPacket = DataPacketWithDomain(domainPacket, this->signal.getDescriptor(), 1);

    // Set the first sample to
    auto dataPtr = static_cast<double*>(dataPacket.getData());
    dataPtr[0] = 123.4;

    this->sendPacket(dataPacket);

    SizeT count{1};
    TypeParam samples[1]{};
    ClockRange stamps[1]{};
    reader->readWithDomain((TypeParam*) &samples, (ClockRange*) &stamps, &count);

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

TYPED_TEST(StreamReaderFromExistingTest, ReadOneSampleWithRangesTimeout)
{
    this->signal.setDescriptor(setupDescriptor(SampleType::Float64));
    auto oldReader = daq::StreamReader<double, ClockRange>(this->signal);

    auto reader = daq::StreamReaderFromExisting<TypeParam, ClockRange>(oldReader);

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
    reader->readWithDomain((void**) &samples, (void**) &stamps, &count, 1000u);

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

TYPED_TEST(StreamReaderFromExistingTest, ReadLessThanOnePacket)
{
    this->signal.setDescriptor(setupDescriptor(SampleType::Float64));
    auto oldReader = daq::StreamReader<TypeParam, ClockRange>(this->signal);

    auto reader = daq::StreamReaderFromExisting<TypeParam, ClockRange>(oldReader);

    const SizeT NUM_SAMPLES = 2;
    auto dataPacket = DataPacket(this->signal.getDescriptor(), NUM_SAMPLES);

    // Set the first sample to
    auto dataPtr = static_cast<double*>(dataPacket.getData());
    dataPtr[0] = 123.4;
    dataPtr[1] = 432.1;

    this->sendPacket(dataPacket);

    SizeT count{1};
    TypeParam samples[1]{};
    reader.read((TypeParam*) &samples, &count);

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

TYPED_TEST(StreamReaderFromExistingTest, ReadBetweenPackets)
{
    this->signal.setDescriptor(setupDescriptor(SampleType::Float64));
    auto oldReader = daq::StreamReader<TypeParam, ClockRange>(this->signal);

    auto reader = daq::StreamReaderFromExisting<TypeParam, ClockRange>(oldReader);
    auto dataPacket = DataPacket(this->signal.getDescriptor(), 2);

    // Set the first sample to
    auto dataPtr = static_cast<double*>(dataPacket.getData());
    dataPtr[0] = 111.1;
    dataPtr[1] = 222.2;

    this->sendPacket(dataPacket);

    SizeT count{1};
    TypeParam samples[1]{};
    reader.read((TypeParam*) &samples, &count);

    ASSERT_EQ(reader.getAvailableCount(), 1u);

    dataPtr[0] = 333.3;
    dataPtr[1] = 444.4;
    this->sendPacket(dataPacket, false);

    ASSERT_EQ(reader.getAvailableCount(), 3u);
}

TYPED_TEST(StreamReaderFromExistingTest, ReadBetweenPacketsTimeout)
{
    this->signal.setDescriptor(setupDescriptor(SampleType::Float64));
    auto oldReader = daq::StreamReader<TypeParam, ClockRange>(this->signal);

    auto reader = daq::StreamReaderFromExisting<TypeParam, ClockRange>(oldReader);
    auto dataPacket = DataPacket(this->signal.getDescriptor(), 2);

    // Set the first sample to
    auto dataPtr = static_cast<double*>(dataPacket.getData());
    dataPtr[0] = 11.1;
    dataPtr[1] = 22.2;

    this->sendPacket(dataPacket);

    SizeT count{1};
    TypeParam samples[1]{};
    reader.read((TypeParam*) &samples, &count);

    ASSERT_EQ(reader.getAvailableCount(), 1u);

    std::thread t([this] {
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
    reader.read((TypeParam*) &samples2, &count, 1000u);

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


TYPED_TEST(StreamReaderFromExistingTest, ReadBetweenPacketsValues)
{
    this->signal.setDescriptor(setupDescriptor(SampleType::Float64));
    auto oldReader = daq::StreamReader<TypeParam, ClockRange>(this->signal);

    auto reader = daq::StreamReaderFromExisting<TypeParam, ClockRange>(oldReader);
    auto dataPacket = DataPacket(this->signal.getDescriptor(), 2);

    // Set the first sample to
    auto dataPtr = static_cast<double*>(dataPacket.getData());
    dataPtr[0] = 11.1;
    dataPtr[1] = 22.2;

    this->sendPacket(dataPacket);

    SizeT count{1};
    TypeParam samples[1]{};
    reader.read((TypeParam*) &samples, &count);

    ASSERT_EQ(reader.getAvailableCount(), 1u);

    auto nextDataPacket = DataPacket(this->signal.getDescriptor(), 2);
    auto nextDataPtr = static_cast<double*>(nextDataPacket.getData());
    nextDataPtr[0] = 33.3;
    nextDataPtr[1] = 44.4;
    this->sendPacket(nextDataPacket);

    ASSERT_EQ(reader.getAvailableCount(), 3u);

    count = 3;
    TypeParam nextSamples[3]{};
    reader.read((TypeParam*) &nextSamples, &count);

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

    ASSERT_EQ(reader.getAvailableCount(), 0u);
}

TYPED_TEST(StreamReaderFromExistingTest, ReadTwoPacketValuesFromInputPort)
{
    this->signal.setDescriptor(setupDescriptor(SampleType::Float64));
    auto oldReader = daq::StreamReader<TypeParam, ClockRange>(this->signal);

    auto reader = daq::StreamReaderFromExisting<TypeParam, ClockRange>(oldReader);
    auto dataPacket = DataPacket(this->signal.getDescriptor(), 2);

    // Set the first sample to
    auto dataPtr = static_cast<double*>(dataPacket.getData());
    dataPtr[0] = 11.1;
    dataPtr[1] = 22.2;

    this->sendPacket(dataPacket);

    SizeT count{1};
    TypeParam samples[1]{};
    reader.read((TypeParam*) &samples, &count);

    ASSERT_EQ(reader.getAvailableCount(), 1u);

    auto nextDataPacket = DataPacket(this->signal.getDescriptor(), 2);
    auto nextDataPtr = static_cast<double*>(nextDataPacket.getData());
    nextDataPtr[0] = 33.3;
    nextDataPtr[1] = 44.4;
    this->sendPacket(nextDataPacket);

    ASSERT_EQ(reader.getAvailableCount(), 3u);

    count = 3;
    TypeParam nextSamples[3]{};
    reader.read((TypeParam*) &nextSamples, &count);

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

    ASSERT_EQ(reader.getAvailableCount(), 0u);
}

TYPED_TEST(StreamReaderFromExistingTest, ReadValuesMoreThanAvailable)
{
    this->signal.setDescriptor(setupDescriptor(SampleType::Float64));
    auto oldReader = daq::StreamReader<TypeParam, ClockRange>(this->signal);

    auto reader = daq::StreamReaderFromExisting<TypeParam, ClockRange>(oldReader);

    const SizeT NUM_SAMPLES = 2;
    this->sendPacket(DataPacket(this->signal.getDescriptor(), NUM_SAMPLES));

    SizeT count{3};
    TypeParam samples[3]{};
    reader.read((void*) &samples, &count);

    ASSERT_EQ(count, 2u);
    ASSERT_EQ(reader.getAvailableCount(), 0u);
}

TYPED_TEST(StreamReaderFromExistingTest, DescriptorChangedConvertible)
{
    this->signal.setDescriptor(setupDescriptor(SampleType::Float64));
    auto oldReader = daq::StreamReader<TypeParam, ClockRange>(this->signal);

    auto reader = daq::StreamReaderFromExisting<TypeParam, ClockRange>(oldReader);

    const SizeT NUM_SAMPLES = 2;
    auto dataPacketDouble = DataPacket(this->signal.getDescriptor(), NUM_SAMPLES);
    auto dataPtrDouble = static_cast<double*>(dataPacketDouble.getData());
    dataPtrDouble[0] = 111.1;
    dataPtrDouble[1] = 222.2;

    this->sendPacket(dataPacketDouble);

    ASSERT_EQ(reader.getAvailableCount(), 2u);

    SizeT count{2};
    TypeParam samplesDouble[2]{};
    reader.read((void*) &samplesDouble, &count);

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
    ASSERT_NO_THROW(reader.read((void*) &sampleInt32, &count));

    ASSERT_EQ(reader.getAvailableCount(), 0u);

    ASSERT_EQ(sampleInt32[0], TypeParam(3));
    ASSERT_EQ(sampleInt32[1], TypeParam(4));
}

TYPED_TEST(StreamReaderFromExistingTest, DescriptorChangedCallbackNotConvertible)
{
    this->signal.setDescriptor(setupDescriptor(SampleType::Float64));
    auto oldReader = daq::StreamReader<TypeParam, ClockRange>(this->signal);

    auto reader = daq::StreamReaderFromExisting<TypeParam, ClockRange>(oldReader);
    reader.setOnDescriptorChanged([](const DataDescriptorPtr& valueDescriptor, const DataDescriptorPtr& domainDescriptor, void* remainingSample, size_t remainingSize)
    {
        return false;
    });

    const SizeT NUM_SAMPLES = 2;
    auto dataPacketDouble = DataPacket(this->signal.getDescriptor(), NUM_SAMPLES);
    auto dataPtrDouble = static_cast<double*>(dataPacketDouble.getData());
    dataPtrDouble[0] = 111.1;
    dataPtrDouble[1] = 222.2;

    this->sendPacket(dataPacketDouble);

    ASSERT_EQ(reader.getAvailableCount(), 2u);

    SizeT count{2};
    TypeParam samplesDouble[2]{};
    reader.read((void*) &samplesDouble, &count);

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
    ErrCode errCode = reader->read((void*) &sampleInt32, &count);
    ASSERT_EQ(errCode, OPENDAQ_ERR_INVALID_DATA);

    ASSERT_THROW_MSG(checkErrorInfo(errCode), InvalidDataException, "Packet samples are no longer convertible to the read type")
}

TYPED_TEST(StreamReaderFromExistingTest, DescriptorChangedNotConvertible)
{
    this->signal.setDescriptor(setupDescriptor(SampleType::Int32));
    auto oldReader = daq::StreamReader<TypeParam, ClockRange>(this->signal);

    auto reader = daq::StreamReaderFromExisting<std::int32_t, ClockRange>(oldReader);
    this->signal.setDescriptor(setupDescriptor(SampleType::ComplexFloat32));

    const SizeT NUM_SAMPLES = 2;
    auto dataPacket = DataPacket(this->signal.getDescriptor(), NUM_SAMPLES);
    auto dataPtr = static_cast<ComplexFloat32*>(dataPacket.getData());
    dataPtr[0] = 111;
    dataPtr[1] = 222;

    this->sendPacket(dataPacket);

    SizeT count{1};
    std::int32_t samples[1];
    ErrCode errCode = reader->read((void*) &samples, &count);
    ASSERT_EQ(errCode, OPENDAQ_ERR_INVALID_DATA);

    ASSERT_THROW_MSG(checkErrorInfo(errCode), InvalidDataException, "Packet samples are no longer convertible to the read type")
}

TYPED_TEST(StreamReaderFromExistingTest, ReadWithZeroAvailableAndTimeoutAny)
{
    const uint64_t FIRST_PACKET_SIZE = 2u;
    const uint64_t SECOND_PACKET_SIZE = 4u;

    this->signal.setDescriptor(setupDescriptor(SampleType::Float64));
    auto oldReader = daq::StreamReader<TypeParam, ClockRange>(this->signal, ReadTimeoutType::Any);

    auto reader = daq::StreamReaderFromExisting<TypeParam, ClockRange>(oldReader);
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
    reader.read((void*) &samples, &count, 1000u);

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

TYPED_TEST(StreamReaderFromExistingTest, StealConnection)
{
    this->signal.setDescriptor(setupDescriptor(SampleType::Int32));
    auto oldReader = daq::StreamReader<TypeParam, ClockRange>(this->signal);

    auto reader = daq::StreamReaderFromExisting<TypeParam, ClockRange>(oldReader);
    this->signal.setDescriptor(setupDescriptor(SampleType::ComplexFloat32));

    const SizeT NUM_SAMPLES = 2;
    auto dataPacket = DataPacket(this->signal.getDescriptor(), NUM_SAMPLES);
    auto dataPtr = static_cast<ComplexFloat32*>(dataPacket.getData());
    dataPtr[0] = 111;
    dataPtr[1] = 222;

    this->sendPacket(dataPacket);

    SizeT count{1};
    TypeParam samples[1];
    ErrCode errCode = reader->read((void*) &samples, &count);

    ErrCode expected;
    if (!IsTemplateOf<TypeParam, Complex_Number>::value)
    {
        expected = OPENDAQ_ERR_INVALID_DATA;
    }
    else
    {
        expected = OPENDAQ_SUCCESS;
    }
    ASSERT_EQ(errCode, expected);

    auto newReader = daq::StreamReaderFromExisting<ComplexFloat32, ClockRange>(reader);

    SizeT complexCount{1};
    ComplexFloat32 complexSamples[1];
    errCode = newReader->read((void*) &complexSamples, &complexCount);
    ASSERT_SUCCEEDED(errCode);

    ASSERT_EQ(complexCount, 1u);

    ComplexFloat32 expectedSample;
    if (!IsTemplateOf<TypeParam, Complex_Number>::value)
    {
        expectedSample = 111;
    }
    else
    {
        expectedSample = 222;
    }
    ASSERT_EQ(complexSamples[0], expectedSample);
}

TYPED_TEST(StreamReaderFromExistingTest, ToString)
{
    this->signal.setDescriptor(setupDescriptor(SampleType::Float64));

    auto oldReader = daq::StreamReader<TypeParam, ClockRange>(this->signal);
    auto reader = daq::StreamReaderFromExisting<TypeParam, ClockRange>(oldReader);

    CharPtr str;
    ASSERT_EQ(reader->toString(&str), OPENDAQ_SUCCESS);

    ASSERT_STREQ(str, "daq::IStreamReader");
    daqFreeMemory(str);
}
