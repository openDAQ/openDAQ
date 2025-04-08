#include <opendaq/device_info_factory.h>
#include <gtest/gtest.h>
#include <coreobjects/property_object_factory.h>
#include <coreobjects/property_factory.h>
#include <opendaq/device_type_factory.h>
#include <opendaq/device_info_internal_ptr.h>
#include <opendaq/context_factory.h>
#include <opendaq/network_interface_factory.h>
#include <opendaq/module_manager_factory.h>
#include <coreobjects/ownable_ptr.h>
#include <opendaq/component_factory.h>
#include <testutils/memcheck_listener.h>

using DeviceInfoTest = testing::Test;

BEGIN_NAMESPACE_OPENDAQ

TEST_F(DeviceInfoTest, Factory)
{
    DeviceInfoPtr deviceInfo;
    ASSERT_NO_THROW(deviceInfo = DeviceInfo("connectionStr", "Name"));
    ASSERT_EQ(deviceInfo.getConnectionString(), "connectionStr");
    ASSERT_EQ(deviceInfo.getName(), "Name");
}

TEST_F(DeviceInfoTest, DefaultValues)
{
    DeviceInfoPtr deviceInfo = DeviceInfo("");

    ASSERT_EQ(deviceInfo.getName(), "");
    ASSERT_EQ(deviceInfo.getConnectionString(), "");
    ASSERT_EQ(deviceInfo.getManufacturer(), "");
    ASSERT_EQ(deviceInfo.getManufacturerUri(), "");
    ASSERT_EQ(deviceInfo.getModel(), "");
    ASSERT_EQ(deviceInfo.getProductCode(), "");
    ASSERT_EQ(deviceInfo.getHardwareRevision(), "");
    ASSERT_EQ(deviceInfo.getSoftwareRevision(), "");
    ASSERT_EQ(deviceInfo.getDeviceManual(), "");
    ASSERT_EQ(deviceInfo.getDeviceClass(), "");
    ASSERT_EQ(deviceInfo.getSerialNumber(), "");
    ASSERT_EQ(deviceInfo.getProductInstanceUri(), "");
    ASSERT_EQ(deviceInfo.getRevisionCounter(), 0);
    ASSERT_EQ(deviceInfo.getDeviceRevision(), "");
    ASSERT_EQ(deviceInfo.getAssetId(), "");
    ASSERT_EQ(deviceInfo.getMacAddress(), "");
    ASSERT_EQ(deviceInfo.getParentMacAddress(), "");
    ASSERT_EQ(deviceInfo.getPlatform(), "");
    ASSERT_EQ(deviceInfo.getPosition(), 0);
    ASSERT_EQ(deviceInfo.getSystemType(), "");
    ASSERT_EQ(deviceInfo.getSystemUuid(), "");
    ASSERT_EQ(deviceInfo.getLocation(), "");
    ASSERT_FALSE(deviceInfo.getDeviceType().assigned());
    ASSERT_EQ(deviceInfo.getNetworkInterfaces().getCount(), 0u);
    ASSERT_EQ(deviceInfo.getConnectedClientsInfo().getCount(), 0u);

    ASSERT_EQ(deviceInfo.getAllProperties().getCount(), 27u);
}

