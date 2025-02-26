#include <opendaq/module_manager_exceptions.h>
#include <opendaq/module_manager_impl.h>
#include <opendaq/module_ptr.h>
#include <opendaq/module_library.h>
#include <testutils/testutils.h>
#include <opendaq/logger_ptr.h>
#include <opendaq/logger_component_ptr.h>
#include <opendaq/logger_factory.h>

#include <chrono>
#include <thread>

using namespace daq;

class ModuleManagerInternalsTest : public testing::Test
{
protected:
    void SetUp() override
    {
        logger = Logger();
        loggerComponent = this->logger.addComponent("ModuleManagerInternalsTest");

        workingDir = fs::current_path();

        auto exePath = boost::dll::program_location().remove_filename();
        fs::current_path(exePath);
    }

    void TearDown() override
    {
        using namespace std::chrono_literals;

        fs::current_path(workingDir);

        // Wait for Async logger to flush
        std::this_thread::sleep_for(100ms);

        logger.removeComponent("ModuleManagerInternalsTest");
    }

    static fs::path GetMockModulePath(const std::string& moduleFileName)
    {
        return fs::path(MODULE_TEST_DIR) / moduleFileName;
    }

    fs::path workingDir;
    LoggerPtr logger;
    LoggerComponentPtr loggerComponent;
    IModuleManager* manager{};
    IContext* context{};
};

TEST_F(ModuleManagerInternalsTest, LoadEmptyDll)
{
    fs::path modulePath = GetMockModulePath(EMPTY_MODULE_FILE_NAME);

    ASSERT_THROW_MSG(
        auto lib = loadModule(loggerComponent, modulePath, context),
        ModuleNoEntryPointException,
        fmt::format("Module \"{}\" has no exported module factory.", fs::relative(modulePath).string())
    )
}

TEST_F(ModuleManagerInternalsTest, LoadCrashingDll)
{
    fs::path modulePath = GetMockModulePath(CRASHING_MODULE_FILE_NAME);

    ASSERT_THROW_MSG(auto lib = loadModule(loggerComponent, modulePath, context),
                     ModuleEntryPointFailedException,
                     fmt::format("Library \"{}\" failed to create a Module.", fs::relative(modulePath).string()))
}

TEST_F(ModuleManagerInternalsTest, ModuleDependenciesCheckFailed)
{
    fs::path modulePath = GetMockModulePath(DEPENDENCIES_FAILED_MODULE_NAME);

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
    fs::path modulePath = GetMockModulePath(DEPENDENCIES_SUCCEEDED_MODULE_NAME);

    ModuleLibrary lib;
    ASSERT_NO_THROW(lib = loadModule(loggerComponent, modulePath, context));

    ASSERT_EQ(lib.module.getModuleInfo().getName(), "MockModule");
}
