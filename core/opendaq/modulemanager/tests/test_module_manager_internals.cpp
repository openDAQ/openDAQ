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

#include "mock/mock_module.h"
#include "mock/mock_module_authenticator.h"

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

TEST_F(ModuleManagerInternalsTest, LoadModuleErrors)
{
    ASSERT_THROW_MSG(
        this->manager.loadModule(""),
        InvalidParameterException,
        "Specified module path is empty"
    );

    ASSERT_THROW_MSG(
        this->manager.loadModule(GetMockModulePath("/invalid.ext").string()),
        InvalidParameterException,
        fmt::format(R"(The openDAQ module file must have an extention "{}")", OPENDAQ_MODULE_SUFFIX)
    );

    fs::path modulePath = GetMockModulePath(fmt::format(R"(/doesNotExist{})", OPENDAQ_MODULE_SUFFIX));
    LOG_I("Load module: \"{}\"", modulePath.string());
    ASSERT_THROW_MSG(
        this->manager.loadModule(modulePath.string()),
        InvalidParameterException,
        fmt::format(R"(Specified module path "{}" does not exist)", modulePath.string())
    );

    auto manager = ModuleManager("[[none]]");
    ASSERT_THROW_MSG(
        manager.loadModule(GetMockModulePath(DEPENDENCIES_SUCCEEDED_MODULE_NAME).string()),
        InvalidStateException,
        "ModuleManager in not initialized. Call loadModules(IContext*) first."
    );
}

TEST_F(ModuleManagerInternalsTest, LoadEmptyDll)
{
    fs::path modulePath = GetMockModulePath(EMPTY_MODULE_FILE_NAME);
    LOG_I("Load module: \"{}\"", modulePath.string());

    ASSERT_THROW_MSG(
        auto module = manager.loadModule(modulePath.string()),
        ModuleNoEntryPointException,
        fmt::format("Module \"{}\" has no exported module factory.", modulePath.string())
    );
}

TEST_F(ModuleManagerInternalsTest, LoadCrashingDll)
{
    fs::path modulePath = GetMockModulePath(CRASHING_MODULE_FILE_NAME);
    LOG_I("Load module: \"{}\"", modulePath.string());

    ASSERT_THROW_MSG(
        auto module = manager.loadModule(modulePath.string()),
        ModuleEntryPointFailedException,
        fmt::format("Library \"{}\" failed to create a Module.", modulePath.string())
    );
}

TEST_F(ModuleManagerInternalsTest, ModuleDepCheckObsoleteFailed)
{
    fs::path modulePath = GetMockModulePath(DEPENDENCIES_OBSOLETE_FAILED_MODULE_NAME);
    LOG_I("Load module: \"{}\"", modulePath.string());

    ASSERT_THROW_MSG(
        auto module = manager.loadModule(modulePath.string()),
        ModuleIncompatibleDependenciesException,
        fmt::format(
            "Module \"{}\" failed dependencies check",
            modulePath.string(),
            OPENDAQ_ERR_GENERALERROR,
            "Mock failure"
        )
    );
}

TEST_F(ModuleManagerInternalsTest, ModuleDepCheckObsoleteSucceed)
{
    fs::path modulePath = GetMockModulePath(DEPENDENCIES_OBSOLETE_SUCCEEDED_MODULE_NAME);
    LOG_I("Load module: \"{}\"", modulePath.string());

    ModulePtr module;
    ASSERT_NO_THROW(
        module = manager.loadModule(modulePath.string())
    );
    ASSERT_EQ(module.getModuleInfo().getName(), "MockModule");
    ASSERT_EQ(module.getModuleInfo().getId(), "mock_dep_obsolete");
}

TEST_F(ModuleManagerInternalsTest, ModuleDependenciesCheckFailed)
{
    fs::path modulePath = GetMockModulePath(DEPENDENCIES_FAILED_MODULE_NAME);
    LOG_I("Load module: \"{}\"", modulePath.string());

    ASSERT_THROW_MSG(
        auto module = manager.loadModule(modulePath.string()),
        ModuleIncompatibleDependenciesException,
        fmt::format(
            "Module \"{}\" failed dependencies check",
            modulePath.string(),
            OPENDAQ_ERR_GENERALERROR,
            "Mock failure"
        )
    );
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
    ASSERT_EQ(module.getModuleInfo().getId(), "mock_dep");
}

