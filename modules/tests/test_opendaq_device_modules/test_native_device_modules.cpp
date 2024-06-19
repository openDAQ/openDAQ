#include <opendaq/component_exceptions.h>
#include "test_helpers/test_helpers.h"
#include <fstream>
#include <coreobjects/authentication_provider_factory.h>
#include "opendaq/mock/mock_device_module.h"
#include <opendaq/device_info_internal_ptr.h>
#include <opendaq/discovery_server_factory.h>
#include <coretypes/json_serializer_factory.h>
#include <coreobjects/user_factory.h>

using NativeDeviceModulesTest = testing::Test;

using namespace daq;

static InstancePtr CreateDefaultServerInstance()
{
    auto logger = Logger();
    auto scheduler = Scheduler(logger);
    auto moduleManager = ModuleManager("");
    auto typeManager = TypeManager();
    auto authenticationProvider = AuthenticationProvider();
    auto context = Context(scheduler, logger, typeManager, moduleManager, authenticationProvider);

    auto instance = InstanceCustom(context, "local");

    const auto statistics = instance.addFunctionBlock("RefFbModuleStatistics");
    const auto refDevice = instance.addDevice("daqref://device0");
    statistics.getInputPorts()[0].connect(refDevice.getSignals(search::Recursive(search::Visible()))[0]);
    statistics.getInputPorts()[0].connect(Signal(context, nullptr, "foo"));

    return instance;
}

static InstancePtr CreateUpdatedServerInstance()
{
    auto logger = Logger();
    auto scheduler = Scheduler(logger);
    auto moduleManager = ModuleManager("");
    auto typeManager = TypeManager();
    auto authenticationProvider = AuthenticationProvider();
    auto context = Context(scheduler, logger, typeManager, moduleManager, authenticationProvider);

    auto instance = InstanceCustom(context, "local");

    const auto statistics = instance.addFunctionBlock("RefFbModuleScaling");
    const auto refDevice = instance.addDevice("daqref://device0");
    refDevice.setPropertyValue("NumberOfChannels", 3);

    const auto testType = EnumerationType("TestEnumType", List<IString>("TestValue1", "TestValue2"));
    instance.getContext().getTypeManager().addType(testType);

    return instance;
}

static InstancePtr CreateServerInstance(InstancePtr instance = CreateDefaultServerInstance())
{
    instance.addServer("openDAQ Native Streaming", nullptr);

    return instance;
}

static InstancePtr CreateClientInstance()
{
    auto instance = Instance();

    auto config = instance.createDefaultAddDeviceConfig();
    PropertyObjectPtr general = config.getPropertyValue("General");
    general.setPropertyValue("PrioritizedStreamingProtocols", List<IString>("opendaq_native_streaming"));

    auto refDevice = instance.addDevice("daq.nd://127.0.0.1", config);
    return instance;
}

TEST_F(NativeDeviceModulesTest, ConnectAndDisconnect)
{
    SKIP_TEST_MAC_CI;
    auto server = CreateServerInstance();
    auto client = CreateClientInstance();

    client->releaseRef();
    server->releaseRef();
    client.detach();
    server.detach();
}

TEST_F(NativeDeviceModulesTest, ConnectViaIpv6)
{
    if (test_helpers::Ipv6IsDisabled())
        return;

    auto server = CreateServerInstance();
    auto client = Instance();
    ASSERT_NO_THROW(client.addDevice("daq.nd://[::1]", nullptr));

    client->releaseRef();
    server->releaseRef();
    client.detach();
    server.detach();
}

TEST_F(NativeDeviceModulesTest, ConnectUsername)
{
    auto users = List<IUser>();
    users.pushBack(User("jure", "jure123"));
    users.pushBack(User("tomaz", "tomaz123"));

    auto authProvider = StaticAuthenticationProvider(false, users);

    auto serverInstance = InstanceBuilder().setAuthenticationProvider(authProvider).build();
    serverInstance.addServer("openDAQ Native Streaming", nullptr);

    auto clientInstance = Instance();

    ASSERT_ANY_THROW(clientInstance.addDevice("daq.nd://127.0.0.1"));

    auto config = clientInstance.createDefaultAddDeviceConfig();
    PropertyObjectPtr generalConfig = config.getPropertyValue("General");

    generalConfig.setPropertyValue("Username", "jure");
    generalConfig.setPropertyValue("Password", "wrongPass");
    ASSERT_ANY_THROW(clientInstance.addDevice("daq.nd://127.0.0.1", config));

    generalConfig.setPropertyValue("Username", "jure");
    generalConfig.setPropertyValue("Password", "jure123");
    auto deviceJure = clientInstance.addDevice("daq.nd://127.0.0.1", config);
    ASSERT_TRUE(deviceJure.assigned());
    clientInstance.removeDevice(deviceJure);

    generalConfig.setPropertyValue("Username", "tomaz");
    generalConfig.setPropertyValue("Password", "tomaz123");
    auto deviceTomaz = clientInstance.addDevice("daq.nd://127.0.0.1", config);
    ASSERT_TRUE(deviceTomaz.assigned());
}

TEST_F(NativeDeviceModulesTest, ConnectAllowAnonymous)
{
    auto users = List<IUser>();
    users.pushBack(User("jure", "jure123"));

    auto authProvider = StaticAuthenticationProvider(true, users);

    auto serverInstance = InstanceBuilder().setAuthenticationProvider(authProvider).build();
    serverInstance.addServer("openDAQ Native Streaming", nullptr);

    auto clientInstance = Instance();

    auto deviceAnonymous = clientInstance.addDevice("daq.nd://127.0.0.1");
    ASSERT_TRUE(deviceAnonymous.assigned());
    clientInstance.removeDevice(deviceAnonymous);

    auto config = clientInstance.createDefaultAddDeviceConfig();
    PropertyObjectPtr generalConfig = config.getPropertyValue("General");

    generalConfig.setPropertyValue("Username", "jure");
    generalConfig.setPropertyValue("Password", "wrongPass");
    ASSERT_ANY_THROW(clientInstance.addDevice("daq.nd://127.0.0.1", config));

    generalConfig.setPropertyValue("Username", "jure");
    generalConfig.setPropertyValue("Password", "jure123");
    auto deviceJure = clientInstance.addDevice("daq.nd://127.0.0.1", config);
    ASSERT_TRUE(deviceJure.assigned());
}

TEST_F(NativeDeviceModulesTest, ConnectUsernameDeviceConfig)
{
    auto users = List<IUser>();
    users.pushBack(User("jure", "jure123"));

    auto authProvider = StaticAuthenticationProvider(false, users);

    auto serverInstance = InstanceBuilder().setAuthenticationProvider(authProvider).build();
    serverInstance.addServer("openDAQ Native Streaming", nullptr);

    auto clientInstance = Instance();

    ASSERT_ANY_THROW(clientInstance.addDevice("daq.nd://127.0.0.1"));

    auto config = clientInstance.createDefaultAddDeviceConfig();
    PropertyObjectPtr deviceConfig = config.getPropertyValue("Device");
    PropertyObjectPtr nativeDeviceConfig = deviceConfig.getPropertyValue("opendaq_native_config");

    nativeDeviceConfig.setPropertyValue("Username", "jure");
    nativeDeviceConfig.setPropertyValue("Password", "wrongPass");
    ASSERT_ANY_THROW(clientInstance.addDevice("daq.nd://127.0.0.1", config));

    nativeDeviceConfig.setPropertyValue("Username", "jure");
    nativeDeviceConfig.setPropertyValue("Password", "jure123");
    auto device = clientInstance.addDevice("daq.nd://127.0.0.1", config);
    ASSERT_TRUE(device.assigned());
}

TEST_F(NativeDeviceModulesTest, ConnectUsernameDeviceAndStreamingConfig)
{
    auto users = List<IUser>();
    users.pushBack(User("jure", "jure123"));
    users.pushBack(User("tomaz", "tomaz123"));

    auto authProvider = StaticAuthenticationProvider(false, users);

    auto serverInstance = InstanceBuilder().setAuthenticationProvider(authProvider).build();
    serverInstance.addServer("openDAQ Native Streaming", nullptr);

    auto clientInstance = Instance();

    auto config = clientInstance.createDefaultAddDeviceConfig();
    PropertyObjectPtr deviceConfig = config.getPropertyValue("Device");
    PropertyObjectPtr nativeDeviceConfig = deviceConfig.getPropertyValue("opendaq_native_config");
    PropertyObjectPtr streamingConfig = config.getPropertyValue("Streaming");
    PropertyObjectPtr nativeStreamingConfig = streamingConfig.getPropertyValue("opendaq_native_streaming");

    nativeDeviceConfig.setPropertyValue("Username", "jure");
    nativeDeviceConfig.setPropertyValue("Password", "jure123");
    nativeStreamingConfig.setPropertyValue("Username", "tomaz");
    nativeStreamingConfig.setPropertyValue("Password", "tomaz123");
    auto device = clientInstance.addDevice("daq.nd://127.0.0.1", config);
    ASSERT_TRUE(device.assigned());
}

