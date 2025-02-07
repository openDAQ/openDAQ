#include <opcuatms/exceptions.h>
#include <opendaq/logger_sink_ptr.h>
#include <opendaq/logger_sink_last_message_private_ptr.h>
#include <opcuashared/opcuaexception.h>
#include "test_helpers/test_helpers.h"
#include <coreobjects/authentication_provider_factory.h>
#include <coreobjects/user_factory.h>
#include <opendaq/device_impl.h>
#include <opendaq/module_info_factory.h>
#include <opendaq/device_type_factory.h>

using OpcuaDeviceModulesTest = testing::Test;

using namespace daq;

static InstancePtr CreateServerInstance(const AuthenticationProviderPtr& authenticationProvider)
{
    auto logger = Logger();
    auto scheduler = Scheduler(logger);
    auto moduleManager = ModuleManager("");
    auto typeManager = TypeManager();
    auto context = Context(scheduler, logger, typeManager, moduleManager, authenticationProvider);

    auto instance = InstanceCustom(context, "local");

    const auto statistics = instance.addFunctionBlock("RefFBModuleStatistics");
    const auto refDevice = instance.addDevice("daqref://device1");
    statistics.getInputPorts()[0].connect(refDevice.getSignals(search::Recursive(search::Visible()))[0]);
    statistics.getInputPorts()[0].connect(Signal(context, nullptr, "foo"));

    instance.addServer("OpenDAQOPCUA", nullptr);

    return instance;
}

static InstancePtr CreateServerInstance()
{
    return CreateServerInstance(AuthenticationProvider());
}

static InstancePtr CreateClientInstance(const InstanceBuilderPtr& builder = InstanceBuilder())
{
    auto instance = builder.build();

    auto config = instance.createDefaultAddDeviceConfig();
    PropertyObjectPtr general = config.getPropertyValue("General");
    general.setPropertyValue("StreamingConnectionHeuristic", 2);

    auto refDevice = instance.addDevice("daq.opcua://127.0.0.1", config);
    return instance;
}


TEST_F(OpcuaDeviceModulesTest, ConnectAndDisconnect)
{
    SKIP_TEST_MAC_CI;
    auto server = CreateServerInstance();
    auto client = CreateClientInstance();

    client->releaseRef();
    server->releaseRef();
    client.detach();
    server.detach();
}

TEST_F(OpcuaDeviceModulesTest, ConnectViaIpv6)
{
    if (test_helpers::Ipv6IsDisabled())
    {
        GTEST_SKIP() << "Ipv6 is disabled";
    }

    auto server = CreateServerInstance();
    auto client = Instance();
    client.addDevice("daq.opcua://[::1]");
}

TEST_F(OpcuaDeviceModulesTest, PopulateDefaultConfigFromProvider)
{
    std::string filename = "populateDefaultConfig.json";
    std::string json = R"(
        {
            "Modules":
            {
                "OpenDAQOPCUAServerModule":
                {
                    "Port": 1234,
                    "Path": "/some/path"
                }
            }
        }
    )";
    auto finally = test_helpers::CreateConfigFile(filename, json);

    auto provider = JsonConfigProvider(filename);
    auto instance = InstanceBuilder().addConfigProvider(provider).build();
    auto serverConfig = instance.getAvailableServerTypes().get("OpenDAQOPCUA").createDefaultConfig();

    ASSERT_EQ(serverConfig.getPropertyValue("Port").asPtr<IInteger>(), 1234);
    ASSERT_EQ(serverConfig.getPropertyValue("Path").asPtr<IString>(), "/some/path");
}

TEST_F(OpcuaDeviceModulesTest, DiscoveringServer)
{
    auto server = InstanceBuilder().addDiscoveryServer("mdns").setDefaultRootDeviceLocalId("local").build();
    server.addDevice("daqref://device1");

    auto serverConfig = server.getAvailableServerTypes().get("OpenDAQOPCUA").createDefaultConfig();
    auto path = "/test/opcua/discoveryServer/";
    serverConfig.setPropertyValue("Path", path);
    server.addServer("OpenDAQOPCUA", serverConfig).enableDiscovery();

    auto client = Instance();
    DevicePtr device;
    for (const auto & deviceInfo : client.getAvailableDevices())
    {
        for (const auto & capability : deviceInfo.getServerCapabilities())
        {
            if (!test_helpers::isSufix(capability.getConnectionString(), path))
                break;

            ASSERT_EQ(capability.getProtocolName(), "OpenDAQOPCUA");
            device = client.addDevice(capability.getConnectionString(), nullptr);
            return;
        }
    }
    ASSERT_TRUE(false) << "Device not found";
}

TEST_F(OpcuaDeviceModulesTest, checkDeviceInfoPopulatedWithProvider)
{
    std::string filename = "populateDefaultConfig.json";
    std::string json = R"(
        {
            "Modules":
            {
                "OpenDAQOPCUAServerModule":
                {
                    "Port": 1234,
                    "Path": "/test/opcua/checkDeviceInfoPopulated/"
                }
            }
        }
    )";
    auto path = "/test/opcua/checkDeviceInfoPopulated/";
    auto finally = test_helpers::CreateConfigFile(filename, json);

    auto rootInfo = DeviceInfo("");
    rootInfo.setName("TestName");
    rootInfo.setManufacturer("TestManufacturer");
    rootInfo.setModel("TestModel");
    rootInfo.setSerialNumber("TestSerialNumber");

    auto provider = JsonConfigProvider(filename);
    auto instance = InstanceBuilder().addDiscoveryServer("mdns").addConfigProvider(provider).setDefaultRootDeviceInfo(rootInfo).build();
    instance.addDevice("daqref://device1");
    auto serverConfig = instance.getAvailableServerTypes().get("OpenDAQOPCUA").createDefaultConfig();
    instance.addServer("OpenDAQOPCUA", serverConfig).enableDiscovery();

    auto client = Instance();
    
    for (const auto & deviceInfo : client.getAvailableDevices())
    {
        for (const auto & capability : deviceInfo.getServerCapabilities())
        {
            if (!test_helpers::isSufix(capability.getConnectionString(), path))
                break;

            ASSERT_EQ(capability.getProtocolName(), "OpenDAQOPCUA");
            client.addDevice(capability.getConnectionString(), nullptr);

            ASSERT_EQ(deviceInfo.getName(), rootInfo.getName());
            ASSERT_EQ(deviceInfo.getManufacturer(), rootInfo.getManufacturer());
            ASSERT_EQ(deviceInfo.getModel(), rootInfo.getModel());
            ASSERT_EQ(deviceInfo.getSerialNumber(), rootInfo.getSerialNumber());
            return;
        }      
    }

    ASSERT_TRUE(false) << "Device not found";
}

#ifdef _WIN32

