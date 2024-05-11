#include <gtest/gtest.h>
#include <opendaq/gmock/input_port.h>
#include <opendaq/gmock/signal.h>
#include <opendaq/packet_factory.h>
#include <opendaq/context_factory.h>
#include <opendaq/connection_factory.h>
#include <opendaq/data_descriptor_factory.h>
#include <opendaq/sample_type_traits.h>

using namespace daq;
using namespace testing;

template <class DT>
class GapCheckTest : public Test
{
protected:
    using DataType = DT;

    std::pair<DataDescriptorPtr, DataDescriptorPtr> getDescriptors(bool explicitDomainRule = false)
    {
        const auto valueDesc = DataDescriptorBuilder().setSampleType(SampleType::Float64).build();
        const auto domainDescBuilder = DataDescriptorBuilder().setSampleType(SampleTypeFromType<DT>::SampleType);

        if (!explicitDomainRule)
            domainDescBuilder.setRule(LinearDataRule(10, 2));

        return {valueDesc, domainDescBuilder.build()};
    }
};

using GapCheckTypes = Types<double, int64_t, uint64_t>;

TYPED_TEST_SUITE(GapCheckTest, GapCheckTypes);

TYPED_TEST(GapCheckTest, Disabled)
{
    const auto ctx = NullContext();

    MockInputPort::Strict inputPort;
    MockSignal::Strict signal;

    EXPECT_CALL(inputPort.mock(), getGapCheckingEnabled(testing::_)).WillOnce(GetBool(False));

    const auto connection = Connection(inputPort.ptr, signal.ptr, ctx);

    auto [ valueDesc, domainDesc ] = this->getDescriptors();
    const auto domainPacket = DataPacket(domainDesc, 10, 0);
    const auto valuePacket = DataPacketWithDomain(domainPacket, valueDesc, 10);

    connection.enqueue(valuePacket);

    const auto pkt = connection.dequeue();
    ASSERT_EQ(pkt, valuePacket);
}

TYPED_TEST(GapCheckTest, NoEventPacket)
{
    const auto ctx = NullContext();

    MockInputPort::Strict inputPort;
    MockSignal::Strict signal;

    EXPECT_CALL(inputPort.mock(), getGapCheckingEnabled(testing::_)).WillOnce(GetBool(True));

    const auto connection = Connection(inputPort.ptr, signal.ptr, ctx);

    auto [valueDesc, domainDesc] = this->getDescriptors();
    const auto domainPacket = DataPacket(domainDesc, 10, 0);
    const auto valuePacket = DataPacketWithDomain(domainPacket, valueDesc, 10);

    ASSERT_THROW(connection.enqueue(valuePacket), InvalidStateException);
}

TYPED_TEST(GapCheckTest, NotAvailableNoDomainPacket)
{
    const auto ctx = NullContext();

    MockInputPort::Strict inputPort;
    MockSignal::Strict signal;

    EXPECT_CALL(inputPort.mock(), getGapCheckingEnabled(testing::_)).WillOnce(GetBool(True));

    const auto connection = Connection(inputPort.ptr, signal.ptr, ctx);

    auto [_, desc] = this->getDescriptors();

    const auto eventPacket = DataDescriptorChangedEventPacket(desc, nullptr);
    connection.enqueue(eventPacket);

    auto pkt = connection.dequeue();
    ASSERT_EQ(pkt, eventPacket);

    const auto packet1 = DataPacket(desc, 10, 0);
    connection.enqueue(packet1);
    pkt = connection.dequeue();
    ASSERT_EQ(pkt, packet1);

    const auto packet2 = DataPacket(desc, 10, 20);
    connection.enqueue(packet2);
    pkt = connection.dequeue();
    ASSERT_EQ(pkt, packet2);
}

TYPED_TEST(GapCheckTest, NoGap)
{
    const auto ctx = NullContext();

    MockInputPort::Strict inputPort;
    MockSignal::Strict signal;

    EXPECT_CALL(inputPort.mock(), getGapCheckingEnabled(testing::_)).WillOnce(GetBool(True));

    const auto connection = Connection(inputPort.ptr, signal.ptr, ctx);

    auto [valueDesc, domainDesc] = this->getDescriptors();

    const auto eventPacket = DataDescriptorChangedEventPacket(valueDesc, domainDesc);
    connection.enqueue(eventPacket);
    auto pkt = connection.dequeue();
    ASSERT_EQ(pkt, eventPacket);

    const auto domainPacket1 = DataPacket(domainDesc, 10, 0);
    const auto packet1 = DataPacketWithDomain(domainPacket1, valueDesc, 10);
    connection.enqueue(packet1);
    pkt = connection.dequeue();
    ASSERT_EQ(pkt, packet1);

    const auto domainPacket2 = DataPacket(domainDesc, 10, 100);
    const auto packet2 = DataPacketWithDomain(domainPacket2, valueDesc, 10);
    connection.enqueue(packet2);
    pkt = connection.dequeue();
    ASSERT_EQ(pkt, packet2);
}

