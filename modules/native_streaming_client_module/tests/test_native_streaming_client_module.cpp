#include <testutils/testutils.h>
#include <testutils/transport_options.h>
#include <native_streaming_client_module/module_dll.h>
#include <native_streaming_client_module/version.h>
#include <native_streaming_client_module/native_device_utils.h>
#include <gmock/gmock.h>

#include <opendaq/module_ptr.h>
#include <opendaq/login_parameters_factory.h>
#include <opendaq/authentication_method_factory.h>
#include <coretypes/common.h>

#include <opendaq/context_factory.h>
#include <coreobjects/property_factory.h>
#include <coreobjects/property_object_factory.h>
#include <opendaq/device_info_factory.h>
#include <opendaq/device_info_internal_ptr.h>
#include <opendaq/address_info_factory.h>
#include <opendaq/instance_factory.h>
#include <opendaq/module_manager_utils_ptr.h>
#include <opendaq/streaming_type_ptr.h>
#include <opendaq/address_info_factory.h>
#include <native_streaming_protocol/native_streaming_constants.h>

#include <tuple>
#include <vector>

using NativeStreamingClientModuleTest = testing::Test;
using namespace daq;

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

static LoginParametersPtr anonymousLogin()
{
    return LoginParametersBuilder().setAuthenticationMethod(StandardAnonymousAuthenticationMethod()).build();
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

    auto config = PropertyObject();
    config.addProperty(StringProperty("TransportOption", "ws"));

    ASSERT_THROW(module.createAuthenticatedDevice("daq.ns://127.0.0.1", nullptr, config, anonymousLogin()), NotFoundException);
    ASSERT_THROW(module.createAuthenticatedDevice("daq.nd://127.0.0.1", nullptr, config, anonymousLogin()), NotFoundException);
}

TEST_F(NativeStreamingClientModuleTest, PlainCreateCreatesNothing)
{
    auto module = CreateModule();

    ASSERT_FALSE(module.createDevice("daq.nd://127.0.0.1", nullptr).assigned());
    ASSERT_FALSE(module.createStreaming("daq.ns://127.0.0.1", nullptr).assigned());
}

// The module manager adds the transport option to the config; the module does not pick one itself
TEST_F(NativeStreamingClientModuleTest, TransportOptionRequired)
{
    auto module = CreateModule();

    for (const auto& connectionString : {"daq.ns://127.0.0.1", "daq.nd://127.0.0.1"})
    {
        try
        {
            module.createAuthenticatedDevice(connectionString, nullptr, nullptr, anonymousLogin());
            FAIL() << connectionString << ": the device was created";
        }
        catch (const InvalidParameterException& e)
        {
            ASSERT_NE(std::string(e.what()).find("TransportOption"), std::string::npos) << connectionString << ": " << e.what();
        }
    }

    ASSERT_THROW(module.createAuthenticatedStreaming("daq.ns://127.0.0.1", nullptr, anonymousLogin()), InvalidParameterException);
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
    ASSERT_EQ(deviceTypes.getCount(), 2u);
    ASSERT_TRUE(deviceTypes.hasKey("OpenDAQNativeStreaming"));
    ASSERT_EQ(deviceTypes.get("OpenDAQNativeStreaming").getId(), "OpenDAQNativeStreaming");
    ASSERT_EQ(deviceTypes.get("OpenDAQNativeStreaming").getDeviceKind(), DeviceKind::Adapter);
    ASSERT_TRUE(deviceTypes.hasKey("OpenDAQNativeConfiguration"));
    ASSERT_EQ(deviceTypes.get("OpenDAQNativeConfiguration").getId(), "OpenDAQNativeConfiguration");
    ASSERT_EQ(deviceTypes.get("OpenDAQNativeConfiguration").getDeviceKind(), DeviceKind::Mirrored);

    DictPtr<IString, IStreamingType> streamingTypes;
    ASSERT_NO_THROW(streamingTypes = module.getAvailableStreamingTypes());
    ASSERT_EQ(streamingTypes.getCount(), 1u);
    ASSERT_EQ(streamingTypes.get("OpenDAQNativeStreaming").getLinkedDeviceType().getId(), "OpenDAQNativeConfiguration");

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
    ASSERT_EQ(deviceTypes.getCount(), 2u);

    for (const auto& [id, deviceType] : deviceTypes)
    {
        auto config = deviceType.createDefaultConfig();
        ASSERT_TRUE(config.assigned()) << "no default config for device type " << id;
    }
}

TEST_F(NativeStreamingClientModuleTest, ModuleLoadsFromPathWithLinkedTypes)
{
    const auto instance = Instance("[[none]]");
    ModulePtr module;
    ASSERT_NO_THROW(module = instance.getModuleManager().loadModule(NATIVE_STREAM_CL_MODULE_PATH));
    ASSERT_EQ(module.getModuleInfo().getId(), "OpenDAQNativeStreamingClientModule");
    ASSERT_EQ(instance.getModuleManager().asPtr<IModuleManagerUtils>().getAvailableStreamingTypes().getCount(), 1u);
}

