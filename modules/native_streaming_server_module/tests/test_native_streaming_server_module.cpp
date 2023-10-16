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
    const auto context = Context(Scheduler(logger), logger, TypeManager(), moduleManager);

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
    auto config = instance.getAvailableServerTypes().get("openDAQ Native Streaming").createDefaultConfig();
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
    ASSERT_EQ(module.getName(), "openDAQ native streaming server module");
}

TEST_F(NativeStreamingServerModuleTest, VersionAvailable)
{
    auto module = CreateModule();
    ASSERT_TRUE(module.getVersionInfo().assigned());
}

TEST_F(NativeStreamingServerModuleTest, VersionCorrect)
{
    auto module = CreateModule();
    auto version = module.getVersionInfo();

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
    ASSERT_TRUE(serverTypes.hasKey("openDAQ Native Streaming"));
    ASSERT_EQ(serverTypes.get("openDAQ Native Streaming").getId(), "openDAQ Native Streaming");
}

TEST_F(NativeStreamingServerModuleTest, ServerConfig)
{
    auto module = CreateModule();

    DictPtr<IString, IServerType> serverTypes = module.getAvailableServerTypes();
    ASSERT_TRUE(serverTypes.hasKey("openDAQ Native Streaming"));
    auto config = serverTypes.get("openDAQ Native Streaming").createDefaultConfig();
    ASSERT_TRUE(config.assigned());

    ASSERT_TRUE(config.hasProperty("NativeStreamingPort"));
    ASSERT_EQ(config.getPropertyValue("NativeStreamingPort"), 7420);
}

TEST_F(NativeStreamingServerModuleTest, CreateServer)
{
    auto device = CreateTestInstance();
    auto module = CreateModule(device.getContext());
    auto config = CreateServerConfig(device);

    ASSERT_NO_THROW(module.createServer("openDAQ Native Streaming", device.getRootDevice(), config));
}

TEST_F(NativeStreamingServerModuleTest, CreateServerFromInstance)
{
    auto device = CreateTestInstance();
    auto config = CreateServerConfig(device);

    ASSERT_NO_THROW(device.addServer("openDAQ Native Streaming", config));
}