TEST_F(OpcuaDeviceModulesTest, TestDiscoveryReachability)
{
    if (test_helpers::Ipv6IsDisabled())
    {
        GTEST_SKIP() << "Ipv6 is disabled";
    }

    auto instance = InstanceBuilder().addDiscoveryServer("mdns").build();
    auto serverConfig = instance.getAvailableServerTypes().get("OpenDAQOPCUA").createDefaultConfig();
    auto path = "/test/opcua/discoveryReachability/";
    serverConfig.setPropertyValue("Path", path);
    instance.addServer("OpenDAQOPCUA", serverConfig).enableDiscovery();

    auto client = Instance();

    for (const auto & deviceInfo : client.getAvailableDevices())
    {
        for (const auto & capability : deviceInfo.getServerCapabilities())
        {
            if (!test_helpers::isSufix(capability.getConnectionString(), path))
                break;

            ASSERT_EQ(capability.getProtocolName(), "OpenDAQOPCUA");
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
            return;
        }      
    }
    ASSERT_TRUE(false) << "Device not found";
}

TEST_F(OpcuaDeviceModulesTest, TestDiscoveryReachabilityAfterConnectIPv6)
{
    if (test_helpers::Ipv6IsDisabled())
    {
        GTEST_SKIP() << "Ipv6 is disabled";
    }

    auto instance = InstanceBuilder().addDiscoveryServer("mdns").build();
    auto serverConfig = instance.getAvailableServerTypes().get("OpenDAQOPCUA").createDefaultConfig();
    auto path = "/test/opcua/discoveryReachabilityAfterConnectIPv6/";
    serverConfig.setPropertyValue("Path", path);

    instance.addServer("OpenDAQOPCUA", serverConfig).enableDiscovery();

    auto client = Instance();
    // client.getAvailableDevices();
    DevicePtr device = client.addDevice(std::string("daq.opcua://[::1]") + path);

    ASSERT_TRUE(device.assigned());

    const auto caps = device.getInfo().getServerCapabilities();
    ASSERT_EQ(caps.getCount(), 1u);

    for (const auto& capability : caps)
    {
        if (!test_helpers::isSufix(capability.getConnectionString(), path))
            break;

        ASSERT_EQ(capability.getProtocolName(), "OpenDAQOPCUA");
        // const auto ipv4Info = capability.getAddressInfo()[0];
        const auto ipv6Info = capability.getAddressInfo()[0];

        // ASSERT_EQ(ipv4Info.getReachabilityStatus(), AddressReachabilityStatus::Reachable);
        ASSERT_EQ(ipv6Info.getReachabilityStatus(), AddressReachabilityStatus::Reachable);
        
        // ASSERT_EQ(ipv4Info.getType(), "IPv4");
        ASSERT_EQ(ipv6Info.getType(), "IPv6");

        // ASSERT_EQ(ipv4Info.getConnectionString(), capability.getConnectionStrings()[0]);
        ASSERT_EQ(ipv6Info.getConnectionString(), capability.getConnectionStrings()[0]);
        
        // ASSERT_EQ(ipv4Info.getAddress(), capability.getAddresses()[0]);
        ASSERT_EQ(ipv6Info.getAddress(), capability.getAddresses()[0]);
        return;
    }
    ASSERT_TRUE(false) << "Device not found"; 
}

#endif

DevicePtr FindOpcuaDeviceByPath(const InstancePtr& instance, const std::string& path, const PropertyObjectPtr& config = nullptr)
{
    for (const auto & deviceInfo : instance.getAvailableDevices())
    {
        for (const auto & capability : deviceInfo.getServerCapabilities())
        {
            if (!test_helpers::isSufix(capability.getConnectionString(), path))
                break;

            if (capability.getProtocolName() == "OpenDAQOPCUA")
            {
                return instance.addDevice(capability.getConnectionString(), config);
            }
        }
    }
    return DevicePtr();
}

TEST_F(OpcuaDeviceModulesTest, TestDiscoveryReachabilityAfterConnect)
{
    if (test_helpers::Ipv6IsDisabled())
    {
        GTEST_SKIP() << "Ipv6 is disabled";
    }

    auto instance = InstanceBuilder().addDiscoveryServer("mdns").build();
    auto serverConfig = instance.getAvailableServerTypes().get("OpenDAQOPCUA").createDefaultConfig();
    auto path = "/test/opcua/discoveryReachabilityAfterConnect/";
    serverConfig.setPropertyValue("Path", path);

    instance.addServer("OpenDAQOPCUA", serverConfig).enableDiscovery();
    auto client = Instance();

    DevicePtr device = FindOpcuaDeviceByPath(client, path);
    ASSERT_TRUE(device.assigned());

    const auto caps = device.getInfo().getServerCapabilities();
    ASSERT_EQ(caps.getCount(), 1u);

    for (const auto& capability : caps)
    {
        if (!test_helpers::isSufix(capability.getConnectionString(), path))
            break;

        ASSERT_EQ(capability.getProtocolName(), "OpenDAQOPCUA");
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
        return;
    }
    ASSERT_TRUE(false) << "Device not found";
}

TEST_F(OpcuaDeviceModulesTest, TestProtocolVersion)
{
    auto instance = InstanceBuilder().addDiscoveryServer("mdns").build();
    auto serverConfig = instance.getAvailableServerTypes().get("OpenDAQOPCUA").createDefaultConfig();
    auto path = "/test/opcua/test_protocol_version/";
    serverConfig.setPropertyValue("Path", path);

    instance.addServer("OpenDAQOPCUA", serverConfig).enableDiscovery();
    auto client = Instance();

    DevicePtr device = FindOpcuaDeviceByPath(client, path);
    ASSERT_TRUE(device.assigned());

    const auto caps = device.getInfo().getServerCapabilities();
    ASSERT_EQ(caps.getCount(), 1u);

    for (const auto& capability : caps)
    {
        if (!test_helpers::isSufix(capability.getConnectionString(), path))
            break;

        ASSERT_EQ(capability.getProtocolName(), "OpenDAQOPCUA");
        ASSERT_EQ(capability.getProtocolVersion(), "");
        ASSERT_EQ(device.getInfo().getConfigurationConnectionInfo().getProtocolVersion(), "");
        return;
    }
    ASSERT_TRUE(false) << "Device not found";
}

TEST_F(OpcuaDeviceModulesTest, GetRemoteDeviceObjects)
{
    SKIP_TEST_MAC_CI;
    auto server = CreateServerInstance();
    auto client = CreateClientInstance();

    auto signals = client.getSignals(search::Recursive(search::Any()));
    auto signalsServer = server.getSignals(search::Recursive(search::Any()));
    ASSERT_EQ(signals.getCount(), 8u);
    auto signalsVisible = client.getSignals(search::Recursive(search::Visible()));
    ASSERT_EQ(signalsVisible.getCount(), 5u);
    auto devices = client.getDevices();
    ASSERT_EQ(devices.getCount(), 1u);
    auto fbs = devices[0].getFunctionBlocks();
    ASSERT_EQ(fbs.getCount(), 1u);
    auto channels = client.getChannels(search::Recursive(search::Any()));
    ASSERT_EQ(channels.getCount(), 2u);
}

