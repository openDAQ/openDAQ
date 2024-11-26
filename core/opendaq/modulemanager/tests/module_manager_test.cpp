#include <testutils/testutils.h>
#include <opendaq/module_manager_factory.h>
#include <opendaq/context_factory.h>
#include <opendaq/logger_factory.h>

#include "mock/mock_module.h"

#include <thread>

using namespace daq;

class ModuleManagerTest : public testing::Test
{
protected:
    static const std::string SearchDir;

    ModuleManagerTest()
        : logger(Logger())
        , context(NullContext(logger))
    {
    }

    void TearDown() override
    {
        using namespace std::chrono_literals;

        // Wait for Async logger to flush
        std::this_thread::sleep_for(100ms);
    }

    LoggerPtr logger;
    ContextPtr context;
};

const std::string ModuleManagerTest::SearchDir{MODULE_TEST_DIR};

TEST_F(ModuleManagerTest, Create)
{
    auto manager = ModuleManager("[[none]]");
    ASSERT_TRUE(manager.assigned());
}

// TODO: Fix memory leak
//
//TEST_F(ModuleManagerTest, DefaultToCurrentDirectory)
//{
//    ASSERT_NO_THROW(ModuleManager(""));
//}

TEST_F(ModuleManagerTest, CreateNullPath)
{
    ASSERT_THROW(ModuleManager(nullptr), InvalidParameterException);
}

TEST_F(ModuleManagerTest, CreateInvalidPath)
{
    auto mngr = ModuleManager("in\\/@|id");
    ASSERT_NO_THROW(Context(nullptr, Logger(), nullptr, mngr, nullptr));
}

TEST_F(ModuleManagerTest, CreateNonExistentPath)
{
    std::string workingDir = SearchDir + "/doesNotExist";
    auto mngr = ModuleManager(workingDir);
    ASSERT_NO_THROW(Context(nullptr, Logger(), nullptr, mngr, nullptr));
}

TEST_F(ModuleManagerTest, CreateNotAFolder)
{
    std::string workingDir = SearchDir + "/file";
    auto mngr = ModuleManager(workingDir);
    ASSERT_NO_THROW(Context(nullptr, Logger(), nullptr, mngr, nullptr));
}

TEST_F(ModuleManagerTest, AddModule)
{
    auto manager = ModuleManager(SearchDir);
    manager.loadModules(NullContext());
    auto mock = createWithImplementation<IModule, MockModuleImpl>();

    ASSERT_NO_THROW(manager.addModule(mock));

    bool added = false;
    for (const auto& module : manager.getModules())
    {
        if (module == mock)
        {
            added = true;
        }
    }

    ASSERT_TRUE(added);
}

TEST_F(ModuleManagerTest, AddDuplicateModule)
{
    auto manager = ModuleManager(SearchDir);
    manager.loadModules(NullContext());
    auto mock = createWithImplementation<IModule, MockModuleImpl>();

    ASSERT_NO_THROW(manager.addModule(mock));
    ASSERT_THROW(manager.addModule(mock), DuplicateItemException);
}

TEST_F(ModuleManagerTest, EnumDriver)
{
    auto moduleManager = ModuleManager(SearchDir);
    moduleManager.loadModules(NullContext());

    ListPtr<IModule> moduleDrivers = moduleManager.getModules();
    auto count = moduleDrivers.getCount();

    ASSERT_EQ(count, 0u);

    std::cout << "Module driver count: " << count << std::endl;

#ifdef __GNUC__
    for (const ObjectPtr<IModule>& module : moduleDrivers)
    {
        ModuleInfoPtr info;
        module->getModuleInfo(&info);

        std::cout << info.getName() << std::endl;
    }

#else
    for (const ModulePtr& module : moduleDrivers)
    {
        std::cout << module.getModuleInfo().getName() << std::endl;
    }
#endif
}

TEST_F(ModuleManagerTest, RegisterDaqTypes)
{
    auto typeManager = context.getTypeManager();

    ASSERT_TRUE(typeManager.hasType("PtpSyncInterface"));
    ASSERT_TRUE(typeManager.hasType("SyncInterfaceBase"));
    ASSERT_TRUE(typeManager.hasType("InterfaceClockSync"));
}