TYPED_TEST(GapCheckTest, Gap)
{
    const auto ctx = NullContext();

    MockInputPort::Strict inputPort;
    MockSignal::Strict signal;

    EXPECT_CALL(inputPort.mock(), getGapCheckingEnabled(testing::_)).WillOnce(GetBool(True));

    const auto connection = Connection(inputPort.ptr, signal.ptr, ctx);

    auto [valueDesc, domainDesc] = this->getDescriptors();

    const auto eventPacket = DataDescriptorChangedEventPacket(valueDesc, domainDesc);
    connection.enqueue(eventPacket);
    auto pkt = connection.dequeue();
    ASSERT_EQ(pkt, eventPacket);

    const auto domainPacket1 = DataPacket(domainDesc, 10, 0);
    const auto packet1 = DataPacketWithDomain(domainPacket1, valueDesc, 10);
    connection.enqueue(packet1);
    pkt = connection.dequeue();
    ASSERT_EQ(pkt, packet1);

    const auto domainPacket2 = DataPacket(domainDesc, 10, 200);
    const auto packet2 = DataPacketWithDomain(domainPacket2, valueDesc, 10);
    connection.enqueue(packet2);

    pkt = connection.dequeue();
    ASSERT_EQ(pkt.asPtrOrNull<IEventPacket>(true).getParameters().get("GapDiff"), 100);

    pkt = connection.dequeue();
    ASSERT_EQ(pkt, packet2);
}

TYPED_TEST(GapCheckTest, NotAvailableExplicitDomainRule)
{
    const auto ctx = NullContext();

    MockInputPort::Strict inputPort;
    MockSignal::Strict signal;

    EXPECT_CALL(inputPort.mock(), getGapCheckingEnabled(testing::_)).WillOnce(GetBool(True));

    const auto connection = Connection(inputPort.ptr, signal.ptr, ctx);

    auto [valueDesc, domainDesc] = this->getDescriptors(true);

    const auto eventPacket = DataDescriptorChangedEventPacket(valueDesc, domainDesc);
    connection.enqueue(eventPacket);
    auto pkt = connection.dequeue();
    ASSERT_EQ(pkt, eventPacket);

    const auto domainPacket1 = DataPacket(domainDesc, 10);
    const auto packet1 = DataPacketWithDomain(domainPacket1, valueDesc, 10);
    connection.enqueue(packet1);
    pkt = connection.dequeue();
    ASSERT_EQ(pkt, packet1);

    const auto domainPacket2 = DataPacket(domainDesc, 10);
    const auto packet2 = DataPacketWithDomain(domainPacket2, valueDesc, 10);
    connection.enqueue(packet2);
    pkt = connection.dequeue();
    ASSERT_EQ(pkt, packet2);
}

TYPED_TEST(GapCheckTest, Overlap)
{
    const auto ctx = NullContext();

    MockInputPort::Strict inputPort;
    MockSignal::Strict signal;

    EXPECT_CALL(inputPort.mock(), getGapCheckingEnabled(testing::_)).WillOnce(GetBool(True));

    const auto connection = Connection(inputPort.ptr, signal.ptr, ctx);

    auto [valueDesc, domainDesc] = this->getDescriptors();

    const auto eventPacket = DataDescriptorChangedEventPacket(valueDesc, domainDesc);
    connection.enqueue(eventPacket);
    auto pkt = connection.dequeue();
    ASSERT_EQ(pkt, eventPacket);

    const auto domainPacket1 = DataPacket(domainDesc, 10, 50);
    const auto packet1 = DataPacketWithDomain(domainPacket1, valueDesc, 10);
    connection.enqueue(packet1);
    pkt = connection.dequeue();
    ASSERT_EQ(pkt, packet1);

    const auto domainPacket2 = DataPacket(domainDesc, 10, 100);
    const auto packet2 = DataPacketWithDomain(domainPacket2, valueDesc, 10);
    connection.enqueue(packet2);

    pkt = connection.dequeue();
    ASSERT_EQ(pkt.asPtrOrNull<IEventPacket>(true).getParameters().get("GapDiff"), -50);

    pkt = connection.dequeue();
    ASSERT_EQ(pkt, packet2);
}

