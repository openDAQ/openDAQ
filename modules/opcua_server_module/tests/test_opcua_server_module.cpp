#include <opcua_server_module/module_dll.h>
#include <opcua_server_module/version.h>
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
#include <opcuaclient/opcuaclient.h>
#include <coreobjects/authentication_provider_factory.h>


class OpcUaServerModuleTest : public testing::Test
{
public:
    void TearDown() override
    {
    }
};

using namespace daq;
using namespace daq::opcua;

static ModulePtr CreateModule(ContextPtr context = NullContext())
{
    ModulePtr module;
    createOpcUaServerModule(&module, context);
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

    const ModulePtr daqServerModule = CreateModule(context);
    moduleManager.addModule(daqServerModule);

    auto instance = InstanceCustom(context, "localInstance");
    for (const auto& deviceInfo : instance.getAvailableDevices())
        instance.addDevice(deviceInfo.getConnectionString());

    for (const auto& [id, _] : instance.getAvailableFunctionBlockTypes())
        instance.addFunctionBlock(id);

    return instance;
}

static PropertyObjectPtr CreateServerConfig(const InstancePtr& instance)
{
    auto config = instance.getAvailableServerTypes().get("openDAQ OpcUa").createDefaultConfig();
    return config;
}

TEST_F(OpcUaServerModuleTest, CreateModule)
{
    IModule* module = nullptr;
    ErrCode errCode = createModule(&module, NullContext());
    ASSERT_TRUE(OPENDAQ_SUCCEEDED(errCode));

    ASSERT_NE(module, nullptr);
    module->releaseRef();
}

TEST_F(OpcUaServerModuleTest, ModuleName)
{
    auto module = CreateModule();
    ASSERT_EQ(module.getName(), "openDAQ OpcUa server module");
}

TEST_F(OpcUaServerModuleTest, VersionAvailable)
{
    auto module = CreateModule();
    ASSERT_TRUE(module.getVersionInfo().assigned());
}

TEST_F(OpcUaServerModuleTest, VersionCorrect)
{
    auto module = CreateModule();
    auto version = module.getVersionInfo();

    ASSERT_EQ(version.getMajor(), OPCUA_SERVER_MODULE_MAJOR_VERSION);
    ASSERT_EQ(version.getMinor(), OPCUA_SERVER_MODULE_MINOR_VERSION);
    ASSERT_EQ(version.getPatch(), OPCUA_SERVER_MODULE_PATCH_VERSION);
}

TEST_F(OpcUaServerModuleTest, GetAvailableComponentTypes)
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
    ASSERT_TRUE(serverTypes.hasKey("openDAQ OpcUa"));
    ASSERT_EQ(serverTypes.get("openDAQ OpcUa").getId(), "openDAQ OpcUa");
}

TEST_F(OpcUaServerModuleTest, ServerConfig)
{
    auto module = CreateModule();

    DictPtr<IString, IServerType> serverTypes = module.getAvailableServerTypes();
    ASSERT_TRUE(serverTypes.hasKey("openDAQ OpcUa"));
    auto config = serverTypes.get("openDAQ OpcUa").createDefaultConfig();
    ASSERT_TRUE(config.assigned());

    ASSERT_TRUE(config.hasProperty("Port"));
    ASSERT_EQ(config.getPropertyValue("Port"), 4840);
}

TEST_F(OpcUaServerModuleTest, CreateServer)
{
    auto device = CreateTestInstance();
    auto module = CreateModule(device.getContext());
    auto config = CreateServerConfig(device);

    ASSERT_NO_THROW(module.createServer("openDAQ OpcUa", device.getRootDevice(), config));
}

TEST_F(OpcUaServerModuleTest, CreateServerFromInstance)
{
    auto device = CreateTestInstance();
    auto config = CreateServerConfig(device);

    ASSERT_NO_THROW(device.addServer("openDAQ OpcUa", config));
}

TEST_F(OpcUaServerModuleTest, TestConnection)
{
    auto device = CreateTestInstance();
    auto config = CreateServerConfig(device);
    device.addServer("openDAQ OpcUa", config);

    OpcUaClient client("opc.tcp://localhost/");
    ASSERT_NO_THROW(client.connect());
}

TEST_F(OpcUaServerModuleTest, TestConnectionDifferentPort)
{
    auto device = CreateTestInstance();
    auto module = CreateModule(device.getContext());
    auto config = CreateServerConfig(device);

    config.setPropertyValue("Port", 4841);

    auto serverPtr = module.createServer("openDAQ OpcUa", device.getRootDevice(), config);

    OpcUaClient client("opc.tcp://localhost:4841/");
    ASSERT_NO_THROW(client.connect());
}

TEST_F(OpcUaServerModuleTest, StopServer)
{
    auto device = CreateTestInstance();
    auto module = CreateModule(device.getContext());
    auto config = CreateServerConfig(device);

    auto serverPtr = module.createServer("openDAQ OpcUa", device.getRootDevice(), config);

    OpcUaClient client("opc.tcp://localhost/");
    ASSERT_NO_THROW(client.connect());
    client.disconnect();

    serverPtr.stop();
    ASSERT_THROW(client.connect(), OpcUaException);
}
