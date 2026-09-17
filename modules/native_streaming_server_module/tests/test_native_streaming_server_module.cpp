#include <native_streaming_server_module/module_dll.h>
#include <native_streaming_server_module/version.h>
#include <opendaq/context_factory.h>
#include <opendaq/instance_factory.h>
#include <opendaq/instance_ptr.h>
#include <opendaq/module_manager_factory.h>
#include <opendaq/module_ptr.h>
#include <coretypes/common.h>
#include <gmock/gmock.h>
#include <testutils/testutils.h>
#include <opendaq/mock/mock_device_module.h>
#include <opendaq/mock/mock_fb_module.h>
#include <coreobjects/authentication_provider_factory.h>

#include <boost/asio/io_context.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/ip/v6_only.hpp>

using NativeStreamingServerModuleTest = testing::Test;
using namespace daq;

static ModulePtr CreateModule(ContextPtr context = NullContext(), ModuleManagerPtr manager = nullptr)
{
    ModulePtr module;
    createNativeStreamingServerModule(&module, context);
    return module;
}

static InstancePtr CreateTestInstance()
{
    const auto logger = Logger();
    const auto moduleManager = ModuleManager("[[none]]");
    const auto authenticationProvider = AuthenticationProvider();
    const auto context = Context(Scheduler(logger), logger, TypeManager(), moduleManager, authenticationProvider);

    const ModulePtr deviceModule(MockDeviceModule_Create(context));
    moduleManager.addModule(deviceModule);

    const ModulePtr fbModule(MockFunctionBlockModule_Create(context));
    moduleManager.addModule(fbModule);

    const ModulePtr daqNativeStreamingServerModule = CreateModule(context, moduleManager);
    moduleManager.addModule(daqNativeStreamingServerModule);

    auto instance = InstanceCustom(context, "localInstance");
    for (const auto& deviceInfo : instance.getAvailableDevices())
        instance.addDevice(deviceInfo.getConnectionString());

    for (const auto& [id, _] : instance.getAvailableFunctionBlockTypes())
        instance.addFunctionBlock(id);

    return instance;
}

static PropertyObjectPtr CreateServerConfig(const InstancePtr& instance)
{
    auto config = instance.getAvailableServerTypes().get("OpenDAQNativeStreaming").createDefaultConfig();
    config.setPropertyValue("NativeStreamingPort", 0);
    return config;
}

// Holds a port the way another process listening on it would. Both address families are taken, since
// the server gives up only when it can open neither; the IPv6 one is best effort, as without IPv6 the
// server cannot open it either. The server binds with reuse_address, which on Windows would let it
// bind over a holder that did not claim the port exclusively.
class PortHolder
{
public:
    PortHolder()
        : v4(ioContext)
        , v6(ioContext)
    {
        using boost::asio::ip::tcp;

        v4.open(tcp::v4());
        claimExclusively(v4);
        v4.bind(tcp::endpoint(tcp::v4(), 0));
        v4.listen();
        port = v4.local_endpoint().port();

        boost::system::error_code ec;
        v6.open(tcp::v6(), ec);
        if (!ec)
            v6.set_option(boost::asio::ip::v6_only(true), ec);
        if (!ec)
            claimExclusively(v6);
        if (!ec)
            v6.bind(tcp::endpoint(tcp::v6(), port), ec);
        if (!ec)
            v6.listen(boost::asio::socket_base::max_listen_connections, ec);
    }

    uint16_t getPort() const
    {
        return port;
    }

private:
    static void claimExclusively([[maybe_unused]] boost::asio::ip::tcp::acceptor& acceptor)
    {
#ifdef _WIN32
        acceptor.set_option(boost::asio::detail::socket_option::boolean<SOL_SOCKET, SO_EXCLUSIVEADDRUSE>(true));
#endif
    }

    boost::asio::io_context ioContext;
    boost::asio::ip::tcp::acceptor v4;
    boost::asio::ip::tcp::acceptor v6;
    uint16_t port = 0;
};

// a port which was free a moment ago, for a test which needs to name the same one twice
static uint16_t FindFreePort()
{
    using boost::asio::ip::tcp;

    boost::asio::io_context ioContext;
    tcp::acceptor acceptor(ioContext, tcp::endpoint(tcp::v4(), 0));
    return acceptor.local_endpoint().port();
}

