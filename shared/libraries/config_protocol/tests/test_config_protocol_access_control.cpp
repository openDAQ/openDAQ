#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <opendaq/instance_factory.h>

#include "test_utils.h"
#include <config_protocol/config_protocol_server.h>
#include <config_protocol/config_protocol_client.h>
#include <config_protocol/config_client_device_impl.h>
#include <coreobjects/user_factory.h>
#include <coreobjects/callable_info_factory.h>
#include <coreobjects/argument_info_factory.h>


using namespace daq;
using namespace config_protocol;
using namespace testing;

class ConfigProtocolAccessControlTest : public Test
{
public:
    const UserPtr UserAdmin = User("amin", "admin123", {"admin"});
    const UserPtr UserGuest = User("guest", "guest123", {"guest"});
    const UserPtr UserRegular = User("tomaz", "tomaz123");

    DevicePtr createDevice()
    {
        auto device = test_utils::createServerDevice();

        // everyone can read
        // admin can read write and execute
        auto permissions = PermissionsBuilder()
                               .inherit(true)
                               .assign("everyone", PermissionMaskBuilder().read())
                               .assign("admin", PermissionMaskBuilder().read().write().execute())
                               .build();

        device.getPermissionManager().setPermissions(permissions);

        // guest has no permissions on 2nd device
        auto permissionsDevice1 = PermissionsBuilder()
                                      .inherit(true)
                                      .deny("guest", PermissionMaskBuilder().read().write().execute())
                                      .build();

        auto device1 = device.getDevices().getItemAt(1);
        device1.getPermissionManager().setPermissions(permissionsDevice1);

        return device;
    }

    void setupServerAndClient(const DevicePtr& device, const UserPtr& user)
    {
        serverDevice = device;
        serverDevice.asPtrOrNull<IPropertyObjectInternal>().enableCoreEventTrigger();
        server = std::make_unique<ConfigProtocolServer>(
            serverDevice, std::bind(&ConfigProtocolAccessControlTest::serverNotificationReady, this, std::placeholders::_1), user);

        auto clientContext = NullContext();
        client = std::make_unique<ConfigProtocolClient<ConfigClientDeviceImpl>>(
            clientContext, std::bind(&ConfigProtocolAccessControlTest::sendRequest, this, std::placeholders::_1), nullptr);

        clientDevice = client->connect();
        clientDevice.asPtr<IPropertyObjectInternal>().enableCoreEventTrigger();
    }

    static StringPtr serializeComponent(const ComponentPtr& component)
    {
        const auto serializer = JsonSerializer(true);
        component.serialize(serializer);
        const auto str = serializer.getOutput();
        return str;
    }

