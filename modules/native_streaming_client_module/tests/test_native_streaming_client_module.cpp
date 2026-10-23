#include <testutils/testutils.h>
#include <native_streaming_client_module/module_dll.h>
#include <native_streaming_client_module/version.h>
#include <gmock/gmock.h>

#include <opendaq/module_ptr.h>
#include <coretypes/common.h>

#include <opendaq/context_factory.h>
#include <coreobjects/property_factory.h>
#include <coreobjects/property_object_factory.h>
#include <opendaq/device_info_factory.h>
#include <opendaq/instance_factory.h>
#include <opendaq/address_info_factory.h>
#include <opendaq/add_device_config_ptr.h>
#include <opendaq/add_streaming_config_ptr.h>
#include <native_streaming_protocol/native_streaming_constants.h>

#include <tuple>
#include <vector>

using NativeStreamingClientModuleTest = testing::Test;
using namespace daq;

static constexpr SizeT ExpectedDeviceTypeCount = 2u;
static constexpr SizeT ExpectedStreamingTypeCount = 1u;

static ListPtr<IString> ExpectedTransportNames()
{
#if NATIVE_STREAMING_ENABLE_TLS
    return List<IString>("plain", "tls");
#else
    return List<IString>("plain");
#endif
}

static ModulePtr CreateModule()
{
    ModulePtr module;
    createModule(&module, NullContext());
    return module;
}

TEST_F(NativeStreamingClientModuleTest, CreateModule)
{
    IModule* module = nullptr;
    ErrCode errCode = createModule(&module, NullContext());
    ASSERT_TRUE(OPENDAQ_SUCCEEDED(errCode));

    ASSERT_NE(module, nullptr);
    module->releaseRef();
}

TEST_F(NativeStreamingClientModuleTest, ModuleName)
{
    auto module = CreateModule();
    ASSERT_EQ(module.getModuleInfo().getName(), "OpenDAQNativeStreamingClientModule");
}

TEST_F(NativeStreamingClientModuleTest, VersionAvailable)
{
    auto module = CreateModule();
    ASSERT_TRUE(module.getModuleInfo().getVersionInfo().assigned());
}

TEST_F(NativeStreamingClientModuleTest, VersionCorrect)
{
    auto module = CreateModule();
    auto version = module.getModuleInfo().getVersionInfo();

    ASSERT_EQ(version.getMajor(), NATIVE_STREAM_CL_MODULE_MAJOR_VERSION);
    ASSERT_EQ(version.getMinor(), NATIVE_STREAM_CL_MODULE_MINOR_VERSION);
    ASSERT_EQ(version.getPatch(), NATIVE_STREAM_CL_MODULE_PATCH_VERSION);
}

TEST_F(NativeStreamingClientModuleTest, EnumerateDevices)
{
    auto module = CreateModule();

    ListPtr<IDeviceInfo> deviceInfo;
    ASSERT_NO_THROW(deviceInfo = module.getAvailableDevices());
}

TEST_F(NativeStreamingClientModuleTest, CreateDeviceConnectionStringNull)
{
    auto module = CreateModule();

    DevicePtr device;
    ASSERT_THROW(device = module.createDevice(nullptr, nullptr), ArgumentNullException);
}

TEST_F(NativeStreamingClientModuleTest, CreateDeviceConnectionFailed)
{
    auto module = CreateModule();

    ASSERT_THROW(module.createDevice("daq.ns://127.0.0.1", nullptr), NotFoundException);
    ASSERT_THROW(module.createDevice("daq.nd://127.0.0.1", nullptr), NotFoundException);
}

TEST_F(NativeStreamingClientModuleTest, CreateStreamingWithNullArguments)
{
    auto module = CreateModule();

    DevicePtr device;
    ASSERT_THROW(device = module.createStreaming(nullptr, nullptr), ArgumentNullException);
}

