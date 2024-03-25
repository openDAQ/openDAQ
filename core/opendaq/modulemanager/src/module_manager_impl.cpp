#include <opendaq/module_manager_impl.h>
#include <opendaq/custom_log.h>
#include <opendaq/module_ptr.h>
#include <opendaq/module_manager_exceptions.h>
#include <opendaq/module_library.h>
#include <boost/dll/runtime_symbol_info.hpp>
#include <opendaq/orphaned_modules.h>
#include <future>
#include <string>
#include <boost/algorithm/string/predicate.hpp>
#include <opendaq/search_filter_factory.h>
#include <coretypes/validation.h>

BEGIN_NAMESPACE_OPENDAQ
static OrphanedModules orphanedModules;

static constexpr char createModuleFactory[] = "createModule";
static constexpr char checkDependenciesFunc[] = "checkDependencies";

static std::vector<ModuleLibrary> enumerateModules(const LoggerComponentPtr& loggerComponent, std::string searchFolder, IContext* context);

ModuleManagerImpl::ModuleManagerImpl(const BaseObjectPtr& path)
    : modulesLoaded(false)
{
    if (const StringPtr pathStr = path.asPtrOrNull<IString>(); pathStr.assigned())
    {
        paths.push_back(pathStr.toStdString());
    }
    else if (const ListPtr<IString> pathList = path.asPtrOrNull<IList>(); pathList.assigned())
    {
        paths.insert(paths.end(), pathList.begin(), pathList.end());
    }
    else
        throw InvalidParameterException{};
}

ModuleManagerImpl::~ModuleManagerImpl()
{
    for (auto& lib: libraries)
    {
        lib.module.release();

        if (!OrphanedModules::canUnloadModule(lib.handle))
            orphanedModules.add(std::move(lib.handle));
    }

    orphanedModules.tryUnload();
}

ErrCode ModuleManagerImpl::getModules(IList** availableModules)
{
    if (availableModules == nullptr)
        return OPENDAQ_ERR_ARGUMENT_NULL;
    
    auto list = List<IModule>();
    for (auto& library : libraries)
    {
        list.pushBack(library.module);
    }

    *availableModules = list.detach();
    return OPENDAQ_SUCCESS;
}

ErrCode ModuleManagerImpl::addModule(IModule* module)
{
    if (module == nullptr)
        return OPENDAQ_ERR_ARGUMENT_NULL;
    
    orphanedModules.tryUnload();

    const auto found = std::find_if(
        libraries.cbegin(),
        libraries.cend(),
        [module](const ModuleLibrary& library)
        {
            return library.module == module;
        }
    );

    if (found == libraries.cend())
    {
        libraries.emplace_back(ModuleLibrary{{}, module});
        return OPENDAQ_SUCCESS;
    }
    return OPENDAQ_ERR_DUPLICATEITEM;
}

ErrCode ModuleManagerImpl::loadModules(IContext* context)
{
    if (modulesLoaded)
        return OPENDAQ_SUCCESS;
    
    const auto contextPtr = ContextPtr::Borrow(context);
    logger = contextPtr.getLogger();
    if (!logger.assigned())
        return makeErrorInfo(OPENDAQ_ERR_ARGUMENT_NULL, "Logger must not be null");

    loggerComponent = this->logger.getOrAddComponent("ModuleManager");

    return daqTry([&](){
        for(const auto& path: paths) {
            auto localLibraries = enumerateModules(loggerComponent, path, context);
            libraries.insert(libraries.end(), localLibraries.begin(), localLibraries.end());
        }
        modulesLoaded = true;
        return OPENDAQ_SUCCESS;
    });
}