TEST_F(ModuleManagerInternalsTest, NullStringIdModule)
{
    fs::path modulePath = GetMockModulePath(NULL_STRING_ID_MODULE_NAME);
    LOG_I("Load module: \"{}\"", modulePath.string());

    ModulePtr module;
    ASSERT_NO_THROW(
        module = manager.loadModule(modulePath.string())
    );
    ASSERT_EQ(module.getModuleInfo().getName(), "MockModule");
    ASSERT_EQ(module.getModuleInfo().getId(), nullptr);
}

TEST_F(ModuleManagerInternalsTest, EmptyStringIdModule)
{
    fs::path modulePath = GetMockModulePath(EMPTY_STRING_ID_MODULE_NAME);
    LOG_I("Load module: \"{}\"", modulePath.string());

    ModulePtr module;
    ASSERT_NO_THROW(
        module = manager.loadModule(modulePath.string())
    );
    ASSERT_EQ(module.getModuleInfo().getName(), "MockModule");
    ASSERT_EQ(module.getModuleInfo().getId(), "");
}

TEST_F(ModuleManagerInternalsTest, LoadSingleModuleTwice1)
{
    fs::path modulePath = GetMockModulePath(NULL_STRING_ID_MODULE_NAME);
    LOG_I("Load module: \"{}\"", modulePath.string());

    manager.loadModule(modulePath.string());

    ModulePtr module;
    ASSERT_EQ(manager->loadModule(String(modulePath.string()), &module), OPENDAQ_IGNORED);
    ASSERT_EQ(module.getModuleInfo().getName(), "MockModule");
    ASSERT_EQ(module.getModuleInfo().getId(), nullptr);
}

TEST_F(ModuleManagerInternalsTest, LoadSingleModuleTwice2)
{
    fs::path modulesPath = exePath / fs::path(MODULE_TEST_DIR);
    auto manager = ModuleManager(modulesPath.string());
    manager.loadModules(NullContext());

    fs::path modulePath = GetMockModulePath(EMPTY_STRING_ID_MODULE_NAME);
    LOG_I("Load module: \"{}\"", modulePath.string());

    ModulePtr module;
    ASSERT_EQ(manager->loadModule(String(modulePath.string()), &module), OPENDAQ_IGNORED);
    ASSERT_EQ(module.getModuleInfo().getName(), "MockModule");
    ASSERT_EQ(module.getModuleInfo().getId(), "");
}

TEST_F(ModuleManagerInternalsTest, LoadAllModulesTwice)
{
    auto ctx = NullContext();

    fs::path modulesPath = exePath / fs::path(MODULE_TEST_DIR);
    auto manager = ModuleManager(modulesPath.string());

    manager.loadModules(ctx);
    ASSERT_GT(manager.getModules().getCount(), 0u);

    ASSERT_THROW_MSG(
        manager.loadModules(NullContext()),
        InvalidParameterException,
        "Context cannot be changed after loading modules"
    );

    ASSERT_EQ(manager->loadModules(ctx), OPENDAQ_IGNORED);
    ASSERT_GT(manager.getModules().getCount(), 0u);
}

TEST_F(ModuleManagerInternalsTest, LoadModuleWithIdAfterAddedFromMemory)
{
    auto manager = ModuleManager("[[none]]");
    manager.loadModules(NullContext());

    auto mock = createWithImplementation<IModule, MockModuleImpl>("mock_ver_match");
    ASSERT_NO_THROW(manager.addModule(mock));
    ASSERT_EQ(manager.getModules().getCount(), 1u);
    ASSERT_EQ(manager.getModules()[0], mock);

    fs::path modulePath = GetMockModulePath(SDK_VERSION_MATCH_MODULE_NAME);
    LOG_I("Load module: \"{}\"", modulePath.string());

    ASSERT_THROW_MSG(
        auto module = manager.loadModule(modulePath.string()),
        AlreadyExistsException,
        fmt::format(
            "Module with id \"mock_ver_match\" was already loaded from memory. Reject loading module from path \"{}\"",
            modulePath.string(),
            OPENDAQ_ERR_GENERALERROR,
            "Mock failure"
        )
    );
    ASSERT_EQ(manager.getModules().getCount(), 1u);
}

