#include <testutils/testutils.h>
#include <websocket_streaming_client_module/module_dll.h>
#include <websocket_streaming_client_module/version.h>
#include <gmock/gmock.h>

#include <opendaq/module_ptr.h>
#include <coretypes/common.h>

#include <opendaq/context_factory.h>
#include <coreobjects/property_factory.h>

using WebsocketStreamingClientModuleTest = testing::Test;
using namespace daq;

static ModulePtr CreateModule()
{
    ModulePtr module;
    createModule(&module, NullContext());
    return module;
}

TEST_F(WebsocketStreamingClientModuleTest, CreateModule)
{
    IModule* module = nullptr;
    ErrCode errCode = createModule(&module, NullContext());
    ASSERT_TRUE(OPENDAQ_SUCCEEDED(errCode));

    ASSERT_NE(module, nullptr);
    module->releaseRef();
}

TEST_F(WebsocketStreamingClientModuleTest, ModuleName)
{
    auto module = CreateModule();
    ASSERT_EQ(module.getName(), "openDAQ websocket client module");
}

TEST_F(WebsocketStreamingClientModuleTest, VersionAvailable)
{
    auto module = CreateModule();
    ASSERT_TRUE(module.getVersionInfo().assigned());
}

TEST_F(WebsocketStreamingClientModuleTest, VersionCorrect)
{
    auto module = CreateModule();
    auto version = module.getVersionInfo();

    ASSERT_EQ(version.getMajor(), WS_STREAM_CL_MODULE_MAJOR_VERSION);
    ASSERT_EQ(version.getMinor(), WS_STREAM_CL_MODULE_MINOR_VERSION);
    ASSERT_EQ(version.getPatch(), WS_STREAM_CL_MODULE_PATCH_VERSION);
}

TEST_F(WebsocketStreamingClientModuleTest, EnumerateDevices)
{
    auto module = CreateModule();

    ListPtr<IDeviceInfo> deviceInfo;
    ASSERT_NO_THROW(deviceInfo = module.getAvailableDevices());
}

TEST_F(WebsocketStreamingClientModuleTest, AcceptsConnectionStringNull)
{
    auto module = CreateModule();
    ASSERT_THROW(module.acceptsConnectionParameters(nullptr), ArgumentNullException);
}

TEST_F(WebsocketStreamingClientModuleTest, AcceptsConnectionStringEmpty)
{
    auto module = CreateModule();

    bool accepts = true;
    ASSERT_NO_THROW(accepts = module.acceptsConnectionParameters(""));
    ASSERT_FALSE(accepts);
}

TEST_F(WebsocketStreamingClientModuleTest, AcceptsConnectionStringInvalid)
{
    auto module = CreateModule();

    bool accepts = true;
    ASSERT_NO_THROW(accepts = module.acceptsConnectionParameters("drfrfgt"));
    ASSERT_FALSE(accepts);
}

TEST_F(WebsocketStreamingClientModuleTest, AcceptsConnectionStringWrongPrefix)
{
    auto module = CreateModule();

    bool accepts = true;
    ASSERT_NO_THROW(accepts = module.acceptsConnectionParameters("daq.opcua://device8"));
    ASSERT_FALSE(accepts);
}

TEST_F(WebsocketStreamingClientModuleTest, AcceptsConnectionStringCorrect)
{
    auto module = CreateModule();

    ASSERT_TRUE(module.acceptsConnectionParameters("daq.ws://device8"));
}

TEST_F(WebsocketStreamingClientModuleTest, CreateDeviceConnectionStringNull)
{
    auto module = CreateModule();

    DevicePtr device;
    ASSERT_THROW(device = module.createDevice(nullptr, nullptr), ArgumentNullException);
}

TEST_F(WebsocketStreamingClientModuleTest, CreateDeviceConnectionStringEmpty)
{
    auto module = CreateModule();

    ASSERT_THROW(module.createDevice("", nullptr), InvalidParameterException);
}

TEST_F(WebsocketStreamingClientModuleTest, CreateDeviceConnectionStringInvalid)
{
    auto module = CreateModule();

    ASSERT_THROW(module.createDevice("fdfdfdfdde", nullptr), InvalidParameterException);
}

TEST_F(WebsocketStreamingClientModuleTest, CreateDeviceConnectionStringInvalidId)
{
    auto module = CreateModule();

    ASSERT_THROW(module.createDevice("daqref://devicett3axxr1", nullptr), InvalidParameterException);
    ASSERT_THROW(module.createDevice("daq.opcua://devicett3axxr1", nullptr), InvalidParameterException);
}

TEST_F(WebsocketStreamingClientModuleTest, CreateDeviceConnectionFailed)
{
    auto module = CreateModule();

    ASSERT_THROW(module.createDevice("daq.ws://127.0.0.1", nullptr), NotFoundException);
}


TEST_F(WebsocketStreamingClientModuleTest, AcceptsStreamingConnectionStringNull)
{
    auto module = CreateModule();
    ASSERT_THROW(module.acceptsStreamingConnectionParameters(nullptr), ArgumentNullException);
}

