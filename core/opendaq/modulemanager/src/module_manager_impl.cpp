#include <opendaq/module_manager_impl.h>
#include <opendaq/module_manager_ptr.h>
#include <opendaq/custom_log.h>
#include <opendaq/module_ptr.h>
#include <opendaq/module_manager_exceptions.h>
#include <opendaq/module_library.h>
#include <boost/dll/runtime_symbol_info.hpp>
#include <opendaq/orphaned_modules.h>
#include <opendaq/device_info_config_ptr.h>
#include <opendaq/device_info_private_ptr.h>
#include <coretypes/validation.h>
#include <opendaq/device_private.h>
#include <string>
#include <future>
#include <boost/algorithm/string/predicate.hpp>

BEGIN_NAMESPACE_OPENDAQ

static OrphanedModules orphanedModules;

static constexpr char createModuleFactory[] = "createModule";
static constexpr char checkDependenciesFunc[] = "checkDependencies";

static std::vector<ModuleLibrary> enumerateModules(const LoggerComponentPtr& loggerComponent, std::string searchFolder, IContext* context);

ModuleManagerImpl::ModuleManagerImpl(const StringPtr& path)
    : modulesLoaded(false)
    , path(path)
{
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
        libraries = enumerateModules(loggerComponent, path, context);
        modulesLoaded = true;
        return OPENDAQ_SUCCESS;
    });
}

ErrCode ModuleManagerImpl::getAvailableDevices(IList** availableDevices)
{
    OPENDAQ_PARAM_NOT_NULL(availableDevices);

    using AsyncEnumerationResult = std::future<ListPtr<IDeviceInfo>>;
    std::vector<std::pair<AsyncEnumerationResult, ModulePtr>> enumerationResults;

    for (const auto module : this->borrowPtr<ModuleManagerPtr>().getModules())
    {
        try
        {
            // Parallelize the process of each module enumerating/discovering available devices,
            // as it may be time-consuming
            AsyncEnumerationResult deviceListFuture =
                std::async([module = module]()
                           {
                               return module.getAvailableDevices();
                           });
            enumerationResults.push_back(std::make_pair(std::move(deviceListFuture), module));
        }
        catch (const std::exception& e)
        {
            LOG_E("Failed to run device enumeration asynchronously within the module: {}. Result {}",
                  module.getName(), e.what())
        }
    }

    auto groupedDevices = Dict<IString, IDeviceInfo>();
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
        catch (const std::exception& e)
        {
            LOG_W("{}: GetAvailableDevices failed: {}", module.getName(), e.what())
        }

        if (!moduleAvailableDevices.assigned())
            continue;

        for (const auto& deviceInfo : moduleAvailableDevices)
        {
            StringPtr id = deviceInfo.getConnectionString();
            if (groupedDevices.hasKey(id))
            {
                DeviceInfoPtr value = groupedDevices.get(id);
                for (const auto & capability : deviceInfo.getServerCapabilities())
                    value.asPtr<IDeviceInfoPrivate>().addServerCapability(capability);
            }
            else
            {
                groupedDevices.set(id, deviceInfo);
            }
        }
    }

    auto availableDevicesList = List<IDeviceInfo>();
    for (const auto & [_, deviceInfo] : groupedDevices)
        availableDevicesList.pushBack(deviceInfo);

    *availableDevices = availableDevicesList.detach();

    availableDevicesGroup = groupedDevices;
    return OPENDAQ_SUCCESS;
}

ErrCode ModuleManagerImpl::getDevice(IString* connectionString, IPropertyObject* config, IComponent* parent, ILoggerComponent* logger, IDevice** device)
{
    OPENDAQ_PARAM_NOT_NULL(connectionString);
    OPENDAQ_PARAM_NOT_NULL(device);
    *device = nullptr;

    auto loggerComponent = LoggerComponentPtr::Borrow(logger);

    auto connectionStringPtr = StringPtr::Borrow(connectionString);
    if (!connectionStringPtr.assigned() || connectionStringPtr.getLength() == 0)
        return OPENDAQ_SUCCESS;

    DeviceInfoPtr deviceInfo;
    ServerCapabilityPtr internalServerCapability;
    
    if (connectionStringPtr.toStdString().find("daq://") == 0)
    {
        if (!availableDevicesGroup.assigned())
        {
            getAvailableDevices(&ListPtr<IDeviceInfo>());
        }
        
        if (availableDevicesGroup.hasKey(connectionStringPtr))
            deviceInfo = availableDevicesGroup.get(connectionStringPtr);
        
        if (!deviceInfo.assigned())
        {
            throw NotFoundException(fmt::format("Device with connection string \"{}\" not found", connectionStringPtr));
        }

        if (deviceInfo.getServerCapabilities().getCount() == 0)
        {
            throw NotFoundException(fmt::format("Device with connection string \"{}\" has no availble server capabilites", connectionStringPtr));
        }

        internalServerCapability = deviceInfo.getServerCapabilities()[0];
        for (const auto & capability : deviceInfo.getServerCapabilities())
        {
            if (capability.getProtocolType().getIntValue() < internalServerCapability.getProtocolType().getIntValue())
                internalServerCapability = capability;
        }
    } 
    else if (availableDevicesGroup.assigned())
    {
        for (const auto & [_, info] : availableDevicesGroup)
        {
            if (info.getServerCapabilities().getCount() == 0)
            {
                if (info.getConnectionString() == connectionStringPtr)
                {
                    deviceInfo = info;
                    break;
                }
            }
            else
            {
                for(const auto & capability : info.getServerCapabilities())
                {
                    if (capability.getConnectionString() == connectionStringPtr)
                    {
                        internalServerCapability = capability;
                        deviceInfo = info;
                        break;
                    }
                }
                if (deviceInfo.assigned())
                    break;
            }
        }
    }

    if (internalServerCapability.assigned())
        connectionStringPtr = internalServerCapability.getConnectionString();
    
    for (const auto module : this->borrowPtr<ModuleManagerPtr>().getModules())
    {
        bool accepted{};
        try
        {
            accepted = module.acceptsConnectionParameters(connectionStringPtr, config);
        }
        catch (NotImplementedException&)
        {
            if (loggerComponent.assigned())
                LOG_I("{}: AcceptsConnectionString not implemented", module.getName())
            accepted = false;
        }
        catch (const std::exception& e)
        {
            if (loggerComponent.assigned())
                LOG_W("{}: AcceptsConnectionString failed: {}", module.getName(), e.what())
            accepted = false;
        }

        if (accepted)
        {
            auto createdDevice = module.createDevice(connectionStringPtr, parent, config);
            if (deviceInfo.assigned())
            {
                auto deviceInfoConfig = createdDevice.getInfo().asPtr<IDeviceInfoPrivate>();
                for (const auto & capability : deviceInfo.getServerCapabilities())
                    deviceInfoConfig.addServerCapability(capability);
            }
            *device = createdDevice.detach();
            return OPENDAQ_SUCCESS;
        }
    }
    throw NotFoundException{"No module supports the specified connection string and configuration [{}]", connectionStringPtr};
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

END_NAMESPACE_OPENDAQ
