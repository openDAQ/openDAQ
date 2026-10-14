#include <gmock/gmock.h>
#include <opendaq/data_descriptor_factory.h>
#include <opendaq/packet_factory.h>
#include <opendaq/reader_factory.h>
#include <opendaq/signal_factory.h>
#include <opendaq/stream_reader_ptr.h>
#include <algorithm>
#include "reader_common.h"

using namespace daq;
using namespace testing;

// A function block can switch the domain of a signal while it keeps sending on it, for example to let the user choose
// whether samples are timestamped. setDomainSignal sends readers no descriptor change. They only see the data packets
// start or stop carrying a domain packet.
class StreamReaderDomainSwitchTest : public ReaderTest<>
{
protected:
    static constexpr double Guard = -1.0;

    SignalConfigPtr domainSignal;

    void SetUp() override
    {
        ReaderTest<>::SetUp();

        signal.setDescriptor(setupDescriptor(SampleType::Float64));

        domainSignal = daq::Signal(context, nullptr, "time");
        domainSignal.setDescriptor(DataDescriptorBuilder()
                                       .setSampleType(SampleType::UInt64)
                                       .setTickResolution(Ratio(1, 1'000'000))
                                       .setOrigin("1970-01-01T00:00:00Z")
                                       .build());
    }

    StreamReaderPtr createReader() const
    {
        return StreamReaderBuilder()
            .setSignal(signal)
            .setValueReadType(SampleType::Float64)
            .setDomainReadType(SampleType::UInt64)
            .setSkipEvents(true)
            .build();
    }

    void sendWithDomain(double value, uint64_t tick) const
    {
        auto domainPacket = DataPacket(domainSignal.getDescriptor(), 1);
        *static_cast<uint64_t*>(domainPacket.getData()) = tick;

        auto dataPacket = DataPacketWithDomain(domainPacket, signal.getDescriptor(), 1);
        *static_cast<double*>(dataPacket.getData()) = value;
        sendPacket(dataPacket);
    }

    void sendWithoutDomain(double value) const
    {
        auto dataPacket = DataPacket(signal.getDescriptor(), 1);
        *static_cast<double*>(dataPacket.getData()) = value;
        sendPacket(dataPacket);
    }
};

TEST_F(StreamReaderDomainSwitchTest, ReadWithDomainPacketWithoutDomain)
{
    auto reader = createReader();

    auto dataPacket = DataPacket(signal.getDescriptor(), 4);
    std::fill_n(static_cast<double*>(dataPacket.getData()), 4, 1.0);
    sendPacket(dataPacket);

    // The guard samples past the requested count catch values copied beyond the caller's buffer.
    constexpr SizeT requested = 2;
    double samples[requested + 4];
    uint64_t domain[requested + 4]{};
    std::fill_n(samples, requested + 4, Guard);

    SizeT count{requested};
    auto status = reader.readWithDomain(&samples, &domain, &count);

    ASSERT_EQ(count, 0u);
    ASSERT_EQ(status.getReadStatus(), ReadStatus::Fail);
    for (SizeT i = requested; i < requested + 4; ++i)
        ASSERT_EQ(samples[i], Guard);

    count = requested;
    status = reader.readWithDomain(&samples, &domain, &count);
    ASSERT_EQ(count, 0u);
    ASSERT_EQ(status.getReadStatus(), ReadStatus::Fail);
}

TEST_F(StreamReaderDomainSwitchTest, DomainRemovedWhileReadingWithDomain)
{
    signal.setDomainSignal(domainSignal);
    auto reader = createReader();

    sendWithDomain(1.0, 100);
    signal.setDomainSignal(nullptr);
    sendWithoutDomain(2.0);
    sendWithoutDomain(3.0);

    double samples[6];
    uint64_t domain[6]{};
    std::fill_n(samples, 6, Guard);

    SizeT count{4};
    auto status = reader.readWithDomain(&samples, &domain, &count);

    ASSERT_EQ(count, 1u);
    ASSERT_EQ(status.getReadStatus(), ReadStatus::Fail);
    ASSERT_EQ(samples[0], 1.0);
    ASSERT_EQ(domain[0], 100u);
    for (SizeT i = 2; i < 6; ++i)
        ASSERT_EQ(samples[i], Guard);
}

