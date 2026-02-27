#include <opendaq/component_factory.h>
#include <opendaq/context_factory.h>
#include <gtest/gtest.h>
#include <coreobjects/property_object_internal_ptr.h>
#include <opendaq/mock/mock_fb_module.h>
#include <opendaq/mock/advanced_components_setup_utils.h>
#include <config_protocol/config_protocol_server.h>
#include <config_protocol/config_protocol_client.h>
#include <config_protocol/config_client_device_impl.h>
#include <coreobjects/user_factory.h>
#include <opendaq/mirrored_device_config_ptr.h>
#include <opendaq/mirrored_device_ptr.h>
#include <opendaq/device_type_factory.h>

using namespace daq;
using namespace daq::config_protocol;

class ConfigClientGatewayDeviceTest : public testing::Test
{
public:
    void SetUp() override
    {
        const auto anonymousUser = User("", "");

        //// Leaf device setup
        leafDevice = test_utils::createTestDevice();
        leafDevice.asPtr<IPropertyObjectInternal>().enableCoreEventTrigger();
        leafServer = std::make_unique<ConfigProtocolServer>(
            leafDevice,
            std::bind(&ConfigClientGatewayDeviceTest::leafServerNotificationReady, this, std::placeholders::_1),
            anonymousUser,
            ClientType::Control,
            test_utils::dummyExtSigFolder(leafDevice.getContext()));

        //// Gateway 1 device setup
        gateway1Context = NullContext();
        gateway1Client =
            std::make_unique<ConfigProtocolClient<ConfigClientDeviceImpl>>(
                gateway1Context,
                std::bind(&ConfigClientGatewayDeviceTest::gateway1SendRequestAndGetReply, this, std::placeholders::_1),
                std::bind(&ConfigClientGatewayDeviceTest::gateway1SendNoReplyRequest, this, std::placeholders::_1),
                nullptr,
                nullptr,
                nullptr
            );
        
        gateway1Device = createWithImplementation<IDevice, Device>(gateway1Context, nullptr, "Gateway1Device");
        FolderConfigPtr config = gateway1Device.asPtr<IFolder>().getItem("Dev");
        gateway1LeafDevice = gateway1Client->connect(config);
        config.addItem(gateway1LeafDevice);
        gateway1Device.asPtr<IPropertyObjectInternal>().enableCoreEventTrigger();

        gateway1Server = std::make_unique<ConfigProtocolServer>(
            gateway1Device,
            std::bind(&ConfigClientGatewayDeviceTest::gateway1ServerNotificationReady, this, std::placeholders::_1),
            anonymousUser,
            ClientType::Control,
            test_utils::dummyExtSigFolder(gateway1Device.getContext()));
        
        //// Gateway 2 device setup

        gateway2Context = NullContext();
        gateway2Client =
            std::make_unique<ConfigProtocolClient<ConfigClientDeviceImpl>>(
                gateway2Context,
                std::bind(&ConfigClientGatewayDeviceTest::gateway2SendRequestAndGetReply, this, std::placeholders::_1),
                std::bind(&ConfigClientGatewayDeviceTest::gateway2SendNoReplyRequest, this, std::placeholders::_1),
                nullptr,
                nullptr,
                nullptr
            );
        
        gateway2Device = createWithImplementation<IDevice, Device>(gateway2Context, nullptr, "Gateway2Device");
        config = gateway2Device.asPtr<IFolder>().getItem("Dev");
        gateway2GatewayDevice = gateway2Client->connect(config);
        config.addItem(gateway2GatewayDevice);
        gateway2Device.asPtr<IPropertyObjectInternal>().enableCoreEventTrigger();

        gateway2Server = std::make_unique<ConfigProtocolServer>(
            gateway2Device,
            std::bind(&ConfigClientGatewayDeviceTest::gateway2ServerNotificationReady, this, std::placeholders::_1),
            anonymousUser,
            ClientType::Control,
            test_utils::dummyExtSigFolder(gateway2Device.getContext()));

        //// Root device setup
        rootContext = NullContext();
        rootDevice = createWithImplementation<IDevice, Device>(rootContext, nullptr, "RootDevice");

        rootClient =
            std::make_unique<ConfigProtocolClient<ConfigClientDeviceImpl>>(
                rootContext,
                std::bind(&ConfigClientGatewayDeviceTest::rootSendRequestAndGetReply, this, std::placeholders::_1),
                std::bind(&ConfigClientGatewayDeviceTest::rootSendNoReplyRequest, this, std::placeholders::_1),
                nullptr,
                nullptr,
                nullptr
            );
        
        config = rootDevice.asPtr<IFolder>().getItem("Dev");
        rootGateway2Device = rootClient->connect(config);
        config.addItem(rootGateway2Device);
        rootDevice.asPtr<IPropertyObjectInternal>().enableCoreEventTrigger();
    }

protected:
    DevicePtr leafDevice;
    std::unique_ptr<ConfigProtocolServer> leafServer;
    
