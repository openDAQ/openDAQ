#include <config_protocol/config_client_device_impl.h>
#include <config_protocol/config_protocol_client.h>
#include <config_protocol/config_protocol_server.h>
#include <coreobjects/argument_info_factory.h>
#include <coreobjects/callable_info_factory.h>
#include <coreobjects/user_factory.h>
#include <gmock/gmock.h>
#include <opendaq/device_private_ptr.h>
#include <opendaq/device_ptr.h>
#include "test_utils.h"

using namespace daq;
using namespace testing;
using namespace config_protocol;

class ConfigProtocolViewOnlyClientTest : public Test
{
public:

    DevicePtr createDevice()
    {
        return daq::config_protocol::test_utils::createTestDevice();
    }

    void setupServerAndClient(ClientType connectionType)
    {
        auto device = createDevice();
        setupServerAndClient(device, connectionType);
    }

    void setupServerAndClient(const DevicePtr& device, ClientType connectionType)
    {
        serverDevice = device;
        serverDevice.asPtrOrNull<IPropertyObjectInternal>().enableCoreEventTrigger();
        server = std::make_unique<ConfigProtocolServer>(
            serverDevice,
            std::bind(&ConfigProtocolViewOnlyClientTest::serverNotificationReady, this, std::placeholders::_1),
            UserTomaz,
            connectionType);

        auto clientContext = NullContext();
        client = std::make_unique<ConfigProtocolClient<ConfigClientDeviceImpl>>(
            clientContext,
            std::bind(&ConfigProtocolViewOnlyClientTest::sendRequestAndGetReply, this, std::placeholders::_1),
            std::bind(&ConfigProtocolViewOnlyClientTest::sendNoReplyRequest, this, std::placeholders::_1),
            nullptr,
            nullptr);

        clientDevice = client->connect();
        clientDevice.asPtr<IPropertyObjectInternal>().enableCoreEventTrigger();
    }

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

    PropertyObjectPtr createAdvancedObject()
    {
        const auto countFunc = Function([this]() { return ++counter; });

        const auto countFuncProp =
            FunctionPropertyBuilder("CountProp", FunctionInfo(ctInt, List<IArgumentInfo>())).setReadOnly(false).build();

        auto advancedObject = PropertyObject();
        advancedObject.addProperty(countFuncProp);
        advancedObject.setPropertyValue("CountProp", countFunc);

        advancedObject.addProperty(StringProperty("StringProp", "-"));
        advancedObject.setPropertyValue("StringProp", "Hello World!");

        return advancedObject;
    }

protected:
    DevicePtr serverDevice;
    DevicePtr clientDevice;
    Int counter = 0;

private:
    const UserPtr UserTomaz = User("tomaz", "tomaz");
    std::unique_ptr<ConfigProtocolServer> server;
    std::unique_ptr<ConfigProtocolClient<ConfigClientDeviceImpl>> client;
};

TEST_F(ConfigProtocolViewOnlyClientTest, SetPropertyValue)
{
    setupServerAndClient(ClientType::ViewOnly);

    ASSERT_THROW(clientDevice.setPropertyValue("StrProp", "Hello"), AccessDeniedException);
    ASSERT_NE(clientDevice.getPropertyValue("StrProp"), "Hello");

    setupServerAndClient(ClientType::Control);

    clientDevice.setPropertyValue("StrProp", "Hello");
    ASSERT_EQ(clientDevice.getPropertyValue("StrProp"), "Hello");
}

TEST_F(ConfigProtocolViewOnlyClientTest, SetProtectedPropertyValue)
{
    setupServerAndClient(ClientType::ViewOnly);

    ASSERT_THROW(clientDevice.asPtr<IPropertyObjectProtected>(true).setProtectedPropertyValue("StrProp", "Hello"), AccessDeniedException);
    ASSERT_NE(clientDevice.getPropertyValue("StrProp"), "Hello");

    setupServerAndClient(ClientType::Control);

    clientDevice.asPtr<IPropertyObjectProtected>(true).setProtectedPropertyValue("StrProp", "Hello");
    ASSERT_EQ(clientDevice.getPropertyValue("StrProp"), "Hello");
}

TEST_F(ConfigProtocolViewOnlyClientTest, ClearValue)
{
    setupServerAndClient(ClientType::ViewOnly);

    serverDevice.setPropertyValue("StrProp", "Hello");
    ASSERT_THROW(clientDevice.clearPropertyValue("StrProp"), AccessDeniedException);
    ASSERT_EQ(clientDevice.getPropertyValue("StrProp"), "Hello");

    setupServerAndClient(ClientType::Control);

    serverDevice.setPropertyValue("StrProp", "Hello");
    clientDevice.clearPropertyValue("StrProp");
    ASSERT_EQ(clientDevice.getPropertyValue("StrProp"), "-");
}