TEST_F(StreamReaderDomainSwitchTest, DomainRemovedWhileReadingValues)
{
    signal.setDomainSignal(domainSignal);
    auto reader = createReader();

    sendWithDomain(1.0, 100);
    signal.setDomainSignal(nullptr);
    sendWithoutDomain(2.0);

    SizeT count{2};
    double samples[2]{};
    auto status = reader.read(&samples, &count);

    ASSERT_EQ(count, 2u);
    ASSERT_EQ(status.getReadStatus(), ReadStatus::Ok);
    ASSERT_THAT(samples, ElementsAre(1.0, 2.0));
}

TEST_F(StreamReaderDomainSwitchTest, DomainAddedWhileReadingValues)
{
    auto reader = createReader();

    sendWithoutDomain(1.0);
    signal.setDomainSignal(domainSignal);
    sendWithDomain(2.0, 200);

    SizeT count{2};
    double samples[2]{};
    auto status = reader.read(&samples, &count);

    ASSERT_EQ(count, 2u);
    ASSERT_EQ(status.getReadStatus(), ReadStatus::Ok);
    ASSERT_THAT(samples, ElementsAre(1.0, 2.0));
}

TEST_F(StreamReaderDomainSwitchTest, DomainAddedWhileReadingWithDomain)
{
    auto reader = createReader();

    signal.setDomainSignal(domainSignal);
    sendWithDomain(1.0, 100);
    sendWithDomain(2.0, 200);

    SizeT count{2};
    double samples[2]{};
    uint64_t domain[2]{};
    auto status = reader.readWithDomain(&samples, &domain, &count);

    ASSERT_EQ(count, 2u);
    ASSERT_EQ(status.getReadStatus(), ReadStatus::Ok);
    ASSERT_THAT(samples, ElementsAre(1.0, 2.0));
    ASSERT_THAT(domain, ElementsAre(100u, 200u));
}

TEST_F(StreamReaderDomainSwitchTest, DomainRemovedAndAddedBackWithoutData)
{
    signal.setDomainSignal(domainSignal);
    auto reader = createReader();

    sendWithDomain(1.0, 100);
    signal.setDomainSignal(nullptr);
    signal.setDomainSignal(domainSignal);
    sendWithDomain(2.0, 200);

    SizeT count{2};
    double samples[2]{};
    uint64_t domain[2]{};
    auto status = reader.readWithDomain(&samples, &domain, &count);

    ASSERT_EQ(count, 2u);
    ASSERT_EQ(status.getReadStatus(), ReadStatus::Ok);
    ASSERT_THAT(samples, ElementsAre(1.0, 2.0));
    ASSERT_THAT(domain, ElementsAre(100u, 200u));
}

TEST_F(StreamReaderDomainSwitchTest, DomainRemovedAndAddedBackWithData)
{
    signal.setDomainSignal(domainSignal);
    auto reader = createReader();

    sendWithDomain(1.0, 100);
    signal.setDomainSignal(nullptr);
    sendWithoutDomain(2.0);
    signal.setDomainSignal(domainSignal);
    sendWithDomain(3.0, 300);

    double samples[4];
    uint64_t domain[4]{};
    std::fill_n(samples, 4, Guard);

    SizeT count{3};
    auto status = reader.readWithDomain(&samples, &domain, &count);

    ASSERT_EQ(count, 1u);
    ASSERT_EQ(status.getReadStatus(), ReadStatus::Fail);
    ASSERT_EQ(samples[0], 1.0);
    for (SizeT i = 2; i < 4; ++i)
        ASSERT_EQ(samples[i], Guard);

    count = 3;
    status = reader.readWithDomain(&samples, &domain, &count);
    ASSERT_EQ(count, 0u);
    ASSERT_EQ(status.getReadStatus(), ReadStatus::Fail);

    // The packet that failed is dropped. A reader that takes over continues with the next one.
    auto newReader = StreamReaderFromExisting(reader, SampleType::Float64, SampleType::UInt64);

    count = 3;
    std::fill_n(samples, 4, Guard);
    status = newReader.readWithDomain(&samples, &domain, &count);

    ASSERT_EQ(count, 1u);
    ASSERT_EQ(samples[0], 3.0);
    ASSERT_EQ(domain[0], 300u);
}
