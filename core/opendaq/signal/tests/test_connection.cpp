#include <array>
#include <opendaq/connection_factory.h>
#include <coretypes/objectptr.h>
#include <gtest/gtest.h>
#include <opendaq/connection_internal.h>

#include "opendaq/context_factory.h"
#include "opendaq/signal_factory.h"
#include "opendaq/input_port_factory.h"
#include "opendaq/packet_factory.h"
#include "opendaq/scheduler_factory.h"

#include "opendaq/gmock/context.h"
#include "opendaq/gmock/input_port.h"
#include "opendaq/gmock/packet.h"
#include "opendaq/gmock/scheduler.h"
#include "opendaq/gmock/signal.h"

using namespace daq;
using namespace testing;

class ConnectionTest : public TestWithParam<bool>
{
protected:
    ContextPtr context = NullContext();
    MockScheduler::Strict scheduler;
    MockInputPort::Strict inputPort;
    MockSignal::Strict signal;
};

TEST_P(ConnectionTest, InitialState)
{
    EXPECT_CALL(inputPort.mock(), getGapCheckingEnabled(testing::_)).WillOnce(GetBool(GetParam()));
    const auto connection = Connection(inputPort->asPtr<IInputPort>(), signal, context);
#if OPENDAQ_LOG_LEVEL <= OPENDAQ_LOG_LEVEL_TRACE
    EXPECT_CALL(inputPort.mock(), getGlobalId(testing::_)).WillOnce(Get(String("Id")));
#endif
    EXPECT_EQ(connection.getPacketCount(), 0u);
    EXPECT_EQ(connection.getSignal(), signal.ptr);
    EXPECT_EQ(connection.getInputPort(), inputPort.ptr);
}

TEST_P(ConnectionTest, DequeueEmpty)
{
    EXPECT_CALL(inputPort.mock(), getGapCheckingEnabled(testing::_)).WillOnce(GetBool(GetParam()));
    const auto connection = Connection(inputPort->asPtr<IInputPort>(), signal, context);
    ASSERT_FALSE(connection.dequeue().assigned());
}

TEST_P(ConnectionTest, PeekEmpty)
{
    EXPECT_CALL(inputPort.mock(), getGapCheckingEnabled(testing::_)).WillOnce(GetBool(GetParam()));
    const auto connection = Connection(inputPort->asPtr<IInputPort>(), signal, context);
    ASSERT_FALSE(connection.peek().assigned());
}

INSTANTIATE_TEST_SUITE_P(GapCheckEnabled, ConnectionTest, testing::Values(true, false));

TEST_F(ConnectionTest, Enqueue)
{
    EXPECT_CALL(inputPort.mock(), getGapCheckingEnabled(testing::_)).WillOnce(GetBool(False));
    const auto connection = Connection(inputPort->asPtr<IInputPort>(), signal, context);
    const std::array packets{
        createWithImplementation<IPacket, MockPacket>(),
        createWithImplementation<IPacket, MockPacket>(),
        createWithImplementation<IPacket, MockPacket>(),
    };

    std::size_t n = 0;

    for (const auto& packet : packets)
    {
        EXPECT_CALL(inputPort.mock(), notifyPacketEnqueued(n == 0 ? True : False)).Times(1);
        ASSERT_NO_THROW(connection.enqueue(packet));
        EXPECT_EQ(connection.getPacketCount(), ++n);
        EXPECT_EQ(connection.peek(), packets[0]);
    }

    while (n)
    {
        EXPECT_EQ(connection.peek(), packets[packets.size() - n]);
        ASSERT_EQ(connection.dequeue(), packets[packets.size() - n]);
        EXPECT_EQ(connection.getPacketCount(), --n);
    }

    ASSERT_FALSE(connection.dequeue().assigned());
}

TEST_F(ConnectionTest, DequeueAll)
{
    EXPECT_CALL(inputPort.mock(), getGapCheckingEnabled(testing::_)).WillOnce(GetBool(False));
    const auto connection = Connection(inputPort->asPtr<IInputPort>(), signal, context);

    std::array packets{
        createWithImplementation<IPacket, MockPacket>(),
        createWithImplementation<IPacket, MockPacket>(),
        createWithImplementation<IPacket, MockPacket>(),
    };

    std::size_t n = 0;

    for (const auto& packet : packets)
    {
        EXPECT_CALL(inputPort.mock(), notifyPacketEnqueued(n++ == 0 ? True : False)).Times(1);
        ASSERT_NO_THROW(connection.enqueue(packet));
    }

    const auto packetsOut = connection.dequeueAll();
    ASSERT_EQ(packetsOut.getCount(), 3u);
    ASSERT_FALSE(connection.dequeue().assigned());
    ASSERT_EQ(connection.dequeueAll().getCount(), 0u);
}