TEST_F(NativeStreamingServerModuleTest, CreateModule)
{
    IModule* module = nullptr;
    ErrCode errCode = createModule(&module, NullContext());
    ASSERT_TRUE(OPENDAQ_SUCCEEDED(errCode));

    ASSERT_NE(module, nullptr);
    module->releaseRef();
}

TEST_F(NativeStreamingServerModuleTest, ModuleName)
{
    auto module = CreateModule();
    ASSERT_EQ(module.getModuleInfo().getName(), "OpenDAQNativeStreamingServerModule");
}

TEST_F(NativeStreamingServerModuleTest, VersionAvailable)
{
    auto module = CreateModule();
    ASSERT_TRUE(module.getModuleInfo().getVersionInfo().assigned());
}

TEST_F(NativeStreamingServerModuleTest, VersionCorrect)
{
    auto module = CreateModule();
    auto version = module.getModuleInfo().getVersionInfo();

    ASSERT_EQ(version.getMajor(), NATIVE_STREAM_SRV_MODULE_MAJOR_VERSION);
    ASSERT_EQ(version.getMinor(), NATIVE_STREAM_SRV_MODULE_MINOR_VERSION);
    ASSERT_EQ(version.getPatch(), NATIVE_STREAM_SRV_MODULE_PATCH_VERSION);
}

TEST_F(NativeStreamingServerModuleTest, GetAvailableComponentTypes)
{
    const auto module = CreateModule();

    DictPtr<IString, IFunctionBlockType> functionBlockTypes;
    ASSERT_NO_THROW(functionBlockTypes = module.getAvailableFunctionBlockTypes());
    ASSERT_EQ(functionBlockTypes.getCount(), 0u);

    DictPtr<IString, IDeviceType> deviceTypes;
    ASSERT_NO_THROW(deviceTypes = module.getAvailableDeviceTypes());
    ASSERT_EQ(deviceTypes.getCount(), 0u);

    DictPtr<IString, IServerType> serverTypes;
    ASSERT_NO_THROW(serverTypes = module.getAvailableServerTypes());
    ASSERT_EQ(serverTypes.getCount(), 1u);
    ASSERT_TRUE(serverTypes.hasKey("OpenDAQNativeStreaming"));
    ASSERT_EQ(serverTypes.get("OpenDAQNativeStreaming").getId(), "OpenDAQNativeStreaming");

    // Check module info for module
    ModuleInfoPtr moduleInfo;
    ASSERT_NO_THROW(moduleInfo = module.getModuleInfo());
    ASSERT_NE(moduleInfo, nullptr);
    ASSERT_EQ(moduleInfo.getName(), "OpenDAQNativeStreamingServerModule");
    ASSERT_EQ(moduleInfo.getId(), "OpenDAQNativeStreamingServerModule");

    // Check version info for module
    VersionInfoPtr versionInfoModule;
    ASSERT_NO_THROW(versionInfoModule = moduleInfo.getVersionInfo());
    ASSERT_NE(versionInfoModule, nullptr);
    ASSERT_EQ(versionInfoModule.getMajor(), NATIVE_STREAM_SRV_MODULE_MAJOR_VERSION);
    ASSERT_EQ(versionInfoModule.getMinor(), NATIVE_STREAM_SRV_MODULE_MINOR_VERSION);
    ASSERT_EQ(versionInfoModule.getPatch(), NATIVE_STREAM_SRV_MODULE_PATCH_VERSION);

    // Check module version info for server types
    for (const auto& serverType : serverTypes)
    {
        ModuleInfoPtr moduleInfoServerType;
        ASSERT_NO_THROW(moduleInfoServerType = serverType.second.getModuleInfo());
        ASSERT_NE(moduleInfoServerType, nullptr);
        ASSERT_EQ(moduleInfoServerType.getName(), "OpenDAQNativeStreamingServerModule");
        ASSERT_EQ(moduleInfoServerType.getId(), "OpenDAQNativeStreamingServerModule");

        VersionInfoPtr versionInfoServerType;
        ASSERT_NO_THROW(versionInfoServerType = moduleInfoServerType.getVersionInfo());
        ASSERT_NE(versionInfoServerType, nullptr);
        ASSERT_EQ(versionInfoServerType.getMajor(), NATIVE_STREAM_SRV_MODULE_MAJOR_VERSION);
        ASSERT_EQ(versionInfoServerType.getMinor(), NATIVE_STREAM_SRV_MODULE_MINOR_VERSION);
        ASSERT_EQ(versionInfoServerType.getPatch(), NATIVE_STREAM_SRV_MODULE_PATCH_VERSION);
    }
}

