#include <new_websocket_streaming_server_module/module_dll.h>
#include <new_websocket_streaming_server_module/version.h>
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

class NewWebsocketStreamingServerModuleTest : public testing::Test
{
public:
    void TearDown() override
    {
    }
};

using namespace daq;

static ModulePtr CreateModule(ContextPtr context = NullContext())
{
    ModulePtr module;
    createNewWebsocketStreamingServerModule(&module, context);
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

    const ModulePtr daqWebsocketStreamingServerModule = CreateModule(context);
    moduleManager.addModule(daqWebsocketStreamingServerModule);

    auto instance = InstanceCustom(context, "localInstance");
    for (const auto& deviceInfo : instance.getAvailableDevices())
        instance.addDevice(deviceInfo.getConnectionString());

    for (const auto& [id, _] : instance.getAvailableFunctionBlockTypes())
        instance.addFunctionBlock(id);

    return instance;
}

static PropertyObjectPtr CreateServerConfig(const InstancePtr& instance)
{
    auto config = instance.getAvailableServerTypes().get("OpenDAQLTStreaming").createDefaultConfig();
    config.setPropertyValue("WebsocketStreamingPort", 0);
    config.setPropertyValue("WebsocketControlPort", 0);
    return config;
}

TEST_F(NewWebsocketStreamingServerModuleTest, CreateModule)
{
    IModule* module = nullptr;
    ErrCode errCode = createModule(&module, NullContext());
    ASSERT_TRUE(OPENDAQ_SUCCEEDED(errCode));

    ASSERT_NE(module, nullptr);
    module->releaseRef();
}

TEST_F(NewWebsocketStreamingServerModuleTest, ModuleName)
{
    auto module = CreateModule();
    ASSERT_EQ(module.getModuleInfo().getName(), "OpenDAQNewWebsocketStreamingServerModule");
}

TEST_F(NewWebsocketStreamingServerModuleTest, VersionAvailable)
{
    auto module = CreateModule();
    ASSERT_TRUE(module.getModuleInfo().getVersionInfo().assigned());
}

TEST_F(NewWebsocketStreamingServerModuleTest, VersionCorrect)
{
    auto module = CreateModule();
    auto version = module.getModuleInfo().getVersionInfo();

    ASSERT_EQ(version.getMajor(), NEW_WS_STREAM_SRV_MODULE_MAJOR_VERSION);
    ASSERT_EQ(version.getMinor(), NEW_WS_STREAM_SRV_MODULE_MINOR_VERSION);
    ASSERT_EQ(version.getPatch(), NEW_WS_STREAM_SRV_MODULE_PATCH_VERSION);
}

TEST_F(NewWebsocketStreamingServerModuleTest, GetAvailableComponentTypes)
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
    ASSERT_TRUE(serverTypes.hasKey("OpenDAQNewLTStreaming"));
    ASSERT_EQ(serverTypes.get("OpenDAQNewLTStreaming").getId(), "OpenDAQNewLTStreaming");
}

TEST_F(NewWebsocketStreamingServerModuleTest, ServerConfig)
{
    auto module = CreateModule();

    DictPtr<IString, IServerType> serverTypes = module.getAvailableServerTypes();
    ASSERT_TRUE(serverTypes.hasKey("OpenDAQNewLTStreaming"));
    auto config = serverTypes.get("OpenDAQNewLTStreaming").createDefaultConfig();
    ASSERT_TRUE(config.assigned());

    ASSERT_TRUE(config.hasProperty("WebsocketStreamingPort"));
    ASSERT_EQ(config.getPropertyValue("WebsocketStreamingPort"), 7414);

    ASSERT_TRUE(config.hasProperty("WebsocketControlPort"));
    ASSERT_EQ(config.getPropertyValue("WebsocketControlPort"), 7438);
}

TEST_F(NewWebsocketStreamingServerModuleTest, CreateServer)
{
    auto device = CreateTestInstance();
    auto module = CreateModule(device.getContext());
    auto config = CreateServerConfig(device);

    ASSERT_NO_THROW(module.createServer("OpenDAQNewLTStreaming", device.getRootDevice(), config));
}

TEST_F(NewWebsocketStreamingServerModuleTest, CreateServerFromInstance)
{
    auto device = CreateTestInstance();
    auto config = CreateServerConfig(device);

    ASSERT_NO_THROW(device.addServer("OpenDAQNewLTStreaming", config));
}
