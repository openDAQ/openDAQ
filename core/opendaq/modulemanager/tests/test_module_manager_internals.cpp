#include <opendaq/module_manager_exceptions.h>
#include <opendaq/module_manager_factory.h>
#include <opendaq/module_ptr.h>
#include <testutils/testutils.h>
#include <opendaq/context_factory.h>
#include <opendaq/custom_log.h>
#include <opendaq/context_internal_ptr.h>
#include <opendaq/module_authenticator_factory.h>

#include <chrono>
#include <thread>
#include <opendaq/boost_dll.h>

#include "mock/mock_module.h"

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

TEST_F(ModuleManagerInternalsTest, ModuleDependenciesCheckFailed)
{
    fs::path modulePath = GetMockModulePath(DEPENDENCIES_FAILED_MODULE_NAME);
    LOG_I("Load module: \"{}\"", modulePath.string());

    ASSERT_THROW_MSG(
        auto module = manager.loadModule(modulePath.string()),
        ModuleIncompatibleDependenciesException,
        fmt::format(
            "Module \"{}\" failed dependencies check",
            modulePath.string()
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

    auto mock = createWithImplementation<IModule, MockModuleImpl>("mock_dep");
    ASSERT_NO_THROW(manager.addModule(mock));
    ASSERT_EQ(manager.getModules().getCount(), 1u);
    ASSERT_EQ(manager.getModules()[0], mock);

    fs::path modulePath = GetMockModulePath(DEPENDENCIES_SUCCEEDED_MODULE_NAME);
    LOG_I("Load module: \"{}\"", modulePath.string());

    ASSERT_THROW_MSG(
        auto module = manager.loadModule(modulePath.string()),
        AlreadyExistsException,
        fmt::format(
            "Module with id \"mock_dep\" was already loaded from memory. Reject loading module from path \"{}\"",
            modulePath.string()
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
            modulePathCopy.string()
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
    StringPtr fName;
    auto pathAuth = std::map<std::string, DictPtr<IString, IBaseObject>>();
    std::string throwPath;
    const auto errMsg = "Test Error Message";

    const auto authF = Function(
        [&](const StringPtr& f, const DictPtr<IString, IBaseObject>& params)
        {
            fName = f;
            const StringPtr path = params.get("BinaryPath");
            if (path == throwPath)
                DAQ_THROW_EXCEPTION(InvalidStateException, errMsg);
            auto it = pathAuth.find(path);
            if (it != pathAuth.end())
                return it->second;
            return Dict<IString, IBaseObject>();  // implicit BinaryValid false
        });

    auto manager = ModuleManager((exePath / fs::path(MODULE_TEST_DIR)).string());
    manager->setAuthenticatedOnly(true);
    // all modules access denied
    manager->setModuleAuthenticator(FunctionModuleAuthenticator(authF));
    manager.loadModules(NullContext());
    ASSERT_EQ(manager.getModules().getCount(), 0u);
    // valid auth, valid module
    auto authorizedMods = std::array<ModulePtr, 2>();
    auto modPath = GetMockModulePath(NULL_STRING_ID_MODULE_NAME).string();
    pathAuth[modPath] = Dict<IString, IBaseObject>({{"BinaryValid", true}});
    ASSERT_NO_THROW(authorizedMods[0] = manager.loadModule(modPath));
    // invalid auth, valid module
    ModulePtr unauthorizedMod;
    modPath = GetMockModulePath(DEPENDENCIES_SUCCEEDED_MODULE_NAME).string();
    const auto testVendorKey = "test";
    pathAuth[modPath] = Dict<IString, IBaseObject>({{"VendorKey", testVendorKey}});
    ASSERT_THROW(unauthorizedMod = manager.loadModule(modPath), AccessDeniedException);  // implicit BinaryValid false
    pathAuth[modPath]["BinaryValid"] = true;
    ASSERT_NO_THROW(authorizedMods[1] = manager.loadModule(modPath));
    // invalid auth, invalid module
    modPath = GetMockModulePath(EMPTY_MODULE_FILE_NAME).string();
    pathAuth[modPath] = Dict<IString, IBaseObject>({{"BinaryValid", false}});  // auth should be the first step to take place
    ASSERT_THROW(unauthorizedMod = manager.loadModule(modPath), AccessDeniedException);
    // invalid return from auth function
    modPath = GetMockModulePath(DEPENDENCIES_FAILED_MODULE_NAME).string();
    pathAuth[modPath] = Dict<IString, IBaseObject>({{"BinaryValid", "True"}});  // should fail
    ASSERT_THROW(unauthorizedMod = manager.loadModule(modPath), NoInterfaceException);
    // error propagation from auth function
    throwPath = GetMockModulePath(EMPTY_STRING_ID_MODULE_NAME).string();
    try
    {
        unauthorizedMod = manager.loadModule(throwPath);
        FAIL();
    }
    catch (const DaqException& e)
    {
        ASSERT_EQ(e.getErrCode(), OPENDAQ_ERR_INVALIDSTATE);
        ASSERT_NE(e.getErrorMessage().find(errMsg), std::string::npos);
    }
    // vendor keys
    auto keys = manager.getVendorKeys();
    ASSERT_EQ(keys[authorizedMods[1].getModuleInfo().getId()], testVendorKey);
    // misc
    ASSERT_EQ(fName, "authenticateModuleBinary");
    ASSERT_EQ(manager.getModules().getCount(), 2u);
    ASSERT_EQ(manager.getModules()[0], authorizedMods[0]);
    ASSERT_EQ(manager.getModules()[1], authorizedMods[1]);
}