TEST_F(NativeDeviceModulesTest, DiscoveringServer)
{
    auto server = InstanceBuilder().addDiscoveryServer("mdns").setDefaultRootDeviceLocalId("local").build();
    server.addDevice("daqref://device1");

    auto serverConfig = server.getAvailableServerTypes().get("openDAQ Native Streaming").createDefaultConfig();
    auto path = "/test/native_configuration/discovery/";
    serverConfig.setPropertyValue("Path", path);
    server.addServer("openDAQ Native Streaming", serverConfig).enableDiscovery();

    auto client = Instance();
    DevicePtr device;
    for (const auto& deviceInfo : client.getAvailableDevices())
    {
        for (const auto& capability : deviceInfo.getServerCapabilities())
        {
            if (!test_helpers::isSufix(capability.getConnectionString(), path))
                break;

            if (capability.getProtocolName() == "openDAQ Native Configuration")
            {
                device = client.addDevice(capability.getConnectionString(), nullptr);
                return;
            }
        }
    }
    ASSERT_TRUE(false);
}

TEST_F(NativeDeviceModulesTest, DiscoveringServerInfoMerge)
{
    const auto info = DeviceInfo("", "foo");
    info.setMacAddress("custom_mac");
    auto server = InstanceBuilder().addDiscoveryServer("mdns")
                                   .setDefaultRootDeviceLocalId("local")
                                   .setDefaultRootDeviceInfo(info)
                                   .build();
    server.addDevice("daqref://device1");

    auto serverConfig = server.getAvailableServerTypes().get("openDAQ Native Streaming").createDefaultConfig();
    auto path = "/test/native_configuration/discovery/";
    serverConfig.setPropertyValue("Path", path);
    server.addServer("openDAQ Native Streaming", serverConfig).enableDiscovery();

    auto client = Instance();
    DevicePtr device;
    for (const auto & deviceInfo : client.getAvailableDevices())
    {
        ASSERT_EQ(deviceInfo.getMacAddress(), "");
        for (const auto & capability : deviceInfo.getServerCapabilities())
        {
            if (!test_helpers::isSufix(capability.getConnectionString(), path))
                break;
            
            if (capability.getProtocolName() == "openDAQ Native Configuration")
            {
                device = client.addDevice(capability.getConnectionString(), nullptr);
                ASSERT_EQ(device.getInfo().getMacAddress(), "custom_mac");
                return;
            }
        }
    }
    ASSERT_TRUE(false);
}

TEST_F(NativeDeviceModulesTest, RemoveServer)
{
    auto server = InstanceBuilder().addDiscoveryServer("mdns")
                                   .setDefaultRootDeviceLocalId("local")
                                   .build();
    server.addDevice("daqref://device1");

    auto serverConfig = server.getAvailableServerTypes().get("openDAQ Native Streaming").createDefaultConfig();
    auto path = "/test/native_configuration/removeServer/";
    serverConfig.setPropertyValue("Path", path);
    auto server1 = server.addServer("openDAQ Native Streaming", serverConfig);
    server1.enableDiscovery();

    // check that server is discoverable
    {
        auto client = Instance();
        size_t deviceFound = 0;
        for (const auto & deviceInfo : client.getAvailableDevices())
        {
            for (const auto & capability : deviceInfo.getServerCapabilities())
            {
                if (!test_helpers::isSufix(capability.getConnectionString(), path))
                    break;
            
                if (capability.getProtocolName() == "openDAQ Native Configuration")
                {
                   deviceFound += 1;
                }
            }
        }
        ASSERT_EQ(deviceFound, 1u);
    }

    // remove device and check that server now is not discoverable
    server.removeServer(server1);
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    {
        auto client = Instance();
        size_t deviceFound = 0;
        for (const auto & deviceInfo : client.getAvailableDevices())
        {
            for (const auto & capability : deviceInfo.getServerCapabilities())
            {
                if (!test_helpers::isSufix(capability.getConnectionString(), path))
                    break;
            
                if (capability.getProtocolName() == "openDAQ Native Configuration")
                {
                   deviceFound += 1;
                }
            }
        }
        ASSERT_EQ(deviceFound, 0u);
    }

    // add server again and check that server is discoverable
    auto path2 = "/test/native_configuration/removeServer2/";
    serverConfig.setPropertyValue("Path", path2);
    auto server2 = server.addServer("openDAQ Native Streaming", serverConfig);
    server2.enableDiscovery();
    {
        auto client = Instance();
        size_t deviceFound = 0;
        for (const auto & deviceInfo : client.getAvailableDevices())
        {
            for (const auto & capability : deviceInfo.getServerCapabilities())
            {
                bool isRemovedServer = test_helpers::isSufix(capability.getConnectionString(), path);
                bool isNewServer = test_helpers::isSufix(capability.getConnectionString(), path2);
                if (!isRemovedServer && !isNewServer)
                    break;

                if (capability.getProtocolName() == "openDAQ Native Configuration")
                {
                   deviceFound += 1;
                }
            }
        }
        ASSERT_EQ(deviceFound, 1u);
    }
}

TEST_F(NativeDeviceModulesTest, checkDeviceInfoPopulatedWithProvider)
{
    std::string filename = "populateDefaultConfig.json";
    std::string json = R"(
        {
            "Modules":
            {
                "NativeStreamingServer":
                {
                    "NativeStreamingPort": 1234,
                    "Path": "/test/native_congifurator/checkDeviceInfoPopulated/"
                }
            }
        }
    )";
    auto path = "/test/native_congifurator/checkDeviceInfoPopulated/";
    auto finally = test_helpers::CreateConfigFile(filename, json);

    auto rootInfo = DeviceInfo("");
    rootInfo.setName("TestName");
    rootInfo.setManufacturer("TestManufacturer");
    rootInfo.setModel("TestModel");
    rootInfo.setSerialNumber("TestSerialNumber");

    auto provider = JsonConfigProvider(filename);
    auto instance = InstanceBuilder().addDiscoveryServer("mdns").addConfigProvider(provider).setDefaultRootDeviceInfo(rootInfo).build();
    auto serverConfig = instance.getAvailableServerTypes().get("openDAQ Native Streaming").createDefaultConfig();
    instance.addServer("openDAQ Native Streaming", serverConfig).enableDiscovery();

    auto client = Instance();

    for (const auto & deviceInfo : client.getAvailableDevices())
    {
        for (const auto & capability : deviceInfo.getServerCapabilities())
        {
            if (!test_helpers::isSufix(capability.getConnectionString(), path))
            {
                break;
            }
            if (capability.getProtocolName() == "openDAQ Native Configuration")
            {
                client.addDevice(capability.getConnectionString(), nullptr);
                ASSERT_EQ(deviceInfo.getName(), rootInfo.getName());
                ASSERT_EQ(deviceInfo.getManufacturer(), rootInfo.getManufacturer());
                ASSERT_EQ(deviceInfo.getModel(), rootInfo.getModel());
                ASSERT_EQ(deviceInfo.getSerialNumber(), rootInfo.getSerialNumber());
                return;
            }
        }      
    }

    ASSERT_TRUE(false);
}

#ifdef _WIN32

TEST_F(NativeDeviceModulesTest, TestDiscoveryReachability)
{
    if (test_helpers::Ipv6IsDisabled())
        return;

    auto instance = InstanceBuilder().addDiscoveryServer("mdns").build();
    auto serverConfig = instance.getAvailableServerTypes().get("openDAQ Native Streaming").createDefaultConfig();
    auto path = "/test/native_congifurator/discovery_reachability/";
    serverConfig.setPropertyValue("Path", path);

    instance.addServer("openDAQ Native Streaming", serverConfig).enableDiscovery();

    auto client = Instance();

    for (const auto & deviceInfo : client.getAvailableDevices())
    {
        for (const auto & capability : deviceInfo.getServerCapabilities())
        {
            if (!test_helpers::isSufix(capability.getConnectionString(), path))
                break;

            if (capability.getProtocolName() == "openDAQ Native Configuration")
            {
                const auto ipv4Info = capability.getAddressInfo()[0];
                const auto ipv6Info = capability.getAddressInfo()[1];
                ASSERT_EQ(ipv4Info.getReachabilityStatus(), AddressReachabilityStatus::Reachable);
                ASSERT_EQ(ipv6Info.getReachabilityStatus(), AddressReachabilityStatus::Unknown);
                
                ASSERT_EQ(ipv4Info.getType(), "IPv4");
                ASSERT_EQ(ipv6Info.getType(), "IPv6");

                ASSERT_EQ(ipv4Info.getConnectionString(), capability.getConnectionStrings()[0]);
                ASSERT_EQ(ipv6Info.getConnectionString(), capability.getConnectionStrings()[1]);
                
                ASSERT_EQ(ipv4Info.getAddress(), capability.getAddresses()[0]);
                ASSERT_EQ(ipv6Info.getAddress(), capability.getAddresses()[1]);
            }
        }      
    }
}