TEST_F(ConfigProtocolViewOnlyClientTest, CallProperty)
{
    auto device = createDevice();
    Int counter = 0;

    {
        const auto func = Function([&counter]() { return ++counter; });

        const auto funcProp = FunctionPropertyBuilder("CountProp", FunctionInfo(ctInt)).setReadOnly(false).build();

        device.addProperty(funcProp);
        device.setPropertyValue("CountProp", func);
    }

    setupServerAndClient(device, ClientType::ViewOnly);

    ASSERT_EQ(counter, 0);
    ASSERT_THROW(clientDevice.getPropertyValue("CountProp").call(), AccessDeniedException);

    setupServerAndClient(device, ClientType::Control);

    ASSERT_EQ(counter, 0);
    auto result = clientDevice.getPropertyValue("CountProp").call();
    ASSERT_EQ(result, 1);
}

TEST_F(ConfigProtocolViewOnlyClientTest, CallConstProperty)
{
    auto device = createDevice();

    {
        const auto func = Function([](Int a, Int b) { return a + b; });

        const auto funcProp =
            FunctionPropertyBuilder("SumProp",
                                    FunctionInfo(ctInt, List<IArgumentInfo>(ArgumentInfo("A", ctInt), ArgumentInfo("B", ctInt)), true))
                .setReadOnly(false)
                .build();

        device.addProperty(funcProp);
        device.setPropertyValue("SumProp", func);
    }

    setupServerAndClient(device, ClientType::ViewOnly);

    auto result = clientDevice.getPropertyValue("SumProp").call(1, 2);
    ASSERT_EQ(result, 3);
}

TEST_F(ConfigProtocolViewOnlyClientTest, BeginEndUpdate)
{
    setupServerAndClient(ClientType::ViewOnly);

    ASSERT_THROW(clientDevice.beginUpdate(), AccessDeniedException);
    ASSERT_THROW(clientDevice.endUpdate(), AccessDeniedException);

    setupServerAndClient(ClientType::Control);

    clientDevice.beginUpdate();
    clientDevice.setPropertyValue("StrProp", "SomeValue");
    ASSERT_EQ(clientDevice.getPropertyValue("StrProp"), "-");
    ASSERT_EQ(serverDevice.getPropertyValue("StrProp"), "-");
    clientDevice.endUpdate();
    ASSERT_EQ(clientDevice.getPropertyValue("StrProp"), "SomeValue");
    ASSERT_EQ(serverDevice.getPropertyValue("StrProp"), "SomeValue");
}

TEST_F(ConfigProtocolViewOnlyClientTest, SetAttributeValue)
{
    setupServerAndClient(ClientType::ViewOnly);

    ASSERT_THROW(clientDevice.setName("Name"), AccessDeniedException);

    setupServerAndClient(ClientType::Control);

    clientDevice.setName("NameChanged");
    ASSERT_EQ(clientDevice.getName(), "NameChanged");
}

TEST_F(ConfigProtocolViewOnlyClientTest, Update)
{
    setupServerAndClient(ClientType::ViewOnly);

    {
        auto updatableDevice = clientDevice.getDevices()[0];
        const auto serializer = JsonSerializer();
        updatableDevice.serialize(serializer);
        const auto str = serializer.getOutput();
        const auto deserializer = JsonDeserializer();
        ASSERT_THROW(deserializer.update(updatableDevice, str), AccessDeniedException);
    }

    setupServerAndClient(ClientType::Control);

    {
        auto updatableDevice = clientDevice.getDevices()[0];
        const auto serializer = JsonSerializer();
        updatableDevice.serialize(serializer);
        const auto str = serializer.getOutput();
        const auto deserializer = JsonDeserializer();
        ASSERT_NO_THROW(deserializer.update(updatableDevice, str));
    }
}

TEST_F(ConfigProtocolViewOnlyClientTest, AddFunctionBlock)
{
    const auto config = PropertyObject();
    config.addProperty(StringPropertyBuilder("Param", "Value").build());

    setupServerAndClient(ClientType::ViewOnly);

    {
        const auto clientSubDevice = clientDevice.getDevices()[0];
        ASSERT_THROW(clientSubDevice.addFunctionBlock("mockfb1", config), AccessDeniedException);
    }

    setupServerAndClient(ClientType::Control);

    {
        const auto clientSubDevice = clientDevice.getDevices()[0];
        const auto fb = clientSubDevice.addFunctionBlock("mockfb1", config);
        ASSERT_TRUE(fb.assigned());
    }
}

