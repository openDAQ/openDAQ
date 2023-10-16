#include <testutils/testutils.h>
#include <audio_device_module/module_dll.h>
#include <audio_device_module/version.h>
#include <gmock/gmock.h>
#include <opendaq/module_ptr.h>
#include <opendaq/device_ptr.h>
#include <coretypes/common.h>
#include <opendaq/context_factory.h>
#include <opendaq/scheduler_factory.h>

using AudioDeviceModuleTest = testing::Test;
using namespace daq;

static ModulePtr CreateModule()
{
    ModulePtr module;
    createModule(&module, NullContext());
    return module;
}

TEST_F(AudioDeviceModuleTest, CreateModule)
{
    IModule* module = nullptr;
    ErrCode errCode = createModule(&module, NullContext());
    ASSERT_TRUE(OPENDAQ_SUCCEEDED(errCode));

    ASSERT_NE(module, nullptr);
    module->releaseRef();
}

TEST_F(AudioDeviceModuleTest, ModuleName)
{
    auto module = CreateModule();
    ASSERT_EQ(module.getName(), "Audio device module");
}

TEST_F(AudioDeviceModuleTest, VersionAvailable)
{
    auto module = CreateModule();
    ASSERT_TRUE(module.getVersionInfo().assigned());
}

TEST_F(AudioDeviceModuleTest, VersionCorrect)
{
    auto module = CreateModule();
    auto version = module.getVersionInfo();

    ASSERT_EQ(version.getMajor(), AUDIO_DEVICE_MODULE_MAJOR_VERSION);
    ASSERT_EQ(version.getMinor(), AUDIO_DEVICE_MODULE_MINOR_VERSION);
    ASSERT_EQ(version.getPatch(), AUDIO_DEVICE_MODULE_PATCH_VERSION);
}

TEST_F(AudioDeviceModuleTest, EnumerateDevices)
{
    auto module = CreateModule();

    ListPtr<IDeviceInfo> deviceInfo;
    ASSERT_NO_THROW(deviceInfo = module.getAvailableDevices());
}

TEST_F(AudioDeviceModuleTest, AcceptsConnectionStringNull)
{
    auto module = CreateModule();
    ASSERT_THROW(module.acceptsConnectionParameters(nullptr), ArgumentNullException);
}

TEST_F(AudioDeviceModuleTest, AcceptsConnectionStringEmpty)
{
    auto module = CreateModule();

    bool accepts = true;
    ASSERT_NO_THROW(accepts = module.acceptsConnectionParameters(""));
    ASSERT_FALSE(accepts);
}

TEST_F(AudioDeviceModuleTest, AcceptsConnectionStringInvalid)
{
    auto module = CreateModule();

    bool accepts = true;
    ASSERT_NO_THROW(accepts = module.acceptsConnectionParameters("drfrfgt"));
    ASSERT_FALSE(accepts);
}

TEST_F(AudioDeviceModuleTest, AcceptsConnectionStringCorrect)
{
    auto module = CreateModule();

    ASSERT_TRUE(module.acceptsConnectionParameters("miniaudio://wasapi/...."));
}

TEST_F(AudioDeviceModuleTest, CreateDeviceConnectionStringNull)
{
    auto module = CreateModule();

    DevicePtr device;
    ASSERT_THROW(device = module.createDevice(nullptr, nullptr), ArgumentNullException);
}

TEST_F(AudioDeviceModuleTest, CreateDeviceConnectionStringEmpty)
{
    auto module = CreateModule();

    ASSERT_THROW(module.createDevice("", nullptr), InvalidParameterException);
}

TEST_F(AudioDeviceModuleTest, CreateDeviceConnectionStringInvalid)
{
    auto module = CreateModule();

    ASSERT_THROW(module.createDevice("fdfdfdfdde", nullptr), InvalidParameterException);
}

TEST_F(AudioDeviceModuleTest, CreateDeviceConnectionStringInvalidId)
{
    auto module = CreateModule();

    ASSERT_THROW(module.createDevice("daqref://devicett3axxr1", nullptr), InvalidParameterException);
}

TEST_F(AudioDeviceModuleTest, CreateDeviceConnectionStringCorrect)
{
    auto module = CreateModule();

    auto deviceInfoList = module.getAvailableDevices();

    if (deviceInfoList.getCount() > 0)
    {
        auto connectionString = deviceInfoList[0].getConnectionString();
        std::cout << connectionString << std::endl;
        DevicePtr device;
        ASSERT_NO_THROW(device = module.createDevice(connectionString, nullptr));
    }
}

TEST_F(AudioDeviceModuleTest, GetAvailableComponentTypes)
{
    const auto module = CreateModule();

    DictPtr<IString, IFunctionBlockType> functionBlockTypes;
    ASSERT_NO_THROW(functionBlockTypes = module.getAvailableFunctionBlockTypes());
    ASSERT_EQ(functionBlockTypes.getCount(), 1u);
    ASSERT_TRUE(functionBlockTypes.hasKey("audio_device_module_wav_writer"));
    ASSERT_EQ(functionBlockTypes.get("audio_device_module_wav_writer").getId(), "audio_device_module_wav_writer");

    DictPtr<IString, IDeviceType> deviceTypes;
    ASSERT_NO_THROW(deviceTypes = module.getAvailableDeviceTypes());
    ASSERT_EQ(deviceTypes.getCount(), 1u);
    ASSERT_TRUE(deviceTypes.hasKey("miniaudio"));
    ASSERT_EQ(deviceTypes.get("miniaudio").getId(), "miniaudio");

    DictPtr<IString, IServerType> serverTypes;
    ASSERT_NO_THROW(serverTypes = module.getAvailableServerTypes());
    ASSERT_EQ(serverTypes.getCount(), 0u);
}