TEST_F(DeviceInfoTest, SetGetProperties)
{
    DeviceInfoConfigPtr deviceInfoConfig = DeviceInfo("", "");
    DeviceInfoPtr deviceInfo = deviceInfoConfig;
    
    ASSERT_NO_THROW(deviceInfoConfig.setName("name"));
    ASSERT_EQ(deviceInfo.getName(), "name");
    ASSERT_EQ(deviceInfo.getPropertyValue("name"), "name");

    ASSERT_NO_THROW(deviceInfoConfig.setConnectionString("connectionString"));
    ASSERT_EQ(deviceInfo.getConnectionString(), "connectionString");
    ASSERT_EQ(deviceInfo.getPropertyValue("connectionString"), "connectionString");

    ASSERT_NO_THROW(deviceInfoConfig.setManufacturer("manufacturer"));
    ASSERT_EQ(deviceInfo.getManufacturer(), "manufacturer");
    ASSERT_EQ(deviceInfo.getPropertyValue("manufacturer"), "manufacturer");

    ASSERT_NO_THROW(deviceInfoConfig.setManufacturerUri("manufacturerUri"));
    ASSERT_EQ(deviceInfo.getManufacturerUri(), "manufacturerUri");
    ASSERT_EQ(deviceInfo.getPropertyValue("manufacturerUri"), "manufacturerUri");

    ASSERT_NO_THROW(deviceInfoConfig.setModel("model"));
    ASSERT_EQ(deviceInfo.getModel(), "model");
    ASSERT_EQ(deviceInfo.getPropertyValue("model"), "model");

    ASSERT_NO_THROW(deviceInfoConfig.setProductCode("productCode"));
    ASSERT_EQ(deviceInfo.getProductCode(), "productCode");
    ASSERT_EQ(deviceInfo.getPropertyValue("productCode"), "productCode");

    ASSERT_NO_THROW(deviceInfoConfig.setHardwareRevision("hardwareRevision"));
    ASSERT_EQ(deviceInfo.getHardwareRevision(), "hardwareRevision");
    ASSERT_EQ(deviceInfo.getPropertyValue("hardwareRevision"), "hardwareRevision");

    ASSERT_NO_THROW(deviceInfoConfig.setSoftwareRevision("softwareRevision"));
    ASSERT_EQ(deviceInfo.getSoftwareRevision(), "softwareRevision");
    ASSERT_EQ(deviceInfo.getPropertyValue("softwareRevision"), "softwareRevision");

    ASSERT_NO_THROW(deviceInfoConfig.setDeviceManual("deviceManual"));
    ASSERT_EQ(deviceInfo.getDeviceManual(), "deviceManual");
    ASSERT_EQ(deviceInfo.getPropertyValue("deviceManual"), "deviceManual");

    ASSERT_NO_THROW(deviceInfoConfig.setDeviceClass("deviceClass"));
    ASSERT_EQ(deviceInfo.getDeviceClass(), "deviceClass");
    ASSERT_EQ(deviceInfo.getPropertyValue("deviceClass"), "deviceClass");

    ASSERT_NO_THROW(deviceInfoConfig.setSerialNumber("serialNumber"));
    ASSERT_EQ(deviceInfo.getSerialNumber(), "serialNumber");
    ASSERT_EQ(deviceInfo.getPropertyValue("serialNumber"), "serialNumber");

    ASSERT_NO_THROW(deviceInfoConfig.setProductInstanceUri("productInstanceUri"));
    ASSERT_EQ(deviceInfo.getProductInstanceUri(), "productInstanceUri");
    ASSERT_EQ(deviceInfo.getPropertyValue("productInstanceUri"), "productInstanceUri");

    ASSERT_NO_THROW(deviceInfoConfig.setRevisionCounter(1));
    ASSERT_EQ(deviceInfo.getRevisionCounter(), 1);
    ASSERT_EQ(deviceInfo.getPropertyValue("revisionCounter"), 1);

    ASSERT_NO_THROW(deviceInfoConfig.setDeviceRevision("deviceRevision"));
    ASSERT_EQ(deviceInfo.getDeviceRevision(), "deviceRevision");
    ASSERT_EQ(deviceInfo.getPropertyValue("deviceRevision"), "deviceRevision");

    ASSERT_NO_THROW(deviceInfoConfig.setAssetId("assetId"));
    ASSERT_EQ(deviceInfo.getAssetId(), "assetId");
    ASSERT_EQ(deviceInfo.getPropertyValue("assetId"), "assetId");

    ASSERT_NO_THROW(deviceInfoConfig.setMacAddress("macAddress"));
    ASSERT_EQ(deviceInfo.getMacAddress(), "macAddress");
    ASSERT_EQ(deviceInfo.getPropertyValue("macAddress"), "macAddress");

    ASSERT_NO_THROW(deviceInfoConfig.setParentMacAddress("parentMacAddress"));
    ASSERT_EQ(deviceInfo.getParentMacAddress(), "parentMacAddress");
    ASSERT_EQ(deviceInfo.getPropertyValue("parentMacAddress"), "parentMacAddress");

    ASSERT_NO_THROW(deviceInfoConfig.setPlatform("platform"));
    ASSERT_EQ(deviceInfo.getPlatform(), "platform");
    ASSERT_EQ(deviceInfo.getPropertyValue("platform"), "platform");

    ASSERT_NO_THROW(deviceInfoConfig.setPosition(1));
    ASSERT_EQ(deviceInfo.getPosition(), 1);
    ASSERT_EQ(deviceInfo.getPropertyValue("position"), 1);

    ASSERT_NO_THROW(deviceInfoConfig.setSystemType("systemType"));
    ASSERT_EQ(deviceInfo.getSystemType(), "systemType");
    ASSERT_EQ(deviceInfo.getPropertyValue("systemType"), "systemType");

    ASSERT_NO_THROW(deviceInfoConfig.setSystemUuid("systemUuid"));
    ASSERT_EQ(deviceInfo.getSystemUuid(), "systemUuid");
    ASSERT_EQ(deviceInfo.getPropertyValue("systemUuid"), "systemUuid");
}