    ContextPtr gateway1Context;
    DevicePtr gateway1LeafDevice;
    DevicePtr gateway1Device;
    std::unique_ptr<ConfigProtocolClient<ConfigClientDeviceImpl>> gateway1Client;
    std::unique_ptr<ConfigProtocolServer> gateway1Server;

    ContextPtr gateway2Context;
    DevicePtr gateway2GatewayDevice;
    DevicePtr gateway2Device;
    std::unique_ptr<ConfigProtocolClient<ConfigClientDeviceImpl>> gateway2Client;
    std::unique_ptr<ConfigProtocolServer> gateway2Server;

    ContextPtr rootContext;
    DevicePtr rootGateway2Device;
    DevicePtr rootDevice;

    std::unique_ptr<ConfigProtocolClient<ConfigClientDeviceImpl>> rootClient;
    
private:
    //// Leaf <-> Gateway 1 callbacks
    void leafServerNotificationReady(const PacketBuffer& notificationPacket) const
    {
        gateway1Client->triggerNotificationPacket(notificationPacket);
    }

    PacketBuffer gateway1SendRequestAndGetReply(const PacketBuffer& requestPacket) const
    {
        auto replyPacket = leafServer->processRequestAndGetReply(requestPacket);
        return replyPacket;
    }

    void gateway1SendNoReplyRequest(const PacketBuffer& requestPacket) const
    {
        // callback is not expected to be called within this test group
        assert(false);
        leafServer->processNoReplyRequest(requestPacket);
    }
    
    //// Gateway 1 <-> Gateway 2 callbacks
    void gateway1ServerNotificationReady(const PacketBuffer& notificationPacket) const
    {
        gateway2Client->triggerNotificationPacket(notificationPacket);
    }

    PacketBuffer gateway2SendRequestAndGetReply(const PacketBuffer& requestPacket) const
    {
        auto replyPacket = gateway1Server->processRequestAndGetReply(requestPacket);
        return replyPacket;
    }

    void gateway2SendNoReplyRequest(const PacketBuffer& requestPacket) const
    {
        // callback is not expected to be called within this test group
        assert(false);
        gateway1Server->processNoReplyRequest(requestPacket);
    }
    
    //// Gateway 2 <-> Root callbacks
    void gateway2ServerNotificationReady(const PacketBuffer& notificationPacket) const
    {
        rootClient->triggerNotificationPacket(notificationPacket);
    }

    PacketBuffer rootSendRequestAndGetReply(const PacketBuffer& requestPacket) const
    {
        auto replyPacket = gateway2Server->processRequestAndGetReply(requestPacket);
        return replyPacket;
    }

    void rootSendNoReplyRequest(const PacketBuffer& requestPacket) const
    {
        // callback is not expected to be called within this test group
        assert(false);
        gateway2Server->processNoReplyRequest(requestPacket);
    }
};

TEST_F(ConfigClientGatewayDeviceTest, TestLeafSignalConnected)
{
    auto leafIp = leafDevice.getDevices()[0].getFunctionBlocks()[0].getInputPorts()[0];
    auto gateway1Ip = gateway1Device.getDevices()[0].getDevices()[0].getFunctionBlocks()[0].getInputPorts()[0];
    auto gateway2Ip = gateway2Device.getDevices()[0].getDevices()[0].getDevices()[0].getFunctionBlocks()[0].getInputPorts()[0];
    auto rootIp = rootDevice.getDevices()[0].getDevices()[0].getDevices()[0].getDevices()[0].getFunctionBlocks()[0].getInputPorts()[0];

    ASSERT_EQ(leafIp.getLocalId(), gateway1Ip.getLocalId());
    ASSERT_EQ(gateway1Ip.getLocalId(), gateway2Ip.getLocalId());
    ASSERT_EQ(gateway2Ip.getLocalId(), rootIp.getLocalId());

    ASSERT_TRUE(leafIp.getSignal().assigned());
    ASSERT_TRUE(gateway1Ip.getSignal().assigned());
    ASSERT_TRUE(gateway2Ip.getSignal().assigned());
    ASSERT_TRUE(rootIp.getSignal().assigned());
}

