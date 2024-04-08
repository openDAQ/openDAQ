#include <array>
#include <opendaq/connection_factory.h>
#include <coretypes/objectptr.h>
#include <gtest/gtest.h>

#include "opendaq/context_factory.h"
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
    EXPECT_CALL(inputPort.mock(), getGlobalId(testing::_)).WillOnce(Get(String("id")));
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