TEST_F(DeviceInfoTest, SetGetDeviceType)
{
    DeviceInfoConfigPtr deviceInfoConfig = DeviceInfo("", "");
    DeviceInfoPtr deviceInfo = deviceInfoConfig;

    auto deviceType = DeviceType("test", "", "", "prefix");
    deviceInfoConfig.setDeviceType(deviceType);
    ASSERT_EQ(deviceInfo.getDeviceType(), deviceType);
    ASSERT_EQ(deviceInfo.getDeviceType().getId(), "test");
}

TEST_F(DeviceInfoTest, Freezable)
{
    DeviceInfoConfigPtr deviceInfoConfig = DeviceInfo("", "");

    ASSERT_FALSE(deviceInfoConfig.isFrozen());
    ASSERT_NO_THROW(deviceInfoConfig.freeze());
    ASSERT_TRUE(deviceInfoConfig.isFrozen());

    ASSERT_THROW(deviceInfoConfig.setName("Name"), FrozenException);
    ASSERT_THROW(deviceInfoConfig.setConnectionString("connection_string"), FrozenException);
    ASSERT_THROW(deviceInfoConfig.setManufacturer("manufacturer"), FrozenException);
    ASSERT_THROW(deviceInfoConfig.setManufacturerUri("manufacturer_uri"), FrozenException);
    ASSERT_THROW(deviceInfoConfig.setModel("model"), FrozenException);
    ASSERT_THROW(deviceInfoConfig.setProductCode("product_code"), FrozenException);
    ASSERT_THROW(deviceInfoConfig.setHardwareRevision("hardware_revision"), FrozenException);
    ASSERT_THROW(deviceInfoConfig.setSoftwareRevision("software_revision"), FrozenException);
    ASSERT_THROW(deviceInfoConfig.setDeviceManual("device_manual"), FrozenException);
    ASSERT_THROW(deviceInfoConfig.setDeviceClass("device_class"), FrozenException);
    ASSERT_THROW(deviceInfoConfig.setSerialNumber("serial_number"), FrozenException);
    ASSERT_THROW(deviceInfoConfig.setProductInstanceUri("product_instance_uri"), FrozenException);
    ASSERT_THROW(deviceInfoConfig.setRevisionCounter(1), FrozenException);

    ASSERT_THROW(deviceInfoConfig.addProperty(StringProperty("test_key", "test_value")), FrozenException);

    auto deviceType = DeviceType("test", "", "", "prefix");
    ASSERT_THROW(deviceInfoConfig.setDeviceType(deviceType), FrozenException);

    SizeT clientNumber = 0;
    auto connectedClientInfo = ConnectedClientInfo("url", ProtocolType::Configuration, "Protocol name", "ExclusiveControl", "Host name");
    ASSERT_NO_THROW(deviceInfoConfig.asPtr<IDeviceInfoInternal>().addConnectedClient(&clientNumber, connectedClientInfo));
    ASSERT_EQ(clientNumber, 1u);
}