TEST_F(ConfigProtocolViewOnlyClientTest, RemoveFunctionBlock)
{
    setupServerAndClient(ClientType::ViewOnly);

    {
        auto subDevice = clientDevice.getDevices()[0];
        ASSERT_EQ(subDevice.getFunctionBlocks().getCount(), 1u);
        auto fb = subDevice.getFunctionBlocks()[0];
        ASSERT_THROW(subDevice.removeFunctionBlock(fb), AccessDeniedException);
    }

    setupServerAndClient(ClientType::Control);

    {
        auto subDevice = clientDevice.getDevices()[0];
        ASSERT_EQ(subDevice.getFunctionBlocks().getCount(), 1u);
        auto fb = subDevice.getFunctionBlocks()[0];
        subDevice.removeFunctionBlock(fb);
        ASSERT_EQ(subDevice.getFunctionBlocks().getCount(), 0u);
    }
}

TEST_F(ConfigProtocolViewOnlyClientTest, ConnectSignal)
{
    auto device = createDevice();

    auto serverInputPort = device.getDevices()[0].getFunctionBlocks()[0].getInputPorts()[0];
    serverInputPort.disconnect();

    setupServerAndClient(device, ClientType::ViewOnly);

    {
        auto inputPort = clientDevice.getDevices()[0].getFunctionBlocks()[0].getInputPorts()[0];
        auto signal = clientDevice.getDevices()[0].getSignals()[0];
        ASSERT_THROW(inputPort.connect(signal), AccessDeniedException);
    }

    setupServerAndClient(device, ClientType::Control);

    {
        auto inputPort = clientDevice.getDevices()[0].getFunctionBlocks()[0].getInputPorts()[0];
        auto signal = clientDevice.getDevices()[0].getSignals()[0];
        inputPort.connect(signal);
        ASSERT_EQ(inputPort.getSignal(), signal);
    }
}

TEST_F(ConfigProtocolViewOnlyClientTest, DisconnectSignal)
{
    setupServerAndClient(ClientType::ViewOnly);

    {
        auto inputPort = clientDevice.getDevices()[0].getFunctionBlocks()[0].getInputPorts()[0];
        ASSERT_THROW(inputPort.disconnect(), AccessDeniedException);
    }

    setupServerAndClient(ClientType::Control);

    {
        auto inputPort = clientDevice.getDevices()[0].getFunctionBlocks()[0].getInputPorts()[0];
        ASSERT_TRUE(inputPort.getSignal().assigned());
        inputPort.disconnect();
        ASSERT_FALSE(inputPort.getSignal().assigned());
    }
}

TEST_F(ConfigProtocolViewOnlyClientTest, NestedSetValue)
{
    auto device = createDevice();

    auto advanced = createAdvancedObject();
    device.addProperty(ObjectProperty("Advanced", advanced));

    setupServerAndClient(device, ClientType::ViewOnly);

    ASSERT_THROW(clientDevice.setPropertyValue("Advanced.StringProp", "Changed"), AccessDeniedException);

    setupServerAndClient(device, ClientType::Control);

    clientDevice.setPropertyValue("Advanced.StringProp", "Changed");
    auto value = clientDevice.getPropertyValue("Advanced.StringProp");
    ASSERT_EQ(value, "Changed");
}

TEST_F(ConfigProtocolViewOnlyClientTest, NestedClearValue)
{
    auto device = createDevice();

    auto advanced = createAdvancedObject();
    device.addProperty(ObjectProperty("Advanced", advanced));

    setupServerAndClient(device, ClientType::ViewOnly);

    ASSERT_THROW(clientDevice.clearPropertyValue("Advanced.StringProp"), AccessDeniedException);

    setupServerAndClient(device, ClientType::Control);

    clientDevice.clearPropertyValue("Advanced.StringProp");
    auto value = clientDevice.getPropertyValue("Advanced.StringProp");
    ASSERT_EQ(value, "-");
}

TEST_F(ConfigProtocolViewOnlyClientTest, NestedCallProperty)
{
    auto device = createDevice();

    auto advanced = createAdvancedObject();
    device.addProperty(ObjectProperty("Advanced", advanced));

    setupServerAndClient(device, ClientType::ViewOnly);

    ASSERT_THROW(clientDevice.getPropertyValue("Advanced.CountProp").call(), AccessDeniedException);

    setupServerAndClient(device, ClientType::Control);

    ASSERT_EQ(counter, 0);
    auto result = clientDevice.getPropertyValue("Advanced.CountProp").call();
    ASSERT_EQ(result, 1);
}