ErrCode ModuleManagerImpl::getAvailableDevices(IList** availableDevices)
{
    OPENDAQ_PARAM_NOT_NULL(availableDevices);
    
    auto availableDevicesPtr = List<IDeviceInfo>();

    using AsyncEnumerationResult = std::future<ListPtr<IDeviceInfo>>;
    std::vector<std::pair<AsyncEnumerationResult, ModulePtr>> enumerationResults;

    for (const auto& library : libraries)
    {
        const auto module = library.module;

        try
        {
            // Parallelize the process of each module enumerating/discovering available devices,
            // as it may be time-consuming
            AsyncEnumerationResult deviceListFuture = std::async([module = module]()
            {
                return module.getAvailableDevices();
            });
            enumerationResults.emplace_back(std::move(deviceListFuture), module);
        }
        catch (const std::exception& e)
        {
            LOG_E("Failed to run device enumeration asynchronously within the module: {}. Result {}",
                  module.getName(),
                  e.what()
            )
        }
    }

    for (auto& enumerationResult : enumerationResults)
    {
        ListPtr<IDeviceInfo> moduleAvailableDevices;
        auto module = enumerationResult.second;
        try
        {
            moduleAvailableDevices = enumerationResult.first.get();
        }
        catch (NotImplementedException&)
        {
            LOG_I("{}: GetAvailableDevices not implemented", module.getName())
        }
        catch ([[maybe_unused]] const std::exception& e)
        {
            LOG_W("{}: GetAvailableDevices failed: {}", module.getName(), e.what())
        }

        if (!moduleAvailableDevices.assigned())
            continue;

        for (const auto& deviceInfo : moduleAvailableDevices)
            availableDevicesPtr.pushBack(deviceInfo);
    }

    *availableDevices = availableDevicesPtr.detach();
    return OPENDAQ_SUCCESS;
}

ErrCode ModuleManagerImpl::getAvailableDeviceTypes(IDict** deviceTypes)
{
    OPENDAQ_PARAM_NOT_NULL(deviceTypes);
    
    auto availableTypes = Dict<IString, IDeviceType>();

    for (const auto& library : libraries)
    {
        const auto module = library.module;

        DictPtr<IString, IDeviceType> moduleDeviceTypes;

        try
        {
            moduleDeviceTypes = module.getAvailableDeviceTypes();
        }
        catch (NotImplementedException&)
        {
            LOG_I("{}: GetAvailableDeviceTypes not implemented", module.getName())
        }
        catch ([[maybe_unused]] const std::exception& e)
        {
            LOG_W("{}: GetAvailableDeviceTypes failed: {}", module.getName(), e.what())
        }

        if (!moduleDeviceTypes.assigned())
            continue;

        for (const auto& [id, type] : moduleDeviceTypes)
            availableTypes.set(id, type);
    }

    *deviceTypes = availableTypes.detach();
    return OPENDAQ_SUCCESS;
}

ErrCode ModuleManagerImpl::createDevice(IDevice** device, IString* connectionString, IComponent* parent, IPropertyObject* config)
{
    OPENDAQ_PARAM_NOT_NULL(device);

    for (const auto& library : libraries)
    {
        const auto module = library.module;

        bool accepted{};
        try
        {
            accepted = module.acceptsConnectionParameters(connectionString, config);
        }
        catch (NotImplementedException&)
        {
            LOG_I("{}: AcceptsConnectionString not implemented", module.getName())
            accepted = false;
        }
        catch ([[maybe_unused]] const std::exception& e)
        {
            LOG_W("{}: AcceptsConnectionString failed: {}", module.getName(), e.what())
            accepted = false;
        }

        if (accepted)
        {
            return module->createDevice(device, connectionString, parent, config);
        }
    }

    return this->makeErrorInfo(
        OPENDAQ_ERR_NOTFOUND, "Device with given connection string and config is not available [{}]", connectionString);
}

ErrCode ModuleManagerImpl::getAvailableFunctionBlockTypes(IDict** functionBlockTypes)
{
    OPENDAQ_PARAM_NOT_NULL(functionBlockTypes);

    auto availableTypes = Dict<IString, IFunctionBlockType>();

    for (const auto& library : libraries)
    {
        const auto module = library.module;
        
        DictPtr<IString, IFunctionBlockType> types;
        try
        {
            types = module.getAvailableFunctionBlockTypes();
        }
        catch (const NotImplementedException&)
        {
            LOG_I("{}: GetAvailableFunctionBlockTypes not implemented", module.getName())
        }
        catch ([[maybe_unused]] const std::exception& e)
        {
            LOG_W("{}: GetAvailableFunctionBlockTypes failed: {}", module.getName(), e.what())
        }

        if (!types.assigned())
            continue;

        for (const auto& [id, type] : types)
            availableTypes.set(id, type);
    }

    *functionBlockTypes = availableTypes.detach();
    return OPENDAQ_SUCCESS;
}