TEST_F(DeviceInfoTest, CustomProperties)
{
    DeviceInfoConfigPtr info = DeviceInfo("", "");

    info.addProperty(StringProperty("NewName", "Chell"));
    ASSERT_EQ(info.getPropertyValue("NewName"), "Chell");

    ASSERT_NO_THROW(info.addProperty(IntProperty("Age", 999)));
    ASSERT_NO_THROW(info.addProperty(FloatProperty("Height", 172.4)));
    ASSERT_NO_THROW(info.addProperty(BoolProperty("IsAsleep", true)));

    ASSERT_EQ(info.getCustomInfoPropertyNames().getCount(), 4u);
}

TEST_F(DeviceInfoTest, SerializeDeserialize)
{
    DeviceInfoConfigPtr info = DeviceInfo("", "");

    info.setName("Name");
    info.setConnectionString("connection_string");
    info.setManufacturer("manufacturer");
    info.setManufacturerUri("manufacturer_uri");
    info.setModel("model");
    info.setProductCode("product_code");
    info.setHardwareRevision("hardware_revision");
    info.setSoftwareRevision("software_revision");
    info.setDeviceManual("device_manual");
    info.setDeviceClass("device_class");
    info.setSerialNumber("serial_number");
    info.setProductInstanceUri("product_instance_uri");
    info.setRevisionCounter(1);
    info.asPtr<IDeviceInfoInternal>().addServerCapability(ServerCapability("test_id1", "test", ProtocolType::Streaming));
    info.asPtr<IDeviceInfoInternal>().addServerCapability(ServerCapability("test_id2", "test", ProtocolType::Configuration));
    SizeT client1Number = 0;
    info.asPtr<IDeviceInfoInternal>().addConnectedClient(
        &client1Number,
        ConnectedClientInfo("url1", ProtocolType::Streaming, "Protocol name", "", "Host name")
    );
    SizeT client2Number = 0;
    info.asPtr<IDeviceInfoInternal>().addConnectedClient(
        &client2Number,
        ConnectedClientInfo("url2", ProtocolType::Configuration, "Protocol name", "ExclusiveControl", "Host name")
    );

    const auto serializer = JsonSerializer();
    info.serialize(serializer);
    const auto serializedDeviceInfo = serializer.getOutput();

    const auto deserializer = JsonDeserializer();

    const DeviceInfoPtr newDeviceInfo = deserializer.deserialize(serializedDeviceInfo, nullptr, nullptr);

    ASSERT_EQ(newDeviceInfo.getServerCapabilities().getCount(), 2u);
    ASSERT_EQ(newDeviceInfo.getServerCapabilities()[0].getProtocolId(), "test_id1");
    ASSERT_EQ(newDeviceInfo.getServerCapabilities()[1].getProtocolId(), "test_id2");
    ASSERT_EQ(newDeviceInfo.getConnectedClientsInfo().getCount(), 2u);

    serializer.reset();
    newDeviceInfo.serialize(serializer);
    const auto newSerializedDeviceInfo = serializer.getOutput();

    ASSERT_EQ(serializedDeviceInfo, newSerializedDeviceInfo);
}