TEST_F(ModuleManagerInternalsTest, LoadModuleCopyWithId)
{
    auto manager = ModuleManager("[[none]]");
    manager.loadModules(NullContext());

    fs::path modulePath = GetMockModulePath(DEPENDENCIES_SUCCEEDED_MODULE_NAME);
    LOG_I("Load module: \"{}\"", modulePath.string());
    ModulePtr module;
    ASSERT_NO_THROW(
        module = manager.loadModule(modulePath.string())
    );
    ASSERT_EQ(manager.getModules().getCount(), 1u);

    fs::path modulePathCopy = GetMockModulePath(COPY_DEPENDENCIES_SUCCEEDED_MODULE_NAME);
    LOG_I("Load module: \"{}\"", modulePathCopy.string());
    ModulePtr moduleCopy;
    ASSERT_THROW_MSG(
        moduleCopy = manager.loadModule(modulePathCopy.string()),
        AlreadyExistsException,
        fmt::format(
            "Module with id \"mock_dep\" was already loaded and added from path \"{}\". Reject loading module from \"{}\"",
            modulePath.string(),
            modulePathCopy.string(),
            OPENDAQ_ERR_GENERALERROR,
            "Mock failure"
        )
    );
    ASSERT_EQ(manager.getModules().getCount(), 1u);
}

TEST_F(ModuleManagerInternalsTest, LoadModuleWithNullOrEmptyIdAfterAddedFromMemory)
{
    auto manager = ModuleManager("[[none]]");
    ModulePtr module;
    manager.loadModules(NullContext());

    ASSERT_NO_THROW(manager.addModule(createWithImplementation<IModule, MockModuleImpl>(nullptr)));
    ASSERT_EQ(manager.getModules().getCount(), 1u);

    ASSERT_NO_THROW(manager.addModule(createWithImplementation<IModule, MockModuleImpl>("")));
    ASSERT_EQ(manager.getModules().getCount(), 2u);

    fs::path nullStringIdModulePath = GetMockModulePath(NULL_STRING_ID_MODULE_NAME);
    LOG_I("Load module: \"{}\"", nullStringIdModulePath.string());
    ASSERT_EQ(manager->loadModule(String(nullStringIdModulePath.string()), &module), OPENDAQ_SUCCESS);
    ASSERT_EQ(manager.getModules().getCount(), 3u);

    fs::path emptyStringIdModulePath = GetMockModulePath(EMPTY_STRING_ID_MODULE_NAME);
    LOG_I("Load module: \"{}\"", emptyStringIdModulePath.string());
    ASSERT_EQ(manager->loadModule(String(emptyStringIdModulePath.string()), &module), OPENDAQ_SUCCESS);
    ASSERT_EQ(manager.getModules().getCount(), 4u);
}

TEST_F(ModuleManagerInternalsTest, TestAuthenticator)
{
    StringPtr certPath = StringPtr("mock/path");
    auto authenticator = createWithImplementation<IModuleAuthenticator, MockModuleAuthenticatorImpl>(certPath);
    auto manager = ModuleManager("[[none]]");
    manager->setAuthenticatedOnly(true);
    manager->setModuleAuthenticator(authenticator);
    manager.loadModules(NullContext());

    fs::path modulePath = GetMockModulePath(DEPENDENCIES_SUCCEEDED_MODULE_NAME);
    LOG_I("Load module: \"{}\"", modulePath.string());

    ModulePtr module;
    ASSERT_NO_THROW(module = manager.loadModule(modulePath.string()));
    ASSERT_EQ(manager.getModules().getCount(), 1u);
    ASSERT_EQ(manager.getModules()[0], module);
}
