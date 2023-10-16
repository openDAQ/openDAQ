#include <opendaq/module_manager_exceptions.h>
#include <opendaq/module_manager_impl.h>
#include <opendaq/module_ptr.h>
#include <opendaq/module_library.h>
#include <testutils/testutils.h>
#include <opendaq/logger_ptr.h>
#include <opendaq/logger_component_ptr.h>
#include <opendaq/logger_factory.h>

#include <thread>

using namespace daq;

class ModuleManagerInternalsTest : public testing::Test
{
protected:
    void SetUp() override
    {
        logger = Logger();
        loggerComponent = this->logger.addComponent("ModuleManagerInternalsTest");
        fs::current_path(MODULE_TEST_DIR);
    }

    void TearDown() override
    {
        using namespace std::chrono_literals;

        // Wait for Async logger to flush
        std::this_thread::sleep_for(100ms);

        logger.removeComponent("ModuleManagerInternalsTest");
    }

    LoggerPtr logger;
    LoggerComponentPtr loggerComponent;
    IModuleManager* manager{};
    IContext* context{};
};

TEST_F(ModuleManagerInternalsTest, LoadEmptyDll)
{
    fs::path modulePath = EMPTY_MODULE_FILE_NAME;

    ASSERT_THROW_MSG(
        auto lib = loadModule(loggerComponent, modulePath, context),
        ModuleNoEntryPointException,
        fmt::format("Module \"{}\" has no exported module factory.", fs::relative(modulePath).string())
    )
}

TEST_F(ModuleManagerInternalsTest, LoadCrashingDll)
{
    fs::path modulePath = CRASHING_MODULE_FILE_NAME;

    ASSERT_THROW_MSG(auto lib = loadModule(loggerComponent, modulePath, context),
                     ModuleEntryPointFailedException,
                     fmt::format("Library \"{}\" failed to create a Module.", fs::relative(modulePath).string()))
}

TEST_F(ModuleManagerInternalsTest, ModuleDependenciesCheckFailed)
{
    fs::path modulePath = DEPENDENCIES_FAILED_MODULE_NAME;

    ASSERT_THROW_MSG(auto lib = loadModule(loggerComponent, modulePath, context),
                     ModuleIncompatibleDependenciesException,
                     fmt::format(
                         "Module \"{}\" failed dependencies check. Error: 0x{:x} [{}]",
                         fs::relative(modulePath).string(),
                         OPENDAQ_ERR_GENERALERROR,
                         "Mock failure"
                     ))
}

TEST_F(ModuleManagerInternalsTest, ModuleDependenciesCheckSucceed)
{
    fs::path modulePath = DEPENDENCIES_SUCCEEDED_MODULE_NAME;

    ModuleLibrary lib;
    ASSERT_NO_THROW(lib = loadModule(loggerComponent, modulePath, context));

    ASSERT_EQ(lib.module.getName(), "MockModule");
}