TEST_F(DeviceInfoTest, SerializeDeserializeForOlderVersion)
{
    DeviceInfoConfigPtr info = DeviceInfo("", "");

    SizeT client1Number = 0;
    info.asPtr<IDeviceInfoInternal>().addConnectedClient(
        &client1Number,
        ConnectedClientInfo("url1", ProtocolType::Streaming, "Protocol name", "", "Host name")
    );
    SizeT client2Number = 0;
    info.asPtr<IDeviceInfoInternal>().addConnectedClient(
        &client2Number,
        ConnectedClientInfo("url2", ProtocolType::Configuration, "Protocol name", "ExclusiveControl", "Host name")
    );

    const auto serializer = JsonSerializerWithVersion(2);
    info.serialize(serializer);
    const auto serializedDeviceInfo = serializer.getOutput();

    const auto deserializer = JsonDeserializer();

    const DeviceInfoPtr newDeviceInfo = deserializer.deserialize(serializedDeviceInfo, nullptr, nullptr);

    // default value of "activeClientConnections" constains no nested properties
    ASSERT_EQ(newDeviceInfo.getConnectedClientsInfo().getCount(), 0u);
}

TEST_F(DeviceInfoTest, ServerCapabilities)
{
    auto context = NullContext();
    DeviceInfoPtr info = DeviceInfo("", "");
    DeviceInfoInternalPtr internalInfo = info;

    auto capability1 = ServerCapability("localId1", "protocolName1", ProtocolType::Streaming);
    auto capability2 = ServerCapability("localId2","protocolName2", ProtocolType::Streaming);
    auto capability3 = ServerCapability("localId3","protocolName3", ProtocolType::Streaming);

    internalInfo.addServerCapability(capability1);
    internalInfo.addServerCapability(capability2);
    internalInfo.addServerCapability(capability3);
    ASSERT_THROW(internalInfo.addServerCapability(capability3), DuplicateItemException);

    ASSERT_EQ(info.getServerCapabilities().getCount(), 3u);
    
    ASSERT_THROW(internalInfo.removeServerCapability("localId0"), NotFoundException);

    internalInfo.removeServerCapability("localId1");
    ASSERT_EQ(info.getServerCapabilities().getCount(), 2u);
    ASSERT_EQ(info.getServerCapabilities()[0].getProtocolId(), capability2.getPropertyValue("protocolId"));

    internalInfo.clearServerStreamingCapabilities();
    ASSERT_EQ(info.getServerCapabilities().getCount(), 0u);
}

TEST_F(DeviceInfoTest, NetworkInterfaces)
{
    MemCheckListener::expectMemoryLeak = true;// memory leak in module manager
    const auto moduleManager = ModuleManager("[[none]]");

    DeviceInfoPtr info = DeviceInfo("", "");
    DeviceInfoInternalPtr internalInfo = info;

    auto iface0 = NetworkInterface("eth0", "manufacturer", "serial_number", moduleManager);
    auto iface1 = NetworkInterface("eth1", "manufacturer", "serial_number", moduleManager);
    auto iface2 = NetworkInterface("eth2", "manufacturer", "serial_number", moduleManager);

    internalInfo.addNetworkInteface("eth0", iface0);
    internalInfo.addNetworkInteface("eth1", iface1);
    internalInfo.addNetworkInteface("eth2", iface2);
    ASSERT_THROW(internalInfo.addNetworkInteface("eth2", iface2), DuplicateItemException);

    ASSERT_EQ(info.getNetworkInterfaces().getCount(), 3u);

    ASSERT_THROW(info.getNetworkInterface("eth3"), NotFoundException);

    const auto defaultConfig = info.getNetworkInterface("eth0").createDefaultConfiguration();
    EXPECT_EQ(defaultConfig.getPropertyValue("dhcp4"), True);
    EXPECT_EQ(defaultConfig.getPropertyValue("address4"), String(""));
    EXPECT_EQ(defaultConfig.getPropertyValue("gateway4"), String(""));
    EXPECT_EQ(defaultConfig.getPropertyValue("dhcp6"), True);
    EXPECT_EQ(defaultConfig.getPropertyValue("address6"), String(""));
    EXPECT_EQ(defaultConfig.getPropertyValue("gateway6"), String(""));
}

