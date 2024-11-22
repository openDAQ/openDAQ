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
}

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