TEST_F(OpcuaDeviceModulesTest, RemoveDevice)
{
    auto server = CreateServerInstance();
    auto client = CreateClientInstance();
    auto device = client.getDevices()[0];

    ASSERT_NO_THROW(client.removeDevice(device));
    ASSERT_TRUE(device.isRemoved());
}

TEST_F(OpcuaDeviceModulesTest, ChangePropAfterRemove)
{
    auto loggerSink = LastMessageLoggerSink();
    loggerSink.setLevel(LogLevel::Warn);
    auto privateSink = loggerSink.asPtrOrNull<ILastMessageLoggerSinkPrivate>();

    auto sinks = DefaultSinks(nullptr);
    sinks.pushBack(loggerSink);
    auto logger = LoggerWithSinks(sinks);

    auto server = CreateServerInstance();
    auto client = CreateClientInstance(InstanceBuilder().setLogger(logger));

    auto device = client.getDevices()[0];
    auto mirroredRefDevice = client.getDevices()[0].getDevices()[0];

    client.removeDevice(device);

    ASSERT_TRUE(mirroredRefDevice.isRemoved());

    // reset messages
    // ReSharper disable once CppExpressionWithoutSideEffects
    privateSink.waitForMessage(0);

    ASSERT_NO_THROW(mirroredRefDevice.setPropertyValue("NumberOfChannels", 1));
    logger.flush();
    ASSERT_TRUE(privateSink.waitForMessage(2000));
    ASSERT_EQ(privateSink.getLastMessage(), "Failed to set value for property \"NumberOfChannels\" on OpcUA client property object: Writing property value");
}

TEST_F(OpcuaDeviceModulesTest, RemoteGlobalIds)
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

TEST_F(OpcuaDeviceModulesTest, GetSetDeviceProperties)
{
    SKIP_TEST_MAC_CI;
    auto loggerSink = LastMessageLoggerSink();
    loggerSink.setLevel(LogLevel::Warn);
    auto sinkPrivate = loggerSink.asPtrOrNull<ILastMessageLoggerSinkPrivate>();

    auto sinks = DefaultSinks(nullptr);
    sinks.pushBack(loggerSink);
    auto logger = LoggerWithSinks(sinks);

    auto server = CreateServerInstance();
    auto client = CreateClientInstance(InstanceBuilder().setLogger(logger));

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

    auto oldProperties = refDevice.getAllProperties();

    // reset messages
    // ReSharper disable once CppExpressionWithoutSideEffects
    sinkPrivate.waitForMessage(0);
    ASSERT_ANY_THROW(refDevice.setPropertyValue("InvalidProp", 100));
    logger.flush();
    ASSERT_TRUE(sinkPrivate.waitForMessage(2000));
    ASSERT_EQ(sinkPrivate.getLastMessage(), "Failed to set value for property \"InvalidProp\" on OpcUA client property object: Property not found");

    auto properties = refDevice.getAllProperties();
    ASSERT_EQ(properties.getCount(), oldProperties.getCount());
}

TEST_F(OpcuaDeviceModulesTest, DeviceInfoAndDomain)
{
    SKIP_TEST_MAC_CI;
    auto server = CreateServerInstance();
    auto client = CreateClientInstance();

    auto refDevice = client.getDevices()[0].getDevices()[0];
    auto serverRefDeviceInfo = server.getDevices()[0].getInfo();
    auto serverRefDeviceDomain = server.getDevices()[0].getDomain();

    auto info = refDevice.getInfo();
    auto domain = refDevice.getDomain();

    ASSERT_EQ(info.getName(), serverRefDeviceInfo.getName());
    ASSERT_EQ(info.getModel(), serverRefDeviceInfo.getModel());
    ASSERT_EQ(info.getSerialNumber(), serverRefDeviceInfo.getSerialNumber());

    ASSERT_EQ(domain.getTickResolution(), serverRefDeviceDomain.getTickResolution());
    ASSERT_EQ(domain.getOrigin(), serverRefDeviceDomain.getOrigin());
    ASSERT_NO_THROW(domain.getUnit());
    ASSERT_NO_THROW(refDevice.getTicksSinceOrigin());
}

TEST_F(OpcuaDeviceModulesTest, DeviceDynamicFeatures)
{
    SKIP_TEST_MAC_CI;
    auto server = CreateServerInstance();
    auto client = CreateClientInstance();
    auto daqDevice = client.getDevices()[0];

    ASSERT_EQ(daqDevice.getAvailableDevices().getCount(), 0u);
    ASSERT_EQ(daqDevice.getAvailableFunctionBlockTypes().getCount(), 10u);
    ASSERT_THROW(daqDevice.addDevice("daqref://device0"),
                 opcua::OpcUaClientCallNotAvailableException);  // Are these the correct errors to return?

    auto refDevice = daqDevice.getDevices()[0];
    ASSERT_THROW(daqDevice.removeDevice(refDevice), opcua::OpcUaClientCallNotAvailableException);

    auto refFb = daqDevice.getFunctionBlocks()[0];
    ASSERT_THROW(daqDevice.addFunctionBlock("test_fb"), daq::GeneralErrorException);

    auto scalingFb = daqDevice.addFunctionBlock("RefFBModuleScaling");
    ASSERT_TRUE(scalingFb.assigned());

    ASSERT_NO_THROW(daqDevice.removeFunctionBlock(refFb));
}

TEST_F(OpcuaDeviceModulesTest, DISABLED_Signal)
{
    auto server = CreateServerInstance();
    auto client = CreateClientInstance();
    auto signal = client.getSignals(search::Recursive(search::Visible()))[0];
    auto serverSignal = server.getSignals(search::Recursive(search::Visible()))[0];

    ASSERT_GT(client.getDevices()[0].getSignals(search::Recursive(search::Visible())).getCount(), 0u);
    ASSERT_GT(client.getDevices()[0].getDevices()[0].getSignals(search::Recursive(search::Visible())).getCount(), 0u);

    ASSERT_EQ(signal.getLocalId(), serverSignal.getLocalId());
    ASSERT_EQ(signal.getActive(), serverSignal.getActive());
    signal.setActive(false);
    ASSERT_EQ(signal.getActive(), serverSignal.getActive());
    ASSERT_EQ(signal.getDescriptor().getName().toStdString(), serverSignal.getDescriptor().getName().toStdString());

    // As we have this method available... Should we form connections on the client side?
    // Those connections should not forward packets.
    ASSERT_EQ(signal.getConnections(), 0u);

    ASSERT_THROW(signal.getDomainSignal(), opcua::OpcUaClientCallNotAvailableException);
    ASSERT_THROW(signal.getRelatedSignals(), opcua::OpcUaClientCallNotAvailableException);
}