//
//TEST_F(NativeStreamingClientModuleTest, CreateConnectionStringIgnored)
//{
//    auto context = NullContext();
//    ModulePtr module;
//    createModule(&module, context);
//
//    StringPtr connectionString;
//    ServerCapabilityConfigPtr serverCapability = ServerCapability("test", "test", ProtocolType::Unknown);
//    ASSERT_NO_THROW(connectionString = module.createConnectionString(serverCapability));
//    ASSERT_FALSE(connectionString.assigned());
//}
//
//TEST_F(NativeStreamingClientModuleTest, CreateStreamingConnectionString)
//{
//    auto context = NullContext();
//    ModulePtr module;
//    createModule(&module, context);
//
//    StringPtr connectionString;
//    ServerCapabilityConfigPtr serverCapability = ServerCapability("OpenDAQNativeStreaming", "OpenDAQNativeStreaming", ProtocolType::Streaming);
//    ASSERT_THROW(module.createConnectionString(serverCapability), InvalidParameterException);
//
//    serverCapability.addAddress("123.123.123.123");
//    ASSERT_EQ(module.createConnectionString(serverCapability), "daq.ns://123.123.123.123:7420");
//
//    serverCapability.setPort(1234);
//    ASSERT_NO_THROW(connectionString = module.createConnectionString(serverCapability));
//    ASSERT_EQ(connectionString, "daq.ns://123.123.123.123:1234");
//
//    serverCapability.addProperty(StringProperty("Path", "/path"));
//    ASSERT_NO_THROW(connectionString = module.createConnectionString(serverCapability));
//    ASSERT_EQ(connectionString, "daq.ns://123.123.123.123:1234/path");
//}
//
//TEST_F(NativeStreamingClientModuleTest, CreateDeviceConnectionString)
//{
//    auto context = NullContext();
//    ModulePtr module;
//    createModule(&module, context);
//
//    StringPtr connectionString;
//    ServerCapabilityConfigPtr serverCapability = ServerCapability("OpenDAQNativeConfiguration", "OpenDAQNativeConfiguration", ProtocolType::ConfigurationAndStreaming);
//    ASSERT_THROW(module.createConnectionString(serverCapability), InvalidParameterException);
//
//    serverCapability.addAddress("123.123.123.123");
//    ASSERT_EQ(module.createConnectionString(serverCapability), "daq.nd://123.123.123.123:7420");
//
//    serverCapability.setPort(1234);
//    ASSERT_NO_THROW(connectionString = module.createConnectionString(serverCapability));
//    ASSERT_EQ(connectionString, "daq.nd://123.123.123.123:1234");
//
//    serverCapability.addProperty(StringProperty("Path", "/path"));
//    ASSERT_NO_THROW(connectionString = module.createConnectionString(serverCapability));
//    ASSERT_EQ(connectionString, "daq.nd://123.123.123.123:1234/path");
//}

