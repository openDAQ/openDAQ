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
    ASSERT_EQ(module.getModuleInfo().getName(), "AudioDeviceModule");
}

TEST_F(AudioDeviceModuleTest, VersionAvailable)
{
    auto module = CreateModule();
    ASSERT_TRUE(module.getModuleInfo().getVersionInfo().assigned());
}

TEST_F(AudioDeviceModuleTest, VersionCorrect)
{
    auto module = CreateModule();
    auto version = module.getModuleInfo().getVersionInfo();

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
    ASSERT_TRUE(functionBlockTypes.hasKey("AudioDeviceModuleWavWriter"));
    ASSERT_EQ(functionBlockTypes.get("AudioDeviceModuleWavWriter").getId(), "AudioDeviceModuleWavWriter");

    DictPtr<IString, IDeviceType> deviceTypes;
    ASSERT_NO_THROW(deviceTypes = module.getAvailableDeviceTypes());
    ASSERT_EQ(deviceTypes.getCount(), 1u);
    ASSERT_TRUE(deviceTypes.hasKey("MiniAudio"));
    ASSERT_EQ(deviceTypes.get("MiniAudio").getId(), "MiniAudio");

    DictPtr<IString, IServerType> serverTypes;
    ASSERT_NO_THROW(serverTypes = module.getAvailableServerTypes());
    ASSERT_EQ(serverTypes.getCount(), 0u);

    // Check module info for module
    ModuleInfoPtr moduleInfo;
    ASSERT_NO_THROW(moduleInfo = module.getModuleInfo());
    ASSERT_NE(moduleInfo, nullptr);
    ASSERT_EQ(moduleInfo.getName(), "AudioDeviceModule");
    ASSERT_EQ(moduleInfo.getId(), "AudioDeviceModule");

    // Check version info for module
    VersionInfoPtr versionInfoModule;
    ASSERT_NO_THROW(versionInfoModule = moduleInfo.getVersionInfo());
    ASSERT_NE(versionInfoModule, nullptr);
    ASSERT_EQ(versionInfoModule.getMajor(), AUDIO_DEVICE_MODULE_MAJOR_VERSION);
    ASSERT_EQ(versionInfoModule.getMinor(), AUDIO_DEVICE_MODULE_MINOR_VERSION);
    ASSERT_EQ(versionInfoModule.getPatch(), AUDIO_DEVICE_MODULE_PATCH_VERSION);

    // Check module and version info for function block types
    for (const auto& functionBlockType : functionBlockTypes)
    {
        ModuleInfoPtr moduleInfoFunctionBlockType;
        ASSERT_NO_THROW(moduleInfoFunctionBlockType = functionBlockType.second.getModuleInfo());
        ASSERT_NE(moduleInfoFunctionBlockType, nullptr);
        ASSERT_EQ(moduleInfoFunctionBlockType.getName(), "AudioDeviceModule");
        ASSERT_EQ(moduleInfoFunctionBlockType.getId(), "AudioDeviceModule");

        VersionInfoPtr versionInfoFunctionBlockType;
        ASSERT_NO_THROW(versionInfoFunctionBlockType = moduleInfoFunctionBlockType.getVersionInfo());
        ASSERT_NE(versionInfoFunctionBlockType, nullptr);
        ASSERT_EQ(versionInfoFunctionBlockType.getMajor(), AUDIO_DEVICE_MODULE_MAJOR_VERSION);
        ASSERT_EQ(versionInfoFunctionBlockType.getMinor(), AUDIO_DEVICE_MODULE_MINOR_VERSION);
        ASSERT_EQ(versionInfoFunctionBlockType.getPatch(), AUDIO_DEVICE_MODULE_PATCH_VERSION);
    }

    // Check module and version info for device types
    for (const auto& deviceType : deviceTypes)
    {
        ModuleInfoPtr moduleInfoDeviceType;
        ASSERT_NO_THROW(moduleInfoDeviceType = deviceType.second.getModuleInfo());
        ASSERT_NE(moduleInfoDeviceType, nullptr);
        ASSERT_EQ(moduleInfoDeviceType.getName(), "AudioDeviceModule");
        ASSERT_EQ(moduleInfoDeviceType.getId(), "AudioDeviceModule");

        VersionInfoPtr versionInfoDeviceType;
        ASSERT_NO_THROW(versionInfoDeviceType = moduleInfoDeviceType.getVersionInfo());
        ASSERT_NE(versionInfoDeviceType, nullptr);
        ASSERT_EQ(versionInfoDeviceType.getMajor(), AUDIO_DEVICE_MODULE_MAJOR_VERSION);
        ASSERT_EQ(versionInfoDeviceType.getMinor(), AUDIO_DEVICE_MODULE_MINOR_VERSION);
        ASSERT_EQ(versionInfoDeviceType.getPatch(), AUDIO_DEVICE_MODULE_PATCH_VERSION);
    }
}