TEST_F(OpcuaDeviceModulesTest, SignalConfig_Server)
{
    const std::string newSignalName{"some new name"};

    auto server = CreateServerInstance();

    auto serverSignal = server.getSignals(search::Recursive(search::Visible()))[0].asPtr<ISignalConfig>();
    auto serverSignalDataDescriptor = DataDescriptorBuilderCopy(serverSignal.getDescriptor()).setName(newSignalName).build();
    serverSignal.setDescriptor(serverSignalDataDescriptor);

    auto client = CreateClientInstance();

    auto clientSignals = client.getDevices()[0].getSignals(search::Recursive(search::Visible()));
    auto clientSignal = clientSignals[0].asPtr<ISignalConfig>();

    auto clientSignalDataDescriptor = DataDescriptorBuilderCopy(clientSignal.getDescriptor()).build();

    ASSERT_EQ(serverSignal.getDescriptor().getName(), newSignalName);
    ASSERT_EQ(serverSignal.getDescriptor().getName(), clientSignal.getDescriptor().getName());
}

TEST_F(OpcuaDeviceModulesTest, DISABLED_SignalConfig_Client)
{
    auto server = CreateServerInstance();
    auto client = CreateClientInstance();

    auto clientSignals = client.getDevices()[0].getSignals(search::Recursive(search::Visible()));
    auto clientSignal = clientSignals[0].asPtr<ISignalConfig>();

    auto descCopy = DataDescriptorBuilderCopy(clientSignal.getDescriptor()).setName("test123").build();

    // This currently fails. Should throw OpcUaClientCallNotAvailableException if we cannot fix it.
    // causes UA_STATUSCODE_BADWRITENOTSUPPORTED: The server does not support writing the combination of value
    ASSERT_NO_THROW(clientSignal.setDescriptor(descCopy));
    ASSERT_TRUE(descCopy.isFrozen());
    ASSERT_EQ(descCopy.getName(), clientSignal.getDescriptor().getName());

    ASSERT_THROW(clientSignal.setDomainSignal(clientSignals[3]), opcua::OpcUaClientCallNotAvailableException);
    ASSERT_THROW(clientSignal.setRelatedSignals(List<ISignal>()), opcua::OpcUaClientCallNotAvailableException);
    ASSERT_THROW(clientSignal.addRelatedSignal(Signal(nullptr, nullptr, "sig")), opcua::OpcUaClientCallNotAvailableException);
    ASSERT_THROW(clientSignal.removeRelatedSignal(Signal(nullptr, nullptr, "sig")), opcua::OpcUaClientCallNotAvailableException);
    ASSERT_THROW(clientSignal.clearRelatedSignals(), opcua::OpcUaClientCallNotAvailableException);
}

TEST_F(OpcuaDeviceModulesTest, DISABLED_SignalLocalConnections)
{
    auto server = CreateServerInstance();
    auto client = CreateClientInstance();

    auto signal = client.getSignals(search::Recursive(search::Visible()))[0].asPtr<ISignalConfig>();
    auto reader = PacketReader(signal);
    ASSERT_EQ(signal.getConnections().getCount(), 1u);

    reader.release();
    ASSERT_EQ(signal.getConnections().getCount(), 0u);
}

TEST_F(OpcuaDeviceModulesTest, SignalDescriptor)
{
    auto server = CreateServerInstance();
    auto client = CreateClientInstance();
    auto signalDescriptor = client.getSignals(search::Recursive(search::Visible()))[0].getDescriptor();
    auto serverSignalDescriptor = server.getSignals(search::Recursive(search::Visible()))[0].getDescriptor();

    ASSERT_EQ(signalDescriptor.getName(), serverSignalDescriptor.getName());
    ASSERT_EQ(signalDescriptor.getMetadata(), serverSignalDescriptor.getMetadata());
}

TEST_F(OpcuaDeviceModulesTest, DataDescriptor)
{
    auto server = CreateServerInstance();
    auto client = CreateClientInstance();

    DataDescriptorPtr dataDescriptor = client.getSignals(search::Recursive(search::Visible()))[0].getDescriptor();
    DataDescriptorPtr serverDataDescriptor = server.getSignals(search::Recursive(search::Visible()))[0].getDescriptor();

    DataDescriptorPtr domainDataDescriptor = client.getSignals(search::Recursive(search::Visible()))[2].getDescriptor();
    DataDescriptorPtr serverDomainDataDescriptor = server.getSignals(search::Recursive(search::Visible()))[2].getDescriptor();

    ASSERT_EQ(dataDescriptor.getName(), serverDataDescriptor.getName());
    ASSERT_EQ(dataDescriptor.getDimensions().getCount(), serverDataDescriptor.getDimensions().getCount());

    ASSERT_EQ(dataDescriptor.getSampleType(), serverDataDescriptor.getSampleType());
    ASSERT_EQ(dataDescriptor.getUnit().getSymbol(), serverDataDescriptor.getUnit().getSymbol());
    ASSERT_EQ(dataDescriptor.getValueRange(), serverDataDescriptor.getValueRange());
    ASSERT_EQ(dataDescriptor.getRule().getType(), serverDataDescriptor.getRule().getType());

    ASSERT_EQ(domainDataDescriptor.getRule().getParameters(), serverDomainDataDescriptor.getRule().getParameters());
    ASSERT_EQ(domainDataDescriptor.getOrigin(), serverDomainDataDescriptor.getOrigin());
    ASSERT_EQ(domainDataDescriptor.getTickResolution(), serverDomainDataDescriptor.getTickResolution());

    auto refChannel = client.getChannels(search::Recursive(search::Visible()))[0];
    refChannel.setPropertyValue("ClientSideScaling", true);

    dataDescriptor = client.getChannels(search::Recursive(search::Visible()))[0].getSignals(search::Recursive(search::Visible()))[0].getDescriptor();
    serverDataDescriptor = server.getChannels(search::Recursive(search::Visible()))[0].getSignals(search::Recursive(search::Visible()))[0].getDescriptor();
    ASSERT_EQ(dataDescriptor.getPostScaling().getParameters(), dataDescriptor.getPostScaling().getParameters());
}

TEST_F(OpcuaDeviceModulesTest, DISABLED_FunctionBlock)
{
    auto server = CreateServerInstance();
    auto client = CreateClientInstance();

    auto fb = client.getDevices()[0].getFunctionBlocks()[0];
    auto serverFb = server.getFunctionBlocks()[0];

    auto fbType = fb.getFunctionBlockType();
    auto serverFbType = serverFb.getFunctionBlockType();

    ASSERT_EQ(fbType.getId(), serverFbType.getId());
    ASSERT_EQ(fbType.getDescription(), serverFbType.getDescription());
    ASSERT_EQ(fbType.getName(), serverFbType.getName());

    ASSERT_EQ(fb.getInputPorts().getCount(), serverFb.getInputPorts().getCount());
    ASSERT_EQ(fb.getSignals(search::Recursive(search::Visible())).getCount(), serverFb.getSignals(search::Recursive(search::Visible())).getCount());

    auto fbSignal = fb.getSignals(search::Recursive(search::Visible()))[0];
    auto serverFbSignal = serverFb.getSignals(search::Recursive(search::Visible()))[0];

    ASSERT_EQ(fbSignal.getLocalId(), serverFbSignal.getLocalId());
    ASSERT_EQ(fbSignal.getActive(), serverFbSignal.getActive());
    ASSERT_EQ(fbSignal.getDescriptor().getName(), serverFbSignal.getDescriptor().getName());
    ASSERT_EQ(fbSignal.getDescriptor().getSampleType(), serverFbSignal.getDescriptor().getSampleType());

    ASSERT_NO_THROW(fb.getStatusSignal());

    auto notifications = fb.asPtr<IInputPortNotifications>();
    ASSERT_THROW(notifications.acceptsSignal(nullptr, nullptr), opcua::OpcUaClientCallNotAvailableException);
    ASSERT_THROW(notifications.connected(nullptr), opcua::OpcUaClientCallNotAvailableException);
    ASSERT_THROW(notifications.disconnected(nullptr), opcua::OpcUaClientCallNotAvailableException);
}