TEST_F(NativeDeviceModulesTest, TestDiscoveryReachabilityAfterConnectIPv6)
{
    if (test_helpers::Ipv6IsDisabled())
        return;

    auto instance = InstanceBuilder().addDiscoveryServer("mdns").build();
    auto serverConfig = instance.getAvailableServerTypes().get("openDAQ OpcUa").createDefaultConfig();
    auto path = "/test/opcua/discovery_reachability/";
    serverConfig.setPropertyValue("Path", path);

    instance.addServer("openDAQ Native Streaming", serverConfig).enableDiscovery();

    auto client = Instance();
    client.getAvailableDevices();
    DevicePtr device = client.addDevice("daq.nd://[::1]/");

    ASSERT_TRUE(device.assigned());

    const auto caps = device.getInfo().getServerCapabilities();
    ASSERT_EQ(caps.getCount(), 2u);

    for (const auto& capability : caps)
    {
        if (!test_helpers::isSufix(capability.getConnectionString(), path))
            break;

        if (capability.getProtocolName() == "openDAQ Native Configuration")
        {
            const auto ipv4Info = capability.getAddressInfo()[0];
            const auto ipv6Info = capability.getAddressInfo()[1];
            ASSERT_EQ(ipv4Info.getReachabilityStatus(), AddressReachabilityStatus::Reachable);
            ASSERT_EQ(ipv6Info.getReachabilityStatus(), AddressReachabilityStatus::Reachable);
            
            ASSERT_EQ(ipv4Info.getType(), "IPv4");
            ASSERT_EQ(ipv6Info.getType(), "IPv6");

            ASSERT_EQ(ipv4Info.getConnectionString(), capability.getConnectionStrings()[0]);
            ASSERT_EQ(ipv6Info.getConnectionString(), capability.getConnectionStrings()[1]);
            
            ASSERT_EQ(ipv4Info.getAddress(), capability.getAddresses()[0]);
            ASSERT_EQ(ipv6Info.getAddress(), capability.getAddresses()[1]);
        }
    }      
}

#endif


TEST_F(NativeDeviceModulesTest, TestDiscoveryReachabilityAfterConnect)
{
    if (test_helpers::Ipv6IsDisabled())
        return;

    auto instance = InstanceBuilder().addDiscoveryServer("mdns").build();
    auto serverConfig = instance.getAvailableServerTypes().get("openDAQ Native Streaming").createDefaultConfig();
    auto path = "/test/native_congifurator/discovery_reachability/";
    serverConfig.setPropertyValue("Path", path);

    instance.addServer("openDAQ Native Streaming", serverConfig).enableDiscovery();

    auto client = Instance();
    DevicePtr device;
    for (const auto & deviceInfo : client.getAvailableDevices())
    {
        for (const auto & capability : deviceInfo.getServerCapabilities())
        {
            if (capability.getProtocolName() != "openDAQ Native Configuration")
                break;

            if (!test_helpers::isSufix(capability.getConnectionString(), path))
                break;

            device = client.addDevice(deviceInfo.getConnectionString(), nullptr);
            break;
        }

        if (device.assigned())
            break;
    }

    ASSERT_TRUE(device.assigned());

    const auto caps = device.getInfo().getServerCapabilities();
    ASSERT_EQ(caps.getCount(), 2u);

    for (const auto& capability : caps)
    {
        if (!test_helpers::isSufix(capability.getConnectionString(), path))
            break;

        if (capability.getProtocolName() == "openDAQ Native Configuration")
        {
            const auto ipv4Info = capability.getAddressInfo()[0];
            const auto ipv6Info = capability.getAddressInfo()[1];
            ASSERT_EQ(ipv4Info.getReachabilityStatus(), AddressReachabilityStatus::Reachable);
            ASSERT_EQ(ipv6Info.getReachabilityStatus(), AddressReachabilityStatus::Unknown);
            
            ASSERT_EQ(ipv4Info.getType(), "IPv4");
            ASSERT_EQ(ipv6Info.getType(), "IPv6");

            ASSERT_EQ(ipv4Info.getConnectionString(), capability.getConnectionStrings()[0]);
            ASSERT_EQ(ipv6Info.getConnectionString(), capability.getConnectionStrings()[1]);
            
            ASSERT_EQ(ipv4Info.getAddress(), capability.getAddresses()[0]);
            ASSERT_EQ(ipv6Info.getAddress(), capability.getAddresses()[1]);
        }
    }      
}

TEST_F(NativeDeviceModulesTest, GetRemoteDeviceObjects)
{
    SKIP_TEST_MAC_CI;
    auto server = CreateServerInstance();
    auto client = CreateClientInstance();

    auto signals = client.getSignals(search::Recursive(search::Any()));
    auto signalsServer = server.getSignals(search::Recursive(search::Any()));
    ASSERT_EQ(signals.getCount(), 7u);
    auto signalsVisible = client.getSignals(search::Recursive(search::Visible()));
    ASSERT_EQ(signalsVisible.getCount(), 4u);
    auto devices = client.getDevices();
    ASSERT_EQ(devices.getCount(), 1u);
    auto fbs = devices[0].getFunctionBlocks();
    ASSERT_EQ(fbs.getCount(), 1u);
    auto channels = client.getChannels(search::Recursive(search::Any()));
    ASSERT_EQ(channels.getCount(), 2u);
}

TEST_F(NativeDeviceModulesTest, RemoveDevice)
{
    SKIP_TEST_MAC_CI;
    auto server = CreateServerInstance();
    auto client = CreateClientInstance();
    auto device = client.getDevices()[0];

    ASSERT_NO_THROW(client.removeDevice(device));
    ASSERT_TRUE(device.isRemoved());
}

TEST_F(NativeDeviceModulesTest, ChangePropAfterRemove)
{
    SKIP_TEST_MAC_CI;
    auto server = CreateServerInstance();
    auto client = CreateClientInstance();
    auto device = client.getDevices()[0];

    auto refDevice = client.getDevices()[0].getDevices()[0];

    ASSERT_NO_THROW(client.removeDevice(device));

    ASSERT_TRUE(refDevice.isRemoved());

    ASSERT_THROW(refDevice.setPropertyValue("NumberOfChannels", 1), ComponentRemovedException);
}

TEST_F(NativeDeviceModulesTest, RemoteGlobalIds)
{
    SKIP_TEST_MAC_CI;
    auto server = CreateServerInstance();
    auto client = CreateClientInstance();

    const auto clientRootId = client.getGlobalId();
    const auto serverDeviceId = server.getGlobalId();
    const auto clientDeviceId = client.getDevices()[0].getGlobalId();

    const auto serverSignalId = server.getSignalsRecursive()[0].getGlobalId();
    const auto clientSignalId = client.getSignalsRecursive()[0].getGlobalId();

    ASSERT_EQ(clientDeviceId, clientRootId + "/Dev" + serverDeviceId);
    ASSERT_EQ(clientSignalId, clientRootId + "/Dev" + serverSignalId);
}

TEST_F(NativeDeviceModulesTest, GetSetDeviceProperties)
{
    SKIP_TEST_MAC_CI;
    auto server = CreateServerInstance();
    auto client = CreateClientInstance();

    auto refDevice = client.getDevices()[0].getDevices()[0];
    auto serverRefDevice = server.getDevices()[0];

    PropertyPtr propInfo;
    ASSERT_NO_THROW(propInfo = refDevice.getProperty("NumberOfChannels"));
    ASSERT_EQ(propInfo.getValueType(), CoreType::ctInt);

    ASSERT_EQ(refDevice.getPropertyValue("NumberOfChannels"), 2);
    refDevice.setPropertyValue("NumberOfChannels", 3);
    ASSERT_EQ(refDevice.getPropertyValue("NumberOfChannels"), 3);
    ASSERT_EQ(serverRefDevice.getPropertyValue("NumberOfChannels"), 3);

    refDevice.setPropertyValue("NumberOfChannels", 1);
    ASSERT_EQ(refDevice.getPropertyValue("NumberOfChannels"), 1);
    ASSERT_EQ(serverRefDevice.getPropertyValue("NumberOfChannels"), 1);

    refDevice.setPropertyValue("GlobalSampleRate", 2000);
    ASSERT_EQ(refDevice.getPropertyValue("GlobalSampleRate"), 2000);
    ASSERT_EQ(serverRefDevice.getPropertyValue("GlobalSampleRate"), 2000);

    ASSERT_ANY_THROW(refDevice.setPropertyValue("InvalidProp", 100));

    auto properties = refDevice.getAllProperties();
    ASSERT_EQ(properties.getCount(), 6u);
}