ErrCode ModuleManagerImpl::createFunctionBlock(IFunctionBlock** functionBlock, IString* id, IComponent* parent, IPropertyObject* config, IString* localId)
{
    OPENDAQ_PARAM_NOT_NULL(functionBlock);
    OPENDAQ_PARAM_NOT_NULL(id);

    const StringPtr typeId = StringPtr::Borrow(id);

    for (const auto& library : libraries)
    {
        const auto module = library.module;
        
        DictPtr<IString, IFunctionBlockType> types;
        try
        {
            types = module.getAvailableFunctionBlockTypes();
        }
        catch (const NotImplementedException&)
        {
            LOG_I("{}: GetAvailableFunctionBlockTypes not implemented", module.getName())
        }
        catch ([[maybe_unused]] const std::exception& e)
        {
            LOG_W("{}: GetAvailableFunctionBlockTypes failed: {}", module.getName(), e.what())
        }

        if (!types.assigned())
            continue;
        
        if (!types.hasKey(typeId))
            continue;

        std::string localIdStr;
        const PropertyObjectPtr configPtr = PropertyObjectPtr::Borrow(config);
        if (localId)
        {
            const auto idPtr = StringPtr::Borrow(localId);
            localIdStr = static_cast<std::string>(idPtr); 
        }
        else if (configPtr.assigned() && configPtr.hasProperty("LocalId"))
        {
            localIdStr = static_cast<std::string>(configPtr.getPropertyValue("LocalId"));
        }
        else
        {
            int maxNum = 0;
            const auto parentPtr = FolderPtr::Borrow(parent);
            for (const auto& item : parentPtr.getItems(search::Any()))
            {
                const std::string fbId = item.getLocalId();
                if (fbId.rfind(static_cast<std::string>(typeId), 0) == 0)
                {
                    const auto lastDelim = fbId.find_last_of('_');
                    if (lastDelim == std::string::npos)
                        continue;

                    const std::string numStr = fbId.substr(lastDelim + 1);
                    try
                    {
                        const auto num = std::stoi(numStr);
                        if (num > maxNum)
                            maxNum = num;
                    }
                    catch(...)
                    {
                        continue;
                    }
                }
            }

            localIdStr = fmt::format("{}_{}", typeId, maxNum + 1);
        }

        return module->createFunctionBlock(functionBlock, typeId, parent, String(localIdStr), config);
    }

    return this->makeErrorInfo(
        OPENDAQ_ERR_NOTFOUND, fmt::format(R"(Function block with given uid and config is not available [{}])", typeId));
}

std::vector<ModuleLibrary> enumerateModules(const LoggerComponentPtr& loggerComponent, std::string searchFolder, IContext* context)
{
    orphanedModules.tryUnload();

    if (searchFolder == "[[none]]")
    {
        LOGP_D("Search folder ignored");
        return {};
    }

    auto envPath = std::getenv("OPENDAQ_MODULES_PATH");
    if (envPath != nullptr)
    {
        searchFolder = envPath;
        LOG_D("Environment variable OPENDAQ_MODULES_PATH found, moduler search folder overriden to {}", searchFolder)
    }

    if (searchFolder.empty())
    {
        searchFolder = boost::dll::program_location().parent_path().string();
        LOG_D("search folder empty, moduler search folder set to program path {}", searchFolder)
    }

    std::error_code errCode;
    if (!fs::exists(searchFolder, errCode))
    {
        throw InvalidParameterException("The specified path does not exist.");
    }

    if (!fs::is_directory(searchFolder, errCode))
    {
        throw InvalidParameterException("The specified path is not a folder.");
    }

    LOG_I("Loading modules from '{}'", fs::absolute(searchFolder).string())

    [[maybe_unused]]
    Finally onExit([workingDir = fs::current_path()]()
    {
        fs::current_path(workingDir);
    });
    fs::current_path(searchFolder);

    std::vector<ModuleLibrary> moduleDrivers;
    fs::recursive_directory_iterator dirIterator(searchFolder);

    const auto endIter = fs::recursive_directory_iterator();
    while (dirIterator != endIter)
    {
        fs::directory_entry entry = *dirIterator++;

        if (!is_regular_file(entry, errCode))
        {
            continue;
        }

        const fs::path& entryPath = entry.path();
        const auto filename = entryPath.filename().u8string();

        if (boost::algorithm::ends_with(filename, OPENDAQ_MODULE_SUFFIX))
        {
            try
            {
                moduleDrivers.push_back(loadModule(loggerComponent, entryPath, context));
            }
            catch (const std::exception& e)
            {
                LOGP_W(e.what())
            }
            catch (...)
            {
                LOG_E("Unknown error occurred wile loading a module", ".")
            }
        }
    }

    return moduleDrivers;
}