TEST_F(OpcuaDeviceModulesTest, FunctionBlockProperties)
{
    auto loggerSink = LastMessageLoggerSink();
    loggerSink.setLevel(LogLevel::Warn);
    auto privateSink = loggerSink.asPtrOrNull<ILastMessageLoggerSinkPrivate>();

    auto sinks = DefaultSinks(nullptr);
    sinks.pushBack(loggerSink);
    auto logger = LoggerWithSinks(sinks);

    auto server = CreateServerInstance();
    auto client = CreateClientInstance(InstanceBuilder().setLogger(logger));

    auto fb = client.getDevices()[0].getFunctionBlocks()[0];
    auto serverFb = server.getFunctionBlocks()[0];

    ASSERT_EQ(fb.getPropertyValue("BlockSize"), serverFb.getPropertyValue("BlockSize"));
    fb.setPropertyValue("BlockSize", 20);
    ASSERT_EQ(fb.getPropertyValue("BlockSize"), serverFb.getPropertyValue("BlockSize"));

    ASSERT_EQ(fb.getPropertyValue("DomainSignalType"), serverFb.getPropertyValue("DomainSignalType"));
    fb.setPropertyValue("DomainSignalType", 2);
    ASSERT_EQ(fb.getPropertyValue("DomainSignalType"), serverFb.getPropertyValue("DomainSignalType"));

    // reset messages
    // ReSharper disable once CppExpressionWithoutSideEffects
    privateSink.waitForMessage(0);
    ASSERT_NO_THROW(fb.setPropertyValue("DomainSignalType" , 1000));
    logger.flush();
    ASSERT_TRUE(privateSink.waitForMessage(2000));
    ASSERT_EQ(privateSink.getLastMessage(), "Failed to set value for property \"DomainSignalType\" on OpcUA client property object: Writing property value");
}

TEST_F(OpcuaDeviceModulesTest, DISABLED_InputPort)
{
    auto server = CreateServerInstance();
    auto client = CreateClientInstance();

    auto port = client.getDevices()[0].getFunctionBlocks()[0].getInputPorts()[0];
    auto serverPort = server.getFunctionBlocks()[0].getInputPorts()[0];

    ASSERT_EQ(port.getLocalId(), serverPort.getLocalId());
    ASSERT_EQ(port.getRequiresSignal(), serverPort.getRequiresSignal());

    ASSERT_THROW(port.acceptsSignal(nullptr), opcua::OpcUaClientCallNotAvailableException);
    ASSERT_THROW(port.connect(nullptr), opcua::OpcUaClientCallNotAvailableException);
    ASSERT_THROW(port.disconnect(), opcua::OpcUaClientCallNotAvailableException);
    ASSERT_THROW(port.getSignal(), opcua::OpcUaClientCallNotAvailableException);
    ASSERT_THROW(port.getConnection(), opcua::OpcUaClientCallNotAvailableException);

    auto portConfig = port.asPtr<IInputPortConfig>();
    ASSERT_THROW(portConfig.getCustomData(), opcua::OpcUaClientCallNotAvailableException);
    ASSERT_THROW(portConfig.notifyPacketEnqueued(True), opcua::OpcUaClientCallNotAvailableException);
    ASSERT_THROW(portConfig.setNotificationMethod(PacketReadyNotification::SameThread), opcua::OpcUaClientCallNotAvailableException);
    ASSERT_THROW(portConfig.setCustomData(nullptr), opcua::OpcUaClientCallNotAvailableException);
}

TEST_F(OpcuaDeviceModulesTest, DISABLED_PublicProp)
{
    auto server = Instance();
    const auto refDevice = server.addDevice("daqref://device1");
    refDevice.getSignals(search::Recursive(search::Visible()))[0].setPublic(false);
    auto id = refDevice.getSignals(search::Recursive(search::Visible()))[0].getLocalId();
    server.addServer("OpenDAQOPCUA", nullptr);
    auto client = CreateClientInstance();

    ASSERT_NE(client.getDevices()[0].getDevices()[0].getSignals(search::Recursive(search::Visible()))[0].getLocalId(), id);
}

TEST_F(OpcuaDeviceModulesTest, ProcedureProp)
{
    auto server = CreateServerInstance();
    auto client = CreateClientInstance();

    auto ch = client.getDevices()[0].getDevices()[0].getChannels()[0];
    ch.setPropertyValue("Waveform", 3);
    ProcedurePtr proc = ch.getPropertyValue("ResetCounter");
    ASSERT_NO_THROW(proc());
}

////////
// Tests defining future requirements
////////

TEST_F(OpcuaDeviceModulesTest, DISABLED_ReferenceMethods)
{
    auto server = CreateServerInstance();
    auto client = CreateClientInstance();

    auto signals = client.getDevices()[0].getDevices()[0].getSignals(search::Recursive(search::Visible()));
    auto connectedSignal = signals[0];
    auto domainSignal = signals[1];

    auto inputPort = client.getDevices()[0].getFunctionBlocks()[0].getInputPorts()[0];

    ASSERT_EQ(inputPort.getSignal(), connectedSignal);
    ASSERT_EQ(inputPort.getConnection(), connectedSignal.getConnections()[0]);
    ASSERT_EQ(connectedSignal.getDomainSignal(), domainSignal);

    // TODO: We have no related signals example. Should be added when we do.
}

TEST_F(OpcuaDeviceModulesTest, DISABLED_DynamicSignalConfig)
{
    auto server = CreateServerInstance();
    auto serverDevice = server.addDevice("daqref://device0");
    auto client = CreateClientInstance();

    auto clientSignals = client.getDevices()[0].getDevices()[0].getSignals(search::Recursive(search::Visible()));
    auto clientSignal = clientSignals[0].asPtr<ISignalConfig>();

    clientSignal.setDomainSignal(clientSignals[1]);
    ASSERT_EQ(serverDevice.getSignals(search::Recursive(search::Visible()))[0].getDomainSignal().getLocalId(), clientSignal.getDomainSignal().getLocalId());
}

TEST_F(OpcuaDeviceModulesTest, FunctionBlocksOnClient)
{
    auto logger = Logger();
    auto scheduler = Scheduler(logger);
    auto moduleManager = ModuleManager("");
    auto typeManager = TypeManager();
    auto authenticationProvider = AuthenticationProvider();
    auto context = Context(scheduler, logger, typeManager, moduleManager, authenticationProvider);

    auto instance = InstanceCustom(context, "local");

    instance.setRootDevice("daqref://device1");

    const auto statistics = instance.addFunctionBlock("RefFBModuleStatistics");
    instance.addServer("OpenDAQOPCUA", nullptr);
    auto client = CreateClientInstance();

    ASSERT_GT(client.getDevices()[0].getFunctionBlocks().getCount(), (SizeT) 0);
}

