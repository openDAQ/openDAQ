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

TEST_F(RefFbModuleTest, AcceptsConnectionStringNull)
{
    auto module = CreateModule();
    ASSERT_THROW(module.acceptsConnectionParameters(nullptr), ArgumentNullException);
}

TEST_F(RefFbModuleTest, AcceptsConnectionStringEmpty)
{
    auto module = CreateModule();

    bool accepts = true;
    ASSERT_NO_THROW(accepts = module.acceptsConnectionParameters(""));
    ASSERT_FALSE(accepts);
}

TEST_F(RefFbModuleTest, AcceptsConnectionStringInvalid)
{
    auto module = CreateModule();

    bool accepts = true;
    ASSERT_NO_THROW(accepts = module.acceptsConnectionParameters("drfrfgt"));
    ASSERT_FALSE(accepts);
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
    ASSERT_EQ(functionBlockTypes.getCount(), 8u);

    ASSERT_TRUE(functionBlockTypes.hasKey("ref_fb_module_renderer"));
    ASSERT_EQ("ref_fb_module_renderer", functionBlockTypes.get("ref_fb_module_renderer").getId());

    ASSERT_TRUE(functionBlockTypes.hasKey("ref_fb_module_statistics"));
    ASSERT_EQ("ref_fb_module_statistics", functionBlockTypes.get("ref_fb_module_statistics").getId());

    ASSERT_TRUE(functionBlockTypes.hasKey("ref_fb_module_power"));
    ASSERT_EQ("ref_fb_module_power", functionBlockTypes.get("ref_fb_module_power").getId());

    ASSERT_TRUE(functionBlockTypes.hasKey("ref_fb_module_power_reader"));
    ASSERT_EQ("ref_fb_module_power_reader", functionBlockTypes.get("ref_fb_module_power_reader").getId());

    ASSERT_TRUE(functionBlockTypes.hasKey("ref_fb_module_scaling"));
    ASSERT_EQ("ref_fb_module_scaling", functionBlockTypes.get("ref_fb_module_scaling").getId());

    ASSERT_TRUE(functionBlockTypes.hasKey("ref_fb_module_classifier"));
    ASSERT_EQ("ref_fb_module_classifier", functionBlockTypes.get("ref_fb_module_classifier").getId());

    ASSERT_TRUE(functionBlockTypes.hasKey("ref_fb_module_trigger"));
    ASSERT_EQ("ref_fb_module_trigger", functionBlockTypes.get("ref_fb_module_trigger").getId());
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

    auto fb = module.createFunctionBlock("ref_fb_module_renderer", nullptr, "id");
    ASSERT_TRUE(fb.assigned());

    // std::this_thread::sleep_for(std::chrono::milliseconds(10000));
}

TEST_F(RefFbModuleTest, CreateFunctionBlockStatistics)
{
    const auto module = CreateModule();

    auto fb = module.createFunctionBlock("ref_fb_module_statistics", nullptr, "id");
    ASSERT_TRUE(fb.assigned());
}

TEST_F(RefFbModuleTest, StatisticsNumOfSignals)
{
    auto module = CreateModule();

    auto fb = module.createFunctionBlock("ref_fb_module_statistics", nullptr, "id");
    ASSERT_EQ(fb.getSignals(search::Recursive(search::Any())).getCount(), 3u);
}

TEST_F(RefFbModuleTest, CreateFunctionBlockClassifier)
{
    const auto module = CreateModule();

    auto fb = module.createFunctionBlock("ref_fb_module_classifier", nullptr, "id");
    ASSERT_TRUE(fb.assigned());
}

TEST_F(RefFbModuleTest, createFunctionBlockTrigger)
{
    const auto module = CreateModule();

    auto fb = module.createFunctionBlock("ref_fb_module_trigger", nullptr, "id");
    ASSERT_TRUE(fb.assigned());
}