TEST_F(NativeStreamingClientModuleTest, GetAvailableComponentTypes)
{
    const auto module = CreateModule();

    DictPtr<IString, IFunctionBlockType> functionBlockTypes;
    ASSERT_NO_THROW(functionBlockTypes = module.getAvailableFunctionBlockTypes());
    ASSERT_EQ(functionBlockTypes.getCount(), 0u);

    DictPtr<IString, IDeviceType> deviceTypes;
    ASSERT_NO_THROW(deviceTypes = module.getAvailableDeviceTypes());
    ASSERT_EQ(deviceTypes.getCount(), ExpectedDeviceTypeCount);
    ASSERT_TRUE(deviceTypes.hasKey("OpenDAQNativeStreaming"));
    ASSERT_EQ(deviceTypes.get("OpenDAQNativeStreaming").getId(), "OpenDAQNativeStreaming");
    ASSERT_TRUE(deviceTypes.hasKey("OpenDAQNativeConfiguration"));
    ASSERT_EQ(deviceTypes.get("OpenDAQNativeConfiguration").getId(), "OpenDAQNativeConfiguration");

    DictPtr<IString, IServerType> serverTypes;
    ASSERT_NO_THROW(serverTypes = module.getAvailableServerTypes());
    ASSERT_EQ(serverTypes.getCount(), 0u);

    // Check module info for module
    ModuleInfoPtr moduleInfo;
    ASSERT_NO_THROW(moduleInfo = module.getModuleInfo());
    ASSERT_NE(moduleInfo, nullptr);
    ASSERT_EQ(moduleInfo.getName(), "OpenDAQNativeStreamingClientModule");
    ASSERT_EQ(moduleInfo.getId(), "OpenDAQNativeStreamingClientModule");

    // Check version info for module
    VersionInfoPtr versionInfoModule;
    ASSERT_NO_THROW(versionInfoModule = moduleInfo.getVersionInfo());
    ASSERT_NE(versionInfoModule, nullptr);
    ASSERT_EQ(versionInfoModule.getMajor(), NATIVE_STREAM_CL_MODULE_MAJOR_VERSION);
    ASSERT_EQ(versionInfoModule.getMinor(), NATIVE_STREAM_CL_MODULE_MINOR_VERSION);
    ASSERT_EQ(versionInfoModule.getPatch(), NATIVE_STREAM_CL_MODULE_PATCH_VERSION);

    // Check module and version info for device types
    for (const auto& deviceType : deviceTypes)
    {
        ModuleInfoPtr moduleInfoDeviceType;
        ASSERT_NO_THROW(moduleInfoDeviceType = deviceType.second.getModuleInfo());
        ASSERT_NE(moduleInfoDeviceType, nullptr);
        ASSERT_EQ(moduleInfoDeviceType.getName(), "OpenDAQNativeStreamingClientModule");
        ASSERT_EQ(moduleInfoDeviceType.getId(), "OpenDAQNativeStreamingClientModule");

        VersionInfoPtr versionInfoDeviceType;
        ASSERT_NO_THROW(versionInfoDeviceType = moduleInfoDeviceType.getVersionInfo());
        ASSERT_NE(versionInfoDeviceType, nullptr);
        ASSERT_EQ(versionInfoDeviceType.getMajor(), NATIVE_STREAM_CL_MODULE_MAJOR_VERSION);
        ASSERT_EQ(versionInfoDeviceType.getMinor(), NATIVE_STREAM_CL_MODULE_MINOR_VERSION);
        ASSERT_EQ(versionInfoDeviceType.getPatch(), NATIVE_STREAM_CL_MODULE_PATCH_VERSION);
    }
}

TEST_F(NativeStreamingClientModuleTest, DefaultDeviceConfig)
{
    const auto module = CreateModule();

    DictPtr<IString, IDeviceType> deviceTypes;
    ASSERT_NO_THROW(deviceTypes = module.getAvailableDeviceTypes());
    ASSERT_EQ(deviceTypes.getCount(), ExpectedDeviceTypeCount);

    for (const auto& [id, deviceType] : deviceTypes)
    {
        auto config = deviceType.createDefaultConfig();
        ASSERT_TRUE(config.assigned()) << "no default config for device type " << id;
    }
}

TEST_F(NativeStreamingClientModuleTest, StreamingTypes)
{
    const auto module = CreateModule();

    DictPtr<IString, IStreamingType> streamingTypes;
    ASSERT_NO_THROW(streamingTypes = module.getAvailableStreamingTypes());
    ASSERT_EQ(streamingTypes.getCount(), ExpectedStreamingTypeCount);
    ASSERT_TRUE(streamingTypes.hasKey("OpenDAQNativeStreaming"));
}

TEST_F(NativeStreamingClientModuleTest, ConnectionStringPrefixes)
{
    const auto module = CreateModule();
    const auto deviceTypes = module.getAvailableDeviceTypes();

    ASSERT_EQ(deviceTypes.get("OpenDAQNativeStreaming").getConnectionStringPrefix(), "daq.ns");
    ASSERT_EQ(deviceTypes.get("OpenDAQNativeConfiguration").getConnectionStringPrefix(), "daq.nd");
}