TEST_F(NativeStreamingClientModuleTest, StreamingTypes)
{
    const auto module = CreateModule();

    DictPtr<IString, IStreamingType> streamingTypes;
    ASSERT_NO_THROW(streamingTypes = module.getAvailableStreamingTypes());
    ASSERT_EQ(streamingTypes.getCount(), 1u);
    ASSERT_TRUE(streamingTypes.hasKey("OpenDAQNativeStreaming"));
}

TEST_F(NativeStreamingClientModuleTest, ConnectionStringPrefixes)
{
    const auto module = CreateModule();
    const auto deviceTypes = module.getAvailableDeviceTypes();

    ASSERT_EQ(deviceTypes.get("OpenDAQNativeStreaming").getConnectionStringPrefix(), "daq.ns");
    ASSERT_EQ(deviceTypes.get("OpenDAQNativeConfiguration").getConnectionStringPrefix(), "daq.nd");
}

// The plain types list "ws" first, then the TLS options this build supports
TEST_F(NativeStreamingClientModuleTest, TypesListTransportOptions)
{
    const auto module = CreateModule();
    const auto deviceTypes = module.getAvailableDeviceTypes();
    const auto streamingTypes = module.getAvailableStreamingTypes();

    auto expectedIds = List<IString>("ws");
#if NATIVE_STREAMING_ENABLE_TLS
    expectedIds.pushBack("wss");
#ifndef NATIVE_STREAMING_TLS_WITHOUT_SIGN
    expectedIds.pushBack("wss-mtls");
#endif
#endif

    const std::vector<ComponentTypePtr> types{deviceTypes.get("OpenDAQNativeConfiguration").asPtr<IComponentType>(),
                                              deviceTypes.get("OpenDAQNativeStreaming").asPtr<IComponentType>(),
                                              streamingTypes.get("OpenDAQNativeStreaming").asPtr<IComponentType>()};
    for (const auto& type : types)
    {
        const DictPtr<IString, ITransportOption> options = type.getAvailableTransportOptions();
        ASSERT_EQ(options.getKeyList(), expectedIds) << type.getId();

        const TransportOptionPtr ws = options.get("ws");
        ASSERT_EQ(ws.getPort(), 7420) << type.getId();
        ASSERT_EQ(ws.getSecurityMode(), SecurityMode::None) << type.getId();
        ASSERT_FALSE(ws.getCertificateRequired()) << type.getId();

#if NATIVE_STREAMING_ENABLE_TLS
        const TransportOptionPtr wss = options.get("wss");
        ASSERT_EQ(wss.getPort(), 7422) << type.getId();
        ASSERT_EQ(wss.getSecurityMode(), SecurityMode::Encrypted) << type.getId();
        ASSERT_FALSE(wss.getCertificateRequired()) << type.getId();
#ifndef NATIVE_STREAMING_TLS_WITHOUT_SIGN
        const TransportOptionPtr mtls = options.get("wss-mtls");
        ASSERT_EQ(mtls.getPort(), 7422) << type.getId();
        ASSERT_EQ(mtls.getSecurityMode(), SecurityMode::Encrypted) << type.getId();
        ASSERT_TRUE(mtls.getCertificateRequired()) << type.getId();
#endif
#endif
    }

    // The selection is not part of the default config: the module manager adds it
    for (const auto& id : {"OpenDAQNativeConfiguration", "OpenDAQNativeStreaming"})
    {
        const auto config = deviceTypes.get(id).createDefaultConfig();
        ASSERT_FALSE(config.hasProperty("TransportOption")) << id;
        ASSERT_EQ(config.getPropertyValue("Port"), 7420) << id;
    }
}

// The module refuses an option it does not list before it tries to connect
TEST_F(NativeStreamingClientModuleTest, UnknownTransportOptionRefused)
{
    const auto module = CreateModule();
    const auto deviceTypes = module.getAvailableDeviceTypes();

    std::vector<std::string> unknownIds{"tls"};
#if !NATIVE_STREAMING_ENABLE_TLS
    unknownIds.emplace_back("wss");
    unknownIds.emplace_back("wss-mtls");
#elif defined(NATIVE_STREAMING_TLS_WITHOUT_SIGN)
    unknownIds.emplace_back("wss-mtls");
#endif

    for (const auto& [id, prefix] : {std::pair{"OpenDAQNativeConfiguration", "daq.nd"}, std::pair{"OpenDAQNativeStreaming", "daq.ns"}})
    {
        for (const auto& option : unknownIds)
        {
            auto config = deviceTypes.get(id).createDefaultConfig();
            config.addProperty(StringProperty("TransportOption", option));

            try
            {
                module.createAuthenticatedDevice(std::string(prefix) + "://127.0.0.1/", nullptr, config, anonymousLogin());
                FAIL() << id << " " << option << ": the device was created";
            }
            catch (const NotFoundException& e)
            {
                ASSERT_NE(std::string(e.what()).find("transport option"), std::string::npos) << id << " " << option << ": " << e.what();
            }
        }
    }
}