TEST_F(NativeStreamingServerModuleTest, ServerConfig)
{
    auto module = CreateModule();

    DictPtr<IString, IServerType> serverTypes = module.getAvailableServerTypes();
    ASSERT_TRUE(serverTypes.hasKey("OpenDAQNativeStreaming"));
    auto config = serverTypes.get("OpenDAQNativeStreaming").createDefaultConfig();
    ASSERT_TRUE(config.assigned());

    ASSERT_TRUE(config.hasProperty("NativeStreamingPort"));
    ASSERT_EQ(config.getPropertyValue("NativeStreamingPort"), 7420);

    ASSERT_TRUE(config.hasProperty("MaxAllowedConfigConnections"));
    ASSERT_EQ(config.getPropertyValue("MaxAllowedConfigConnections"), 0);

    ASSERT_TRUE(config.hasProperty("StreamingPacketSendTimeout"));
    ASSERT_EQ(config.getPropertyValue("StreamingPacketSendTimeout"), 0);

    ASSERT_TRUE(config.hasProperty("StreamingDataPollingPeriod"));
    ASSERT_EQ(config.getPropertyValue("StreamingDataPollingPeriod"), 20);

    ASSERT_TRUE(config.hasProperty("StreamingCacheablePayloadSizeMax"));
    ASSERT_EQ(config.getPropertyValue("StreamingCacheablePayloadSizeMax"), 10);

    ASSERT_TRUE(config.hasProperty("StreamingPacketReleaseThreshold"));
    ASSERT_EQ(config.getPropertyValue("StreamingPacketReleaseThreshold"), 10);

    ASSERT_TRUE(config.hasProperty("ConfigurationRpcWorkerCount"));
    ASSERT_EQ(config.getPropertyValue("ConfigurationRpcWorkerCount"), 1);

    ASSERT_TRUE(config.hasProperty("EnablePort"));
    ASSERT_EQ(config.getPropertyValue("EnablePort"), True);
}

TEST_F(NativeStreamingServerModuleTest, TlsServerConfig)
{
    auto module = CreateModule();
    auto config = module.getAvailableServerTypes().get("OpenDAQNativeStreaming").createDefaultConfig();

#if NATIVE_STREAMING_ENABLE_TLS
    ASSERT_TRUE(config.hasProperty("EnableTlsPort"));
    ASSERT_EQ(config.getPropertyValue("EnableTlsPort"), False);

    ASSERT_TRUE(config.hasProperty("NativeStreamingTlsPort"));
    ASSERT_EQ(config.getPropertyValue("NativeStreamingTlsPort"), 7422);

    ASSERT_TRUE(config.hasProperty("EnableMutualTls"));
    ASSERT_EQ(config.getPropertyValue("EnableMutualTls"), True);

    ASSERT_TRUE(config.hasProperty("CertificateFilePath"));
    ASSERT_TRUE(config.hasProperty("KeyFilePath"));
    ASSERT_TRUE(config.hasProperty("CaCertificateFilePath"));
#else
    // without the option the TLS channel does not exist
    ASSERT_FALSE(config.hasProperty("EnableTlsPort"));
    ASSERT_FALSE(config.hasProperty("NativeStreamingTlsPort"));
    ASSERT_FALSE(config.hasProperty("CertificateFilePath"));
    ASSERT_FALSE(config.hasProperty("KeyFilePath"));
    ASSERT_FALSE(config.hasProperty("CaCertificateFilePath"));
#endif
}

TEST_F(NativeStreamingServerModuleTest, RefusesConfigurationWithoutAnyListener)
{
    auto device = CreateTestInstance();
    auto config = CreateServerConfig(device);
    config.setPropertyValue("EnablePort", False);

    ASSERT_THROW(device.addServer("OpenDAQNativeStreaming", config), InvalidParameterException);
}

