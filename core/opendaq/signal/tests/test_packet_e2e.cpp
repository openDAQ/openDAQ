#include <array>
#include <chrono>
#include <thread>
#include <gtest/gtest.h>
#include <opendaq/context_factory.h>
#include <opendaq/input_port_factory.h>
#include <opendaq/scheduler_factory.h>
#include <opendaq/logger_factory.h>
#include <opendaq/signal_factory.h>
#include <opendaq/event_packet.h>
#include <opendaq/gmock/context.h>
#include <opendaq/gmock/input_port_notifications.h>
#include <opendaq/gmock/packet.h>

using namespace daq;
using namespace testing;

class TestPacketE2E : public Test
{
protected:
    void TearDown() override
    {
        using namespace std::chrono_literals;
        // per Martin, give the async logger time to finish to avoid spurious memory leak errors
        std::this_thread::sleep_for(70ms);
    }
};

TEST_F(TestPacketE2E, Basic)
{
    std::array packets {
        createWithImplementation<IPacket, MockPacket>(),
        createWithImplementation<IPacket, MockPacket>(),
        createWithImplementation<IPacket, MockPacket>(),
    };

    // As of 9 Dec 2022 the Context is not implemented, so we use the mock instead.
    // But configure the mock to return a real scheduler instead of the mocked one.
    auto scheduler = Scheduler(Logger());
    auto context = MockContext::Strict();
    context.mock().scheduler = scheduler;
    context.mock().logger = Logger();

    auto listener = MockInputPortNotifications::Strict();
    auto signal = Signal(context, nullptr, String("Signal"));

    std::array inputPorts {
        InputPort(context, nullptr, "Port1"),
        InputPort(context, nullptr, "Port2"),
        InputPort(context, nullptr, "Port3"),
    };
    for (const auto& inputPort : inputPorts)
        inputPort.setListener(listener);

    // Connect the signal to each input port
    for (const auto& inputPort : inputPorts)
    {
        EXPECT_CALL(
            listener.mock(),
            connected(inputPort.getObject())
        )
        .WillOnce(Return(OPENDAQ_SUCCESS));

        ASSERT_NO_THROW(inputPort.connect(signal));
        auto connection = inputPort.getConnection();
        EXPECT_EQ(connection, connection);
        EXPECT_EQ(inputPort.getSignal(), signal);
        EXPECT_EQ(connection.getInputPort(), inputPort);
        EXPECT_EQ(connection.getSignal(), signal);

        // TODO: We should really attach tasks to the input port's notification graph
        // and process the packets inside this task as it would be done in a real use-case.
        // For now we only check that the packets really got queued up properly and
        // nothing crashes.
    }
    EXPECT_EQ(signal.getConnections().getCount(), inputPorts.size());

    // Send each packet
    for (const auto& packet : packets)
    {
        ASSERT_NO_THROW(signal.sendPacket(packet));
    }

    // Make sure all the packets arrived; see TODO note about about notification tasks
    for (const auto& inputPort : inputPorts)
    {
        EXPECT_EQ(inputPort.getConnection().getPacketCount(), packets.size() + 1);

        // first packet is signal descriptor changed event packet
        ASSERT_NO_THROW(inputPort.getConnection().dequeue().asPtr<IEventPacket>());

        for (std::size_t i = 0; i < packets.size(); ++i)
        {
            EXPECT_EQ(inputPort.getConnection().peek(), packets[i]);
            ASSERT_EQ(inputPort.getConnection().dequeue(), packets[i]);
        }

        EXPECT_EQ(inputPort.getConnection().getPacketCount(), 0u);
    }

    scheduler.stop();
}
