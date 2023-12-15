#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <config_protocol/config_protocol_server.h>
#include <config_protocol/config_protocol_client.h>
#include <opendaq/device_ptr.h>
#include <opendaq/gmock/device.h>
#include <opendaq/gmock/component.h>

#include "config_packet_transmission.h"

using namespace daq;
using namespace config_protocol;
using namespace testing;

class MockComponentFinder: public IComponentFinder
{
public:
    MOCK_METHOD(ComponentPtr, findComponent, (const std::string&), (override));
};

class ConfigProtocolTest : public Test
{
public:
    ConfigProtocolTest()
        : Test()
        , server(device,
                 std::bind(&ConfigProtocolTest::serverNotificationReady, this, std::placeholders::_1))
        , client(std::bind(&ConfigProtocolTest::sendRequest, this, std::placeholders::_1),
                 std::bind(&ConfigProtocolTest::onServerNotificationReceived, this, std::placeholders::_1))
    {
    }

    void SetUp() override
    {
        std::unique_ptr<IComponentFinder> m = std::make_unique<MockComponentFinder>();
        server.setComponentFinder(m);
    }

protected:

    MockDevice::Strict device;
    ConfigProtocolServer server;
    ConfigProtocolClient client;
    ConfigPacketTransmission transmission{};
    BaseObjectPtr notificationObj;

    // server handling
    void serverNotificationReady(const PacketBuffer& notificationPacket)
    {
        client.triggerNotificationPacket(notificationPacket);
    }


    // client handling 
    PacketBuffer sendRequest(const PacketBuffer& requestPacket)
    {
        auto replyPacket = server.processRequestAndGetReply(requestPacket);
        return replyPacket;
    }

    bool onServerNotificationReceived(const BaseObjectPtr& obj)
    {
        notificationObj = obj;
        return false;
    }

    MockComponentFinder& getMockComponentFinder()
    {
        const auto& componentFinder = server.getComponentFinder();
        return *dynamic_cast<MockComponentFinder*>(componentFinder.get());
    }
};

TEST_F(ConfigProtocolTest, Connect)
{
    client.connect();
}

TEST_F(ConfigProtocolTest, ServerNotification)
{
    auto dict = Dict<IString, IBaseObject>();
    dict.set("key", "value");
    server.sendNotification(dict);
    ASSERT_EQ(notificationObj, dict);
}

TEST_F(ConfigProtocolTest, SetPropertyValueComponentNotFound)
{
    EXPECT_CALL(getMockComponentFinder(), findComponent(_)).WillOnce(Return(nullptr));
    ASSERT_THROW(client.getClientComm()->setPropertyValue("/dev/comp/test", "PropName", "PropValue"), NotFoundException);
}

TEST_F(ConfigProtocolTest, SetPropertyValueComponent)
{
    MockComponent::Strict component;
    component->addProperty(StringPropertyBuilder("PropName", "-").build());

    EXPECT_CALL(getMockComponentFinder(), findComponent(_)).WillOnce(Return(component));

    client.getClientComm()->setPropertyValue("/dev/comp/test", "PropName", "PropValue");

    ASSERT_EQ(component->getPropertyValue("PropName"), "PropValue");
}

TEST_F(ConfigProtocolTest, SetPropertyValueRoot)
{
    device->addProperty(StringPropertyBuilder("PropName", "-").build());

    client.getClientComm()->setPropertyValue("//root", "PropName", "PropValue");

    ASSERT_EQ(device->getPropertyValue("PropName"), "PropValue");
}
