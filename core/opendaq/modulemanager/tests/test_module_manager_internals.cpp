#include <opendaq/module_manager_exceptions.h>
#include <opendaq/module_manager_factory.h>
#include <opendaq/module_ptr.h>
#include <testutils/testutils.h>
#include <opendaq/context_factory.h>
#include <opendaq/custom_log.h>
#include <opendaq/context_internal_ptr.h>

#include <chrono>
#include <thread>

#include <opendaq/boost_dll.h>

using namespace daq;

class ModuleManagerInternalsTest : public testing::Test
{
protected:
    void SetUp() override
    {
        context = Context(nullptr, Logger(), TypeManager(), ModuleManager("[[none]]"));
        manager = context.asPtr<IContextInternal>().moveModuleManager();
        loggerComponent = context.getLogger().addComponent("ModuleManagerInternalsTest");
        exePath = boost::dll::program_location().remove_filename();
    }

    void TearDown() override
    {
        using namespace std::chrono_literals;

        // Wait for Async logger to flush
        std::this_thread::sleep_for(100ms);

        context.getLogger().removeComponent("ModuleManagerInternalsTest");
    }

    fs::path GetMockModulePath(const std::string& moduleFileName)
    {
        return exePath / fs::path(MODULE_TEST_DIR) / moduleFileName;
    }

    fs::path exePath;
    LoggerComponentPtr loggerComponent;
    ModuleManagerPtr manager;
    ContextPtr context;
};

TEST_F(ModuleManagerInternalsTest, LoadEmptyDll)
{
    fs::path modulePath = GetMockModulePath(EMPTY_MODULE_FILE_NAME);
    LOG_I("Load module: \"{}\"", modulePath.string());

    ASSERT_THROW_MSG(
        auto module = manager.loadModule(modulePath.string()),
        ModuleNoEntryPointException,
        fmt::format("Module \"{}\" has no exported module factory.", fs::relative(modulePath).string())
    )
}

TEST_F(ModuleManagerInternalsTest, LoadCrashingDll)
{
    fs::path modulePath = GetMockModulePath(CRASHING_MODULE_FILE_NAME);
    LOG_I("Load module: \"{}\"", modulePath.string());

    ASSERT_THROW_MSG(
        auto module = manager.loadModule(modulePath.string()),
        ModuleEntryPointFailedException,
        fmt::format("Library \"{}\" failed to create a Module.", fs::relative(modulePath).string())
    )
}

TEST_F(ModuleManagerInternalsTest, ModuleDependenciesCheckFailed)
{
    fs::path modulePath = GetMockModulePath(DEPENDENCIES_FAILED_MODULE_NAME);
    LOG_I("Load module: \"{}\"", modulePath.string());

    ASSERT_THROW_MSG(
        auto module = manager.loadModule(modulePath.string()),
        ModuleIncompatibleDependenciesException,
        fmt::format(
            "Module \"{}\" failed dependencies check. Error: 0x{:x} [{}]",
            fs::relative(modulePath).string(),
            OPENDAQ_ERR_GENERALERROR,
            "Mock failure"
        )
    )
}

TEST_F(ModuleManagerInternalsTest, ModuleDependenciesCheckSucceed)
{
    fs::path modulePath = GetMockModulePath(DEPENDENCIES_SUCCEEDED_MODULE_NAME);
    LOG_I("Load module: \"{}\"", modulePath.string());

    ModulePtr module;
    ASSERT_NO_THROW(
        module = manager.loadModule(modulePath.string())
    );
    ASSERT_EQ(module.getModuleInfo().getName(), "MockModule");
}