TEST_F(NativeDeviceModulesTest, DeviceInfo)
{
    SKIP_TEST_MAC_CI;
    auto server = CreateServerInstance();
    auto client = CreateClientInstance();

    auto info = client.getDevices()[0].getInfo();
    ASSERT_TRUE(info.assigned());
    ASSERT_EQ(info.getConnectionString(), "daq.nd://127.0.0.1");
    ASSERT_EQ(info.getServerCapabilities().getCount(), 2u);
    ASSERT_EQ(info.getServerCapabilities()[0].getProtocolId(), "opendaq_native_streaming");
    ASSERT_EQ(info.getServerCapabilities()[1].getProtocolId(), "opendaq_native_config");

    auto subDeviceInfo = client.getDevices()[0].getDevices()[0].getInfo();
    ASSERT_EQ(subDeviceInfo.getName(), "Device 0");
    ASSERT_EQ(subDeviceInfo.getConnectionString(), "daqref://device0");
    ASSERT_EQ(subDeviceInfo.getModel(), "Reference device");
    ASSERT_EQ(subDeviceInfo.getSerialNumber(), "dev_ser_0");
}

TEST_F(NativeDeviceModulesTest, ChannelProps)
{
    auto server = CreateServerInstance();
    auto client = CreateClientInstance();
    auto dev = client.getDevices()[0].getDevices()[0];
    auto customRangeValue = dev.getChannels()[0].getPropertyValue("CustomRange").asPtr<IStruct>();

    ASSERT_EQ(customRangeValue.get("lowValue"), -10.0);
    ASSERT_EQ(customRangeValue.get("highValue"), 10.0);
}

TEST_F(NativeDeviceModulesTest, FunctionBlockProperties)
{
    auto server = CreateServerInstance();
    auto client = CreateClientInstance();

    auto fb = client.getDevices()[0].getFunctionBlocks()[0];
    auto serverFb = server.getFunctionBlocks()[0];

    ASSERT_EQ(fb.getPropertyValue("BlockSize"), serverFb.getPropertyValue("BlockSize"));
    fb.setPropertyValue("BlockSize", 20);
    ASSERT_EQ(fb.getPropertyValue("BlockSize"), serverFb.getPropertyValue("BlockSize"));

    ASSERT_EQ(fb.getPropertyValue("DomainSignalType"), serverFb.getPropertyValue("DomainSignalType"));
    fb.setPropertyValue("DomainSignalType", 2);
    ASSERT_EQ(fb.getPropertyValue("DomainSignalType"), serverFb.getPropertyValue("DomainSignalType"));

    ASSERT_ANY_THROW(fb.setPropertyValue("DomainSignalType", 1000));
}

TEST_F(NativeDeviceModulesTest, ProcedureProp)
{
    auto server = CreateServerInstance();
    auto client = CreateClientInstance();

    auto ch = client.getDevices()[0].getDevices()[0].getChannels()[0];
    ch.setPropertyValue("Waveform", 3);
    ProcedurePtr proc = ch.getPropertyValue("ResetCounter");
    ASSERT_NO_THROW(proc());
}

TEST_F(NativeDeviceModulesTest, SignalDescriptors)
{
    auto server = CreateServerInstance();
    auto client = CreateClientInstance();

    DataDescriptorPtr dataDescriptor = client.getSignals(search::Recursive(search::Visible()))[0].getDescriptor();
    DataDescriptorPtr serverDataDescriptor = server.getSignals(search::Recursive(search::Visible()))[0].getDescriptor();

    DataDescriptorPtr domainDataDescriptor = client.getSignals(search::Recursive(search::Visible()))[2].getDescriptor();
    DataDescriptorPtr serverDomainDataDescriptor = server.getSignals(search::Recursive(search::Visible()))[2].getDescriptor();

    ASSERT_TRUE(dataDescriptor.assigned());
    ASSERT_TRUE(domainDataDescriptor.assigned());

    ASSERT_EQ(dataDescriptor, serverDataDescriptor);
    ASSERT_EQ(domainDataDescriptor, serverDomainDataDescriptor);

//    auto refChannel = client.getChannels(search::Recursive(search::Visible()))[0];
//    refChannel.setPropertyValue("ClientSideScaling", true);

//    dataDescriptor = client.getChannels(search::Recursive(search::Visible()))[0].getSignals(search::Recursive(search::Visible()))[0].getDescriptor();
//    serverDataDescriptor = server.getChannels(search::Recursive(search::Visible()))[0].getSignals(search::Recursive(search::Visible()))[0].getDescriptor();
//    ASSERT_EQ(dataDescriptor.getPostScaling().getParameters(), dataDescriptor.getPostScaling().getParameters());
}

TEST_F(NativeDeviceModulesTest, SubscribeReadUnsubscribe)
{
    SKIP_TEST_MAC_CI;
    auto server = CreateServerInstance();
    auto client = CreateClientInstance();

    auto device = client.getDevices()[0].getDevices()[0];
    const auto deviceChannel0 = device.getChannels()[0];
    const auto deviceSignal0 = deviceChannel0.getSignals(search::Recursive(search::Visible()))[0];

    ASSERT_TRUE(deviceSignal0.getDomainSignal().assigned());

    auto signal = deviceSignal0.template asPtr<IMirroredSignalConfig>();
    auto domainSignal = deviceSignal0.getDomainSignal().template asPtr<IMirroredSignalConfig>();

    StringPtr streamingSource = signal.getActiveStreamingSource();
    ASSERT_EQ(streamingSource, domainSignal.getActiveStreamingSource());

    std::promise<StringPtr> signalSubscribePromise;
    std::future<StringPtr> signalSubscribeFuture;
    test_helpers::setupSubscribeAckHandler(signalSubscribePromise, signalSubscribeFuture, signal);

    std::promise<StringPtr> domainSubscribePromise;
    std::future<StringPtr> domainSubscribeFuture;
    test_helpers::setupSubscribeAckHandler(domainSubscribePromise, domainSubscribeFuture, domainSignal);

    std::promise<StringPtr> signalUnsubscribePromise;
    std::future<StringPtr> signalUnsubscribeFuture;
    test_helpers::setupUnsubscribeAckHandler(signalUnsubscribePromise, signalUnsubscribeFuture, signal);

    std::promise<StringPtr> domainUnsubscribePromise;
    std::future<StringPtr> domainUnsubscribeFuture;
    test_helpers::setupUnsubscribeAckHandler(domainUnsubscribePromise, domainUnsubscribeFuture, domainSignal);

    using namespace std::chrono_literals;
    StreamReaderPtr reader = daq::StreamReader<double, uint64_t>(signal);

    ASSERT_TRUE(test_helpers::waitForAcknowledgement(signalSubscribeFuture));
    ASSERT_EQ(signalSubscribeFuture.get(), streamingSource);

    ASSERT_TRUE(test_helpers::waitForAcknowledgement(domainSubscribeFuture));
    ASSERT_EQ(domainSubscribeFuture.get(), streamingSource);

    double samples[100];
    for (int i = 0; i < 10; ++i)
    {
        std::this_thread::sleep_for(100ms);
        daq::SizeT count = 100;
        reader.read(samples, &count);
        EXPECT_GT(count, 0u) << "iteration " << i;
    }

    reader.release();

    ASSERT_TRUE(test_helpers::waitForAcknowledgement(signalUnsubscribeFuture));
    ASSERT_EQ(signalUnsubscribeFuture.get(), streamingSource);

    ASSERT_TRUE(test_helpers::waitForAcknowledgement(domainUnsubscribeFuture));
    ASSERT_EQ(domainUnsubscribeFuture.get(), streamingSource);
}

TEST_F(NativeDeviceModulesTest, DISABLED_RendererSimple)
{
    SKIP_TEST_MAC_CI;
    auto server = CreateServerInstance();
    auto client = CreateClientInstance();

    auto device = client.getDevices()[0].getDevices()[0];
    const auto deviceChannel0 = device.getChannels()[0];
    const auto deviceSignal0 = deviceChannel0.getSignals(search::Recursive(search::Visible()))[0];

    ASSERT_TRUE(deviceSignal0.getDomainSignal().assigned());

    const auto rendererFb = client.addFunctionBlock("RefFbModuleRenderer");
    const auto rendererInputPort0 = rendererFb.getInputPorts()[0];
    rendererInputPort0.connect(deviceSignal0);

    std::this_thread::sleep_for(std::chrono::milliseconds(5000));
}

TEST_F(NativeDeviceModulesTest, NotPublicSignals)
{
    auto server = InstanceBuilder().setDefaultRootDeviceLocalId("customLocal").build();
    auto serverDevice = server.addDevice("daqref://device1");

    auto serverChannels = serverDevice.getChannels();
    ASSERT_TRUE(serverChannels.getCount() > 0);
    for (const auto& signal : serverChannels[0].getSignals(search::Any()))
        signal.setPublic(false);

    server.addServer("openDAQ Native Streaming", nullptr);

    auto client = CreateClientInstance();
    auto clientDevice = client.getDevices()[0].getDevices()[0];

    auto clientChannels = clientDevice.getChannels();
    ASSERT_TRUE(clientChannels.getCount() > 0);
    for (const auto & signal : clientChannels[0].getSignals(search::Any()))
    {
        ASSERT_FALSE(signal.getPublic());
    }
}

