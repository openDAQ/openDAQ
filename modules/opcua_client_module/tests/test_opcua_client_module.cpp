#include <testutils/testutils.h>
#include <opcua_client_module/module_dll.h>
#include <opcua_client_module/version.h>
#include <gmock/gmock.h>

#include <opendaq/module_ptr.h>
#include <coretypes/common.h>
#include <coreobjects/property_object_factory.h>

#include <opendaq/context_factory.h>

using OpcUaClientModuleTest = testing::Test;
using namespace daq;

static ModulePtr CreateModule()
{
    ModulePtr module;
    createModule(&module, NullContext());
    return module;
}

TEST_F(OpcUaClientModuleTest, CreateModule)
{
    IModule* module = nullptr;
    ErrCode errCode = createModule(&module, NullContext());
    ASSERT_TRUE(OPENDAQ_SUCCEEDED(errCode));

    ASSERT_NE(module, nullptr);
    module->releaseRef();
}

TEST_F(OpcUaClientModuleTest, ModuleName)
{
    auto module = CreateModule();
    ASSERT_EQ(module.getName(), "openDAQ OpcUa client module");
}

TEST_F(OpcUaClientModuleTest, VersionAvailable)
{
    auto module = CreateModule();
    ASSERT_TRUE(module.getVersionInfo().assigned());
}

TEST_F(OpcUaClientModuleTest, VersionCorrect)
{
    auto module = CreateModule();
    auto version = module.getVersionInfo();

    ASSERT_EQ(version.getMajor(), OPCUA_CLIENT_MODULE_MAJOR_VERSION);
    ASSERT_EQ(version.getMinor(), OPCUA_CLIENT_MODULE_MINOR_VERSION);
    ASSERT_EQ(version.getPatch(), OPCUA_CLIENT_MODULE_PATCH_VERSION);
}

TEST_F(OpcUaClientModuleTest, EnumerateDevices)
{
    auto module = CreateModule();

    ListPtr<IDeviceInfo> deviceInfo;
    ASSERT_NO_THROW(deviceInfo = module.getAvailableDevices());
}

TEST_F(OpcUaClientModuleTest, AcceptsConnectionStringNull)
{
    auto module = CreateModule();
    ASSERT_THROW(module.acceptsConnectionParameters(nullptr), ArgumentNullException);
}

TEST_F(OpcUaClientModuleTest, AcceptsConnectionStringEmpty)
{
    auto module = CreateModule();

    bool accepts = true;
    ASSERT_NO_THROW(accepts = module.acceptsConnectionParameters(""));
    ASSERT_FALSE(accepts);
}

TEST_F(OpcUaClientModuleTest, AcceptsConnectionStringInvalid)
{
    auto module = CreateModule();

    bool accepts = true;
    ASSERT_NO_THROW(accepts = module.acceptsConnectionParameters("drfrfgt"));
    ASSERT_FALSE(accepts);
}

TEST_F(OpcUaClientModuleTest, AcceptsConnectionStringCorrect)
{
    auto module = CreateModule();

    ASSERT_TRUE(module.acceptsConnectionParameters("daq.opcua://device8"));
}

TEST_F(OpcUaClientModuleTest, CreateDeviceConnectionStringNull)
{
    auto module = CreateModule();

    DevicePtr device;
    ASSERT_THROW(device = module.createDevice(nullptr, nullptr), ArgumentNullException);
}

TEST_F(OpcUaClientModuleTest, CreateDeviceConnectionStringEmpty)
{
    auto module = CreateModule();

    ASSERT_THROW(module.createDevice("", nullptr), InvalidParameterException);
}

TEST_F(OpcUaClientModuleTest, CreateDeviceConnectionStringInvalid)
{
    auto module = CreateModule();

    ASSERT_THROW(module.createDevice("fdfdfdfdde", nullptr), InvalidParameterException);
}

TEST_F(OpcUaClientModuleTest, CreateDeviceConnectionStringInvalidId)
{
    auto module = CreateModule();

    ASSERT_THROW(module.createDevice("daqref://devicett3axxr1", nullptr), InvalidParameterException);
}

