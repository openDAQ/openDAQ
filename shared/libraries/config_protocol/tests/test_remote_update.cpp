#include <opendaq/component_factory.h>
#include <opendaq/context_factory.h>
#include <coreobjects/property_factory.h>
#include <gtest/gtest.h>
#include <coreobjects/property_object_internal_ptr.h>
#include <opendaq/mock/mock_fb_module.h>
#include <opendaq/data_descriptor_factory.h>
#include "test_utils.h"
#include "config_protocol/config_protocol_server.h"
#include "config_protocol/config_protocol_client.h"
#include "config_protocol/config_client_device_impl.h"

using namespace daq;
using namespace daq::config_protocol;

class ConfigRemoteUpdateTest : public testing::Test
{
public:
    void SetUp() override
    {
        referenceDevice = test_utils::createServerDevice();
        setUpDevice(referenceDevice);
        serverDevice = test_utils::createServerDevice();
        server = std::make_unique<ConfigProtocolServer>(serverDevice, std::bind(&ConfigRemoteUpdateTest::serverNotificationReady, this, std::placeholders::_1), nullptr);

        clientContext = NullContext();
        client = std::make_unique<ConfigProtocolClient<ConfigClientDeviceImpl>>(clientContext, std::bind(&ConfigRemoteUpdateTest::sendRequest, this, std::placeholders::_1), nullptr);

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

    // server handling
    void serverNotificationReady(const PacketBuffer& notificationPacket) const
    {
        client->triggerNotificationPacket(notificationPacket);
    }

    // client handling
    PacketBuffer sendRequest(const PacketBuffer& requestPacket) const
    {
        auto replyPacket = server->processRequestAndGetReply(requestPacket);
        return replyPacket;
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