TEST_F(NativeDeviceModulesTest, AddStreamingPostConnection)
{
    SKIP_TEST_MAC_CI;
    auto server = CreateServerInstance();
    auto client = CreateClientInstance();

    auto clientMirroredDevice = client.getDevices()[0].template asPtrOrNull<IMirroredDevice>();
    ASSERT_TRUE(clientMirroredDevice.assigned());
    ASSERT_EQ(clientMirroredDevice.getStreamingSources().getCount(), 1u);

    const auto clientSignals = client.getSignals(search::Recursive(search::Any()));
    for (const auto& signal : clientSignals)
    {
        auto mirorredSignal = signal.template asPtr<IMirroredSignalConfig>();
        ASSERT_EQ(mirorredSignal.getStreamingSources().getCount(), 1u);
    }

    server.addServer("openDAQ LT Streaming", nullptr);
    StreamingPtr streaming;
    ASSERT_NO_THROW(streaming = client.getDevices()[0].addStreaming("daq.lt://127.0.0.1"));
    ASSERT_EQ(clientMirroredDevice.getStreamingSources().getCount(), 2u);
    ASSERT_EQ(streaming, clientMirroredDevice.getStreamingSources()[1]);

    streaming.addSignals(clientSignals);
    for (const auto& signal : clientSignals)
    {
        auto mirorredSignal = signal.template asPtr<IMirroredSignalConfig>();
        ASSERT_EQ(mirorredSignal.getStreamingSources().getCount(), 2u);
        ASSERT_NO_THROW(mirorredSignal.setActiveStreamingSource(streaming.getConnectionString()));
    }
}

class AddComponentsTest : public NativeDeviceModulesTest, public testing::WithParamInterface<std::vector<std::string>>
{
public:
    InstancePtr createServerInstance()
    {
        InstancePtr instance = CreateDefaultServerInstance();

        instance.addServer("openDAQ LT Streaming", nullptr);
        instance.addServer("openDAQ Native Streaming", nullptr);

        return instance;
    }

    InstancePtr createClientInstance()
    {
        auto logger = Logger();
        auto scheduler = Scheduler(logger);
        auto moduleManager = ModuleManager("");
        auto typeManager = TypeManager();
        auto authenticationProvider = AuthenticationProvider();
        auto context = Context(scheduler, logger, typeManager, moduleManager, authenticationProvider);
        auto instance = InstanceCustom(context, "client");
        
        auto config = instance.createDefaultAddDeviceConfig();
        PropertyObjectPtr general = config.getPropertyValue("General");

        auto prioritizedStreamingProtocols = List<IString>();
        for (const auto& protocolId : GetParam())
            prioritizedStreamingProtocols.pushBack(protocolId);

        general.setPropertyValue("PrioritizedStreamingProtocols", prioritizedStreamingProtocols);

        instance.addDevice("daq.nd://127.0.0.1", config);
        return instance;
    }

    size_t getStreamingSourcesCount()
    {
        return GetParam().size();
    }

    std::string getActiveStreamingSource()
    {
        auto streamingProtocolIDs = GetParam();
        if (streamingProtocolIDs.empty())
            return "unknown";
        if (streamingProtocolIDs[0] == "opendaq_lt_streaming")
            return "daq.lt://127.0.0.1:7414";
        else if (streamingProtocolIDs[0] == "opendaq_native_streaming")
            return "daq.ns://127.0.0.1:7420";
        else
            return "unknown";
    }
};

TEST_P(AddComponentsTest, ConnectOnly)
{
    SKIP_TEST_MAC_CI;
    auto server = createServerInstance();
    auto client = createClientInstance();

    auto clientSignals = client.getSignals(search::Recursive(search::Any()));
    for (const auto& signal : clientSignals)
    {
        auto mirroredSignalPtr = signal.asPtr<IMirroredSignalConfig>();
        ASSERT_EQ(mirroredSignalPtr.getStreamingSources().getCount(), getStreamingSourcesCount()) << signal.getGlobalId();
        ASSERT_TRUE(mirroredSignalPtr.getActiveStreamingSource().assigned()) << signal.getGlobalId();
        ASSERT_EQ(mirroredSignalPtr.getActiveStreamingSource(), getActiveStreamingSource()) << signal.getGlobalId();
    }
}

TEST_P(AddComponentsTest, AddFunctionBlock)
{
    SKIP_TEST_MAC_CI;
    auto server = createServerInstance();
    auto client = createClientInstance();

    std::promise<void> addFbPromise;
    std::future<void> addFbFuture = addFbPromise.get_future();

    client.getContext().getOnCoreEvent() +=
        [&](const ComponentPtr& comp, const CoreEventArgsPtr& args)
    {
        auto params = args.getParameters();
        if (static_cast<CoreEventId>(args.getEventId()) == CoreEventId::ComponentAdded)
        {
            ComponentPtr component = params.get("Component");
            if (component.asPtrOrNull<IFunctionBlock>().assigned())
            {
                auto addedFb = component.asPtr<IFunctionBlock>();
                if (addedFb.getFunctionBlockType().getId() == "RefFbModuleStatistics")
                {
                    addFbPromise.set_value();
                }
            }
        }
    };

    const auto serverAddedFb = server.addFunctionBlock("RefFbModuleStatistics");
    ASSERT_TRUE(addFbFuture.wait_for(std::chrono::seconds(1)) == std::future_status::ready);

    auto clientAddedFb = client.getDevices()[0].getFunctionBlocks()[1];
    auto clientAddedFbSignals = clientAddedFb.getSignals(search::Recursive(search::Any()));
    for (const auto& signal : clientAddedFbSignals)
    {
        auto mirroredSignalPtr = signal.asPtr<IMirroredSignalConfig>();
        ASSERT_EQ(mirroredSignalPtr.getStreamingSources().getCount(), getStreamingSourcesCount()) << signal.getGlobalId();
        ASSERT_TRUE(mirroredSignalPtr.getActiveStreamingSource().assigned()) << signal.getGlobalId();
        ASSERT_EQ(mirroredSignalPtr.getActiveStreamingSource(), getActiveStreamingSource()) << signal.getGlobalId();
    }
}

TEST_F(NativeDeviceModulesTest, RemoveFunctionBlock)
{
    SKIP_TEST_MAC_CI;
    auto server = CreateServerInstance();
    auto client = CreateClientInstance();

    auto serverFb = server.getFunctionBlocks()[0];
    auto clientFb = client.getDevices()[0].getFunctionBlocks()[0];

    StringPtr removedFbId = clientFb.getGlobalId();
    std::promise<void> removeFbPromise;
    std::future<void> removeFbFuture = removeFbPromise.get_future();

    client.getContext().getOnCoreEvent() +=
        [&](const ComponentPtr& comp, const CoreEventArgsPtr& args)
    {
        auto params = args.getParameters();
        if (static_cast<CoreEventId>(args.getEventId()) == CoreEventId::ComponentRemoved)
        {
            StringPtr id = params.get("Id");
            if ((comp.getGlobalId() + "/" + id) == removedFbId)
                removeFbPromise.set_value();
        }
    };

    const auto clientFbSignals = clientFb.getSignals(search::Recursive(search::Any()));
    server.removeFunctionBlock(serverFb);
    ASSERT_TRUE(removeFbFuture.wait_for(std::chrono::seconds(1)) == std::future_status::ready);

    for (const auto& signal : clientFbSignals)
    {
        auto mirroredSignalPtr = signal.asPtr<IMirroredSignalConfig>();
        ASSERT_EQ(mirroredSignalPtr.getStreamingSources().getCount(), 0u) << signal.getGlobalId();
        ASSERT_EQ(mirroredSignalPtr.getActiveStreamingSource(), nullptr) << signal.getGlobalId();
        ASSERT_TRUE(signal.isRemoved());
    }
}

TEST_P(AddComponentsTest, AddChannel)
{
    SKIP_TEST_MAC_CI;
    auto server = createServerInstance();
    auto client = createClientInstance();

    std::promise<void> addChPromise;
    std::future<void> addChFuture = addChPromise.get_future();

    client.getContext().getOnCoreEvent() +=
        [&](const ComponentPtr& comp, const CoreEventArgsPtr& args)
    {
        auto params = args.getParameters();
        if (static_cast<CoreEventId>(args.getEventId()) == CoreEventId::ComponentAdded)
        {
            ComponentPtr component = params.get("Component");
            if (component.asPtrOrNull<IChannel>().assigned())
            {
                auto addedCh = component.asPtr<IChannel>();
                if (addedCh.getFunctionBlockType().getId() == "ref_channel")
                {
                    addChPromise.set_value();
                }
            }
        }
    };

    auto refDevice = client.getDevices()[0].getDevices()[0];
    refDevice.setPropertyValue("NumberOfChannels", 3);
    ASSERT_TRUE(addChFuture.wait_for(std::chrono::seconds(1)) == std::future_status::ready);

    auto clientAddedCh = client.getDevices()[0].getDevices()[0].getChannels()[2];
    auto clientAddedChSignals = clientAddedCh.getSignals(search::Recursive(search::Any()));
    for (const auto& signal : clientAddedChSignals)
    {
        auto mirroredSignalPtr = signal.asPtr<IMirroredSignalConfig>();
        ASSERT_EQ(mirroredSignalPtr.getStreamingSources().getCount(), getStreamingSourcesCount()) << signal.getGlobalId();
        ASSERT_TRUE(mirroredSignalPtr.getActiveStreamingSource().assigned()) << signal.getGlobalId();
        ASSERT_EQ(mirroredSignalPtr.getActiveStreamingSource(), getActiveStreamingSource()) << signal.getGlobalId();
    }
}

