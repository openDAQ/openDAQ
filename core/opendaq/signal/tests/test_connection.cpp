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
        EXPECT_CALL(inputPort.mock(), notifyPacketEnqueued).Times(1);
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