TEST_F(ConfigClientGatewayDeviceTest, TestLeafSignalDisconnectedAndReconnected)
{
    auto leafIp = leafDevice.getDevices()[0].getFunctionBlocks()[0].getInputPorts()[0];
    auto gateway1Ip = gateway1Device.getDevices()[0].getDevices()[0].getFunctionBlocks()[0].getInputPorts()[0];
    auto gateway2Ip = gateway2Device.getDevices()[0].getDevices()[0].getDevices()[0].getFunctionBlocks()[0].getInputPorts()[0];
    auto rootIp = rootDevice.getDevices()[0].getDevices()[0].getDevices()[0].getDevices()[0].getFunctionBlocks()[0].getInputPorts()[0];

    leafIp.disconnect();
    ASSERT_FALSE(leafIp.getSignal().assigned());
    ASSERT_FALSE(gateway1Ip.getSignal().assigned());
    ASSERT_FALSE(gateway2Ip.getSignal().assigned());
    ASSERT_FALSE(rootIp.getSignal().assigned());

    leafIp.connect(leafDevice.getSignalsRecursive()[0]);
    ASSERT_TRUE(leafIp.getSignal().assigned());
    ASSERT_TRUE(gateway1Ip.getSignal().assigned());
    ASSERT_TRUE(gateway2Ip.getSignal().assigned());
    ASSERT_TRUE(rootIp.getSignal().assigned());
}

TEST_F(ConfigClientGatewayDeviceTest, TestLeafSignalConnectedAfterRootLoad)
{
    auto ser = JsonSerializer();
    rootDevice.asPtr<ISerializable>().serialize(ser);
    auto str = ser.getOutput();

    auto leafIp = leafDevice.getDevices()[0].getFunctionBlocks()[0].getInputPorts()[0];
    auto gateway1Ip = gateway1Device.getDevices()[0].getDevices()[0].getFunctionBlocks()[0].getInputPorts()[0];
    auto gateway2Ip = gateway2Device.getDevices()[0].getDevices()[0].getDevices()[0].getFunctionBlocks()[0].getInputPorts()[0];
    auto rootIp = rootDevice.getDevices()[0].getDevices()[0].getDevices()[0].getDevices()[0].getFunctionBlocks()[0].getInputPorts()[0];

    leafIp.disconnect();

    const auto deserializer = JsonDeserializer();
    deserializer.update(rootDevice, str, nullptr);

    ASSERT_TRUE(leafIp.getSignal().assigned());
    ASSERT_TRUE(gateway1Ip.getSignal().assigned());
    ASSERT_TRUE(gateway2Ip.getSignal().assigned());
    ASSERT_TRUE(rootIp.getSignal().assigned());
}

TEST_F(ConfigClientGatewayDeviceTest, TestLeafSignalConnectedAfterLeafLoad)
{
    auto ser = JsonSerializer();
    leafDevice.asPtr<ISerializable>().serialize(ser);
    auto str = ser.getOutput();

    auto leafIp = leafDevice.getDevices()[0].getFunctionBlocks()[0].getInputPorts()[0];
    auto gateway1Ip = gateway1Device.getDevices()[0].getDevices()[0].getFunctionBlocks()[0].getInputPorts()[0];
    auto gateway2Ip = gateway2Device.getDevices()[0].getDevices()[0].getDevices()[0].getFunctionBlocks()[0].getInputPorts()[0];
    auto rootIp = rootDevice.getDevices()[0].getDevices()[0].getDevices()[0].getDevices()[0].getFunctionBlocks()[0].getInputPorts()[0];

    leafIp.disconnect();

    const auto deserializer = JsonDeserializer();
    deserializer.update(leafDevice, str, nullptr);

    ASSERT_TRUE(leafIp.getSignal().assigned());
    ASSERT_TRUE(gateway1Ip.getSignal().assigned());
    ASSERT_TRUE(gateway2Ip.getSignal().assigned());
    ASSERT_TRUE(rootIp.getSignal().assigned());
}