TEST_F(NativeDeviceModulesTest, RemoveChannel)
{
    SKIP_TEST_MAC_CI;
    auto server = CreateServerInstance();
    auto client = CreateClientInstance();

    auto refDevice = client.getDevices()[0].getDevices()[0];
    auto clientCh = refDevice.getChannels()[1];

    StringPtr removedChId = clientCh.getGlobalId();
    std::promise<void> removeChPromise;
    std::future<void> removeChFuture = removeChPromise.get_future();

    client.getContext().getOnCoreEvent() +=
        [&](const ComponentPtr& comp, const CoreEventArgsPtr& args)
    {
        auto params = args.getParameters();
        if (static_cast<CoreEventId>(args.getEventId()) == CoreEventId::ComponentRemoved)
        {
            StringPtr id = params.get("Id");
            if ((comp.getGlobalId() + "/" + id) == removedChId)
                removeChPromise.set_value();
        }
    };

    const auto clientChSignals = clientCh.getSignals(search::Recursive(search::Any()));
    refDevice.setPropertyValue("NumberOfChannels", 1);
    ASSERT_TRUE(removeChFuture.wait_for(std::chrono::seconds(1)) == std::future_status::ready);

    for (const auto& signal : clientChSignals)
    {
        auto mirroredSignalPtr = signal.asPtr<IMirroredSignalConfig>();
        ASSERT_EQ(mirroredSignalPtr.getStreamingSources().getCount(), 0u) << signal.getGlobalId();
        ASSERT_EQ(mirroredSignalPtr.getActiveStreamingSource(), nullptr) << signal.getGlobalId();
        ASSERT_TRUE(signal.isRemoved());
    }
}

TEST_P(AddComponentsTest, AddDevice)
{
    SKIP_TEST_MAC_CI;
    auto server = createServerInstance();
    auto client = createClientInstance();

    std::promise<void> addDevPromise;
    std::future<void> addDevFuture = addDevPromise.get_future();

    client.getContext().getOnCoreEvent() +=
        [&](const ComponentPtr& comp, const CoreEventArgsPtr& args)
    {
        auto params = args.getParameters();
        if (static_cast<CoreEventId>(args.getEventId()) == CoreEventId::ComponentAdded)
        {
            ComponentPtr component = params.get("Component");
            if (component.asPtrOrNull<IDevice>().assigned())
            {
                auto addedDev = component.asPtr<IDevice>();
                if (addedDev.getLocalId() == "RefDev1")
                {
                    addDevPromise.set_value();
                }
            }
        }
    };

    const auto serverAddedDev = server.addDevice("daqref://device1");
    ASSERT_TRUE(addDevFuture.wait_for(std::chrono::seconds(1)) == std::future_status::ready);

    auto clientAddedDev = client.getDevices()[0].getDevices()[1];
    auto clientAddedDevSignals = clientAddedDev.getSignals(search::Recursive(search::Any()));
    for (const auto& signal : clientAddedDevSignals)
    {
        auto mirroredSignalPtr = signal.asPtr<IMirroredSignalConfig>();
        ASSERT_EQ(mirroredSignalPtr.getStreamingSources().getCount(), getStreamingSourcesCount()) << signal.getGlobalId();
        ASSERT_TRUE(mirroredSignalPtr.getActiveStreamingSource().assigned()) << signal.getGlobalId();
        ASSERT_EQ(mirroredSignalPtr.getActiveStreamingSource(), getActiveStreamingSource()) << signal.getGlobalId();
    }
}

TEST_F(NativeDeviceModulesTest, RemoveSubDevice)
{
    SKIP_TEST_MAC_CI;
    auto server = CreateServerInstance();
    auto client = CreateClientInstance();

    auto serverDevice = server.getDevices()[0];
    auto clientDevice = client.getDevices()[0].getDevices()[0];
    auto clientCh = clientDevice.getChannels()[1];

    StringPtr removedDevId = clientDevice.getGlobalId();
    std::promise<void> removeDevPromise;
    std::future<void> removeDevFuture = removeDevPromise.get_future();

    client.getContext().getOnCoreEvent() +=
        [&](const ComponentPtr& comp, const CoreEventArgsPtr& args)
    {
        auto params = args.getParameters();
        if (static_cast<CoreEventId>(args.getEventId()) == CoreEventId::ComponentRemoved)
        {
            StringPtr id = params.get("Id");
            if ((comp.getGlobalId() + "/" + id) == removedDevId)
                removeDevPromise.set_value();
        }
    };

    const auto clientDevSignals = clientDevice.getSignals(search::Recursive(search::Any()));
    server.removeDevice(serverDevice);
    ASSERT_TRUE(removeDevFuture.wait_for(std::chrono::seconds(1)) == std::future_status::ready);

    for (const auto& signal : clientDevSignals)
    {
        auto mirroredSignalPtr = signal.asPtr<IMirroredSignalConfig>();
        ASSERT_EQ(mirroredSignalPtr.getStreamingSources().getCount(), 0u) << signal.getGlobalId();
        ASSERT_EQ(mirroredSignalPtr.getActiveStreamingSource(), nullptr) << signal.getGlobalId();
        ASSERT_TRUE(signal.isRemoved());
    }
}

INSTANTIATE_TEST_SUITE_P(
    DynamicComponentsTestGroup,
    AddComponentsTest,
    testing::Values(
        std::vector<std::string>({"opendaq_native_streaming"}),
        std::vector<std::string>({"opendaq_lt_streaming"}),
        std::vector<std::string>({"opendaq_lt_streaming", "opendaq_native_streaming"}),
        std::vector<std::string>({"opendaq_native_streaming", "opendaq_lt_streaming"})
    )
);

TEST_F(NativeDeviceModulesTest, SdkPackageVersion)
{
    SKIP_TEST_MAC_CI;
    auto instance = InstanceBuilder().setDefaultRootDeviceInfo(DeviceInfo("", "dev", "custom")).build();
    instance.addServer("openDAQ Native Streaming", nullptr);
    auto client = CreateClientInstance();

    ASSERT_EQ(client.getDevices()[0].getInfo().getSdkVersion(),  "custom");
}

static void CreateConfigFile(const std::string& data)
{
    std::ofstream file;
    file.open("opendaq-config.json");
    if (!file.is_open()) 
        throw std::runtime_error("can not open file for writing");

    file << data;
    file.close();
}

static void RemoveConfigFile()
{
    remove("opendaq-config.json");
}

TEST_F(NativeDeviceModulesTest, ConfiguringWithOptions)
{
    std::string options = R"(
    {
    "Modules": {
        "NativeStreamingClient": {
            "MonitoringEnabled": true,
            "HeartbeatPeriod": 100,
            "InactivityTimeout": 200,
            "ConnectionTimeout": 300,
            "StreamingInitTimeout": 400,
            "ReconnectionPeriod": 500
            }
        }
    }
    )";
    
    CreateConfigFile(options);
    Finally final([] { RemoveConfigFile(); });

    InstancePtr instance;
    ASSERT_NO_THROW(instance = InstanceBuilder().addConfigProvider(JsonConfigProvider("opendaq-config.json")).build());

    auto deviceConfig = instance.getAvailableDeviceTypes().get("opendaq_native_config").createDefaultConfig();
    ASSERT_TRUE(deviceConfig.hasProperty("TransportLayerConfig"));
    PropertyObjectPtr transportLayerConfig = deviceConfig.getPropertyValue("TransportLayerConfig");

    ASSERT_EQ(transportLayerConfig.getPropertyValue("MonitoringEnabled"), True);
    ASSERT_EQ(transportLayerConfig.getPropertyValue("HeartbeatPeriod"), 100);
    ASSERT_EQ(transportLayerConfig.getPropertyValue("InactivityTimeout"), 200);
    ASSERT_EQ(transportLayerConfig.getPropertyValue("ConnectionTimeout"), 300);
    ASSERT_EQ(transportLayerConfig.getPropertyValue("StreamingInitTimeout"), 400);
    ASSERT_EQ(transportLayerConfig.getPropertyValue("ReconnectionPeriod"), 500);

    auto pseudoDeviceConfig = instance.getAvailableDeviceTypes().get("opendaq_native_streaming").createDefaultConfig();
    ASSERT_TRUE(pseudoDeviceConfig.hasProperty("TransportLayerConfig"));
    transportLayerConfig = pseudoDeviceConfig.getPropertyValue("TransportLayerConfig");

    ASSERT_EQ(transportLayerConfig.getPropertyValue("MonitoringEnabled"), True);
    ASSERT_EQ(transportLayerConfig.getPropertyValue("HeartbeatPeriod"), 100);
    ASSERT_EQ(transportLayerConfig.getPropertyValue("InactivityTimeout"), 200);
    ASSERT_EQ(transportLayerConfig.getPropertyValue("ConnectionTimeout"), 300);
    ASSERT_EQ(transportLayerConfig.getPropertyValue("StreamingInitTimeout"), 400);
    ASSERT_EQ(transportLayerConfig.getPropertyValue("ReconnectionPeriod"), 500);
}

