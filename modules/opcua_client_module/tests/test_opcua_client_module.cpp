#include <testutils/testutils.h>
#include <opcua_client_module/module_dll.h>
#include <opcua_client_module/version.h>
#include <gmock/gmock.h>

#include <opendaq/module_ptr.h>
#include <coretypes/common.h>
#include <coreobjects/property_factory.h>
#include <coreobjects/property_object_factory.h>
#include <opendaq/device_info_factory.h>

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
    ASSERT_EQ(module.getModuleInfo().getName(), "OpenDAQOPCUAClientModule");
}

TEST_F(OpcUaClientModuleTest, VersionAvailable)
{
    auto module = CreateModule();
    ASSERT_TRUE(module.getModuleInfo().getVersionInfo().assigned());
}

TEST_F(OpcUaClientModuleTest, VersionCorrect)
{
    auto module = CreateModule();
    auto version = module.getModuleInfo().getVersionInfo();

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

//TEST_F(OpcUaClientModuleTest, CreateConnectionString)
//{
//    auto context = NullContext();
//    ModulePtr module;
//    createModule(&module, context);
//
//    StringPtr connectionString;
//
//    ServerCapabilityConfigPtr serverCapabilityIgnored = ServerCapability("test", "test", ProtocolType::Unknown);
//    ASSERT_NO_THROW(connectionString = module.createConnectionString(serverCapabilityIgnored));
//    ASSERT_FALSE(connectionString.assigned());
//
//    ServerCapabilityConfigPtr serverCapability = ServerCapability("OpenDAQOPCUAConfiguration", "OpenDAQOPCUA", ProtocolType::Configuration);
//    ASSERT_THROW(module.createConnectionString(serverCapability), InvalidParameterException);
//
//    serverCapability.addAddress("123.123.123.123");
//    ASSERT_EQ(module.createConnectionString(serverCapability), "daq.opcua://123.123.123.123:4840");
//
//    serverCapability.setPort(1234);
//    ASSERT_NO_THROW(connectionString = module.createConnectionString(serverCapability));
//    ASSERT_EQ(connectionString, "daq.opcua://123.123.123.123:1234");
//}

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

TEST_F(OpcUaClientModuleTest, GetAvailableComponentTypes)
{
    const auto module = CreateModule();

    DictPtr<IString, IFunctionBlockType> functionBlockTypes;
    ASSERT_NO_THROW(functionBlockTypes = module.getAvailableFunctionBlockTypes());
    ASSERT_EQ(functionBlockTypes.getCount(), 0u);

    DictPtr<IString, IDeviceType> deviceTypes;
    ASSERT_NO_THROW(deviceTypes = module.getAvailableDeviceTypes());
    ASSERT_EQ(deviceTypes.getCount(), 1u);
    ASSERT_TRUE(deviceTypes.hasKey("OpenDAQOPCUAConfiguration"));
    ASSERT_EQ(deviceTypes.get("OpenDAQOPCUAConfiguration").getId(), "OpenDAQOPCUAConfiguration");

    DictPtr<IString, IServerType> serverTypes;
    ASSERT_NO_THROW(serverTypes = module.getAvailableServerTypes());
    ASSERT_EQ(serverTypes.getCount(), 0u);

    // Check module info for module
    ModuleInfoPtr moduleInfo;
    ASSERT_NO_THROW(moduleInfo = module.getModuleInfo());
    ASSERT_NE(moduleInfo, nullptr);
    ASSERT_EQ(moduleInfo.getName(), "OpenDAQOPCUAClientModule");
    ASSERT_EQ(moduleInfo.getId(), "OpenDAQOPCUAClientModule");

    // Check version info for module
    VersionInfoPtr versionInfoModule;
    ASSERT_NO_THROW(versionInfoModule = moduleInfo.getVersionInfo());
    ASSERT_NE(versionInfoModule, nullptr);
    ASSERT_EQ(versionInfoModule.getMajor(), OPCUA_CLIENT_MODULE_MAJOR_VERSION);
    ASSERT_EQ(versionInfoModule.getMinor(), OPCUA_CLIENT_MODULE_MINOR_VERSION);
    ASSERT_EQ(versionInfoModule.getPatch(), OPCUA_CLIENT_MODULE_PATCH_VERSION);

    // Check module and version info for device types
    for (const auto& deviceType : deviceTypes)
    {
        ModuleInfoPtr moduleInfoDeviceType;
        ASSERT_NO_THROW(moduleInfoDeviceType = deviceType.second.getModuleInfo());
        ASSERT_NE(moduleInfoDeviceType, nullptr);
        ASSERT_EQ(moduleInfoDeviceType.getName(), "OpenDAQOPCUAClientModule");
        ASSERT_EQ(moduleInfoDeviceType.getId(), "OpenDAQOPCUAClientModule");

        VersionInfoPtr versionInfoDeviceType;
        ASSERT_NO_THROW(versionInfoDeviceType = moduleInfoDeviceType.getVersionInfo());
        ASSERT_NE(versionInfoDeviceType, nullptr);
        ASSERT_EQ(versionInfoDeviceType.getMajor(), OPCUA_CLIENT_MODULE_MAJOR_VERSION);
        ASSERT_EQ(versionInfoDeviceType.getMinor(), OPCUA_CLIENT_MODULE_MINOR_VERSION);
        ASSERT_EQ(versionInfoDeviceType.getPatch(), OPCUA_CLIENT_MODULE_PATCH_VERSION);
    }
}

TEST_F(OpcUaClientModuleTest, DefaultDeviceConfig)
{
    const auto module = CreateModule();

    DictPtr<IString, IDeviceType> deviceTypes;
    ASSERT_NO_THROW(deviceTypes = module.getAvailableDeviceTypes());
    ASSERT_EQ(deviceTypes.getCount(), 1u);
    ASSERT_TRUE(deviceTypes.hasKey("OpenDAQOPCUAConfiguration"));
    auto config = deviceTypes.get("OpenDAQOPCUAConfiguration").createDefaultConfig();
    ASSERT_TRUE(config.assigned());
    ASSERT_EQ(config.getAllProperties().getCount(), 3u);
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