TEST_F(OpcuaDeviceModulesTest, AddedRemovedSignalsStreaming)
{
    auto logger = Logger();
    auto scheduler = Scheduler(logger);
    auto moduleManager = ModuleManager("");
    auto typeManager = TypeManager();
    auto authenticationProvider = AuthenticationProvider();
    auto context = Context(scheduler, logger, typeManager, moduleManager, authenticationProvider);

    auto instance = InstanceCustom(context, "local");
    instance.setRootDevice("daqref://device1");
    instance.addServer("OpenDAQNativeStreaming", nullptr);
    instance.addServer("OpenDAQOPCUA", nullptr);

    auto client = InstanceBuilder().build();
    auto clientDevice = client.addDevice("daq.opcua://127.0.0.1");

    const auto newFb = clientDevice.addFunctionBlock("RefFBModuleScaling");
    const auto fbSignals = newFb.getSignals(search::Recursive(search::Any()));

    for (const auto& signal : fbSignals)
    {
        ASSERT_GT(signal.asPtr<IMirroredSignalConfig>().getStreamingSources().getCount(), 0u);
        ASSERT_TRUE(signal.asPtr<IMirroredSignalConfig>().getActiveStreamingSource().assigned());
    }

    clientDevice.removeFunctionBlock(newFb);

    for (const auto& signal : fbSignals)
    {
        auto mirroredSignalPtr = signal.asPtr<IMirroredSignalConfig>();
        ASSERT_EQ(mirroredSignalPtr.getStreamingSources().getCount(), 0u) << signal.getGlobalId();
        ASSERT_EQ(mirroredSignalPtr.getActiveStreamingSource(), nullptr) << signal.getGlobalId();
        ASSERT_TRUE(signal.isRemoved());
    }
}

TEST_F(OpcuaDeviceModulesTest, SdkPackageVersion)
{
    auto instance = InstanceBuilder().setDefaultRootDeviceInfo(DeviceInfo("", "dev", "custom")).build();
    instance.addServer("OpenDAQOPCUA", nullptr);
    auto client = CreateClientInstance();

    ASSERT_EQ(client.getDevices()[0].getInfo().getSdkVersion(), "custom");
}

TEST_F(OpcuaDeviceModulesTest, SdkPackageVersion1)
{
    auto server = CreateServerInstance();
    auto client = CreateClientInstance();
    auto info = client.getDevices()[0].getInfo();
    ASSERT_EQ(info.getPropertyValue("sdkVersion"), OPENDAQ_PACKAGE_VERSION);
}

TEST_F(OpcuaDeviceModulesTest, AuthenticationDefault)
{
    auto serverInstance = CreateServerInstance();
    auto clientInstance = InstanceBuilder().build();

    auto config = clientInstance.getAvailableDeviceTypes().get("OpenDAQOPCUAConfiguration").createDefaultConfig();
    config.setPropertyValue("Username", "");
    config.setPropertyValue("Password", "");

    auto device = clientInstance.addDevice("daq.opcua://127.0.0.1", config);
    ASSERT_TRUE(device.assigned());
}

TEST_F(OpcuaDeviceModulesTest, AuthenticationDefinedUsers)
{
    auto users = List<IUser>();
    users.pushBack(User("jure", "jure123"));
    users.pushBack(User("tomaz", "tomaz123"));

    auto authenticationProvider = StaticAuthenticationProvider(false, users);
    auto serverInstance = CreateServerInstance(authenticationProvider);

    auto clientInstance = InstanceBuilder().build();
    auto config = clientInstance.getAvailableDeviceTypes().get("OpenDAQOPCUAConfiguration").createDefaultConfig();

    ASSERT_THROW(clientInstance.addDevice("daq.opcua://127.0.0.1", config), AuthenticationFailedException);

    config.setPropertyValue("Username", "jure");
    config.setPropertyValue("Password", "wrongPass");
    ASSERT_THROW(clientInstance.addDevice("daq.opcua://127.0.0.1", config), AuthenticationFailedException);

    config.setPropertyValue("Username", "andrej");
    config.setPropertyValue("Password", "andrej123");
    ASSERT_THROW(clientInstance.addDevice("daq.opcua://127.0.0.1", config), AuthenticationFailedException);

    config.setPropertyValue("Username", "jure");
    config.setPropertyValue("Password", "jure123");
    auto device = clientInstance.addDevice("daq.opcua://127.0.0.1", config);
    ASSERT_TRUE(device.assigned());
    clientInstance.removeDevice(device);

    config.setPropertyValue("Username", "tomaz");
    config.setPropertyValue("Password", "tomaz123");
    device = clientInstance.addDevice("daq.opcua://127.0.0.1", config);
    ASSERT_TRUE(device.assigned());
    clientInstance.removeDevice(device);
}

TEST_F(OpcuaDeviceModulesTest, AuthenticationAllowNoOne)
{
    auto authenticationProvider = AuthenticationProvider(false);
    auto serverInstance = CreateServerInstance(authenticationProvider);

    auto clientInstance = InstanceBuilder().build();
    auto config = clientInstance.getAvailableDeviceTypes().get("OpenDAQOPCUAConfiguration").createDefaultConfig();

    ASSERT_THROW(clientInstance.addDevice("daq.opcua://127.0.0.1", config), AuthenticationFailedException);

    config.setPropertyValue("Username", "jure");
    config.setPropertyValue("Password", "jure123");
    ASSERT_THROW(clientInstance.addDevice("daq.opcua://127.0.0.1", config), AuthenticationFailedException);
}

TEST_F(OpcuaDeviceModulesTest, AddStreamingPostConnection)
{
    SKIP_TEST_MAC_CI;
    auto server = CreateServerInstance();
    auto client = CreateClientInstance();

    auto clientMirroredDevice = client.getDevices()[0].template asPtrOrNull<IMirroredDevice>();
    ASSERT_TRUE(clientMirroredDevice.assigned());
    ASSERT_EQ(clientMirroredDevice.getStreamingSources().getCount(), 0u);

    const auto clientSignals = client.getSignals(search::Recursive(search::Any()));
    for (const auto& signal : clientSignals)
    {
        auto mirorredSignal = signal.template asPtr<IMirroredSignalConfig>();
        ASSERT_EQ(mirorredSignal.getStreamingSources().getCount(), 0u);
    }

    server.addServer("OpenDAQLTStreaming", nullptr);
    StreamingPtr streaming;
    ASSERT_NO_THROW(streaming = client.getDevices()[0].addStreaming("daq.lt://127.0.0.1"));
    ASSERT_EQ(clientMirroredDevice.getStreamingSources().getCount(), 1u);
    ASSERT_EQ(streaming, clientMirroredDevice.getStreamingSources()[0]);

    streaming.addSignals(clientSignals);
    for (const auto& signal : clientSignals)
    {
        auto mirorredSignal = signal.template asPtr<IMirroredSignalConfig>();
        ASSERT_EQ(mirorredSignal.getStreamingSources().getCount(), 1u);
        ASSERT_NO_THROW(mirorredSignal.setActiveStreamingSource(streaming.getConnectionString()));
    }
}