TEST_F(NativeStreamingClientModuleTest, TypesListTransports)
{
    const auto module = CreateModule();

    std::vector<ComponentTypePtr> types;
    for (const auto& [_, type] : module.getAvailableDeviceTypes())
        types.push_back(type);
    for (const auto& [_, type] : module.getAvailableStreamingTypes())
        types.push_back(type);

    for (const auto& type : types)
    {
        const auto transports = type.getAvailableTransports();
        ASSERT_EQ(transports.getKeyList(), ExpectedTransportNames()) << type.getId();
        ASSERT_EQ(transports.get("plain"), 7420) << type.getId();
#if NATIVE_STREAMING_ENABLE_TLS
        ASSERT_EQ(transports.get("tls"), 7422) << type.getId();
#endif
    }
}

TEST_F(NativeStreamingClientModuleTest, DefaultConfigsCarryTransportSelection)
{
    const auto module = CreateModule();

    for (const auto& [id, deviceType] : module.getAvailableDeviceTypes())
    {
        const PropertyObjectPtr config = deviceType.createDefaultConfig();
        const AddDeviceConfigPtr addDeviceConfig = config.getPropertyValue("AddDeviceConfig");
        ASSERT_EQ(addDeviceConfig.getAvailableTransports(), ExpectedTransportNames()) << id;
        ASSERT_EQ(addDeviceConfig.getTransport(), "plain") << id;
    }

    for (const auto& [id, streamingType] : module.getAvailableStreamingTypes())
    {
        const PropertyObjectPtr config = streamingType.createDefaultConfig();
        const AddStreamingConfigPtr addStreamingConfig = config.getPropertyValue("AddStreamingConfig");
        ASSERT_EQ(addStreamingConfig.getAvailableTransports(), ExpectedTransportNames()) << id;
        ASSERT_EQ(addStreamingConfig.getTransport(), "plain") << id;
    }
}

#if NATIVE_STREAMING_ENABLE_TLS

TEST_F(NativeStreamingClientModuleTest, TlsDefaultConfig)
{
    const auto module = CreateModule();
    const auto deviceTypes = module.getAvailableDeviceTypes();

    for (const auto& id : {"OpenDAQNativeConfiguration", "OpenDAQNativeStreaming"})
    {
        auto config = deviceTypes.get(id).createDefaultConfig();

#ifdef NATIVE_STREAMING_TLS_WITHOUT_SIGN
        ASSERT_FALSE(config.hasProperty("VerifyServerCertificate")) << id;
        ASSERT_FALSE(config.hasProperty("CaCertificateFilePath")) << id;
        ASSERT_FALSE(config.hasProperty("EnableMutualTls")) << id;
        ASSERT_FALSE(config.hasProperty("CertificateFilePath")) << id;
        ASSERT_FALSE(config.hasProperty("KeyFilePath")) << id;
#else
        ASSERT_TRUE(config.hasProperty("VerifyServerCertificate")) << id;
        ASSERT_EQ(config.getPropertyValue("VerifyServerCertificate"), True) << id;
        ASSERT_TRUE(config.hasProperty("CaCertificateFilePath")) << id;
        ASSERT_TRUE(config.hasProperty("CertificateFilePath")) << id;
        ASSERT_TRUE(config.hasProperty("KeyFilePath")) << id;

        ASSERT_TRUE(config.hasProperty("EnableMutualTls")) << id;
        ASSERT_EQ(config.getPropertyValue("EnableMutualTls"), False) << id;
#endif

        // left at its default, the port of the selected transport is used instead
        ASSERT_EQ(config.getPropertyValue("Port"), 7420) << id;

        const PropertyObjectPtr transportLayerConfig = config.getPropertyValue("TransportLayerConfig");
        ASSERT_FALSE(transportLayerConfig.hasProperty("CaCertificateFilePath")) << id;
        ASSERT_FALSE(transportLayerConfig.hasProperty("VerifyServerCertificate")) << id;
    }
}

