#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include "test_utils.h"
#include <opendaq/instance_factory.h>
#include <config_protocol/config_protocol_server.h>
#include <config_protocol/config_protocol_client.h>
#include <config_protocol/config_client_device_impl.h>
#include <coreobjects/user_factory.h>
#include <coreobjects/callable_info_factory.h>
#include <coreobjects/argument_info_factory.h>
#include <opendaq/device_private_ptr.h>


using namespace daq;
using namespace config_protocol;
using namespace testing;

class ConfigProtocolDeviceLockingTest : public Test
{
public:
    const UserPtr UserTomaz = User("tomaz", "tomaz");
    const UserPtr UserJure = User("jure", "jure");

    DevicePtr createDevice()
    {
        return test_utils::createTestDevice();
    }

    void setupServerAndClient(const DevicePtr& device, const UserPtr& user)
    {
        serverDevice = device;
        serverDevice.asPtrOrNull<IPropertyObjectInternal>().enableCoreEventTrigger();
        server = std::make_unique<ConfigProtocolServer>(
            serverDevice, std::bind(&ConfigProtocolDeviceLockingTest::serverNotificationReady, this, std::placeholders::_1), user);

        auto clientContext = NullContext();
        client = std::make_unique<ConfigProtocolClient<ConfigClientDeviceImpl>>(
            clientContext,
            std::bind(&ConfigProtocolDeviceLockingTest::sendRequestAndGetReply, this, std::placeholders::_1),
            std::bind(&ConfigProtocolDeviceLockingTest::sendNoReplyRequest, this, std::placeholders::_1),
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
        const auto func = Function([](Int a, Int b) { return a + b; });

        const auto funcProp =
            FunctionPropertyBuilder("SumProp", FunctionInfo(ctInt, List<IArgumentInfo>(ArgumentInfo("A", ctInt), ArgumentInfo("B", ctInt))))
                .setReadOnly(false)
                .build();

        auto advancedObject = PropertyObject();

        advancedObject.addProperty(funcProp);
        advancedObject.setPropertyValue("SumProp", func);

        advancedObject.addProperty(StringProperty("StringProp", "-"));
        advancedObject.setPropertyValue("StringProp", "Hello World!");

        return advancedObject;
    }

protected:
    DevicePtr serverDevice;
    DevicePtr clientDevice;

private:
    std::unique_ptr<ConfigProtocolServer> server;
    std::unique_ptr<ConfigProtocolClient<ConfigClientDeviceImpl>> client;
};


TEST_F(ConfigProtocolDeviceLockingTest, LockRecursive)
{
    auto device = createDevice();

    ASSERT_FALSE(device.isLocked());

    auto subDevices = device.getDevices(search::Recursive(search::Any()));
    ASSERT_EQ(subDevices.getCount(), 2u);
    ASSERT_FALSE(subDevices[0].isLocked());
    ASSERT_FALSE(subDevices[1].isLocked());

    device.asPtr<IDevicePrivate>().lock(UserTomaz);
    ASSERT_TRUE(device.isLocked());
    ASSERT_TRUE(subDevices[0].isLocked());
    ASSERT_TRUE(subDevices[1].isLocked());
}

TEST_F(ConfigProtocolDeviceLockingTest, UnlockRecursive)
{
    auto device = createDevice();

    device.asPtr<IDevicePrivate>().lock(UserTomaz);
    ASSERT_TRUE(device.isLocked());

    auto subDevices = device.getDevices(search::Recursive(search::Any()));
    ASSERT_EQ(subDevices.getCount(), 2u);
    ASSERT_TRUE(subDevices[0].isLocked());
    ASSERT_TRUE(subDevices[1].isLocked());

    device.asPtr<IDevicePrivate>().unlock(UserTomaz);
    ASSERT_FALSE(device.isLocked());
    ASSERT_FALSE(subDevices[0].isLocked());
    ASSERT_FALSE(subDevices[1].isLocked());
}

TEST_F(ConfigProtocolDeviceLockingTest, SetPropertyValue)
{
    auto device = createDevice();
    setupServerAndClient(device, UserTomaz);

    clientDevice.lock();

    ASSERT_THROW(clientDevice.getChannels()[0].setPropertyValue("StrProp", "SomeValue"), DeviceLockedException);

    clientDevice.unlock();

    clientDevice.getChannels()[0].setPropertyValue("StrProp", "SomeValue");
    auto value = clientDevice.getChannels()[0].getPropertyValue("StrProp");
    ASSERT_EQ(value, "SomeValue");
}

TEST_F(ConfigProtocolDeviceLockingTest, SetPropertyValueLockedOnServer)
{
    auto device = createDevice();
    device.lock();
    setupServerAndClient(device, UserTomaz);

    ASSERT_THROW(clientDevice.getChannels()[0].setPropertyValue("StrProp", "SomeValue"), DeviceLockedException);

    clientDevice.unlock();

    clientDevice.getChannels()[0].setPropertyValue("StrProp", "SomeValue");
    auto value = clientDevice.getChannels()[0].getPropertyValue("StrProp");
    ASSERT_EQ(value, "SomeValue");
}

TEST_F(ConfigProtocolDeviceLockingTest, SetPropertyValueLockedWithDifferentUser)
{
    auto device = createDevice();
    device.asPtr<IDevicePrivate>().lock(UserJure);
    setupServerAndClient(device, UserTomaz);

    ASSERT_THROW(clientDevice.getChannels()[0].setPropertyValue("StrProp", "SomeValue"), DeviceLockedException);
    ASSERT_THROW(clientDevice.unlock(), AccessDeniedException);
    ASSERT_THROW(clientDevice.asPtr<IDevicePrivate>().unlock(UserJure), AccessDeniedException);

    device.asPtr<IDevicePrivate>().unlock(UserJure);

    clientDevice.getChannels()[0].setPropertyValue("StrProp", "SomeValue");
    auto value = clientDevice.getChannels()[0].getPropertyValue("StrProp");
    ASSERT_EQ(value, "SomeValue");
}


TEST_F(ConfigProtocolDeviceLockingTest, SetProtectedPropertyValue)
{
    auto device = createDevice();
    setupServerAndClient(device, UserTomaz);

    clientDevice.lock();

    ASSERT_THROW(
        clientDevice.getChannels()[0].asPtr<IPropertyObjectProtected>(true).setProtectedPropertyValue("StrPropProtected", "SomeValue"),
        DeviceLockedException);

    clientDevice.unlock();

    clientDevice.getChannels()[0].asPtr<IPropertyObjectProtected>(true).setProtectedPropertyValue("StrPropProtected", "SomeValue");
    auto value = clientDevice.getChannels()[0].getPropertyValue("StrPropProtected");
    ASSERT_EQ(value, "SomeValue");
}

TEST_F(ConfigProtocolDeviceLockingTest, CallProperty)
{
    auto device = createDevice();

    {
        const auto func = Function([](Int a, Int b) { return a + b; });

        const auto funcProp =
            FunctionPropertyBuilder("SumProp", FunctionInfo(ctInt, List<IArgumentInfo>(ArgumentInfo("A", ctInt), ArgumentInfo("B", ctInt))))
                .setReadOnly(false)
                .build();

        device.addProperty(funcProp);
        device.setPropertyValue("SumProp", func);
    }

    setupServerAndClient(device, UserTomaz);

    clientDevice.lock();

    ASSERT_THROW(clientDevice.getPropertyValue("SumProp").call(1, 2), DeviceLockedException);

    clientDevice.unlock();

    auto result = clientDevice.getPropertyValue("SumProp").call(1, 2);
    ASSERT_EQ(result, 3);
}

TEST_F(ConfigProtocolDeviceLockingTest, BeginEndUpdate)
{
    auto device = createDevice();
    setupServerAndClient(device, UserTomaz);

    clientDevice.lock();

    clientDevice.beginUpdate();
    ASSERT_THROW(clientDevice.endUpdate(), DeviceLockedException);

    clientDevice.unlock();

    clientDevice.beginUpdate();
    clientDevice.setPropertyValue("StrProp", "SomeValue");
    ASSERT_EQ(clientDevice.getPropertyValue("StrProp"), "-");
    ASSERT_EQ(serverDevice.getPropertyValue("StrProp"), "-");
    clientDevice.endUpdate();
    ASSERT_EQ(clientDevice.getPropertyValue("StrProp"), "SomeValue");
    ASSERT_EQ(serverDevice.getPropertyValue("StrProp"), "SomeValue");
}

TEST_F(ConfigProtocolDeviceLockingTest, SetAttributeValue)
{
    auto device = createDevice();
    setupServerAndClient(device, UserTomaz);

    clientDevice.lock();

    ASSERT_THROW(clientDevice.setName("Name"), DeviceLockedException);

    clientDevice.unlock();

    clientDevice.setName("NameChanged");
    ASSERT_EQ(clientDevice.getName(), "NameChanged");
}

TEST_F(ConfigProtocolDeviceLockingTest, Update)
{
    auto device = createDevice();
    setupServerAndClient(device, UserTomaz);

    clientDevice.lock();

    {
        auto updatableDevice = clientDevice.getDevices()[0];
        const auto serializer = JsonSerializer();
        updatableDevice.serialize(serializer);
        const auto str = serializer.getOutput();
        const auto deserializer = JsonDeserializer();
        ASSERT_THROW(deserializer.update(updatableDevice, str), DeviceLockedException);
    }

    clientDevice.unlock();

    {
        auto updatableDevice = clientDevice.getDevices()[0];
        const auto serializer = JsonSerializer();
        updatableDevice.serialize(serializer);
        const auto str = serializer.getOutput();
        const auto deserializer = JsonDeserializer();
        ASSERT_NO_THROW(deserializer.update(updatableDevice, str));
    }
}

TEST_F(ConfigProtocolDeviceLockingTest, AddFunctionBlock)
{
    auto device = createDevice();

    const auto config = PropertyObject();
    config.addProperty(StringPropertyBuilder("Param", "Value").build());

    setupServerAndClient(device, UserTomaz);

    clientDevice.lock();

    {
        const auto clientSubDevice = clientDevice.getDevices()[0];
        ASSERT_THROW(clientSubDevice.addFunctionBlock("mockfb1", config), DeviceLockedException);
    }

    clientDevice.unlock();

    {
        const auto clientSubDevice = clientDevice.getDevices()[0];
        const auto fb = clientSubDevice.addFunctionBlock("mockfb1", config);
        ASSERT_TRUE(fb.assigned());
    }
}

TEST_F(ConfigProtocolDeviceLockingTest, RemoveFunctionBlock)
{
    auto device = createDevice();
    setupServerAndClient(device, UserTomaz);

    clientDevice.lock();

    {
        auto subDevice = clientDevice.getDevices()[0];
        ASSERT_EQ(subDevice.getFunctionBlocks().getCount(), 1u);
        auto fb = subDevice.getFunctionBlocks()[0];
        ASSERT_THROW(subDevice.removeFunctionBlock(fb), DeviceLockedException);
    }

    clientDevice.unlock();

    {
        auto subDevice = clientDevice.getDevices()[0];
        ASSERT_EQ(subDevice.getFunctionBlocks().getCount(), 1u);
        auto fb = subDevice.getFunctionBlocks()[0];
        subDevice.removeFunctionBlock(fb);
        ASSERT_EQ(subDevice.getFunctionBlocks().getCount(), 0u);
    }
}

TEST_F(ConfigProtocolDeviceLockingTest, ConnectSignal)
{
    auto device = createDevice();

    auto serverInputPort = device.getDevices()[0].getFunctionBlocks()[0].getInputPorts()[0];
    serverInputPort.disconnect();

    setupServerAndClient(device, UserTomaz);

    clientDevice.lock();

    {
        auto inputPort = clientDevice.getDevices()[0].getFunctionBlocks()[0].getInputPorts()[0];
        auto signal = clientDevice.getDevices()[0].getSignals()[0];
        ASSERT_THROW(inputPort.connect(signal), DeviceLockedException);
    }

    clientDevice.unlock();

    {
        auto inputPort = clientDevice.getDevices()[0].getFunctionBlocks()[0].getInputPorts()[0];
        auto signal = clientDevice.getDevices()[0].getSignals()[0];
        inputPort.connect(signal);
        ASSERT_EQ(inputPort.getSignal(), signal);
    }
}

TEST_F(ConfigProtocolDeviceLockingTest, DisconnectSignal)
{
    auto device = createDevice();
    setupServerAndClient(device, UserTomaz);

    clientDevice.lock();

    {
        auto inputPort = clientDevice.getDevices()[0].getFunctionBlocks()[0].getInputPorts()[0];
        ASSERT_THROW(inputPort.disconnect(), DeviceLockedException);
    }

    clientDevice.unlock();

    {
        auto inputPort = clientDevice.getDevices()[0].getFunctionBlocks()[0].getInputPorts()[0];
        ASSERT_TRUE(inputPort.getSignal().assigned());
        inputPort.disconnect();
        ASSERT_FALSE(inputPort.getSignal().assigned());
    }
}

TEST_F(ConfigProtocolDeviceLockingTest, NestedSetValue)
{
    auto device = createDevice();

    auto advanced = createAdvancedObject();
    device.addProperty(ObjectProperty("Advanced", advanced));

    setupServerAndClient(device, UserTomaz);

    clientDevice.lock();

    ASSERT_THROW(clientDevice.setPropertyValue("Advanced.StringProp", "Changed"), DeviceLockedException);

    clientDevice.unlock();

    clientDevice.setPropertyValue("Advanced.StringProp", "Changed");
    auto value = clientDevice.getPropertyValue("Advanced.StringProp");
    ASSERT_EQ(value, "Changed");
}

TEST_F(ConfigProtocolDeviceLockingTest, NestedClearValue)
{
    auto device = createDevice();

    auto advanced = createAdvancedObject();
    device.addProperty(ObjectProperty("Advanced", advanced));

    setupServerAndClient(device, UserTomaz);

    clientDevice.lock();

    ASSERT_THROW(clientDevice.clearPropertyValue("Advanced.StringProp"), DeviceLockedException);

    clientDevice.unlock();

    clientDevice.clearPropertyValue("Advanced.StringProp");
    auto value = clientDevice.getPropertyValue("Advanced.StringProp");
    ASSERT_EQ(value, "-");
}

TEST_F(ConfigProtocolDeviceLockingTest, NestedCallProperty)
{
    auto device = createDevice();

    auto advanced = createAdvancedObject();
    device.addProperty(ObjectProperty("Advanced", advanced));

    setupServerAndClient(device, UserTomaz);

    clientDevice.lock();

    ASSERT_THROW(clientDevice.getPropertyValue("Advanced.SumProp").call(1, 2), DeviceLockedException);

    clientDevice.unlock();

    auto result = clientDevice.getPropertyValue("Advanced.SumProp").call(1, 2);
    ASSERT_EQ(result, 3);
}