TEST_F(NativeStreamingServerModuleTest, PlaintextServerPublishesTwoCapabilities)
{
    auto device = CreateTestInstance();
    device.addServer("OpenDAQNativeStreaming", CreateServerConfig(device));

    const auto capabilities = device.getInfo().getServerCapabilities();
    ASSERT_EQ(capabilities.getCount(), 2u);
    for (const auto& capability : capabilities)
    {
        ASSERT_EQ(capability.getProtocolGroupId(), "NativeStreaming");
        ASSERT_EQ(capability.getProtocolSecurityLevel(), 0);
    }
}

// Opening the listener fails after the server has started its threads. The failure has to reach the
// caller as an exception, rather than end the process on a still joinable thread, and leave nothing
// published behind.
TEST_F(NativeStreamingServerModuleTest, RefusesPortInUse)
{
    PortHolder holder;

    auto device = CreateTestInstance();
    auto config = CreateServerConfig(device);
    config.setPropertyValue("NativeStreamingPort", holder.getPort());

    ASSERT_THROW(device.addServer("OpenDAQNativeStreaming", config), GeneralErrorException);
    ASSERT_EQ(device.getInfo().getServerCapabilities().getCount(), 0u);

    // nothing of the failed server is left in the way of one on a port which is free
    ASSERT_NO_THROW(device.addServer("OpenDAQNativeStreaming", CreateServerConfig(device)));
    ASSERT_EQ(device.getInfo().getServerCapabilities().getCount(), 2u);
}

#if NATIVE_STREAMING_ENABLE_TLS

// a secret which fails to load is found only when the listener opens, after the threads have started
TEST_F(NativeStreamingServerModuleTest, RefusesTlsWithUnreadableCertificate)
{
    auto device = CreateTestInstance();
    auto config = CreateServerConfig(device);
    config.setPropertyValue("EnablePort", False);
    config.setPropertyValue("EnableTlsPort", True);
    config.setPropertyValue("NativeStreamingTlsPort", 0);
    config.setPropertyValue("CertificateFilePath", "nonexistent/server.crt");
    config.setPropertyValue("KeyFilePath", "nonexistent/server.key");
    config.setPropertyValue("EnableMutualTls", False);

    ASSERT_THROW(device.addServer("OpenDAQNativeStreaming", config), GeneralErrorException);
    ASSERT_EQ(device.getInfo().getServerCapabilities().getCount(), 0u);
}

// The plaintext channel opens and publishes its capabilities before the secure one fails. Both have
// to be withdrawn: a server which failed to construct must not keep listening or stay advertised.
TEST_F(NativeStreamingServerModuleTest, WithdrawsPlaintextChannelWhenTlsFails)
{
    const auto plainPort = FindFreePort();

    auto device = CreateTestInstance();
    auto config = CreateServerConfig(device);
    config.setPropertyValue("NativeStreamingPort", plainPort);
    config.setPropertyValue("EnableTlsPort", True);
    config.setPropertyValue("NativeStreamingTlsPort", 0);
    config.setPropertyValue("CertificateFilePath", "nonexistent/server.crt");
    config.setPropertyValue("KeyFilePath", "nonexistent/server.key");
    config.setPropertyValue("EnableMutualTls", False);

    ASSERT_THROW(device.addServer("OpenDAQNativeStreaming", config), GeneralErrorException);
    ASSERT_EQ(device.getInfo().getServerCapabilities().getCount(), 0u);

    // the plaintext listener is gone too: a server can open the same port again
    auto plainOnly = CreateServerConfig(device);
    plainOnly.setPropertyValue("NativeStreamingPort", plainPort);
    ASSERT_NO_THROW(device.addServer("OpenDAQNativeStreaming", plainOnly));
    ASSERT_EQ(device.getInfo().getServerCapabilities().getCount(), 2u);
}

#endif

TEST_F(NativeStreamingServerModuleTest, CreateServer)
{
    auto device = CreateTestInstance();
    auto module = CreateModule(device.getContext());
    auto config = CreateServerConfig(device);

    ASSERT_NO_THROW(module.createServer("OpenDAQNativeStreaming", device.getRootDevice(), config));
}

TEST_F(NativeStreamingServerModuleTest, CreateServerFromInstance)
{
    auto device = CreateTestInstance();
    auto config = CreateServerConfig(device);

    ASSERT_NO_THROW(device.addServer("OpenDAQNativeStreaming", config));
}