#endif

// A capability which arrives without a port is completed with the default port
TEST_F(NativeStreamingClientModuleTest, CompleteCapabilityPortDefault)
{
    const auto module = CreateModule();

    const auto sourceAddressInfo = AddressInfoBuilder()
                                       .setAddress("127.0.0.1")
                                       .setType("IPv4")
                                       .setReachabilityStatus(AddressReachabilityStatus::Unknown)
                                       .setConnectionString("daq.opcua://127.0.0.1")
                                       .build();

    const auto source = ServerCapability("OpenDAQOPCUAConfiguration", "OpenDAQOPCUAConfiguration", ProtocolType::Configuration)
                            .setConnectionType("TCP/IP")
                            .setPrefix("daq.opcua")
                            .addAddress("127.0.0.1")
                            .addAddressInfo(sourceAddressInfo);

    std::vector<std::tuple<std::string, std::string, Int>> expected{
        {"OpenDAQNativeStreaming", "daq.ns", 7420},
        {"OpenDAQNativeConfiguration", "daq.nd", 7420},
    };

    for (const auto& [protocolId, prefix, port] : expected)
    {
        auto target = ServerCapability(protocolId, protocolId, ProtocolType::Streaming).setPrefix(prefix);
        ASSERT_EQ(target.getPort(), -1) << protocolId;

        ASSERT_TRUE(module.completeServerCapability(source, target)) << protocolId;

        ASSERT_EQ(target.getPort(), port) << protocolId;
        ASSERT_EQ(target.getConnectionString(), prefix + "://127.0.0.1:" + std::to_string(port)) << protocolId;
    }
}

// a port the capability already carries is left alone
TEST_F(NativeStreamingClientModuleTest, CompleteCapabilityKeepsGivenPort)
{
    const auto module = CreateModule();

    const auto sourceAddressInfo = AddressInfoBuilder()
                                       .setAddress("127.0.0.1")
                                       .setType("IPv4")
                                       .setReachabilityStatus(AddressReachabilityStatus::Unknown)
                                       .setConnectionString("daq.opcua://127.0.0.1")
                                       .build();

    const auto source = ServerCapability("OpenDAQOPCUAConfiguration", "OpenDAQOPCUAConfiguration", ProtocolType::Configuration)
                            .setConnectionType("TCP/IP")
                            .setPrefix("daq.opcua")
                            .addAddress("127.0.0.1")
                            .addAddressInfo(sourceAddressInfo);

    const std::string protocolId = "OpenDAQNativeConfiguration";
    const std::string prefix = "daq.nd";

    auto target = ServerCapability(protocolId, protocolId, ProtocolType::ConfigurationAndStreaming).setPrefix(prefix).setPort(1234);

    ASSERT_TRUE(module.completeServerCapability(source, target));
    ASSERT_EQ(target.getPort(), 1234);
    ASSERT_EQ(target.getConnectionString(), prefix + "://127.0.0.1:1234");
}

// the capability of a connected server keeps only the transports this client supports too
TEST_F(NativeStreamingClientModuleTest, CompleteCapabilityKeepsSupportedTransports)
{
    const auto module = CreateModule();

    const auto sourceAddressInfo = AddressInfoBuilder()
                                       .setAddress("127.0.0.1")
                                       .setType("IPv4")
                                       .setReachabilityStatus(AddressReachabilityStatus::Unknown)
                                       .setConnectionString("daq.opcua://127.0.0.1")
                                       .build();

    const auto source = ServerCapability("OpenDAQOPCUAConfiguration", "OpenDAQOPCUAConfiguration", ProtocolType::Configuration)
                            .setConnectionType("TCP/IP")
                            .setPrefix("daq.opcua")
                            .addAddress("127.0.0.1")
                            .addAddressInfo(sourceAddressInfo);

    auto target = ServerCapability("OpenDAQNativeConfiguration", "OpenDAQNativeConfiguration", ProtocolType::ConfigurationAndStreaming)
                      .setPrefix("daq.nd")
                      .setAvailableTransports(Dict<IString, IInteger>({{"wss", 9000}, {"plain", 7420}, {"tls", 7422}}));

    ASSERT_TRUE(module.completeServerCapability(source, target));
    ASSERT_EQ(target.getAvailableTransports().getKeyList(), ExpectedTransportNames());
    ASSERT_EQ(target.getPort(), 7420);
    ASSERT_EQ(target.getConnectionString(), "daq.nd://127.0.0.1:7420");

    auto unsupported = ServerCapability("OpenDAQNativeConfiguration", "OpenDAQNativeConfiguration", ProtocolType::ConfigurationAndStreaming)
                           .setPrefix("daq.nd")
                           .setAvailableTransports(Dict<IString, IInteger>({{"wss", 9000}}));
    ASSERT_FALSE(module.completeServerCapability(source, unsupported));
}

