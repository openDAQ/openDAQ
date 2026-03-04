#include <opendaq/component.h>
#include <coreobjects/core_event_args.h>
#include <config_protocol/config_client_object_ptr.h>
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <config_protocol/config_protocol_server.h>
#include <config_protocol/config_protocol_client.h>
#include <opendaq/mock/advanced_components_setup_utils.h>
#include <coreobjects/argument_info_factory.h>
#include <coreobjects/callable_info_factory.h>
#include <opendaq/context_factory.h>
#include <config_protocol/config_client_device_impl.h>
#include <opendaq/packet_factory.h>
#include <coreobjects/user_factory.h>
#include <config_protocol/exceptions.h>
#include <testutils/testutils.h>
#include <opendaq/recorder_ptr.h>

using namespace daq;
using namespace config_protocol;
using namespace testing;
using namespace std::placeholders;

class ConfigProtocolIntegrationTestNonPublic : public Test
{
public:
    void SetUp() override
    {
        auto anonymousUser = User("", "");

        // Only public signals
        serverDevice = test_utils::createTestDevice("root_dev", false, false);
        serverDevice.asPtr<IPropertyObjectInternal>().enableCoreEventTrigger();
        server = std::make_unique<ConfigProtocolServer>(
            serverDevice,
            std::bind(&ConfigProtocolIntegrationTestNonPublic::serverNotificationReady, this, std::placeholders::_1),
            anonymousUser,
            ClientType::Control,
            test_utils::dummyExtSigFolder(serverDevice.getContext()));

        clientContext = NullContext();
        client = std::make_unique<ConfigProtocolClient<ConfigClientDeviceImpl>>(
            clientContext,
            std::bind(&ConfigProtocolIntegrationTestNonPublic::sendRequestAndGetReply, this, std::placeholders::_1),
            std::bind(&ConfigProtocolIntegrationTestNonPublic::sendNoReplyRequest, this, std::placeholders::_1),
            nullptr,
            nullptr,
            nullptr);
        clientDevice = client->connect();
        clientDevice.asPtr<IPropertyObjectInternal>().enableCoreEventTrigger();
    }

    static StringPtr serializeComponent(const ComponentPtr& component)
    {
        const auto serializer = JsonSerializer(True);
        component.serialize(serializer);
        const auto str = serializer.getOutput();
        return str;
    }

    // client handling
    PacketBuffer sendRequestAndGetReply(const PacketBuffer& requestPacket) const
    {
        auto replyPacket = server->processRequestAndGetReply(requestPacket);
        return replyPacket;
    }

    void sendNoReplyRequest(const PacketBuffer& requestPacket) const
    {
        // callback is not expected to be called within this test group
        assert(false);
        server->processNoReplyRequest(requestPacket);
    }

    void serverNotificationReady(const PacketBuffer& notificationPacket) const
    {
        client->triggerNotificationPacket(notificationPacket);
    }

protected:
    DevicePtr serverDevice;
    DevicePtr clientDevice;
    std::unique_ptr<ConfigProtocolServer> server;
    std::unique_ptr<ConfigProtocolClient<ConfigClientDeviceImpl>> client;
    ContextPtr clientContext;
    BaseObjectPtr notificationObj;
};

TEST_F(ConfigProtocolIntegrationTestNonPublic, Connect)
{
    const auto serverDeviceSerialized = serializeComponent(serverDevice);
    const auto clientDeviceSerialized = serializeComponent(clientDevice);

    // Client side component tree does not include non-public signals (and input ports), therefore
    // the serialized JSON strings must not be equal.
    ASSERT_NE(serverDeviceSerialized, clientDeviceSerialized);
}

TEST_F(ConfigProtocolIntegrationTestNonPublic, FilterNonPublicComponents)
{
    auto childDevices = serverDevice.getDevices(search::LocalId("mock_phys_dev"));
    ASSERT_EQ(childDevices.getCount(), 1);
    auto signals = childDevices[0].getSignals(search::LocalId("devicetimesigprivate"));
    ASSERT_EQ(signals.getCount(), 1);
    ASSERT_EQ(signals[0].getPublic(), False);

    auto childDevicesClient = clientDevice.getDevices(search::LocalId("mock_phys_dev"));
    ASSERT_EQ(childDevicesClient.getCount(), 1);
    auto signalsClient = childDevicesClient[0].getSignals(search::LocalId("devicetimesigprivate"));
    ASSERT_EQ(signalsClient.getCount(), 0);
}