TEST_F(DeviceInfoTest, ConnectedClientsInfo)
{
    auto context = NullContext();
    DeviceInfoPtr info = DeviceInfo("", "");
    DeviceInfoInternalPtr internalInfo = info;

    SizeT client1Number = 0;
    auto clientInfo1 =
        ConnectedClientInfo("url1", ProtocolType::Streaming, "Protocol name", "", "Host name");

    SizeT client2Number = 0;
    auto clientInfo2 =
        ConnectedClientInfo("url2", ProtocolType::Configuration, "Protocol name", "ExclusiveControl", "Host name");

    SizeT client3Number = 10;
    auto clientInfo3 =
        ConnectedClientInfo("url3", ProtocolType::ConfigurationAndStreaming, "Protocol name", "ViewOnly", "Host name");

    internalInfo.addConnectedClient(&client1Number, clientInfo1);
    ASSERT_EQ(client1Number, 1u);
    internalInfo.addConnectedClient(&client2Number, clientInfo2);
    ASSERT_EQ(client2Number, 2u);
    internalInfo.addConnectedClient(&client3Number, clientInfo3);
    ASSERT_EQ(client3Number, 3u);
    ASSERT_THROW(internalInfo.addConnectedClient(&client3Number, clientInfo3), AlreadyExistsException);

    ASSERT_EQ(info.getConnectedClientsInfo().getCount(), 3u);

    ASSERT_THROW(internalInfo.removeConnectedClient(4), NotFoundException);

    internalInfo.removeConnectedClient(3);
    ASSERT_EQ(info.getConnectedClientsInfo().getCount(), 2u);
    ASSERT_EQ(info.getConnectedClientsInfo()[0].getAddress(), clientInfo1.getPropertyValue("Address"));

    internalInfo.removeConnectedClient(2);
    internalInfo.removeConnectedClient(1);
    ASSERT_EQ(info.getConnectedClientsInfo().getCount(), 0u);

    internalInfo.addConnectedClient(&client3Number, clientInfo3);
    ASSERT_EQ(client3Number, 3u);
    internalInfo.addConnectedClient(&client2Number, clientInfo2);
    ASSERT_EQ(client2Number, 2u);
    internalInfo.addConnectedClient(&client1Number, clientInfo1);
    ASSERT_EQ(client1Number, 1u);
}

TEST_F(DeviceInfoTest, OwnerName)
{
    DeviceInfoConfigPtr info = DeviceInfo("", "foo");
    ComponentPtr component = Component(NullContext(), nullptr, "id");

    ASSERT_EQ(info.getName(), "foo");
    info.setName("test");
    ASSERT_EQ(info.getName(), "test");

    info.asPtr<IOwnable>().setOwner(component);

    ASSERT_EQ(info.getName(), "id");
    component.setName("new_id");
    ASSERT_EQ(info.getName(), "new_id");

    info.setName("new_id1");
    ASSERT_EQ(info.getName(), "new_id1");
    ASSERT_EQ(component.getName(), "new_id1");
}

TEST_F(DeviceInfoTest, PropertyWriteAfterOwnerSet)
{
    DeviceInfoConfigPtr info = DeviceInfo("", "foo");
    ComponentPtr component = Component(NullContext(), nullptr, "id");

    ASSERT_NO_THROW(info.setManufacturer("test"));
    ASSERT_EQ(info.getManufacturer(), "test");
    
    ASSERT_NO_THROW(info.setLocation("test"));
    ASSERT_EQ(info.getLocation(), "test");

    info.asPtr<IOwnable>().setOwner(component);
    
    ASSERT_THROW(info.setManufacturer("test1"), AccessDeniedException);
    ASSERT_EQ(info.getManufacturer(), "test");
    
    ASSERT_THROW(info.setLocation("test1"), AccessDeniedException);
    ASSERT_EQ(info.getLocation(), "test");
}

END_NAMESPACE_OPENDAQ