// TLS is selected with the transport option, there are no secure prefixes
TEST_F(NativeStreamingClientModuleTest, SecurePrefixesRefused)
{
    const auto module = CreateModule();
    ASSERT_THROW(module.createAuthenticatedDevice("daq.nds://127.0.0.1:7422/", nullptr, nullptr, anonymousLogin()), InvalidParameterException);
    ASSERT_THROW(module.createAuthenticatedDevice("daq.nss://127.0.0.1:7422/", nullptr, nullptr, anonymousLogin()), InvalidParameterException);
}

#if NATIVE_STREAMING_ENABLE_TLS

// The client TLS settings sit next to the plain ones, and are used only when a TLS option is selected
TEST_F(NativeStreamingClientModuleTest, DefaultConfigTlsSettings)
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

        const PropertyObjectPtr transportLayerConfig = config.getPropertyValue("TransportLayerConfig");
        ASSERT_FALSE(transportLayerConfig.hasProperty("CaCertificateFilePath")) << id;
        ASSERT_FALSE(transportLayerConfig.hasProperty("VerifyServerCertificate")) << id;
    }
}

#endif

namespace
{
IntegerPtr capabilityPort(const ServerCapabilityPtr& capability)
{
#if defined(__GNUC__)
    #pragma GCC diagnostic push
    #pragma GCC diagnostic ignored "-Wdeprecated-declarations"
#endif
#ifdef _MSC_VER
    #pragma warning(push)
    #pragma warning(disable : 4996)
#endif
    return capability.getPort();
#ifdef _MSC_VER
    #pragma warning(pop)
#endif
#if defined(__GNUC__)
    #pragma GCC diagnostic pop
#endif
}
}

// A capability which arrives without a port is completed with the default port
TEST_F(NativeStreamingClientModuleTest, CompleteCapabilityPortDefaults)
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

    const std::vector<std::tuple<std::string, std::string, Int>> expected{
        {"OpenDAQNativeStreaming", "daq.ns", 7420},
        {"OpenDAQNativeConfiguration", "daq.nd", 7420},
    };

    for (const auto& [protocolId, prefix, port] : expected)
    {
        auto target = ServerCapability(protocolId, protocolId, ProtocolType::Streaming).setPrefix(prefix);
        ASSERT_EQ(capabilityPort(target), -1) << protocolId;

        ASSERT_TRUE(module.completeServerCapability(source, target)) << protocolId;

        ASSERT_EQ(capabilityPort(target), port) << protocolId;
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
    ASSERT_EQ(capabilityPort(target), 1234);
    ASSERT_EQ(target.getConnectionString(), prefix + "://127.0.0.1:1234");
}

// The connection string takes the port of the first option the capability announces, here of a TLS-only server
TEST_F(NativeStreamingClientModuleTest, CompleteCapabilityTakesPortOfFirstOption)
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

    auto target = ServerCapability(protocolId, protocolId, ProtocolType::ConfigurationAndStreaming).setPrefix(prefix);
    target.setAvailableTransportOptions(TransportOptionsById({TestTransportOption("wss", 7502, SecurityMode::Encrypted)}));

    ASSERT_TRUE(module.completeServerCapability(source, target));
    ASSERT_EQ(target.getConnectionString(), prefix + "://127.0.0.1:7502");
}

TEST_F(NativeStreamingClientModuleTest, CollectAlternativeAddresses)
{
    using Status = AddressReachabilityStatus;

    auto capability = ServerCapability("cap", "cap", ProtocolType::Streaming);
    for (const auto& [address, type, status] : {std::make_tuple("a", "IPv6", Status::Unknown),
                                                std::make_tuple("b", "IPv4", Status::Unreachable),
                                                std::make_tuple("c", "IPv4", Status::Unknown),
                                                std::make_tuple("d", "IPv6", Status::Reachable)})
        capability.addAddressInfo(
            AddressInfoBuilder().setAddress(address).setConnectionString(address).setType(type).setReachabilityStatus(status).build());

    const auto info = DeviceInfo("x://s");
    info.asPtr<IDeviceInfoInternal>().addServerCapability(capability);

    ListPtr<IString> addresses;
    ASSERT_EQ(modules::native_streaming_client_module::collectAlternativeAddresses(nullptr, info, "cap", addresses), OPENDAQ_SUCCESS);
    ASSERT_EQ(addresses, List<IString>("d", "c", "a"));
}

class ConnectionStringTest : public NativeStreamingClientModuleTest,
                             public testing::WithParamInterface<StringPtr>
{
};

TEST_P(ConnectionStringTest, CreateDeviceWrongConnectionString)
{
    auto module = CreateModule();

    StringPtr connectionString = GetParam();

    ASSERT_THROW(module.createAuthenticatedDevice(connectionString, nullptr, nullptr, anonymousLogin()), InvalidParameterException);
}

TEST_P(ConnectionStringTest, CreateStreamingWrongConnectionString)
{
    auto module = CreateModule();

    StringPtr connectionString = GetParam();

    ASSERT_THROW(module.createAuthenticatedStreaming(connectionString, nullptr, anonymousLogin()), InvalidParameterException);
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