// a server which does not list its transports serves plain on the capability port
TEST_F(NativeStreamingClientModuleTest, CompleteCapabilityWithoutTransportsServesPlain)
{
    const auto module = CreateModule();

    const auto sourceAddressInfo = AddressInfoBuilder()
                                       .setAddress("127.0.0.1")
                                       .setType("IPv4")
                                       .setReachabilityStatus(AddressReachabilityStatus::Unknown)
                                       .setConnectionString("daq.opcua://127.0.0.1")
                                       .build();

    const auto source = ServerCapability("OpenDAQOPCUAConfiguration", "OpenDAQOPCUAConfiguration", ProtocolType::Configuration)
                            .setConnectionType("TCP/IP")
                            .setPrefix("daq.opcua")
                            .addAddress("127.0.0.1")
                            .addAddressInfo(sourceAddressInfo);

    auto target = ServerCapability("OpenDAQNativeStreaming", "OpenDAQNativeStreaming", ProtocolType::Streaming).setPrefix("daq.ns").setPort(1234);
    ASSERT_EQ(target.getAvailableTransports().getCount(), 0u);

    ASSERT_TRUE(module.completeServerCapability(source, target));

    const auto transports = target.getAvailableTransports();
    ASSERT_EQ(transports.getKeyList(), List<IString>("plain"));
    ASSERT_EQ(transports.get("plain"), 1234);
    ASSERT_EQ(target.getPort(), 1234);
    ASSERT_EQ(target.getConnectionString(), "daq.ns://127.0.0.1:1234");

    // without a port the default plain port is taken
    auto withoutPort = ServerCapability("OpenDAQNativeStreaming", "OpenDAQNativeStreaming", ProtocolType::Streaming).setPrefix("daq.ns");
    ASSERT_TRUE(module.completeServerCapability(source, withoutPort));
    ASSERT_EQ(withoutPort.getAvailableTransports().get("plain"), 7420);
}

class ConnectionStringTest : public NativeStreamingClientModuleTest,
                             public testing::WithParamInterface<StringPtr>
{
};

TEST_P(ConnectionStringTest, CreateDeviceWrongConnectionString)
{
    auto module = CreateModule();

    StringPtr connectionString = GetParam();

    ASSERT_THROW(module.createDevice(connectionString, nullptr), InvalidParameterException);
}

TEST_P(ConnectionStringTest, CreateStreamingWrongConnectionString)
{
    auto module = CreateModule();

    StringPtr connectionString = GetParam();

    ASSERT_THROW(module.createStreaming(connectionString, nullptr), InvalidParameterException);
}

INSTANTIATE_TEST_SUITE_P(
    ConnectionString,
    ConnectionStringTest,
    testing::Values(
        "",
        "drfrfgt",
        "daq.opcua://device8",
        "daqref://devicett3axxr1",
        "daq.opcua://devicett3axxr1"
        "daq.ns://",
        "daq.ns:///",
        "daq.opcua://[::1]",
        "daq.nss://",
        "daq.nss:///",
        "daq.nds://",
        "daq.nds:///"
    )
);
