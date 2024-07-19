#include <testutils/testutils.h>
#include <empty_module/module_dll.h>
#include <empty_module/version.h>

#include <opendaq/module_ptr.h>
#include <coretypes/common.h>

#include <opendaq/context_factory.h>

using EmptyModuleTest = testing::Test;
using namespace daq;

static ModulePtr createModule()
{
    ModulePtr module;
    createModule(&module, NullContext());
    return module;
}

TEST_F(EmptyModuleTest, CreateModule)
{
    IModule* module = nullptr;
    ErrCode errCode = createModule(&module, NullContext());
    ASSERT_TRUE(OPENDAQ_SUCCEEDED(errCode));

    ASSERT_NE(module, nullptr);
    module->releaseRef();
}

TEST_F(EmptyModuleTest, ModuleName)
{
    auto module = createModule();
    ASSERT_EQ(module.getName(), "Empty module");
}

TEST_F(EmptyModuleTest, VersionAvailable)
{
    auto module = createModule();
    ASSERT_TRUE(module.getVersionInfo().assigned());
}

TEST_F(EmptyModuleTest, VersionCorrect)
{
    auto module = createModule();
    auto version = module.getVersionInfo();

    ASSERT_EQ(version.getMajor(), EMPTY_MODULE_MAJOR_VERSION);
    ASSERT_EQ(version.getMinor(), EMPTY_MODULE_MINOR_VERSION);
    ASSERT_EQ(version.getPatch(), EMPTY_MODULE_PATCH_VERSION);
}

TEST_F(EmptyModuleTest, EnumerateDevices)
{
    auto module = createModule();

    ListPtr<IDeviceInfo> deviceInfo;
    ASSERT_NO_THROW(deviceInfo = module.getAvailableDevices());
    ASSERT_EQ(deviceInfo.getCount(), static_cast<SizeT>(0));
}

TEST_F(EmptyModuleTest, CreateDeviceConnectionStringNull)
{
    auto module = createModule();

    DevicePtr device;
    ASSERT_THROW(device = module.createDevice(nullptr, nullptr), ArgumentNullException);
}

TEST_F(EmptyModuleTest, CreateDeviceConnectionStringEmpty)
{
    auto module = createModule();

    DevicePtr device;
    ASSERT_NO_THROW(device = module.createDevice("", nullptr));
    ASSERT_EQ(device, nullptr);
}

TEST_F(EmptyModuleTest, GetAvailableComponentTypes)
{
    const auto module = createModule();

    DictPtr<IString, IFunctionBlockType> functionBlockTypes;
    ASSERT_NO_THROW(functionBlockTypes = module.getAvailableFunctionBlockTypes());
    ASSERT_EQ(functionBlockTypes.getCount(), 0u);

    DictPtr<IString, IDeviceType> deviceTypes;
    ASSERT_NO_THROW(deviceTypes = module.getAvailableDeviceTypes());
    ASSERT_EQ(deviceTypes.getCount(), 0u);

    DictPtr<IString, IServerType> serverTypes;
    ASSERT_NO_THROW(serverTypes = module.getAvailableServerTypes());
    ASSERT_EQ(serverTypes.getCount(), 0u);
}

TEST_F(EmptyModuleTest, CreateFunctionBlockIdNull)
{
    auto module = createModule();

    FunctionBlockPtr functionBlock;
    ASSERT_THROW(functionBlock = module.createFunctionBlock(nullptr, nullptr, "id"), ArgumentNullException);
}

TEST_F(EmptyModuleTest, CreateFunctionBlockIdEmpty)
{
    auto module = createModule();

    ASSERT_THROW(module.createFunctionBlock("", nullptr, "id"), NotFoundException);
}