TEST_F(OpcUaClientModuleTest, CreateDeviceConfigInvalid)
{
    auto module = CreateModule();
    auto config = PropertyObject();

    ASSERT_THROW(module.createDevice("daq.opcua://device8", nullptr, config), InvalidParameterException);
}

TEST_F(OpcUaClientModuleTest, GetAvailableComponentTypes)
{
    const auto module = CreateModule();

    DictPtr<IString, IFunctionBlockType> functionBlockTypes;
    ASSERT_NO_THROW(functionBlockTypes = module.getAvailableFunctionBlockTypes());
    ASSERT_EQ(functionBlockTypes.getCount(), 0u);

    DictPtr<IString, IDeviceType> deviceTypes;
    ASSERT_NO_THROW(deviceTypes = module.getAvailableDeviceTypes());
    ASSERT_EQ(deviceTypes.getCount(), 1u);
    ASSERT_TRUE(deviceTypes.hasKey("daq.opcua"));
    ASSERT_EQ(deviceTypes.get("daq.opcua").getId(), "daq.opcua");

    DictPtr<IString, IServerType> serverTypes;
    ASSERT_NO_THROW(serverTypes = module.getAvailableServerTypes());
    ASSERT_EQ(serverTypes.getCount(), 0u);
}

TEST_F(OpcUaClientModuleTest, DefaultDeviceConfig)
{
    const auto module = CreateModule();

    DictPtr<IString, IDeviceType> deviceTypes;
    ASSERT_NO_THROW(deviceTypes = module.getAvailableDeviceTypes());
    ASSERT_EQ(deviceTypes.getCount(), 1u);
    ASSERT_TRUE(deviceTypes.hasKey("daq.opcua"));
    auto config = deviceTypes.get("daq.opcua").createDefaultConfig();
    ASSERT_TRUE(config.assigned());

    ASSERT_TRUE(config.hasProperty("StreamingConnectionHeuristic"));
    ASSERT_EQ(config.getPropertySelectionValue("StreamingConnectionHeuristic"), "MinConnections");

#if defined(OPENDAQ_ENABLE_NATIVE_STREAMING)
    ASSERT_TRUE(config.hasProperty("AllowedStreamingProtocols"));
    ASSERT_EQ(config.getPropertyValue("AllowedStreamingProtocols"), List<IString>("daq.ns"));

    ASSERT_TRUE(config.hasProperty("PrimaryStreamingProtocol"));
    ASSERT_EQ(config.getPropertyValue("PrimaryStreamingProtocol"), "daq.ns");
#elif defined(OPENDAQ_ENABLE_WEBSOCKET_STREAMING) && !defined(OPENDAQ_ENABLE_NATIVE_STREAMING)
    ASSERT_TRUE(config.hasProperty("AllowedStreamingProtocols"));
    ASSERT_EQ(config.getPropertyValue("AllowedStreamingProtocols"), List<IString>("daq.wss"));

    ASSERT_TRUE(config.hasProperty("PrimaryStreamingProtocol"));
    ASSERT_EQ(config.getPropertyValue("PrimaryStreamingProtocol"), "daq.wss");
#endif
}

TEST_F(OpcUaClientModuleTest, InvalidDeviceConfig)
{
    auto module = CreateModule();
    auto config = PropertyObject();

    ASSERT_FALSE(module.acceptsConnectionParameters("daq.opcua://device8", config));
}

TEST_F(OpcUaClientModuleTest, CreateFunctionBlockIdNull)
{
    auto module = CreateModule();

    FunctionBlockPtr functionBlock;
    ASSERT_THROW(functionBlock = module.createFunctionBlock(nullptr, nullptr, "fb"), ArgumentNullException);
}

TEST_F(OpcUaClientModuleTest, CreateFunctionBlockIdEmpty)
{
    auto module = CreateModule();

    ASSERT_THROW(module.createFunctionBlock("", nullptr, "fb"), NotFoundException);
}