TYPED_TEST(GapCheckTest, DataDescriptorChanged)
{
    const auto ctx = NullContext();

    MockInputPort::Strict inputPort;
    MockSignal::Strict signal;

    EXPECT_CALL(inputPort.mock(), getGapCheckingEnabled(testing::_)).WillOnce(GetBool(True));

    const auto connection = Connection(inputPort.ptr, signal.ptr, ctx);

    auto [valueDesc, domainDesc] = this->getDescriptors();

    const auto eventPacket1 = DataDescriptorChangedEventPacket(valueDesc, domainDesc);
    connection.enqueue(eventPacket1);
    auto pkt = connection.dequeue();
    ASSERT_EQ(pkt, eventPacket1);

    const auto domainPacket1 = DataPacket(domainDesc, 10, 50);
    const auto packet1 = DataPacketWithDomain(domainPacket1, valueDesc, 10);
    connection.enqueue(packet1);
    pkt = connection.dequeue();
    ASSERT_EQ(pkt, packet1);

    const auto eventPacket2 = DataDescriptorChangedEventPacket(valueDesc, domainDesc);
    connection.enqueue(eventPacket2);
    pkt = connection.dequeue();
    ASSERT_EQ(pkt, eventPacket2);

    const auto domainPacket2 = DataPacket(domainDesc, 10, 1000);
    const auto packet2 = DataPacketWithDomain(domainPacket2, valueDesc, 10);
    connection.enqueue(packet2);
    pkt = connection.dequeue();
    ASSERT_EQ(pkt, packet2);
}

TYPED_TEST(GapCheckTest, MultiplePackets)
{
    const auto ctx = NullContext();

    MockInputPort::Strict inputPort;
    MockSignal::Strict signal;

    EXPECT_CALL(inputPort.mock(), getGapCheckingEnabled(testing::_)).WillOnce(GetBool(True));

    const auto connection = Connection(inputPort.ptr, signal.ptr, ctx);

    auto [valueDesc, domainDesc] = this->getDescriptors();

    const auto eventPacket1 = DataDescriptorChangedEventPacket(valueDesc, domainDesc);
    connection.enqueue(eventPacket1);
    auto pkt = connection.dequeue();
    ASSERT_EQ(pkt, eventPacket1);

    const auto domainPacket1 = DataPacket(domainDesc, 10, 50);
    const auto packet1 = DataPacketWithDomain(domainPacket1, valueDesc, 10);
    connection.enqueue(packet1);
    pkt = connection.dequeue();
    ASSERT_EQ(pkt, packet1);

    const auto eventPacket2 = DataDescriptorChangedEventPacket(valueDesc, domainDesc);
    connection.enqueue(eventPacket2);
    pkt = connection.dequeue();
    ASSERT_EQ(pkt, eventPacket2);

    const auto domainPacket2 = DataPacket(domainDesc, 10, 1000);
    const auto packet2 = DataPacketWithDomain(domainPacket2, valueDesc, 10);
    connection.enqueue(packet2);
    pkt = connection.dequeue();
    ASSERT_EQ(pkt, packet2);

    const auto domainPacket3 = DataPacket(domainDesc, 10, 1100);
    const auto packet3 = DataPacketWithDomain(domainPacket3, valueDesc, 10);
    connection.enqueue(packet3);
    pkt = connection.dequeue();
    ASSERT_EQ(pkt, packet3);

    const auto domainPacket4 = DataPacket(domainDesc, 10, 1500);
    const auto packet4 = DataPacketWithDomain(domainPacket4, valueDesc, 10);
    connection.enqueue(packet4);
    pkt = connection.dequeue();
    ASSERT_EQ(pkt.asPtrOrNull<IEventPacket>(true).getParameters().get("GapDiff"), 300);
    pkt = connection.dequeue();
    ASSERT_EQ(pkt, packet4);

    const auto domainPacket5 = DataPacket(domainDesc, 10, 1600);
    const auto packet5 = DataPacketWithDomain(domainPacket5, valueDesc, 10);
    connection.enqueue(packet5);
    pkt = connection.dequeue();
    ASSERT_EQ(pkt, packet5);

    const auto domainPacket6 = DataPacket(domainDesc, 10, 1600);
    const auto packet6 = DataPacketWithDomain(domainPacket6, valueDesc, 10);
    connection.enqueue(packet6);
    pkt = connection.dequeue();
    ASSERT_EQ(pkt.asPtrOrNull<IEventPacket>(true).getParameters().get("GapDiff"), -100);
    pkt = connection.dequeue();
    ASSERT_EQ(pkt, packet6);

    const auto domainPacket7 = DataPacket(domainDesc, 10, 1700);
    const auto packet7 = DataPacketWithDomain(domainPacket7, valueDesc, 10);
    connection.enqueue(packet7);
    pkt = connection.dequeue();
    ASSERT_EQ(pkt, packet7);
}
