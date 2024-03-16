#include <array>
#include <opendaq/connection_factory.h>
#include <coretypes/objectptr.h>
#include <gtest/gtest.h>
#include "opendaq/gmock/context.h"
#include "opendaq/gmock/input_port.h"
#include "opendaq/gmock/packet.h"
#include "opendaq/gmock/scheduler.h"
#include "opendaq/gmock/signal.h"

using namespace daq;
using namespace testing;

class ConnectionTest : public Test
{
protected:
    MockScheduler::Strict scheduler;
    MockContext::Strict context;
    MockInputPort::Strict inputPort;
    MockSignal::Strict signal;
    ConnectionPtr connection = Connection(inputPort->asPtr<IInputPort>(), signal, context);
    ConnectionTest() { context.mock().scheduler = scheduler; }
};

TEST_F(ConnectionTest, InitialState)
{
    EXPECT_EQ(connection.getPacketCount(), 0u);
    EXPECT_EQ(connection.getSignal(), signal.ptr);
    EXPECT_EQ(connection.getInputPort(), inputPort.ptr);
}

TEST_F(ConnectionTest, Enqueue)
{
    std::array packets {
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

TEST_F(ConnectionTest, DequeueEmpty)
{
    ASSERT_FALSE(connection.dequeue().assigned());
}

TEST_F(ConnectionTest, PeekEmpty)
{
    ASSERT_FALSE(connection.peek().assigned());
}

TEST_F(ConnectionTest, DequeueAll)
{
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
    ASSERT_EQ(packetsOut.getCount(), 3);
    ASSERT_FALSE(connection.dequeue().assigned());
    ASSERT_EQ(connection.dequeueAll().getCount(),0);
}

TEST_F(ConnectionTest, EnqueueAndSteal)
{
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
        ASSERT_NO_THROW(checkErrorInfo(connection->enqueueAndSteal(pkt.detach())));
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
    auto packet = createWithImplementation<IPacket, MockPacket>();
    size_t packetRefCount{};
    EXPECT_CALL(inputPort.mock(), notifyPacketEnqueued(True)).Times(1).WillOnce([this, &packetRefCount](daq::Bool queueWasEmpty)
        {
            const auto packet = connection.dequeue();
            packetRefCount = packet.getRefCount();
            return OPENDAQ_SUCCESS;
        });

    ASSERT_NO_THROW(checkErrorInfo(connection->enqueueAndSteal(packet.detach())));
    ASSERT_EQ(packetRefCount, 1);
}

TEST_F(ConnectionTest, EnqueueQueueWasEmpty)
{
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
    auto packets = List<IPacket>(createWithImplementation<IPacket, MockPacket>(),
                                 createWithImplementation<IPacket, MockPacket>(),
                                 createWithImplementation<IPacket, MockPacket>());

    size_t packetRefCount{0};
    EXPECT_CALL(inputPort.mock(), notifyPacketEnqueued(True))
        .Times(1)
        .WillOnce(
            [this, &packetRefCount](daq::Bool queueWasEmpty)
            {
                auto packet = connection.dequeue();
                while (packet.assigned())
                {
                    packetRefCount += packet.getRefCount();
                    packet = connection.dequeue();
                }
                return OPENDAQ_SUCCESS;
            });

    checkErrorInfo(connection->enqueueAndStealMultiple(packets.detach()));
    ASSERT_EQ(packetRefCount, 3);

    EXPECT_CALL(inputPort.mock(), notifyPacketEnqueued(True)).Times(1);
    connection.enqueue(createWithImplementation<IPacket, MockPacket>());
}

TEST_F(ConnectionTest, NotifyPacketEnqueuedSameThread)
{
    auto packet = createWithImplementation<IPacket, MockPacket>();
    EXPECT_CALL(inputPort.mock(), notifyPacketEnqueuedOnThisThread()).Times(1);
    connection.enqueueOnThisThread(packet);
}