TEST_F(OpcuaDeviceModulesTest, GetConfigurationConnectionInfo)
{
    SKIP_TEST_MAC_CI;
    auto server = CreateServerInstance();
    auto client = CreateClientInstance();

    auto devices = client.getDevices();
    ASSERT_EQ(devices.getCount(), 1u);

    auto connectionInfo = devices[0].getInfo().getConfigurationConnectionInfo();
    ASSERT_EQ(connectionInfo.getProtocolId(), "OpenDAQOPCUAConfiguration");
    ASSERT_EQ(connectionInfo.getProtocolName(), "OpenDAQOPCUA");
    ASSERT_EQ(connectionInfo.getProtocolType(), ProtocolType::Configuration);
    ASSERT_EQ(connectionInfo.getConnectionType(), "TCP/IP");
    ASSERT_EQ(connectionInfo.getAddresses()[0], "127.0.0.1");
    ASSERT_EQ(connectionInfo.getPort(), 4840);
    ASSERT_EQ(connectionInfo.getPrefix(), "daq.opcua");
    ASSERT_EQ(connectionInfo.getConnectionString(), "daq.opcua://127.0.0.1");
}

TEST_F(OpcuaDeviceModulesTest, TestAddressInfoIPv4)
{
    auto server = InstanceBuilder().setRootDevice("daqref://device0").build();
    server.addServer("OpenDAQNativeStreaming", nullptr);
    server.addServer("OpenDAQLTStreaming", nullptr);
    server.addServer("OpenDAQOPCUA", nullptr);

    auto client = Instance();
    const auto dev = client.addDevice("daq.opcua://127.0.0.1");
    const auto info = dev.getInfo();

    ASSERT_TRUE(info.hasServerCapability("OpenDAQNativeConfiguration"));
    ASSERT_TRUE(info.hasServerCapability("OpenDAQOPCUAConfiguration"));
    ASSERT_TRUE(info.hasServerCapability("OpenDAQNativeStreaming"));
    ASSERT_TRUE(info.hasServerCapability("OpenDAQLTStreaming"));

    const auto opcuaCapability = info.getServerCapability("OpenDAQOPCUAConfiguration");
    const auto nativeConfigCapability = info.getServerCapability("OpenDAQNativeConfiguration");
    const auto nativeStreamingCapability = info.getServerCapability("OpenDAQNativeStreaming");
    const auto LTCapability = info.getServerCapability("OpenDAQLTStreaming");

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

TEST_F(OpcuaDeviceModulesTest, TestAddressInfoIPv6)
{
    if (test_helpers::Ipv6IsDisabled())
    {
        GTEST_SKIP() << "Ipv6 is disabled";
    }

    auto server = InstanceBuilder().setRootDevice("daqref://device0").build();
    server.addServer("OpenDAQNativeStreaming", nullptr);
    server.addServer("OpenDAQLTStreaming", nullptr);
    server.addServer("OpenDAQOPCUA", nullptr);

    auto client = Instance();
    const auto dev = client.addDevice("daq.opcua://[::1]");
    const auto info = dev.getInfo();

    ASSERT_TRUE(info.hasServerCapability("OpenDAQNativeConfiguration"));
    ASSERT_TRUE(info.hasServerCapability("OpenDAQOPCUAConfiguration"));
    ASSERT_TRUE(info.hasServerCapability("OpenDAQNativeStreaming"));
    ASSERT_TRUE(info.hasServerCapability("OpenDAQLTStreaming"));

    const auto opcuaCapability = info.getServerCapability("OpenDAQOPCUAConfiguration");
    const auto nativeConfigCapability = info.getServerCapability("OpenDAQNativeConfiguration");
    const auto nativeStreamingCapability = info.getServerCapability("OpenDAQNativeStreaming");
    const auto LTCapability = info.getServerCapability("OpenDAQLTStreaming");

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

TEST_F(OpcuaDeviceModulesTest, DISABLED_TestAddressInfoGatewayDevice)
{
    auto server = InstanceBuilder().setRootDevice("daqref://device0").build();
    server.addServer("OpenDAQNativeStreaming", nullptr);
    server.addServer("OpenDAQLTStreaming", nullptr);
    server.addServer("OpenDAQOPCUA", nullptr);
    
    auto gateway = Instance();
    auto serverConfig = gateway.getAvailableServerTypes().get("OpenDAQOPCUA").createDefaultConfig();
    serverConfig.setPropertyValue("Port", 4841);
    gateway.addDevice("daq.opcua://127.0.0.1");
    gateway.addServer("OpenDAQOPCUA", serverConfig);

    auto client = Instance();
    const auto dev = client.addDevice("daq.opcua://127.0.0.1:4841/");
    const auto info = dev.getDevices()[0].getInfo();

    ASSERT_TRUE(info.hasServerCapability("OpenDAQNativeConfiguration"));
    ASSERT_TRUE(info.hasServerCapability("OpenDAQOPCUAConfiguration"));
    ASSERT_TRUE(info.hasServerCapability("OpenDAQNativeStreaming"));
    ASSERT_TRUE(info.hasServerCapability("OpenDAQLTStreaming"));

    const auto opcuaCapability = info.getServerCapability("OpenDAQOPCUAConfiguration");
    const auto nativeConfigCapability = info.getServerCapability("OpenDAQNativeConfiguration");
    const auto nativeStreamingCapability = info.getServerCapability("OpenDAQNativeStreaming");
    const auto LTCapability = info.getServerCapability("OpenDAQLTStreaming");

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

TEST_F(OpcuaDeviceModulesTest, GetSetDeviceUserNameLocation)
{
    auto server = CreateServerInstance();
    auto client = CreateClientInstance();

    auto serverDevice = server.getDevices()[0];
    auto clientDevice = client.getDevices()[0].getDevices()[0];

    // set from server
    {
        serverDevice.setPropertyValue("userName", "testUser");
        serverDevice.setPropertyValue("location", "testLocation");

        ASSERT_EQ(clientDevice.getPropertyValue("userName"), "testUser");
        ASSERT_EQ(clientDevice.getPropertyValue("location"), "testLocation");
    }

    // set from client
    {
        clientDevice.setPropertyValue("userName", "newUser");
        clientDevice.setPropertyValue("location", "newLocation");

        ASSERT_EQ(serverDevice.getPropertyValue("userName"), "newUser");
        ASSERT_EQ(serverDevice.getPropertyValue("location"), "newLocation");
    }
}

class TestDevice : public daq::Device
{
public:
    TestDevice(const ContextPtr& ctx, const ComponentPtr& parent, const PropertyObjectPtr& config)
        : daq::Device(ctx, parent, "dev")
        , config(config)
    {
    }

    daq::DeviceInfoPtr onGetInfo() override
    {
        auto deviceInfo = daq::DeviceInfoWithChanegableFields(config.getPropertyValue("cheangableFields"));
        deviceInfo.setUserName("default_userName");
        deviceInfo.setLocation("default_location");
        return deviceInfo;
    }

    PropertyObjectPtr config;
};

class TestDeviceModuleImpl : public daq::ImplementationOf<daq::IModule>
{
public:
    TestDeviceModuleImpl(daq::ContextPtr ctx)
        : ctx(ctx)
    {
    }

    daq::ErrCode INTERFACE_FUNC getModuleInfo(daq::IModuleInfo** info) override
    {
        *info = ModuleInfo(VersionInfo(0, 0, 0), "TestDeviceModule", "TestDevice").detach();
        return OPENDAQ_SUCCESS;
    }

    daq::ErrCode INTERFACE_FUNC getAvailableDevices(daq::IList** availableDevices) override
    {
        OPENDAQ_PARAM_NOT_NULL(availableDevices);

        auto daqClientDeviceInfo = DeviceInfo("daqtest://test_device");
        daqClientDeviceInfo.setDeviceType(DeviceType("test_device", "test_device", "test_device", "daqtest"));
        *availableDevices = List<IDeviceInfo>(daqClientDeviceInfo).detach();
        return OPENDAQ_SUCCESS;
    }

    daq::ErrCode INTERFACE_FUNC getAvailableDeviceTypes(daq::IDict** deviceTypes) override
    {
        OPENDAQ_PARAM_NOT_NULL(deviceTypes);

        auto mockConfig = PropertyObject();
        mockConfig.addProperty(ListProperty("cheangableFields", List<IString>()));

        auto types = Dict<IString, IDeviceType>();
        types.set("test_device", DeviceType("test_device", "test_device", "test_device", "daqtest", mockConfig));

        *deviceTypes = types.detach();
        return OPENDAQ_SUCCESS;
    }

    daq::ErrCode INTERFACE_FUNC createDevice(daq::IDevice** device, daq::IString* connectionString, daq::IComponent* parent, daq::IPropertyObject* config) override
    {
        OPENDAQ_PARAM_NOT_NULL(device);
        OPENDAQ_PARAM_NOT_NULL(connectionString);

        StringPtr connStr = connectionString;
        if (connStr == "daqtest://test_device")
        {
            *device = daq::createWithImplementation<daq::IDevice, TestDevice>(ctx, parent, config).detach();
            return OPENDAQ_SUCCESS;
        }
        return OPENDAQ_ERR_INVALIDPARAMETER;
    }

    daq::ErrCode INTERFACE_FUNC getAvailableFunctionBlockTypes(daq::IDict**) override
    {
        return OPENDAQ_ERR_NOTIMPLEMENTED;
    }
    daq::ErrCode INTERFACE_FUNC createFunctionBlock(daq::IFunctionBlock**, daq::IString*, daq::IComponent*, daq::IString*, daq::IPropertyObject*) override
    {
        return OPENDAQ_ERR_NOTIMPLEMENTED;
    }
    daq::ErrCode INTERFACE_FUNC getAvailableServerTypes(daq::IDict**) override
    {
        return OPENDAQ_ERR_NOTIMPLEMENTED;
    }
    daq::ErrCode INTERFACE_FUNC createServer(daq::IServer**, daq::IString*, daq::IDevice*, daq::IPropertyObject*) override
    {
        return OPENDAQ_ERR_NOTIMPLEMENTED;
    }
    daq::ErrCode INTERFACE_FUNC createStreaming(daq::IStreaming**, daq::IString*, daq::IPropertyObject*) override
    {
        return OPENDAQ_ERR_NOTIMPLEMENTED;
    }
    daq::ErrCode INTERFACE_FUNC getAvailableStreamingTypes(daq::IDict**) override
    {
        return OPENDAQ_ERR_NOTIMPLEMENTED;
    }
    daq::ErrCode INTERFACE_FUNC completeServerCapability(daq::Bool*, daq::IServerCapability*, daq::IServerCapabilityConfig*) override
    {
        return OPENDAQ_ERR_NOTIMPLEMENTED;
    }

private:
    daq::ContextPtr ctx;
};

InstancePtr CreateTestDeviceInstance()
{
    auto instance = Instance();
    auto moduleManager = instance.getModuleManager();
    moduleManager.addModule(daq::createWithImplementation<daq::IModule, TestDeviceModuleImpl>(instance.getContext()));
    return instance;
}

TEST_F(OpcuaDeviceModulesTest, GetSetNonCheangableUserNameLocation)
{
    auto serverInstance = CreateTestDeviceInstance();
    auto serverDevice = serverInstance.addDevice("daqtest://test_device");
    serverInstance.addServer("OpenDAQOPCUA", nullptr);

    auto clientInstance = CreateTestDeviceInstance();
    auto clientDevice = clientInstance.addDevice("daq.opcua://127.0.0.1").getDevices()[0];

    auto serverDeviceInfo = serverDevice.getInfo();
    auto clientDeviceInfo = clientDevice.getInfo();

    for (const auto & propertyName: {"userName", "location"})
    {
        ASSERT_TRUE(serverDeviceInfo.getProperty(propertyName).getReadOnly());
        ASSERT_FALSE(clientDeviceInfo.getProperty(propertyName).getReadOnly());

        ASSERT_EQ(serverDeviceInfo.getPropertyValue(propertyName), std::string("default_") + propertyName);
        ASSERT_EQ(clientDeviceInfo.getPropertyValue(propertyName), serverDeviceInfo.getPropertyValue(propertyName));

        ASSERT_ANY_THROW(serverDeviceInfo.setPropertyValue(propertyName, "serverValue"));
        ASSERT_EQ(serverDeviceInfo.getPropertyValue(propertyName), std::string("default_") + propertyName);
        ASSERT_EQ(clientDeviceInfo.getPropertyValue(propertyName), std::string("default_") + propertyName);

        serverDeviceInfo.asPtr<IPropertyObjectProtected>(true).setProtectedPropertyValue(propertyName, "serverValue");
        ASSERT_EQ(serverDeviceInfo.getPropertyValue(propertyName), "serverValue");
        ASSERT_EQ(clientDeviceInfo.getPropertyValue(propertyName), "serverValue");

        clientDeviceInfo.setPropertyValue(propertyName, "clientValue");
        ASSERT_EQ(clientDeviceInfo.getPropertyValue(propertyName), "serverValue");
        ASSERT_EQ(serverDeviceInfo.getPropertyValue(propertyName), "serverValue");

        clientDeviceInfo.asPtr<IPropertyObjectProtected>(true).setProtectedPropertyValue(propertyName, "NewClientValue");
        ASSERT_EQ(clientDeviceInfo.getPropertyValue(propertyName), "serverValue");
        ASSERT_EQ(serverDeviceInfo.getPropertyValue(propertyName), "serverValue");      
    }
}