TEST_F(ConfigClientGatewayDeviceTest, TestLeafSignalConnectedAfterGateway1Load)
{
    auto ser = JsonSerializer();
    gateway1Device.asPtr<ISerializable>().serialize(ser);
    auto str = ser.getOutput();

    auto leafIp = leafDevice.getDevices()[0].getFunctionBlocks()[0].getInputPorts()[0];
    auto gateway1Ip = gateway1Device.getDevices()[0].getDevices()[0].getFunctionBlocks()[0].getInputPorts()[0];
    auto gateway2Ip = gateway2Device.getDevices()[0].getDevices()[0].getDevices()[0].getFunctionBlocks()[0].getInputPorts()[0];
    auto rootIp = rootDevice.getDevices()[0].getDevices()[0].getDevices()[0].getDevices()[0].getFunctionBlocks()[0].getInputPorts()[0];

    leafIp.disconnect();

    const auto deserializer = JsonDeserializer();
    deserializer.update(gateway1Device, str, nullptr);

    ASSERT_TRUE(leafIp.getSignal().assigned());
    ASSERT_TRUE(gateway1Ip.getSignal().assigned());
    ASSERT_TRUE(gateway2Ip.getSignal().assigned());
    ASSERT_TRUE(rootIp.getSignal().assigned());
}

TEST_F(ConfigClientGatewayDeviceTest, TestLeafSignalConnectedAfterGateway2Load)
{
    auto ser = JsonSerializer();
    gateway2Device.asPtr<ISerializable>().serialize(ser);
    auto str = ser.getOutput();

    auto leafIp = leafDevice.getDevices()[0].getFunctionBlocks()[0].getInputPorts()[0];
    auto gateway1Ip = gateway1Device.getDevices()[0].getDevices()[0].getFunctionBlocks()[0].getInputPorts()[0];
    auto gateway2Ip = gateway2Device.getDevices()[0].getDevices()[0].getDevices()[0].getFunctionBlocks()[0].getInputPorts()[0];
    auto rootIp = rootDevice.getDevices()[0].getDevices()[0].getDevices()[0].getDevices()[0].getFunctionBlocks()[0].getInputPorts()[0];

    leafIp.disconnect();

    const auto deserializer = JsonDeserializer();
    deserializer.update(gateway2Device, str, nullptr);

    ASSERT_TRUE(leafIp.getSignal().assigned());
    ASSERT_TRUE(gateway1Ip.getSignal().assigned());
    ASSERT_TRUE(gateway2Ip.getSignal().assigned());
    ASSERT_TRUE(rootIp.getSignal().assigned());
}

TEST_F(ConfigClientGatewayDeviceTest, MirroredDeviceTypeIsPropagatedViaConfigProtocol)
{
    // Set mirroredDeviceType on gateway1LeafDevice (simulates what the gateway device module does)
    const auto deviceType = DeviceType("typeId", "typeName", "typeDescription", "daq.test");
    gateway1LeafDevice.asPtr<IMirroredDeviceConfig>().setMirroredDeviceType(deviceType);

    // The actual server-side leaf device must remain unaffected
    ASSERT_FALSE(leafDevice.getInfo().getDeviceType().assigned());

    // Connect a fresh client to gateway1Server — serializes gateway1Device including
    // gateway1LeafDevice with mirroredDeviceType now set
    const auto freshContext = NullContext();
    auto freshClient = std::make_unique<ConfigProtocolClient<ConfigClientDeviceImpl>>(
        freshContext,
        [this](const PacketBuffer& packet) { return gateway1Server->processRequestAndGetReply(packet); },
        [](const PacketBuffer&) { assert(false); },
        nullptr,
        nullptr,
        nullptr);
    const DevicePtr freshDevice = freshClient->connect();

    // freshDevice.getDevices()[0] is the ConfigClientDeviceImpl for gateway1LeafDevice
    const auto mirroredLeafDevice = freshDevice.getDevices()[0];
    ASSERT_TRUE(mirroredLeafDevice.assigned());

    const auto mirroredType = mirroredLeafDevice.asPtr<IMirroredDevice>().getMirroredDeviceType();
    ASSERT_TRUE(mirroredType.assigned());
    ASSERT_EQ(mirroredType.getId(), "typeId");
    ASSERT_EQ(mirroredType.getName(), "typeName");
    ASSERT_EQ(mirroredType.getDescription(), "typeDescription");

    const auto info = mirroredLeafDevice.asPtr<IDevice>().getInfo();
    ASSERT_TRUE(info.assigned());
    const auto deviceTypeFromInfo = info.getDeviceType();
    ASSERT_TRUE(deviceTypeFromInfo.assigned());
    ASSERT_EQ(deviceTypeFromInfo.getId(), "typeId");
}