    PacketBuffer sendRequest(const PacketBuffer& requestPacket) const
    {
        auto replyPacket = server->processRequestAndGetReply(requestPacket);
        return replyPacket;
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

};


TEST_F(ConfigProtocolAccessControlTest, DeniedRoot)
{
    auto device = createDevice();

    auto permissions = PermissionsBuilder().inherit(true).deny("everyone", PermissionMaskBuilder().read()).build();
    device.getPermissionManager().setPermissions(permissions);

    ASSERT_THROW(setupServerAndClient(device, UserRegular), AccessDeniedException);
}

TEST_F(ConfigProtocolAccessControlTest, GetDevices)
{
    auto device = createDevice();

    setupServerAndClient(device, UserRegular);
    ASSERT_EQ(clientDevice.getDevices().getCount(), 2u);

    setupServerAndClient(device, UserGuest);
    ASSERT_EQ(clientDevice.getDevices().getCount(), 1u);
}

TEST_F(ConfigProtocolAccessControlTest, GetChannels)
{
    auto device = createDevice();

    auto channels = device.getChannelsRecursive();
    ASSERT_EQ(channels.getCount(), 6u);

    auto permissions = PermissionsBuilder()
                           .inherit(false)
                           .allow("admin", PermissionMaskBuilder().read())
                           .build();

    channels[0].getPermissionManager().setPermissions(permissions);

    setupServerAndClient(device, UserRegular);
    ASSERT_EQ(clientDevice.getChannelsRecursive().getCount(), 5u);

    setupServerAndClient(device, UserAdmin);
    ASSERT_EQ(clientDevice.getChannelsRecursive().getCount(), 6u);
}

TEST_F(ConfigProtocolAccessControlTest, GetAvailableFunctionBlockTypes)
{
    auto device = createDevice();

    auto types = device.getDevices()[0].getAvailableFunctionBlockTypes();
    ASSERT_EQ(types.getCount(), 1u);

    setupServerAndClient(device, UserRegular);

    auto clientTypes = clientDevice.getDevices()[0].getAvailableFunctionBlockTypes();
    ASSERT_EQ(clientTypes.getCount(), 1u);
}

TEST_F(ConfigProtocolAccessControlTest, SetPropertyValue)
{
    auto device = createDevice();

    setupServerAndClient(device, UserRegular);

    ASSERT_THROW(clientDevice.getChannels()[0].setPropertyValue("StrProp", "SomeValue"), AccessDeniedException);

    setupServerAndClient(device, UserAdmin);

    clientDevice.getChannels()[0].setPropertyValue("StrProp", "SomeValue");
    auto value = clientDevice.getChannels()[0].getPropertyValue("StrProp");
    ASSERT_EQ(value, "SomeValue");
}

TEST_F(ConfigProtocolAccessControlTest, SetProtectedPropertyValue)
{
    auto device = createDevice();

    setupServerAndClient(device, UserRegular);

    ASSERT_THROW(
        clientDevice.getChannels()[0].asPtr<IPropertyObjectProtected>(true).setProtectedPropertyValue("StrPropProtected", "SomeValue"),
        AccessDeniedException);

    setupServerAndClient(device, UserAdmin);

    clientDevice.getChannels()[0].asPtr<IPropertyObjectProtected>(true).setProtectedPropertyValue("StrPropProtected", "SomeValue");
    auto value = clientDevice.getChannels()[0].getPropertyValue("StrPropProtected");
    ASSERT_EQ(value, "SomeValue");
}

TEST_F(ConfigProtocolAccessControlTest, ClearPropertyValue)
{
    auto device = createDevice();

    setupServerAndClient(device, UserRegular);

    ASSERT_THROW(clientDevice.getChannels()[0].clearPropertyValue("StrProp"), AccessDeniedException);

    setupServerAndClient(device, UserAdmin);

    clientDevice.getChannels()[0].setPropertyValue("StrProp", "SomeValue");
    auto value = clientDevice.getChannels()[0].getPropertyValue("StrProp");
    ASSERT_EQ(value, "SomeValue");

    clientDevice.getChannels()[0].clearPropertyValue("StrProp");
    auto valueCleared = clientDevice.getChannels()[0].getPropertyValue("StrProp");
    ASSERT_EQ(valueCleared, "-");
}

TEST_F(ConfigProtocolAccessControlTest, CallProperty)
{
    auto device = createDevice();

    {
        const auto func = Function([](Int a, Int b) { return a + b; });

        const auto funcProp = FunctionPropertyBuilder(
                                  "SumProp", FunctionInfo(ctInt, List<IArgumentInfo>(ArgumentInfo("A", ctInt), ArgumentInfo("B", ctInt))))
                                  .setReadOnly(false)
                                  .build();

        device.addProperty(funcProp);
        device.setPropertyValue("SumProp", func);
    }

    setupServerAndClient(device, UserRegular);

    ASSERT_THROW(clientDevice.getPropertyValue("SumProp").call(1, 2), AccessDeniedException);

    setupServerAndClient(device, UserAdmin);

    auto result = clientDevice.getPropertyValue("SumProp").call(1, 2);
    ASSERT_EQ(result, 3);
}

TEST_F(ConfigProtocolAccessControlTest, BeginEndUpdate)
{
    auto device = createDevice();

    setupServerAndClient(device, UserRegular);

    ASSERT_THROW(clientDevice.beginUpdate(), AccessDeniedException);
    ASSERT_THROW(clientDevice.endUpdate(), AccessDeniedException);

    setupServerAndClient(device, UserAdmin);

    clientDevice.beginUpdate();
    clientDevice.setPropertyValue("StrProp", "SomeValue");
    ASSERT_EQ(clientDevice.getPropertyValue("StrProp"), "-");
    ASSERT_EQ(serverDevice.getPropertyValue("StrProp"), "-");
    clientDevice.endUpdate();
    ASSERT_EQ(clientDevice.getPropertyValue("StrProp"), "SomeValue");
    ASSERT_EQ(serverDevice.getPropertyValue("StrProp"), "SomeValue");
}

TEST_F(ConfigProtocolAccessControlTest, SetAttributeValue)
{
    auto device = createDevice();

    setupServerAndClient(device, UserRegular);

    ASSERT_THROW(clientDevice.setName("Name"), AccessDeniedException);

    setupServerAndClient(device, UserAdmin);

    clientDevice.setName("NameChanged");
    ASSERT_EQ(clientDevice.getName(), "NameChanged");
}

TEST_F(ConfigProtocolAccessControlTest, Update)
{
    auto device = createDevice();

    setupServerAndClient(device, UserRegular);

    {
        auto updatableDevice = clientDevice.getDevices()[0];
        const auto serializer = JsonSerializer();
        updatableDevice.serialize(serializer);
        const auto str = serializer.getOutput();
        const auto deserializer = JsonDeserializer();
        ASSERT_THROW(deserializer.update(updatableDevice, str), AccessDeniedException);
    }

    setupServerAndClient(device, UserAdmin);

    {
        auto updatableDevice = clientDevice.getDevices()[0];
        const auto serializer = JsonSerializer();
        updatableDevice.serialize(serializer);
        const auto str = serializer.getOutput();
        const auto deserializer = JsonDeserializer();
        ASSERT_NO_THROW(deserializer.update(updatableDevice, str));
    }
}

TEST_F(ConfigProtocolAccessControlTest, AddFunctionBlock)
{
    auto device = createDevice();

    const auto config = PropertyObject();
    config.addProperty(StringPropertyBuilder("Param", "Value").build());

    setupServerAndClient(device, UserRegular);

    {
        const auto clientSubDevice = clientDevice.getDevices()[0];
        ASSERT_THROW(clientSubDevice.addFunctionBlock("mockfb1", config), AccessDeniedException);
    }

    setupServerAndClient(device, UserAdmin);

    {
        const auto clientSubDevice = clientDevice.getDevices()[0];
        const auto fb = clientSubDevice.addFunctionBlock("mockfb1", config);
        ASSERT_TRUE(fb.assigned());
    }
}

TEST_F(ConfigProtocolAccessControlTest, RemoveFunctionBlock)
{
    auto device = createDevice();

    setupServerAndClient(device, UserRegular);

    {
        auto subDevice = clientDevice.getDevices()[0];
        ASSERT_EQ(subDevice.getFunctionBlocks().getCount(), 1u);
        auto fb = subDevice.getFunctionBlocks()[0];
        ASSERT_THROW(subDevice.removeFunctionBlock(fb), AccessDeniedException);
    }

    setupServerAndClient(device, UserAdmin);

    {
        auto subDevice = clientDevice.getDevices()[0];
        ASSERT_EQ(subDevice.getFunctionBlocks().getCount(), 1u);
        auto fb = subDevice.getFunctionBlocks()[0];
        subDevice.removeFunctionBlock(fb);
        ASSERT_EQ(subDevice.getFunctionBlocks().getCount(), 0u);
    }
}

TEST_F(ConfigProtocolAccessControlTest, ConnectSignal)
{
    auto device = createDevice();

    auto serverInputPort = device.getDevices()[0].getFunctionBlocks()[0].getInputPorts()[0];
    serverInputPort.disconnect();

    setupServerAndClient(device, UserRegular);

    {
        auto inputPort = clientDevice.getDevices()[0].getFunctionBlocks()[0].getInputPorts()[0];
        auto signal = clientDevice.getDevices()[0].getSignals()[0];
        ASSERT_THROW(inputPort.connect(signal), AccessDeniedException);
    }

    setupServerAndClient(device, UserAdmin);

    {
        auto inputPort = clientDevice.getDevices()[0].getFunctionBlocks()[0].getInputPorts()[0];
        auto signal = clientDevice.getDevices()[0].getSignals()[0];
        inputPort.connect(signal);
        ASSERT_EQ(inputPort.getSignal(), signal);
    }
}

TEST_F(ConfigProtocolAccessControlTest, DisconnectSignal)
{
    auto device = createDevice();

    setupServerAndClient(device, UserRegular);

    {
        auto inputPort = clientDevice.getDevices()[0].getFunctionBlocks()[0].getInputPorts()[0];
        ASSERT_THROW(inputPort.disconnect(), AccessDeniedException);
    }

    setupServerAndClient(device, UserAdmin);

    {
        auto inputPort = clientDevice.getDevices()[0].getFunctionBlocks()[0].getInputPorts()[0];
        ASSERT_TRUE(inputPort.getSignal().assigned());
        inputPort.disconnect();
        ASSERT_FALSE(inputPort.getSignal().assigned());
    }
}
