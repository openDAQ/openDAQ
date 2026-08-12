#include <opendaq/component_factory.h>
#include <opendaq/context_factory.h>
#include <coreobjects/property_factory.h>
#include <gtest/gtest.h>
#include <coreobjects/property_object_internal_ptr.h>
#include <opendaq/mock/mock_fb_module.h>
#include <opendaq/data_descriptor_factory.h>
#include <opendaq/mock/advanced_components_setup_utils.h>
#include <config_protocol/config_protocol_server.h>
#include <config_protocol/config_protocol_client.h>
#include <config_protocol/config_client_device_impl.h>
#include <coreobjects/user_factory.h>

using namespace daq;
using namespace daq::config_protocol;

class ConfigRemoteUpdateTest : public testing::Test
{
public:
    void SetUp() override
    {
        const auto anonymousUser = User("", "");

        referenceDevice = test_utils::createTestDevice("root_dev", true);
        setUpDevice(referenceDevice);
        serverDevice = test_utils::createTestDevice("root_dev", true);
        server =
            std::make_unique<ConfigProtocolServer>(serverDevice,
                                                   std::bind(&ConfigRemoteUpdateTest::serverNotificationReady, this, std::placeholders::_1),
                                                   anonymousUser,
                                                   ClientType::Control,
                                                   test_utils::dummyExtSigFolder(serverDevice.getContext()));

        clientContext = NullContext();
        client =
            std::make_unique<ConfigProtocolClient<ConfigClientDeviceImpl>>(
                clientContext,
                std::bind(&ConfigRemoteUpdateTest::sendRequestAndGetReply, this, std::placeholders::_1),
                std::bind(&ConfigRemoteUpdateTest::sendNoReplyRequest, this, std::placeholders::_1),
                nullptr,
                nullptr,
                nullptr
            );
        clientDevice = client->connect();
        clientDevice.asPtr<IPropertyObjectInternal>().enableCoreEventTrigger();
    }

protected:
    DevicePtr serverDevice;
    DevicePtr clientDevice;
    DevicePtr referenceDevice;
    std::unique_ptr<ConfigProtocolServer> server;
    std::unique_ptr<ConfigProtocolClient<ConfigClientDeviceImpl>> client;
    ContextPtr clientContext;
    BaseObjectPtr notificationObj;
    bool muteNotifications = false;

