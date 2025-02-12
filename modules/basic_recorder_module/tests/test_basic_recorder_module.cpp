#include <testutils/testutils.h>
#include <basic_recorder_module/module_dll.h>
#include <basic_recorder_module/version.h>
#include <gmock/gmock.h>
#include <opendaq/module_ptr.h>
#include <opendaq/device_ptr.h>
#include <coretypes/common.h>
#include <opendaq/context_factory.h>
#include <opendaq/scheduler_factory.h>

using BasicRecorderModuleTest = testing::Test;
using namespace daq;

static ModulePtr CreateModule()
{
    ModulePtr module;
    createModule(&module, NullContext());
    return module;
}

TEST_F(BasicRecorderModuleTest, CreateModule)
{
    IModule* module = nullptr;
    ErrCode errCode = createModule(&module, NullContext());
    ASSERT_TRUE(OPENDAQ_SUCCEEDED(errCode));

    ASSERT_NE(module, nullptr);
    module->releaseRef();
}

TEST_F(BasicRecorderModuleTest, ModuleName)
{
    auto module = CreateModule();
    ASSERT_EQ(module.getName(), "BasicRecorderModule");
}

TEST_F(BasicRecorderModuleTest, VersionAvailable)
{
    auto module = CreateModule();
    ASSERT_TRUE(module.getVersionInfo().assigned());
}

TEST_F(BasicRecorderModuleTest, VersionCorrect)
{
    auto module = CreateModule();
    auto version = module.getVersionInfo();

    ASSERT_EQ(version.getMajor(), BASIC_RECORDER_MODULE_MAJOR_VERSION);
    ASSERT_EQ(version.getMinor(), BASIC_RECORDER_MODULE_MINOR_VERSION);
    ASSERT_EQ(version.getPatch(), BASIC_RECORDER_MODULE_PATCH_VERSION);
}
