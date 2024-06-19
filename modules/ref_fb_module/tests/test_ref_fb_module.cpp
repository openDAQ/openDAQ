#include <coretypes/common.h>
#include <opendaq/context_factory.h>
#include <opendaq/module_ptr.h>
#include <opendaq/scheduler_factory.h>
#include <opendaq/search_filter_factory.h>
#include <opendaq/signal_factory.h>
#include <ref_fb_module/module_dll.h>
#include <ref_fb_module/version.h>
#include <testutils/testutils.h>
#include <thread>
#include "testutils/memcheck_listener.h"

using RefFbModuleTest = testing::Test;
using namespace daq;

static ModulePtr CreateModule()
{
    ModulePtr module;
    auto logger = Logger();
    createModule(&module, Context(Scheduler(logger), logger, nullptr, nullptr, nullptr));
    return module;
}

TEST_F(RefFbModuleTest, CreateModule)
{
    IModule* module = nullptr;
    ErrCode errCode = createModule(&module, NullContext());
    ASSERT_TRUE(OPENDAQ_SUCCEEDED(errCode));

    ASSERT_NE(module, nullptr);
    module->releaseRef();
}

TEST_F(RefFbModuleTest, ModuleName)
{
    auto module = CreateModule();
    ASSERT_EQ(module.getName(), "Reference function block module");
}

TEST_F(RefFbModuleTest, VersionAvailable)
{
    auto module = CreateModule();
    ASSERT_TRUE(module.getVersionInfo().assigned());
}

TEST_F(RefFbModuleTest, VersionCorrect)
{
    auto module = CreateModule();
    auto version = module.getVersionInfo();

    ASSERT_EQ(version.getMajor(), REF_FB_MODULE_MAJOR_VERSION);
    ASSERT_EQ(version.getMinor(), REF_FB_MODULE_MINOR_VERSION);
    ASSERT_EQ(version.getPatch(), REF_FB_MODULE_PATCH_VERSION);
}

TEST_F(RefFbModuleTest, EnumerateDevices)
{
    auto module = CreateModule();

    ListPtr<IDeviceInfo> deviceInfoDict;
    ASSERT_NO_THROW(deviceInfoDict = module.getAvailableDevices());
    ASSERT_EQ(deviceInfoDict.getCount(), 0u);
}

TEST_F(RefFbModuleTest, GetAvailableComponentTypes)
{
    const auto module = CreateModule();

    DictPtr<IString, IDeviceType> deviceTypes;
    ASSERT_NO_THROW(deviceTypes = module.getAvailableDeviceTypes());
    ASSERT_EQ(deviceTypes.getCount(), 0u);

    DictPtr<IString, IServerType> serverTypes;
    ASSERT_NO_THROW(serverTypes = module.getAvailableServerTypes());
    ASSERT_EQ(serverTypes.getCount(), 0u);

    DictPtr<IString, IFunctionBlockType> functionBlockTypes;
    ASSERT_NO_THROW(functionBlockTypes = module.getAvailableFunctionBlockTypes());
    ASSERT_TRUE(functionBlockTypes.assigned());
    ASSERT_EQ(functionBlockTypes.getCount(), 7u);

    ASSERT_TRUE(functionBlockTypes.hasKey("RefFbModuleRenderer"));
    ASSERT_EQ("RefFbModuleRenderer", functionBlockTypes.get("RefFbModuleRenderer").getId());

    ASSERT_TRUE(functionBlockTypes.hasKey("RefFbModuleStatistics"));
    ASSERT_EQ("RefFbModuleStatistics", functionBlockTypes.get("RefFbModuleStatistics").getId());

    ASSERT_TRUE(functionBlockTypes.hasKey("RefFbModulePower"));
    ASSERT_EQ("RefFbModulePower", functionBlockTypes.get("RefFbModulePower").getId());

    ASSERT_TRUE(functionBlockTypes.hasKey("RefFbModuleScaling"));
    ASSERT_EQ("RefFbModuleScaling", functionBlockTypes.get("RefFbModuleScaling").getId());

    ASSERT_TRUE(functionBlockTypes.hasKey("RefFbModuleClassifier"));
    ASSERT_EQ("RefFbModuleClassifier", functionBlockTypes.get("RefFbModuleClassifier").getId());

    ASSERT_TRUE(functionBlockTypes.hasKey("RefFbModuleTrigger"));
    ASSERT_EQ("RefFbModuleTrigger", functionBlockTypes.get("RefFbModuleTrigger").getId());
}

TEST_F(RefFbModuleTest, CreateFunctionBlockNotFound)
{
    const auto module = CreateModule();

    ASSERT_THROW(module.createFunctionBlock("test", nullptr, "id"), NotFoundException);
}

TEST_F(RefFbModuleTest, DISABLED_CreateFunctionBlockRenderer)
{
    MemCheckListener::expectMemoryLeak = true;

    const auto module = CreateModule();

    auto fb = module.createFunctionBlock("RefFbModuleRenderer", nullptr, "id");
    ASSERT_TRUE(fb.assigned());

    // std::this_thread::sleep_for(std::chrono::milliseconds(10000));
}

TEST_F(RefFbModuleTest, CreateFunctionBlockStatistics)
{
    const auto module = CreateModule();

    auto fb = module.createFunctionBlock("RefFbModuleStatistics", nullptr, "id");
    ASSERT_TRUE(fb.assigned());
}

TEST_F(RefFbModuleTest, StatisticsNumOfSignals)
{
    auto module = CreateModule();

    auto fb = module.createFunctionBlock("RefFbModuleStatistics", nullptr, "id");
    ASSERT_EQ(fb.getSignals(search::Recursive(search::Any())).getCount(), 3u);
}

TEST_F(RefFbModuleTest, CreateFunctionBlockClassifier)
{
    const auto module = CreateModule();

    auto fb = module.createFunctionBlock("RefFbModuleClassifier", nullptr, "id");
    ASSERT_TRUE(fb.assigned());
}

TEST_F(RefFbModuleTest, createFunctionBlockTrigger)
{
    const auto module = CreateModule();

    auto fb = module.createFunctionBlock("RefFbModuleTrigger", nullptr, "id");
    ASSERT_TRUE(fb.assigned());
}