    // server handling
    void serverNotificationReady(const PacketBuffer& notificationPacket) const
    {
        if (!muteNotifications)
            client->triggerNotificationPacket(notificationPacket);
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

    void setUpDevice(DevicePtr& device)
    {
        device.setPropertyValue("MockString", "new_string");
        device.getCustomComponents()[0].setPropertyValue("Ratio", Ratio(1, 2000));
        const auto dev = device.getDevices()[0];
        dev.getChannels()[0].setPropertyValue("TestStringProp", "test1");
        dev.getFunctionBlocks()[0].setPropertyValue("MockString", "new_string");
        dev.getFunctionBlocks()[0].getInputPorts()[0].connect(dev.getSignals()[0]);

        device.setPropertyValue("ObjectProperty.child1.child1_2.child1_2_1.String", "new_string");
        device.setPropertyValue("ObjectProperty.child1.child1_1.Float", 2.1);
        device.setPropertyValue("ObjectProperty.child1.child1_2.Int", 2);
        device.setPropertyValue("ObjectProperty.child2.child2_1.Ratio", Ratio(1, 5));
    }

    StringPtr serializeHelper(const SerializablePtr& serializable)
    {
        const auto serializer = JsonSerializer();
        serializable.serialize(serializer);
        const auto str = serializer.getOutput();
        return str;
    }

    void updateHelper(const UpdatablePtr& updatable, const StringPtr& str)
    {
        const auto deserializer = JsonDeserializer();
        deserializer.update(updatable, str);
    }
};

TEST_F(ConfigRemoteUpdateTest, TestDeviceUpdate1)
{
    const auto str = serializeHelper(referenceDevice);

    clientContext.getOnCoreEvent() +=
        [&](const ComponentPtr& comp, const CoreEventArgsPtr& args)
        {
            ASSERT_EQ(serializeHelper(comp), str);
            ASSERT_EQ(args.getEventId(), static_cast<Int>(CoreEventId::ComponentUpdateEnd));
        };

    updateHelper(clientDevice, str);
    ASSERT_EQ(serializeHelper(serverDevice), str);
    ASSERT_EQ(serializeHelper(clientDevice), str);
}

TEST_F(ConfigRemoteUpdateTest, TestDeviceUpdate2)
{
    const auto str = serializeHelper(referenceDevice.getDevices()[0]);
    updateHelper(clientDevice.getDevices()[0], str);
    ASSERT_EQ(serializeHelper(serverDevice.getDevices()[0]), str);
    ASSERT_EQ(serializeHelper(clientDevice.getDevices()[0]), str);
}

TEST_F(ConfigRemoteUpdateTest, TestFbUpdate)
{
    const auto str = serializeHelper(referenceDevice.getDevices()[0].getFunctionBlocks()[0]);
    updateHelper(clientDevice.getDevices()[0].getFunctionBlocks()[0], str);
    ASSERT_EQ(serializeHelper(serverDevice.getDevices()[0].getFunctionBlocks()[0]), str);
    ASSERT_EQ(serializeHelper(clientDevice.getDevices()[0].getFunctionBlocks()[0]), str);
}

TEST_F(ConfigRemoteUpdateTest, TestNestedPropertyObjectUpdate1)
{
    const auto str = serializeHelper(referenceDevice.getPropertyValue("ObjectProperty"));
    updateHelper(clientDevice.getPropertyValue("ObjectProperty"), str);
    ASSERT_EQ(serializeHelper(serverDevice.getPropertyValue("ObjectProperty")), str);
    ASSERT_EQ(serializeHelper(clientDevice.getPropertyValue("ObjectProperty")), str);
}

TEST_F(ConfigRemoteUpdateTest, TestNestedPropertyObjectUpdate2)
{
    const auto str = serializeHelper(referenceDevice.getPropertyValue("ObjectProperty.child1"));
    updateHelper(clientDevice.getPropertyValue("ObjectProperty.child1"), str);
    ASSERT_EQ(serializeHelper(serverDevice.getPropertyValue("ObjectProperty.child1")), str);
    ASSERT_EQ(serializeHelper(clientDevice.getPropertyValue("ObjectProperty.child1")), str);
}

TEST_F(ConfigRemoteUpdateTest, TestNestedPropertyObjectUpdate3)
{
    const auto str = serializeHelper(referenceDevice.getPropertyValue("ObjectProperty.child1.child1_2"));
    updateHelper(clientDevice.getPropertyValue("ObjectProperty.child1.child1_2"), str);
    ASSERT_EQ(serializeHelper(serverDevice.getPropertyValue("ObjectProperty.child1.child1_2")), str);
    ASSERT_EQ(serializeHelper(clientDevice.getPropertyValue("ObjectProperty.child1.child1_2")), str);
}

TEST_F(ConfigRemoteUpdateTest, TestClientSideSerializedString)
{
    int callCount = 0;   
    clientContext.getOnCoreEvent() +=
        [&](const ComponentPtr& comp, const CoreEventArgsPtr& args)
        {
            ASSERT_EQ(args.getEventId(), static_cast<Int>(CoreEventId::ComponentUpdateEnd));
            callCount++;
        };

    const auto strReference = serializeHelper(referenceDevice);
    const auto strDefault = serializeHelper(serverDevice);

    updateHelper(clientDevice, strReference);
    const auto strClient = serializeHelper(clientDevice);

    updateHelper(serverDevice, strDefault);
    ASSERT_EQ(serializeHelper(serverDevice), strDefault);
    ASSERT_EQ(serializeHelper(clientDevice), strDefault);

    updateHelper(clientDevice, strClient);
    ASSERT_EQ(serializeHelper(serverDevice), strClient);
    ASSERT_EQ(serializeHelper(clientDevice), strClient);

    ASSERT_EQ(callCount, 3);
}

TEST_F(ConfigRemoteUpdateTest, TestRemoveStaticComponents)
{
    auto dev = clientDevice.getDevices()[1];
    ASSERT_THROW(clientDevice.removeDevice(dev), InvalidOperationException);

    auto fb = clientDevice.getFunctionBlocks()[0];
    ASSERT_THROW(clientDevice.removeFunctionBlock(fb), InvalidOperationException);
}

TEST_F(ConfigRemoteUpdateTest, UpdateReplacesChangedProperty)
{
    auto serverComponent = serverDevice.getCustomComponents()[0];
    auto clientComponent = clientDevice.getCustomComponents()[0];

    // Replace a property on the server while notifications are muted; the client keeps the old metadata
    muteNotifications = true;
    serverComponent.removeProperty("Ratio");
    serverComponent.addProperty(RatioPropertyBuilder("Ratio", Ratio(1, 10)).setDescription("changed").build());
    muteNotifications = false;

    ASSERT_EQ(clientComponent.getProperty("Ratio").getDefaultValue(), Ratio(1, 1000));

    // A server-side update re-syncs the client via remoteUpdate
    updateHelper(serverComponent, serializeHelper(serverComponent));

    const auto clientProp = clientComponent.getProperty("Ratio");
    ASSERT_EQ(clientProp.getDefaultValue(), Ratio(1, 10));
    ASSERT_EQ(clientProp.getDescription(), "changed");
    ASSERT_EQ(clientComponent.getPropertyValue("Ratio"), Ratio(1, 10));

    // The property order matches the server after the replacement
    auto serverNames = List<IString>();
    for (const auto& prop : serverComponent.getAllProperties())
        serverNames.pushBack(prop.getName());
    auto clientNames = List<IString>();
    for (const auto& prop : clientComponent.getAllProperties())
        clientNames.pushBack(prop.getName());
    ASSERT_EQ(serverNames, clientNames);
}

TEST_F(ConfigRemoteUpdateTest, UpdateKeepsUnchangedProperties)
{
    auto serverComponent = serverDevice.getCustomComponents()[0];
    auto clientComponent = clientDevice.getCustomComponents()[0];

    const auto clientPropBefore = clientComponent.getProperty("Ratio");
    updateHelper(serverComponent, serializeHelper(serverComponent));

    // Unchanged properties are not replaced
    ASSERT_EQ(clientComponent.getProperty("Ratio").getObject(), clientPropBefore.getObject());
}

TEST_F(ConfigRemoteUpdateTest, UpdateChannelActive)
{
    auto serverChannel = serverDevice.getChannelsRecursive()[0];
    auto clientChannel = clientDevice.getChannelsRecursive()[0];

    ASSERT_TRUE(serverChannel.getActive());
    ASSERT_TRUE(clientChannel.getActive());
    const auto str = serializeHelper(serverChannel);
    
    serverChannel.setActive(false);

    ASSERT_FALSE(serverChannel.getActive());
    ASSERT_FALSE(clientChannel.getActive());

    updateHelper(serverChannel, str);
    
    ASSERT_TRUE(serverChannel.getActive());
    ASSERT_TRUE(clientChannel.getActive());
}

TEST_F(ConfigRemoteUpdateTest, UpdateHierarchicalActive)
{
    auto serverChannel = serverDevice.getChannelsRecursive()[0];
    auto clientChannel = clientDevice.getChannelsRecursive()[0];

    ASSERT_TRUE(serverChannel.getActive());
    ASSERT_TRUE(clientChannel.getActive());
    const auto str = serializeHelper(serverDevice);

    serverDevice.setActive(false);

    ASSERT_FALSE(serverChannel.getActive());
    ASSERT_FALSE(clientChannel.getActive());

    updateHelper(serverDevice, str);

    ASSERT_TRUE(serverChannel.getActive());
    ASSERT_TRUE(clientChannel.getActive());
}