TEST_F(ConnectionTest, EnqueueAndSteal)
{
    EXPECT_CALL(inputPort.mock(), getGapCheckingEnabled(testing::_)).WillOnce(GetBool(False));
    const auto connection = Connection(inputPort->asPtr<IInputPort>(), signal, context);

    std::array packets{
        createWithImplementation<IPacket, MockPacket>(),
        createWithImplementation<IPacket, MockPacket>(),
        createWithImplementation<IPacket, MockPacket>(),
    };

    std::size_t n = 0;

    for (const auto& packet : packets)
    {
        auto pkt = packet;
        EXPECT_CALL(inputPort.mock(), notifyPacketEnqueued(n == 0 ? True : False)).Times(1);
        ASSERT_NO_THROW(connection.enqueue(std::move(pkt)));
        EXPECT_EQ(connection.getPacketCount(), ++n);
        EXPECT_EQ(connection.peek(), packets[0]);
    }

    while (n)
    {
        EXPECT_EQ(connection.peek(), packets[packets.size() - n]);
        ASSERT_EQ(connection.dequeue(), packets[packets.size() - n]);
        EXPECT_EQ(connection.getPacketCount(), --n);
    }

    ASSERT_FALSE(connection.dequeue().assigned());
}

TEST_F(ConnectionTest, EnqueueAndStealRefCount1)
{
    EXPECT_CALL(inputPort.mock(), getGapCheckingEnabled(testing::_)).WillOnce(GetBool(False));
    const auto connection = Connection(inputPort->asPtr<IInputPort>(), signal, context);

    auto packet = createWithImplementation<IPacket, MockPacket>();
    size_t packetRefCount{};
    EXPECT_CALL(inputPort.mock(), notifyPacketEnqueued(True)).Times(1).WillOnce([this, &connection, &packetRefCount](daq::Bool queueWasEmpty)
        {
            const auto packet = connection.dequeue();
            packetRefCount = packet.getRefCount();
            return OPENDAQ_SUCCESS;
        });

    ASSERT_NO_THROW(connection.enqueue(std::move(packet)));
    ASSERT_EQ(packetRefCount, 1u);
}

TEST_F(ConnectionTest, EnqueueQueueWasEmpty)
{
    EXPECT_CALL(inputPort.mock(), getGapCheckingEnabled(testing::_)).WillOnce(GetBool(False));
    const auto connection = Connection(inputPort->asPtr<IInputPort>(), signal, context);

    EXPECT_CALL(inputPort.mock(), notifyPacketEnqueued(True)).Times(1);
    connection.enqueue(createWithImplementation<IPacket, MockPacket>());
    EXPECT_CALL(inputPort.mock(), notifyPacketEnqueued(False)).Times(2);
    connection.enqueue(createWithImplementation<IPacket, MockPacket>());
    connection.enqueue(createWithImplementation<IPacket, MockPacket>());

    ASSERT_TRUE(connection.dequeue().assigned());

    EXPECT_CALL(inputPort.mock(), notifyPacketEnqueued(False)).Times(1);
    connection.enqueue(createWithImplementation<IPacket, MockPacket>());

    ASSERT_TRUE(connection.dequeue().assigned());
    ASSERT_TRUE(connection.dequeue().assigned());
    ASSERT_TRUE(connection.dequeue().assigned());
    ASSERT_FALSE(connection.dequeue().assigned());

    EXPECT_CALL(inputPort.mock(), notifyPacketEnqueued(True)).Times(1);
    connection.enqueue(createWithImplementation<IPacket, MockPacket>());
}

TEST_F(ConnectionTest, EnqueueMultiple)
{
    EXPECT_CALL(inputPort.mock(), getGapCheckingEnabled(testing::_)).WillOnce(GetBool(False));
    const auto connection = Connection(inputPort->asPtr<IInputPort>(), signal, context);

    const auto packets = List<IPacket>(
        createWithImplementation<IPacket, MockPacket>(),
        createWithImplementation<IPacket, MockPacket>(),
        createWithImplementation<IPacket, MockPacket>());

    EXPECT_CALL(inputPort.mock(), notifyPacketEnqueued(True)).Times(1);
    connection.enqueueMultiple(packets);

    ASSERT_TRUE(connection.dequeue().assigned());
    ASSERT_TRUE(connection.dequeue().assigned());
    ASSERT_TRUE(connection.dequeue().assigned());
    ASSERT_FALSE(connection.dequeue().assigned());

    EXPECT_CALL(inputPort.mock(), notifyPacketEnqueued(True)).Times(1);
    connection.enqueue(createWithImplementation<IPacket, MockPacket>());
}