template <typename Functor>
static void printComponentTypes(Functor func, const std::string& kind, const LoggerComponentPtr& loggerComponent)
{
    try
    {
        DictPtr<IString, IComponentType> componentTypes = func();
        if (componentTypes.getCount() > 0)
        {
            for (auto [id, type] : componentTypes)
            {
                LOG_I("\t{0:<3} [{1}] {2}: \"{3}\"",
                      kind,
                      id,
                      type.getName(),
                      type.getDescription()
                );
            }
        }
    }
    catch (const std::exception& e)
    {
        LOG_E("Failed to enumerate module's supported {} types: {}", kind, e.what());
    }
    catch (...)
    {
        LOG_E("Failed to enumerate module's supported {} types because of unknown exception", kind);
    }
}

static void printAvailableTypes(const ModulePtr& module, const LoggerComponentPtr& loggerComponent)
{
    printComponentTypes([&module](){return module.getAvailableDeviceTypes(); }, "DEV", loggerComponent);
    printComponentTypes([&module](){return module.getAvailableFunctionBlockTypes(); }, "FB", loggerComponent);
    printComponentTypes([&module](){return module.getAvailableServerTypes(); }, "SRV", loggerComponent);
}

ModuleLibrary loadModule(const LoggerComponentPtr& loggerComponent, const fs::path& path, IContext* context)
{
    auto relativePath = fs::relative(path).string();
    LOG_T("Loading module \"{}\".", relativePath);

    std::error_code libraryErrCode;
    boost::dll::shared_library moduleLibrary(path, libraryErrCode);

    if (libraryErrCode)
    {
        throw ModuleLoadFailedException(
            "Module \"{}\" failed to load. Error: {} [{}]",
            relativePath,
            libraryErrCode.value(),
#if defined(__linux__) || defined(linux) || defined(__linux)
            // boost does not propagate `dlopen()` error messages
            dlerror()
#else
            libraryErrCode.message()
#endif
        );
    }

    if (moduleLibrary.has(checkDependenciesFunc))
    {
        using CheckDependenciesFunc = ErrCode (*)(IString**);
        CheckDependenciesFunc checkDeps = moduleLibrary.get<ErrCode(IString**)>(checkDependenciesFunc);

        LOG_T("Checking dependencies of \"{}\".", relativePath);

        StringPtr errMsg;
        ErrCode errCode = checkDeps(&errMsg);
        if (OPENDAQ_FAILED(errCode))
        {
            LOG_T("Failed to check dependencies for \"{}\".", relativePath);

            throw ModuleIncompatibleDependenciesException(
                "Module \"{}\" failed dependencies check. Error: 0x{:x} [{}]",
                relativePath,
                errCode,
                errMsg
            );
        }
    }

    if (!moduleLibrary.has(createModuleFactory))
    {
        LOG_T("Module \"{}\" has no exported module factory.", relativePath);

        throw ModuleNoEntryPointException("Module \"{}\" has no exported module factory.", relativePath);
    }

    using ModuleFactory = ErrCode(IModule**, IContext*);
    ModuleFactory* factory = moduleLibrary.get<ModuleFactory>(createModuleFactory);

    LOG_T("Creating module from \"{}\".", relativePath);

    ModulePtr module;
    ErrCode errCode = factory(&module, context);
    if (OPENDAQ_FAILED(errCode))
    {
        LOG_T("Failed creating module from \"{}\".", relativePath);

        throw ModuleEntryPointFailedException("Library \"{}\" failed to create a Module.", relativePath);
    }

    if (auto version = module.getVersionInfo(); version.assigned())
    {
        LOG_I("Loaded module [v{}.{}.{} {}] from \"{}\".",
              version.getMajor(),
              version.getMinor(),
              version.getPatch(),
              module.getName(),
              relativePath);
    }
    else
    {
        LOG_I("Loaded module UNKNOWN VERSION of {} from \"{}\".", module.getName(), relativePath);
    }


    printAvailableTypes(module, loggerComponent);

    return { std::move(moduleLibrary), module };
}

OPENDAQ_DEFINE_CLASS_FACTORY(LIBRARY_FACTORY, ModuleManager,
    IString*, path
)

OPENDAQ_DEFINE_CLASS_FACTORY_WITH_INTERFACE_AND_CREATEFUNC(
    LIBRARY_FACTORY, ModuleManager,
    IModuleManager, createModuleManagerMultiplePaths,
    IList*, paths
)

END_NAMESPACE_OPENDAQ