TEST_F(WebsocketStreamingClientModuleTest, AcceptsStreamingConnectionStringEmpty)
{
    auto module = CreateModule();

    bool accepts = true;
    ASSERT_NO_THROW(accepts = module.acceptsStreamingConnectionParameters(""));
    ASSERT_FALSE(accepts);
}

TEST_F(WebsocketStreamingClientModuleTest, AcceptsStreamingConnectionStringInvalid)
{
    auto module = CreateModule();

    bool accepts = true;
    ASSERT_NO_THROW(accepts = module.acceptsStreamingConnectionParameters("drfrfgt"));
    ASSERT_FALSE(accepts);
}

TEST_F(WebsocketStreamingClientModuleTest, AcceptsStreamingConnectionStringWrongPrefix)
{
    auto module = CreateModule();

    bool accepts = true;
    ASSERT_NO_THROW(accepts = module.acceptsStreamingConnectionParameters("daq.opcua://device8"));
    ASSERT_FALSE(accepts);
}

TEST_F(WebsocketStreamingClientModuleTest, AcceptsStreamingConnectionStringCorrect)
{
    auto module = CreateModule();

    ASSERT_TRUE(module.acceptsStreamingConnectionParameters("daq.wss://device8"));
}


TEST_F(WebsocketStreamingClientModuleTest, AcceptsStreamingConfig)
{
    auto module = CreateModule();

    StreamingInfoConfigPtr streamingInfoConfig = StreamingInfo("daq.wss");
    ASSERT_FALSE(module.acceptsStreamingConnectionParameters(nullptr, streamingInfoConfig));

    streamingInfoConfig.setPrimaryAddress("123.123.123.123");
    ASSERT_FALSE(module.acceptsStreamingConnectionParameters(nullptr, streamingInfoConfig));

    streamingInfoConfig.addProperty(IntProperty("Port", 1234));
    ASSERT_TRUE(module.acceptsStreamingConnectionParameters(nullptr, streamingInfoConfig));
}

TEST_F(WebsocketStreamingClientModuleTest, CreateStreamingWithNullArguments)
{
    auto module = CreateModule();

    DevicePtr device;
    ASSERT_THROW(device = module.createStreaming(nullptr, nullptr), ArgumentNullException);
}

TEST_F(WebsocketStreamingClientModuleTest, CreateStreamingWithConnectionStringEmpty)
{
    auto module = CreateModule();

    ASSERT_THROW(module.createStreaming("", nullptr), InvalidParameterException);
}

TEST_F(WebsocketStreamingClientModuleTest, CreateStreamingConnectionStringInvalid)
{
    auto module = CreateModule();

    ASSERT_THROW(module.createStreaming("fdfdfdfdde", nullptr), InvalidParameterException);
}

TEST_F(WebsocketStreamingClientModuleTest, CreateStreamingConnectionStringInvalidId)
{
    auto module = CreateModule();

    ASSERT_THROW(module.createStreaming("daqref://devicett3axxr1", nullptr), InvalidParameterException);
    ASSERT_THROW(module.createStreaming("daq.opcua://devicett3axxr1", nullptr), InvalidParameterException);
    ASSERT_THROW(module.createStreaming("daq.ws://devicett3axxr1", nullptr), InvalidParameterException);
}

TEST_F(WebsocketStreamingClientModuleTest, GetAvailableComponentTypes)
{
    const auto module = CreateModule();

    DictPtr<IString, IFunctionBlockType> functionBlockTypes;
    ASSERT_NO_THROW(functionBlockTypes = module.getAvailableFunctionBlockTypes());
    ASSERT_EQ(functionBlockTypes.getCount(), 0u);

    DictPtr<IString, IDeviceType> deviceTypes;
    ASSERT_NO_THROW(deviceTypes = module.getAvailableDeviceTypes());
    ASSERT_EQ(deviceTypes.getCount(), 2u);
    ASSERT_TRUE(deviceTypes.hasKey("daq.ws"));
    ASSERT_EQ(deviceTypes.get("daq.ws").getId(), "daq.ws");
    ASSERT_TRUE(deviceTypes.hasKey("daq.tcp"));
    ASSERT_EQ(deviceTypes.get("daq.tcp").getId(), "daq.tcp");

    DictPtr<IString, IServerType> serverTypes;
    ASSERT_NO_THROW(serverTypes = module.getAvailableServerTypes());
    ASSERT_EQ(serverTypes.getCount(), 0u);
}

TEST_F(WebsocketStreamingClientModuleTest, CreateFunctionBlockIdNull)
{
    auto module = CreateModule();

    FunctionBlockPtr functionBlock;
    ASSERT_THROW(functionBlock = module.createFunctionBlock(nullptr, nullptr, "fb"), ArgumentNullException);
}

TEST_F(WebsocketStreamingClientModuleTest, CreateFunctionBlockIdEmpty)
{
    auto module = CreateModule();

    ASSERT_THROW(module.createFunctionBlock("", nullptr, "fb"), NotFoundException);
}
