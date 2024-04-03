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
    ASSERT_EQ(module.getName(), "openDAQ native streaming client module");
}

TEST_F(NativeStreamingClientModuleTest, VersionAvailable)
{
    auto module = CreateModule();
    ASSERT_TRUE(module.getVersionInfo().assigned());
}

TEST_F(NativeStreamingClientModuleTest, VersionCorrect)
{
    auto module = CreateModule();
    auto version = module.getVersionInfo();

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

TEST_F(NativeStreamingClientModuleTest, AcceptsConnectionStringNull)
{
    auto module = CreateModule();
    ASSERT_THROW(module.acceptsConnectionParameters(nullptr), ArgumentNullException);
}

TEST_F(NativeStreamingClientModuleTest, AcceptsConnectionStringCorrect)
{
    auto module = CreateModule();

    ASSERT_TRUE(module.acceptsConnectionParameters("daq.ns://device8"));
    ASSERT_TRUE(module.acceptsConnectionParameters("daq.nd://device8"));
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

TEST_F(NativeStreamingClientModuleTest, AcceptsStreamingConnectionStringNull)
{
    auto module = CreateModule();
    ASSERT_THROW(module.acceptsStreamingConnectionParameters(nullptr), ArgumentNullException);
}

TEST_F(NativeStreamingClientModuleTest, AcceptsStreamingConnectionStringCorrect)
{
    auto module = CreateModule();

    ASSERT_TRUE(module.acceptsStreamingConnectionParameters("daq.ns://host"));
}

TEST_F(NativeStreamingClientModuleTest, CreateStreamingWithNullArguments)
{
    auto module = CreateModule();

    DevicePtr device;
    ASSERT_THROW(device = module.createStreaming(nullptr, nullptr), ArgumentNullException);
}

TEST_F(NativeStreamingClientModuleTest, AcceptsStreamingConfig)
{
    auto context = NullContext();
    ModulePtr module;
    createModule(&module, context);
 
    ServerCapabilityPtr serverCapability = ServerCapability("openDAQ Native Streaming", ProtocolType::Streaming);
    serverCapability.setPropertyValue("protocolId", "daq.ns");
    ASSERT_FALSE(module.acceptsStreamingConnectionParameters(nullptr, serverCapability));

    serverCapability.setPropertyValue("address", "123.123.123.123");
    ASSERT_FALSE(module.acceptsStreamingConnectionParameters(nullptr, serverCapability));

    serverCapability.addProperty(IntProperty("Port", 1234));
    ASSERT_TRUE(module.acceptsStreamingConnectionParameters(nullptr, serverCapability));
}

TEST_F(NativeStreamingClientModuleTest, GetAvailableComponentTypes)
{
    const auto module = CreateModule();

    DictPtr<IString, IFunctionBlockType> functionBlockTypes;
    ASSERT_NO_THROW(functionBlockTypes = module.getAvailableFunctionBlockTypes());
    ASSERT_EQ(functionBlockTypes.getCount(), 0u);

    DictPtr<IString, IDeviceType> deviceTypes;
    ASSERT_NO_THROW(deviceTypes = module.getAvailableDeviceTypes());
    ASSERT_EQ(deviceTypes.getCount(), 2u);
    ASSERT_TRUE(deviceTypes.hasKey("daq.ns"));
    ASSERT_EQ(deviceTypes.get("daq.ns").getId(), "daq.ns");
    ASSERT_TRUE(deviceTypes.hasKey("daq.nd"));
    ASSERT_EQ(deviceTypes.get("daq.nd").getId(), "daq.nd");

    DictPtr<IString, IServerType> serverTypes;
    ASSERT_NO_THROW(serverTypes = module.getAvailableServerTypes());
    ASSERT_EQ(serverTypes.getCount(), 0u);
}

TEST_F(NativeStreamingClientModuleTest, DefaultDeviceConfig)
{
    const auto module = CreateModule();

    DictPtr<IString, IDeviceType> deviceTypes;
    ASSERT_NO_THROW(deviceTypes = module.getAvailableDeviceTypes());
    ASSERT_EQ(deviceTypes.getCount(), 2u);

    ASSERT_TRUE(deviceTypes.hasKey("daq.nd"));
    auto deviceConfig = deviceTypes.get("daq.nd").createDefaultConfig();
    ASSERT_TRUE(deviceConfig.assigned());
    ASSERT_TRUE(module.acceptsConnectionParameters("daq.nd://address", deviceConfig));

    ASSERT_TRUE(deviceTypes.hasKey("daq.ns"));
    auto pseudoDeviceConfig = deviceTypes.get("daq.ns").createDefaultConfig();
    ASSERT_TRUE(pseudoDeviceConfig.assigned());
    ASSERT_TRUE(module.acceptsConnectionParameters("daq.ns://address", pseudoDeviceConfig));
}

TEST_F(NativeStreamingClientModuleTest, InvalidDeviceConfig)
{
    auto module = CreateModule();
    auto config = PropertyObject();

    ASSERT_FALSE(module.acceptsConnectionParameters("daq.nd://device8", config));
    ASSERT_FALSE(module.acceptsConnectionParameters("daq.ns://device8", config));
}

class ConnectionStringTest : public NativeStreamingClientModuleTest,
                             public testing::WithParamInterface<StringPtr>
{
};

TEST_P(ConnectionStringTest, ConnectionStringNotAccepted)
{
    auto module = CreateModule();

    StringPtr connectionString = GetParam();

    bool accepts = true;
    ASSERT_NO_THROW(accepts = module.acceptsConnectionParameters(connectionString));
    ASSERT_FALSE(accepts);
}

TEST_P(ConnectionStringTest, CreateDeviceWrongConnectionString)
{
    auto module = CreateModule();

    StringPtr connectionString = GetParam();

    ASSERT_THROW(module.createDevice(connectionString, nullptr), InvalidParameterException);
}

TEST_P(ConnectionStringTest, StreamingConnectionStringNotAccepted)
{
    auto module = CreateModule();

    StringPtr connectionString = GetParam();

    bool accepts = true;
    ASSERT_NO_THROW(accepts = module.acceptsStreamingConnectionParameters(connectionString));
    ASSERT_FALSE(accepts);
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
        "daq.ns:///"
    )
);