TEST_F(ConnectionTest, EnqueueAndStealMultipleRefCount1)
{
    EXPECT_CALL(inputPort.mock(), getGapCheckingEnabled(testing::_)).WillOnce(GetBool(False));
    const auto connection = Connection(inputPort->asPtr<IInputPort>(), signal, context);

    auto packets = List<IPacket>(createWithImplementation<IPacket, MockPacket>(),
                                 createWithImplementation<IPacket, MockPacket>(),
                                 createWithImplementation<IPacket, MockPacket>());

    size_t packetRefCount{0};
    EXPECT_CALL(inputPort.mock(), notifyPacketEnqueued(True))
        .Times(1)
        .WillOnce(
            [this, &connection, &packetRefCount](daq::Bool queueWasEmpty)
            {
                auto packet = connection.dequeue();
                while (packet.assigned())
                {
                    packetRefCount += packet.getRefCount();
                    packet = connection.dequeue();
                }
                return OPENDAQ_SUCCESS;
            });

    checkErrorInfo(connection->enqueueMultipleAndStealRef(packets.detach()));
    ASSERT_EQ(packetRefCount, 3u);

    EXPECT_CALL(inputPort.mock(), notifyPacketEnqueued(True)).Times(1);
    connection.enqueue(createWithImplementation<IPacket, MockPacket>());
}

TEST_F(ConnectionTest, NotifyPacketEnqueuedSameThread)
{
    EXPECT_CALL(inputPort.mock(), getGapCheckingEnabled(testing::_)).WillOnce(GetBool(False));
    const auto connection = Connection(inputPort->asPtr<IInputPort>(), signal, context);

    auto packet = createWithImplementation<IPacket, MockPacket>();
    EXPECT_CALL(inputPort.mock(), notifyPacketEnqueuedOnThisThread()).Times(1);
    connection.enqueueOnThisThread(packet);
}

TEST_F(ConnectionTest, DequeueUpTo1)
{
    auto logger = Logger();
    auto context = Context(Scheduler(logger, 1), logger, nullptr, nullptr, nullptr);
    auto scheduler = context.getScheduler();

    auto signal = Signal(context, nullptr, "sig");
    signal.setDescriptor(DataDescriptorBuilder().setSampleType(SampleType::Float64).build());

    auto ip = InputPort(context, nullptr, "ip");
    ip.connect(signal);

    auto packet = DataPacket(signal.getDescriptor(), 1);
    double* data = static_cast<double*>(packet.getRawData());
    *data = 5.2;

    for (int i = 0; i < 7; ++i)
        signal.sendPacket(packet);

    scheduler.waitAll();

    ObjectPtr<IConnectionInternal> connection = ip.getConnection(); 
    std::vector<IPacket*> buf;
    SizeT count = 3;
    buf.resize(count);

    connection->dequeueUpTo(buf.data(), &count);
    ASSERT_EQ(count, 3u);
    for (SizeT i = 0; i < count; ++i)
        buf[i]->releaseRef();
    
    connection->dequeueUpTo(buf.data(), &count);
    ASSERT_EQ(count, 3u);
    for (SizeT i = 0; i < count; ++i)
        buf[i]->releaseRef();
    
    connection->dequeueUpTo(buf.data(), &count);
    ASSERT_EQ(count, 2u);
    for (SizeT i = 0; i < count; ++i)
        buf[i]->releaseRef();
}

TEST_F(ConnectionTest, DequeueUpTo2)
{
    auto logger = Logger();
    auto context = Context(Scheduler(logger, 1), logger, nullptr, nullptr, nullptr);
    auto scheduler = context.getScheduler();

    auto signal = Signal(context, nullptr, "sig");
    signal.setDescriptor(DataDescriptorBuilder().setSampleType(SampleType::Float64).build());

    auto ip = InputPort(context, nullptr, "ip");
    ip.connect(signal);

    auto packet = DataPacket(signal.getDescriptor(), 1);
    double* data = static_cast<double*>(packet.getRawData());
    *data = 5.2;

    for (int i = 0; i < 7; ++i)
        signal.sendPacket(packet);

    scheduler.waitAll();

    ObjectPtr<IConnectionInternal> connection = ip.getConnection(); 
    std::vector<IPacket*> buf;
    SizeT count = 3;
    buf.resize(count);

    connection->dequeueUpTo(buf.data(), &count);
    ASSERT_EQ(count, 3u);
    for (SizeT i = 0; i < count; ++i)
        PacketPtr pkt = std::move(buf[i]);
    
    connection->dequeueUpTo(buf.data(), &count);
    ASSERT_EQ(count, 3u);
    for (SizeT i = 0; i < count; ++i)
        PacketPtr pkt = std::move(buf[i]);
    
    connection->dequeueUpTo(buf.data(), &count);
    ASSERT_EQ(count, 2u);
    for (SizeT i = 0; i < count; ++i)
        PacketPtr pkt = std::move(buf[i]);
}