TEST_F(NativeDeviceModulesTest, Reconnection)
{
    SKIP_TEST_MAC_CI;
    auto server = CreateServerInstance();
    auto client = CreateClientInstance();

    ASSERT_EQ(client.getDevices()[0].getStatusContainer().getStatus("ConnectionStatus"), "Connected");

    std::promise<StringPtr> reconnectionStatusPromise;
    std::future<StringPtr> reconnectionStatusFuture = reconnectionStatusPromise.get_future();
    client.getDevices()[0].getOnComponentCoreEvent() +=
        [&](const ComponentPtr& /*comp*/, const CoreEventArgsPtr& args)
    {
        auto params = args.getParameters();
        if (static_cast<CoreEventId>(args.getEventId()) == CoreEventId::StatusChanged)
        {
            ASSERT_TRUE(args.getParameters().hasKey("ConnectionStatus"));
            reconnectionStatusPromise.set_value(args.getParameters().get("ConnectionStatus").toString());
        }
    };

    // destroy server to emulate disconnection
    server.release();
    ASSERT_TRUE(reconnectionStatusFuture.wait_for(std::chrono::seconds(5)) == std::future_status::ready);
    ASSERT_EQ(reconnectionStatusFuture.get(), "Reconnecting");
    ASSERT_EQ(client.getDevices()[0].getStatusContainer().getStatus("ConnectionStatus"), "Reconnecting");

    // reset future / promise
    reconnectionStatusPromise = std::promise<StringPtr>();
    reconnectionStatusFuture = reconnectionStatusPromise.get_future();

    // re-create updated server
    server = CreateServerInstance(CreateUpdatedServerInstance());

    ASSERT_TRUE(reconnectionStatusFuture.wait_for(std::chrono::seconds(5)) == std::future_status::ready);
    ASSERT_EQ(reconnectionStatusFuture.get(), "Connected");
    ASSERT_EQ(client.getDevices()[0].getStatusContainer().getStatus("ConnectionStatus"), "Connected");

    auto channels = client.getDevices()[0].getChannels(search::Recursive(search::Any()));
    ASSERT_EQ(channels.getCount(), 3u);

    auto fbs = client.getDevices()[0].getFunctionBlocks(search::Recursive(search::Any()));
    ASSERT_EQ(fbs.getCount(), 1u);
    ASSERT_EQ(fbs[0].getFunctionBlockType().getId(), "RefFbModuleScaling");

    ASSERT_TRUE(client.getContext().getTypeManager().hasType("TestEnumType"));

    auto signals = client.getSignals(search::Recursive(search::Any()));
    for (const auto& signal : signals)
    {
        auto mirroredSignalPtr = signal.asPtr<IMirroredSignalConfig>();
        ASSERT_GT(mirroredSignalPtr.getStreamingSources().getCount(), 0u) << signal.getGlobalId();
        ASSERT_TRUE(mirroredSignalPtr.getActiveStreamingSource().assigned()) << signal.getGlobalId();
    }
}

TEST_F(NativeDeviceModulesTest, Update)
{
    SKIP_TEST_MAC_CI;
    auto server = CreateServerInstance();
    auto client = CreateClientInstance();

    std::promise<void> updatedPromise;
    std::future<void> updatedFuture = updatedPromise.get_future();

    client.getContext().getOnCoreEvent() +=
        [&](const ComponentPtr& component, const CoreEventArgsPtr& args)
    {
        auto params = args.getParameters();
        if (static_cast<CoreEventId>(args.getEventId()) == CoreEventId::ComponentUpdateEnd)
        {
            updatedPromise.set_value();
        }
    };

    // update device
    auto updatableDevice = client.getDevices()[0].getDevices()[0];
    const auto serializer = JsonSerializer();
    updatableDevice.serialize(serializer);
    const auto str = serializer.getOutput();
    const auto deserializer = JsonDeserializer();
    deserializer.update(updatableDevice, str);

    ASSERT_TRUE(updatedFuture.wait_for(std::chrono::seconds(1)) == std::future_status::ready);

    auto clientSignals = client.getSignals(search::Recursive(search::Any()));
    for (const auto& signal : clientSignals)
    {
        auto mirroredSignalPtr = signal.asPtr<IMirroredSignalConfig>();
        ASSERT_GT(mirroredSignalPtr.getStreamingSources().getCount(), 0u) << signal.getGlobalId();
        ASSERT_TRUE(mirroredSignalPtr.getActiveStreamingSource().assigned()) << signal.getGlobalId();
    }
}

TEST_F(NativeDeviceModulesTest, GetConfigurationConnectionInfo)
{
    SKIP_TEST_MAC_CI;
    auto server = CreateServerInstance();
    auto client = CreateClientInstance();

    auto devices = client.getDevices();
    ASSERT_EQ(devices.getCount(), 1u);

    auto connectionInfo = devices[0].getInfo().getConfigurationConnectionInfo();
    ASSERT_EQ(connectionInfo.getProtocolId(), "opendaq_native_config");
    ASSERT_EQ(connectionInfo.getProtocolName(), "openDAQ Native Configuration");
    ASSERT_EQ(connectionInfo.getProtocolType(), ProtocolType::ConfigurationAndStreaming);
    ASSERT_EQ(connectionInfo.getConnectionType(), "TCP/IP");
    ASSERT_EQ(connectionInfo.getAddresses()[0], "127.0.0.1");
    ASSERT_EQ(connectionInfo.getPort(), 7420);
    ASSERT_EQ(connectionInfo.getPrefix(), "daq.nd");
    ASSERT_EQ(connectionInfo.getConnectionString(), "daq.nd://127.0.0.1");
}

TEST_F(NativeDeviceModulesTest, TestAddressInfoIPv4)
{
    auto server = InstanceBuilder().setRootDevice("daqref://device0").build();
    server.addServer("openDAQ Native Streaming", nullptr);
    server.addServer("openDAQ LT Streaming", nullptr);
    server.addServer("openDAQ OpcUa", nullptr);

    auto client = Instance();
    const auto dev = client.addDevice("daq.nd://127.0.0.1");
    const auto info = dev.getInfo();

    ASSERT_TRUE(info.hasServerCapability("opendaq_native_config"));
    ASSERT_TRUE(info.hasServerCapability("opendaq_opcua_config"));
    ASSERT_TRUE(info.hasServerCapability("opendaq_native_streaming"));
    ASSERT_TRUE(info.hasServerCapability("opendaq_lt_streaming"));

    const auto opcuaCapability = info.getServerCapability("opendaq_opcua_config");
    const auto nativeConfigCapability = info.getServerCapability("opendaq_native_config");
    const auto nativeStreamingCapability = info.getServerCapability("opendaq_native_streaming");
    const auto LTCapability = info.getServerCapability("opendaq_lt_streaming");

    ASSERT_TRUE(opcuaCapability.getConnectionString().assigned() && opcuaCapability.getConnectionString() != "");
    ASSERT_TRUE(nativeConfigCapability.getConnectionString().assigned() && nativeConfigCapability.getConnectionString() != "");
    ASSERT_TRUE(nativeStreamingCapability.getConnectionString().assigned() && nativeStreamingCapability.getConnectionString() != "");
    ASSERT_TRUE(LTCapability.getConnectionString().assigned() && LTCapability.getConnectionString() != "");

    const auto opcuaAddressInfo = opcuaCapability.getAddressInfo()[0];
    const auto nativeConfigAddressInfo= nativeConfigCapability.getAddressInfo()[0];
    const auto nativeStreamingAddressInfo = nativeStreamingCapability.getAddressInfo()[0];
    const auto LTAddressInfo = LTCapability.getAddressInfo()[0];

    ASSERT_EQ(opcuaAddressInfo.getType(), "IPv4");
    ASSERT_EQ(nativeConfigAddressInfo.getType(), "IPv4");
    ASSERT_EQ(nativeStreamingAddressInfo.getType(), "IPv4");
    ASSERT_EQ(LTAddressInfo.getType(), "IPv4");

    ASSERT_EQ(opcuaAddressInfo.getReachabilityStatus(), AddressReachabilityStatus::Reachable);
    ASSERT_EQ(nativeConfigAddressInfo.getReachabilityStatus(), AddressReachabilityStatus::Reachable);
    ASSERT_EQ(nativeStreamingAddressInfo.getReachabilityStatus(), AddressReachabilityStatus::Reachable);
    ASSERT_EQ(LTAddressInfo.getReachabilityStatus(), AddressReachabilityStatus::Reachable);
}

