#include <opendaq/input_port_factory.h>
#include <coretypes/objectptr.h>
#include <gtest/gtest.h>
#include <opendaq/gmock/context.h>
#include <opendaq/gmock/input_port.h>
#include <opendaq/gmock/input_port_notifications.h>
#include <opendaq/gmock/signal.h>
#include <opendaq/deserialize_component_ptr.h>
#include <opendaq/context_factory.h>
#include <opendaq/component_deserialize_context_factory.h>

using namespace daq;
using namespace testing;

class InputPortTest : public Test
{
public:
    InputPortTest()
    {
        inputPort = InputPort(daq::NullContext(), nullptr, "TestPort");
        inputPort.setListener(notifications);
    }

protected:
    MockInputPortNotifications::Strict notifications;
    MockSignal::Strict signal;
    InputPortConfigPtr inputPort;
};

TEST_F(InputPortTest, InitialState)
{
    // Unconnected input ports should have no connection and no signal
    ASSERT_EQ(inputPort.getSignal(), nullptr);
    ASSERT_EQ(inputPort.getConnection(), nullptr);
    ASSERT_EQ(inputPort.getCustomData(), nullptr);
}

TEST_F(InputPortTest, Removed)
{
    const auto errCode = inputPort.asPtr<IRemovable>()->remove();
    ASSERT_EQ(errCode, OPENDAQ_SUCCESS);

    ASSERT_TRUE(inputPort.isRemoved());
}

TEST_F(InputPortTest, AcceptsSignal)
{
    // acceptsSignal() should pass through to the notification object and honor its result
    EXPECT_CALL(notifications.mock(), acceptsSignal(inputPort.getObject(), &signal.mock(), _))
        .WillOnce(DoAll(SetArgPointee<2>(True), Return(OPENDAQ_SUCCESS)))
        .WillOnce(DoAll(SetArgPointee<2>(False), Return(OPENDAQ_SUCCESS)));
    EXPECT_CALL(signal.mock(), isRemoved(_)).WillRepeatedly(DoAll(SetArgPointee<0>(False), Return(OPENDAQ_SUCCESS)));
    EXPECT_TRUE(inputPort.acceptsSignal(signal));
    EXPECT_FALSE(inputPort.acceptsSignal(signal));
}

TEST_F(InputPortTest, ConnectAndDisconnect)
{
    // connect() should work (even if connected() hook returns an error)
    // check object graph for proper bookkeeping
    EXPECT_CALL(notifications.mock(), connected(inputPort.getObject())).WillOnce(Return(OPENDAQ_SUCCESS));
    EXPECT_CALL(signal.mock(), listenerConnected).Times(1);
    EXPECT_CALL(signal.mock(), isRemoved(_)).WillRepeatedly(DoAll(SetArgPointee<0>(False), Return(OPENDAQ_SUCCESS)));
    EXPECT_NO_THROW(inputPort.connect(signal));
    auto connection = inputPort.getConnection();
    EXPECT_EQ(connection, connection);
    EXPECT_EQ(inputPort.getSignal(), *signal);
    EXPECT_EQ(connection.getInputPort(), inputPort);
    EXPECT_EQ(connection.getSignal(), *signal);
    EXPECT_EQ(signal->getConnections().getCount(), 1u);
    EXPECT_EQ(signal->getConnections().getItemAt(0), connection);

    // disconnect() should work (even if disconnected() hook returns an error)
    // check object graph for proper bookkeeping
    EXPECT_CALL(notifications.mock(), disconnected(inputPort.getObject())).WillOnce(Return(OPENDAQ_SUCCESS));
    EXPECT_CALL(signal.mock(), listenerDisconnected(connection.getObject())).Times(1);
    EXPECT_NO_THROW(inputPort.disconnect());
    EXPECT_EQ(inputPort.getConnection(), nullptr);
    EXPECT_EQ(inputPort.getSignal(), nullptr);
    EXPECT_EQ(signal->getConnections().getCount(), 0u);
}

TEST_F(InputPortTest, ChangeCustomData)
{
    inputPort.setCustomData(2);
    ASSERT_EQ(inputPort.getCustomData(), 2);
}

TEST_F(InputPortTest, UnconnectableSignal)
{
    EXPECT_CALL(signal.mock(), isRemoved(_))
        .Times(1)
        .WillOnce(DoAll(
            SetArgPointee<0>(True),
            Return(OPENDAQ_SUCCESS)));

    EXPECT_THROW(inputPort.connect(signal), InvalidStateException);
}

TEST_F(InputPortTest, SwitchToSameThreadNotification)
{
    inputPort.setNotificationMethod(PacketReadyNotification::Scheduler);
    ASSERT_NO_THROW(inputPort.notifyPacketEnqueued(True));
}

TEST_F(InputPortTest, StandardProperties)
{
    const auto name = "foo";
    const auto desc = "bar";
    const auto signal = InputPort(NullContext(), nullptr, "sig");

    signal.setName(name);
    signal.setDescription(desc);

    ASSERT_EQ(signal.getName(), name);
    ASSERT_EQ(signal.getDescription(), desc);
}

TEST_F(InputPortTest, SerializeAndDeserialize)
{
    EXPECT_CALL(notifications.mock(), connected(inputPort.getObject())).WillOnce(Return(OPENDAQ_SUCCESS));
    EXPECT_CALL(signal.mock(), listenerConnected).Times(2);
    EXPECT_CALL(signal.mock(), isRemoved(_)).WillRepeatedly(DoAll(SetArgPointee<0>(False), Return(OPENDAQ_SUCCESS)));
    EXPECT_CALL(signal.mock(), getGlobalId(_)).WillRepeatedly(daq::Get<daq::StringPtr>("sig"));

    inputPort.setName("sig_name");
    inputPort.setDescription("sig_description");

    inputPort.connect(signal);

    const auto serializer = JsonSerializer(True);
    inputPort.serialize(serializer);
    const auto str1 = serializer.getOutput();

    const auto deserializer = JsonDeserializer();
    const auto deserializeContext = ComponentDeserializeContext(daq::NullContext(), nullptr, nullptr, "TestPort");

    const InputPortPtr newInputPort = deserializer.deserialize(str1, deserializeContext, nullptr);

    const auto deserializedSignalId = newInputPort.asPtr<IDeserializeComponent>(true).getDeserializedParameter("signalId");
    ASSERT_EQ(deserializedSignalId, "sig");

    ASSERT_EQ(newInputPort.getName(), inputPort.getName());
    ASSERT_EQ(newInputPort.getDescription(), inputPort.getDescription());

    newInputPort.connect(signal);

    const auto serializer2 = JsonSerializer(True);
    newInputPort.serialize(serializer2);
    const auto str2 = serializer2.getOutput();

    ASSERT_EQ(str1, str2);
}