TEST_F(NativeDeviceModulesTest, TestAddressInfoIPv6)
{
    if (test_helpers::Ipv6IsDisabled())
        return;

    auto server = InstanceBuilder().setRootDevice("daqref://device0").build();
    server.addServer("openDAQ Native Streaming", nullptr);
    server.addServer("openDAQ OpcUa", nullptr);
    server.addServer("openDAQ LT Streaming", nullptr);

    auto client = Instance();
    const auto dev = client.addDevice("daq.nd://[::1]");
    const auto info = dev.getInfo();

    ASSERT_TRUE(info.hasServerCapability("opendaq_native_config"));
    ASSERT_TRUE(info.hasServerCapability("opendaq_opcua_config"));
    ASSERT_TRUE(info.hasServerCapability("opendaq_native_streaming"));
    ASSERT_TRUE(info.hasServerCapability("opendaq_lt_streaming"));

    const auto opcuaCapability = info.getServerCapability("opendaq_opcua_config");
    const auto nativeConfigCapability = info.getServerCapability("opendaq_native_config");
    const auto nativeStreamingCapability = info.getServerCapability("opendaq_native_streaming");
    const auto LTCapability = info.getServerCapability("opendaq_lt_streaming");

    ASSERT_TRUE(opcuaCapability.getConnectionString().assigned() && opcuaCapability.getConnectionString() != "");
    ASSERT_TRUE(nativeConfigCapability.getConnectionString().assigned() && nativeConfigCapability.getConnectionString() != "");
    ASSERT_TRUE(nativeStreamingCapability.getConnectionString().assigned() && nativeStreamingCapability.getConnectionString() != "");
    ASSERT_TRUE(LTCapability.getConnectionString().assigned() && LTCapability.getConnectionString() != "");

    const auto opcuaAddressInfo = opcuaCapability.getAddressInfo()[0];
    const auto nativeConfigAddressInfo= nativeConfigCapability.getAddressInfo()[0];
    const auto nativeStreamingAddressInfo = nativeStreamingCapability.getAddressInfo()[0];
    const auto LTAddressInfo = LTCapability.getAddressInfo()[0];

    ASSERT_EQ(opcuaAddressInfo.getType(), "IPv6");
    ASSERT_EQ(nativeConfigAddressInfo.getType(), "IPv6");
    ASSERT_EQ(nativeStreamingAddressInfo.getType(), "IPv6");
    ASSERT_EQ(LTAddressInfo.getType(), "IPv6");

    ASSERT_EQ(opcuaAddressInfo.getReachabilityStatus(), AddressReachabilityStatus::Reachable);
    ASSERT_EQ(nativeConfigAddressInfo.getReachabilityStatus(), AddressReachabilityStatus::Reachable);
    ASSERT_EQ(nativeStreamingAddressInfo.getReachabilityStatus(), AddressReachabilityStatus::Reachable);
    ASSERT_EQ(LTAddressInfo.getReachabilityStatus(), AddressReachabilityStatus::Reachable);
}

TEST_F(NativeDeviceModulesTest, TestAddressInfoGatewayDevice)
{
    auto server = InstanceBuilder().setRootDevice("daqref://device0").build();
    server.addServer("openDAQ Native Streaming", nullptr);
    server.addServer("openDAQ LT Streaming", nullptr);
    server.addServer("openDAQ OpcUa", nullptr);

    auto gateway = Instance();
    auto serverConfig = gateway.getAvailableServerTypes().get("openDAQ Native Streaming").createDefaultConfig();
    serverConfig.setPropertyValue("NativeStreamingPort", 7421);
    gateway.addDevice("daq.nd://127.0.0.1");
    gateway.addServer("openDAQ Native Streaming", serverConfig);

    auto client = Instance();
    const auto dev = client.addDevice("daq.nd://127.0.0.1:7421/");
    const auto info = dev.getDevices()[0].getInfo();

    ASSERT_TRUE(info.hasServerCapability("opendaq_native_config"));
    ASSERT_TRUE(info.hasServerCapability("opendaq_opcua_config"));
    ASSERT_TRUE(info.hasServerCapability("opendaq_native_streaming"));
    ASSERT_TRUE(info.hasServerCapability("opendaq_lt_streaming"));

    const auto opcuaCapability = info.getServerCapability("opendaq_opcua_config");
    const auto nativeConfigCapability = info.getServerCapability("opendaq_native_config");
    const auto nativeStreamingCapability = info.getServerCapability("opendaq_native_streaming");
    const auto LTCapability = info.getServerCapability("opendaq_lt_streaming");

    ASSERT_TRUE(opcuaCapability.getConnectionString().assigned() && opcuaCapability.getConnectionString() != "");
    ASSERT_TRUE(nativeConfigCapability.getConnectionString().assigned() && nativeConfigCapability.getConnectionString() != "");
    ASSERT_TRUE(nativeStreamingCapability.getConnectionString().assigned() && nativeStreamingCapability.getConnectionString() != "");
    ASSERT_TRUE(LTCapability.getConnectionString().assigned() && LTCapability.getConnectionString() != "");

    const auto opcuaAddressInfo = opcuaCapability.getAddressInfo()[0];
    const auto nativeConfigAddressInfo= nativeConfigCapability.getAddressInfo()[0];
    const auto nativeStreamingAddressInfo = nativeStreamingCapability.getAddressInfo()[0];
    const auto LTAddressInfo = LTCapability.getAddressInfo()[0];

    ASSERT_EQ(opcuaAddressInfo.getType(), "IPv4");
    ASSERT_EQ(nativeConfigAddressInfo.getType(), "IPv4");
    ASSERT_EQ(nativeStreamingAddressInfo.getType(), "IPv4");
    ASSERT_EQ(LTAddressInfo.getType(), "IPv4");

    ASSERT_EQ(opcuaAddressInfo.getReachabilityStatus(), AddressReachabilityStatus::Reachable);
    ASSERT_EQ(nativeConfigAddressInfo.getReachabilityStatus(), AddressReachabilityStatus::Reachable);
    ASSERT_EQ(nativeStreamingAddressInfo.getReachabilityStatus(), AddressReachabilityStatus::Reachable);
    ASSERT_EQ(LTAddressInfo.getReachabilityStatus(), AddressReachabilityStatus::Reachable);
}

TEST_F(NativeDeviceModulesTest, MultiClientReadChangingSignal)
{
    SKIP_TEST_MAC_CI;
    auto server = CreateServerInstance();

    // Set up first client
    auto client1 = CreateClientInstance();
    auto client1Signal = client1.getDevices()[0].getDevices()[0].getSignalsRecursive()[0].template asPtr<IMirroredSignalConfig>();

    std::promise<StringPtr> client1SignalSubscribePromise;
    std::future<StringPtr> client1SignalSubscribeFuture;
    test_helpers::setupSubscribeAckHandler(client1SignalSubscribePromise, client1SignalSubscribeFuture, client1Signal);

    StreamReaderPtr client1Reader = daq::StreamReader<double, uint64_t>(client1Signal);
    ASSERT_TRUE(test_helpers::waitForAcknowledgement(client1SignalSubscribeFuture));

    // change samplerate to trigger signal changes
    auto serverRefDevice = server.getDevices()[0];
    serverRefDevice.setPropertyValue("GlobalSampleRate", 2000);

    // Set up second client
    auto client2 = CreateClientInstance();
    auto client2Signal = client2.getDevices()[0].getDevices()[0].getSignalsRecursive()[0].template asPtr<IMirroredSignalConfig>();

    std::promise<StringPtr> client2SignalSubscribePromise;
    std::future<StringPtr> client2SignalSubscribeFuture;
    test_helpers::setupSubscribeAckHandler(client2SignalSubscribePromise, client2SignalSubscribeFuture, client2Signal);

    StreamReaderPtr client2Reader = daq::StreamReader<double, uint64_t>(client2Signal);
    ASSERT_TRUE(test_helpers::waitForAcknowledgement(client2SignalSubscribeFuture));

    // read some
    daq::SizeT client1SamplesRead = 0;
    daq::SizeT client2SamplesRead = 0;
    double samples[100];
    for (int i = 0; i < 3; ++i)
    {
        using namespace std::chrono_literals;
        std::this_thread::sleep_for(100ms);

        daq::SizeT count = 100;
        client1Reader.read(samples, &count);
        client1SamplesRead += count;

        count = 100;
        client2Reader.read(samples, &count);
        client2SamplesRead += count;
    }
    EXPECT_GT(client1SamplesRead, 0u);
    EXPECT_GT(client2SamplesRead, 0u);
}
