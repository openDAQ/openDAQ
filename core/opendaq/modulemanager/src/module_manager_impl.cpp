#include <coreobjects/property_factory.h>
#include <opendaq/module_manager_impl.h>
#include <opendaq/module_manager_ptr.h>
#include <opendaq/transport_option_impl.h>
#include <opendaq/custom_log.h>
#include <opendaq/module_ptr.h>
#include <opendaq/module_manager_exceptions.h>
#include <opendaq/module_library.h>
#include <boost/dll/runtime_symbol_info.hpp>
#include <opendaq/orphaned_modules.h>
#include <opendaq/device_info_config_ptr.h>
#include <opendaq/device_info_internal_ptr.h>
#include <coretypes/ctutils.h>
#include <coretypes/validation.h>
#include <opendaq/device_private.h>
#include <string>
#include <future>
#include <boost/algorithm/string.hpp>
#include <opendaq/search_filter_factory.h>
#include <coretypes/validation.h>
#include <opendaq/server_capability_config_ptr.h>
#include <opendaq/icmp_ping.h>
#include <coreobjects/property_object_factory.h>
#include <coreobjects/property_factory.h>
#include <opendaq/mirrored_signal_config_ptr.h>
#include <map>
#include <opendaq/security_exceptions.h>
#include <opendaq/version.h>
#include <opendaq/logger_factory.h>
#include <opendaq/device_info_factory.h>
#include <opendaq/address_info_private_ptr.h>
#include <coreobjects/property_object_protected_ptr.h>
#include <coreobjects/property_internal_ptr.h>
#include <coreobjects/property_object_internal_ptr.h>
#include <coreobjects/eval_value_factory.h>
#include <opendaq/client_type.h>
#include <opendaq/add_device_config_legacy.h>
#include <opendaq/add_device_config_private_ptr.h>
#include <opendaq/address_info_utils.h>
#include <opendaq/network_interface_factory.h>
#include <opendaq/component_private_ptr.h>
#include <opendaq/component_type_private_ptr.h>
#include <opendaq/device_type_ptr.h>
#include <opendaq/streaming_type_ptr.h>
#include <opendaq/connection_string_utils.h>
#include <opendaq/authentication_method_ptr.h>
#include <opendaq/authentication_method_internal_ptr.h>
#include <opendaq/authentication_config_ptr.h>
#include <opendaq/credential_provider_ptr.h>
#include <opendaq/credential_request_factory.h>
#include <opendaq/login_parameters_factory.h>
#include <opendaq/module_impl.h>
#include <coretypes/dictobject_factory.h>

#include <opendaq/module_manager_check_dependencies.h>
#include <opendaq/add_component_config_factory.h>
#include <opendaq/add_streaming_config_ptr.h>
#include <opendaq/add_streaming_options_ptr.h>
#include <opendaq/add_device_config_impl.h>
#include <coretypes/integer_factory.h>
#include <opendaq/streaming_type_utils.h>

BEGIN_NAMESPACE_OPENDAQ

using namespace std::chrono_literals;

static OrphanedModules orphanedModules;

static constexpr std::chrono::milliseconds DefaultrescanTimer = 5000ms;
static constexpr char createModuleFactory[] = "createModule";
static constexpr char checkDependenciesFunc[] = "checkDependencies";
static constexpr char getCoreVersionMetadataFunc[] = "getCoreVersionMetadata";
static void GetModulesPath(std::vector<fs::path>& modulesPath, const LoggerComponentPtr& loggerComponent, std::string searchFolder);
static ModuleLibrary loadModuleInternal(const LoggerComponentPtr& loggerComponent, const fs::path& path, IContext* context, Bool safeLoadingMode);

// Temporary bridge, until `IAuthenticationConfig` becomes a real part of the add-device/add-streaming
// config schema (see `IDevice::createDefaultAddDeviceConfig`). Until then, a caller smuggles the `IAuthenticationConfig`
// through the plain `config` property object under this property name instead. Not part of the public add-device config schema - a hack,
// deliberately, until the real integration replaces it. Any other place that needs to smuggle one through
// (e.g. an application, or `GenericDevice::updateDevice`'s reload path)
// must match this exact key.
static constexpr const char* AuthenticationConfigConfigKey = "__AuthenticationConfig";

// The module manager's own extraction of the `IAuthenticationConfig` smuggled onto an add-device/add-streaming
// `config` (see `AuthenticationConfigConfigKey`) - returns unassigned if `config` is unassigned or carries none.
inline AuthenticationConfigPtr ExtractAuthenticationConfig(const PropertyObjectPtr& config)
{
    if (!config.assigned() || !config.hasProperty(AuthenticationConfigConfigKey))
        return nullptr;
    return config.getPropertyValue(AuthenticationConfigConfigKey).asPtrOrNull<IAuthenticationConfig>();
}

// Only Root devices are created as the root. Unspecified devices can be created either way.
inline bool IsKindAllowed(DeviceKind kind, bool asChild)
{
    return kind == DeviceKind::Unspecified || (kind == DeviceKind::Root) != asChild;
}

inline ErrCode CheckKindAllowed(const DeviceTypePtr& deviceType, bool asChild)
{
    if (IsKindAllowed(deviceType.getDeviceKind(), asChild))
        return OPENDAQ_SUCCESS;

    if (asChild)
    {
        return DAQ_MAKE_ERROR_INFO(
            OPENDAQ_ERR_INVALIDPARAMETER,
            fmt::format(R"(Device type "{}" is of Root kind and can only be set as the root device)", deviceType.getId()));
    }
    return DAQ_MAKE_ERROR_INFO(
        OPENDAQ_ERR_INVALIDPARAMETER,
        fmt::format(R"(Device type "{}" is not of Root kind and cannot be set as the root device)", deviceType.getId()));
}

template <typename TypePtr, typename Types>
TypePtr FindTypeByPrefix(const Types& types, const StringPtr& prefix)
{
    for (const auto& [_, type] : types)
        if (type.getConnectionStringPrefix() == prefix)
            return type;
    return nullptr;
}

// The transport option the type config of a device or streaming type names, or unassigned if it names none
inline StringPtr GetSelectedTransportOption(const PropertyObjectPtr& typeConfig)
{
    if (!typeConfig.assigned() || !typeConfig.hasProperty(TransportOptionConfigProperty))
        return nullptr;
    return typeConfig.getPropertyValue(TransportOptionConfigProperty);
}

// Fails with OPENDAQ_ERR_NOTFOUND when the type config names a transport option the type does not list
inline ErrCode CheckSelectedTransportOption(const ComponentTypePtr& type, const PropertyObjectPtr& typeConfig)
{
    const StringPtr selected = GetSelectedTransportOption(typeConfig);
    if (!selected.assigned())
        return OPENDAQ_SUCCESS;

    ListPtr<ITransportOption> options;
    if (OPENDAQ_FAILED(type->getTransportOptions(&options)))
    {
        daqClearErrorInfo();
        return OPENDAQ_SUCCESS;
    }

    // A type that lists no options leaves the property to its module
    if (!options.assigned() || options.getCount() == 0 || FindTransportOption(options, selected).assigned())
        return OPENDAQ_SUCCESS;

    std::string names;
    for (const auto& option : options)
        names += (names.empty() ? "" : ", ") + option.getName().toStdString();

    return DAQ_MAKE_ERROR_INFO(OPENDAQ_ERR_NOTFOUND,
                               fmt::format("The {} type '{}' has no transport option '{}'; it lists: {}",
                                           type.asPtrOrNull<IStreamingType>().assigned() ? "streaming" : "device",
                                           type.getId(),
                                           selected,
                                           names));
}

// The transport options the server of a device info announces for the type with the given connection string prefix;
// unassigned when it announces none
inline ListPtr<ITransportOption> AnnouncedTransportOptions(const ListPtr<IServerCapability>& capabilities, const StringPtr& prefix)
{
    for (const auto& capability : capabilities)
    {
        if (capability.getPrefix() != prefix)
            continue;

        if (const auto options = capability.getTransportOptions(); options.assigned() && options.getCount() > 0)
            return options;
    }

    return nullptr;
}

ModuleManagerImpl::ModuleManagerImpl(const BaseObjectPtr& path, const CredentialProviderPtr& credentialProvider)
    : authenticatedModulesOnly(false)
    , moduleAuthenticator(nullptr)
    , moduleKeys(Dict<IString, IString>())
    , credentialProvider(credentialProvider)
    , modulesLoaded(false)
    , rescanTimer(DefaultrescanTimer)
    , safeLoadingMode(False)
    , scanOnAdd(True)
{
    if (const StringPtr pathStr = path.asPtrOrNull<IString>(true); pathStr.assigned())
    {
        paths.push_back(pathStr.toStdString());
    }
    else if (const ListPtr<IString> pathList = path.asPtrOrNull<IList>(true); pathList.assigned())
    {
        paths.insert(paths.end(), pathList.begin(), pathList.end());
    }
    else
    {
        DAQ_THROW_EXCEPTION(InvalidParameterException);
    }

    if (paths.empty())
        DAQ_THROW_EXCEPTION(InvalidParameterException, "No valid paths provided!");

    discoveryClient.initMdnsClient(List<IString>(discovery_common::IpModificationUtils::DAQ_IP_MODIFICATION_SERVICE_NAME));
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
    OPENDAQ_PARAM_NOT_NULL(availableModules);

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
    OPENDAQ_PARAM_NOT_NULL(module);

    if (authenticatedModulesOnly)
    {
        return DAQ_MAKE_ERROR_INFO(OPENDAQ_ERR_INVALID_OPERATION, "Cannot add modules directly without verification!");
    }

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
        const auto modulePtr = ModulePtr::Borrow(module);
        if (const auto violation = CheckLinkedStreamingTypes(modulePtr); !violation.empty())
        {
            return DAQ_MAKE_ERROR_INFO(OPENDAQ_ERR_MODULE_LOAD_FAILED,
                                       fmt::format(R"(Module with id "{}" rejected: {})", modulePtr.getModuleInfo().getId(), violation));
        }
        logUnspecifiedDeviceKinds(modulePtr);

        libraries.emplace_back(ModuleLibrary{{}, module, nullptr});
        return OPENDAQ_SUCCESS;
    }
    return DAQ_MAKE_ERROR_INFO(OPENDAQ_ERR_DUPLICATEITEM);
}

ErrCode ModuleManagerImpl::loadModules(IContext* context)
{
    if (authenticatedModulesOnly && !moduleAuthenticator.assigned())
    {
        return DAQ_MAKE_ERROR_INFO(OPENDAQ_ERR_INVALID_OPERATION, "ModuleAuthenticator missing, cannot load modules!");
    }

    if (!modulesLoaded)
    {
        this->context = ContextPtr::Borrow(context);
        logger = this->context.getLogger();
        if (!logger.assigned())
            return DAQ_MAKE_ERROR_INFO(OPENDAQ_ERR_ARGUMENT_NULL, "Logger must not be null");

        auto options = this->context.getOptions();
        if (options.hasKey("ModuleManager"))
        {
            DictPtr<IString, IBaseObject> inner = options.get("ModuleManager");
            if (inner.hasKey("AddDeviceRescanTimer"))
            {
                this->rescanTimer = std::chrono::milliseconds(static_cast<int>(inner.get("AddDeviceRescanTimer")));
            }
            if (inner.hasKey("SafeLoadingMode"))
            {
                this->safeLoadingMode = static_cast<bool>(inner.get("SafeLoadingMode"));
            }
            if (inner.hasKey("ScanOnAdd"))
            {
                this->scanOnAdd = static_cast<bool>(inner.get("ScanOnAdd"));
            }
        }

        loggerComponent = this->logger.getOrAddComponent("ModuleManager");
    }
    else if (this->context != ContextPtr::Borrow(context))
    {
        return DAQ_MAKE_ERROR_INFO(OPENDAQ_ERR_INVALIDPARAMETER, "Context cannot be changed after loading modules");
    }

    std::string sdkVersionMetadataAsString =
        fmt::format(R"([[ 'major': '{}'; 'minor': '{}'; 'patch': '{}'; 'branch': '{}'; 'sha': '{}'; ]])",
                    OPENDAQ_OPENDAQ_MAJOR_VERSION,
                    OPENDAQ_OPENDAQ_MINOR_VERSION,
                    OPENDAQ_OPENDAQ_PATCH_VERSION,
                    OPENDAQ_OPENDAQ_BRANCH_NAME,
                    OPENDAQ_OPENDAQ_REVISION_HASH);
    LOG_I("Loading modules ... the running SDK core version: \"{}\"", sdkVersionMetadataAsString);

    std::vector<std::string> paths;
    auto envPath = std::getenv("OPENDAQ_MODULES_PATH");
    if (envPath != nullptr)
    {
        LOG_D("Environment variable OPENDAQ_MODULES_PATH found, moduler search folder overriden to {}", envPath)

#if defined(_WIN32) || defined(_WIN64)
        const char sep = ';';
#else
        const char sep = ':';
#endif

        std::stringstream ss(envPath);
        std::string token;

        while (std::getline(ss, token, sep))
            paths.push_back(token);
    }
    else
    {
        paths = this->paths;
    }

    std::vector<fs::path> modulesPath;
    for (const auto& path: paths)
    {
        try
        {
            GetModulesPath(modulesPath, loggerComponent, path);
        }
        catch (const daq::DaqException& e)
        {
            LOG_W(R"(Error scanning directory "{}": {} [{:#x}])", path, e.what(), e.getErrCode())
        }
        catch (const std::exception& e)
        {
            LOG_W(R"(Error scanning directory "{}": {})", path, e.what())
        }
        catch (...)
        {
            LOG_W(R"(Unknown error occured scanning directory "{}")", path)
        }
    }

    libraries.reserve(modulesPath.size());

    orphanedModules.tryUnload();

    bool newModulesAdded = false;
    for (const auto& modulePath: modulesPath)
    {
        auto path = String(modulePath.string());
        auto errCode = tryLoadAndAddModule(path, nullptr);
        if (OPENDAQ_FAILED(errCode))
        {
            ObjectPtr<IErrorInfo> errorInfo;
            daqGetErrorInfo(&errorInfo);
            daqClearErrorInfo();

            StringPtr message;
            errorInfo->getMessage(&message);
            if (message.assigned())
            {
                LOG_W("{}", message);
            }
        }
        if (errCode == OPENDAQ_SUCCESS)
            newModulesAdded = true;
    }

    modulesLoaded = true;

    if (newModulesAdded)
        return OPENDAQ_SUCCESS;
    else
        return OPENDAQ_IGNORED;
}

ErrCode ModuleManagerImpl::loadModule(IString* path, IModule** module)
{
    OPENDAQ_PARAM_NOT_NULL(path);
    OPENDAQ_PARAM_NOT_NULL(module);

    auto pathString = StringPtr::Borrow(path);

    if (pathString.getLength() == 0)
    {
        return DAQ_MAKE_ERROR_INFO(OPENDAQ_ERR_INVALIDPARAMETER, "Specified module path is empty");
    }

    if (!boost::algorithm::ends_with(pathString.toStdString(), OPENDAQ_MODULE_SUFFIX))
    {
        return DAQ_MAKE_ERROR_INFO(OPENDAQ_ERR_INVALIDPARAMETER,
                                   fmt::format(R"(The openDAQ module file must have an extention "{}")", OPENDAQ_MODULE_SUFFIX));
    }

    if (!modulesLoaded)
    {
        return DAQ_MAKE_ERROR_INFO(OPENDAQ_ERR_INVALIDSTATE, "ModuleManager in not initialized. Call loadModules(IContext*) first.");
    }

    if (authenticatedModulesOnly && !moduleAuthenticator.assigned())
    {
        return DAQ_MAKE_ERROR_INFO(OPENDAQ_ERR_INVALID_OPERATION, "ModuleAuthenticator missing, cannot load modules!");
    }

    orphanedModules.tryUnload();

    return tryLoadAndAddModule(pathString, module);
}

ErrCode ModuleManagerImpl::tryLoadAndAddModule(const StringPtr& path, IModule** module)
{
    std::error_code errCode;
    fs::path fileSystemPath(path.toStdString());

    if (!fs::exists(fileSystemPath, errCode))
    {
        return DAQ_MAKE_ERROR_INFO(OPENDAQ_ERR_INVALIDPARAMETER, fmt::format(R"(Specified module path "{}" does not exist)", path));
    }

    if (!fs::is_regular_file(fileSystemPath, errCode))
    {
        return DAQ_MAKE_ERROR_INFO(OPENDAQ_ERR_INVALIDPARAMETER, fmt::format(R"(Specified module path "{}" is not a file)", path));
    }

    {
        auto iter = std::find_if(
            libraries.begin(),
            libraries.end(),
            [&path](const ModuleLibrary& lib)
            {
                return lib.path.assigned() &&
                       lib.path == path;
            }
        );
        if (iter != libraries.end())
        {
            LOG_W(R"(Module was already loaded and added from the same path: "{}".)",  path);
            if (module != nullptr)
                *module = iter->module.addRefAndReturn();
            return OPENDAQ_IGNORED;
        }
    }

    StringPtr moduleKey;
    if (authenticatedModulesOnly)
    {
        Bool valid = false;
        auto authErrCode = moduleAuthenticator->authenticateModuleBinary(&valid, &moduleKey, path);
        if (OPENDAQ_FAILED(authErrCode))
        {
            const auto authErrMsg = getErrorInfoMessage(authErrCode, true);
            return DAQ_MAKE_ERROR_INFO(authErrCode, fmt::format(R"(Error authenticating module "{}" [{:#x}]: {})", path, authErrCode, authErrMsg));
        }
        if (!valid)
            return DAQ_MAKE_ERROR_INFO(OPENDAQ_ERR_ACCESSDENIED, fmt::format(R"(Module ({}) authentication failed!)", path));
    }

    try
    {
        auto moduleLibrary = loadModuleInternal(loggerComponent, fileSystemPath, context, safeLoadingMode);
        const auto loadedModule = moduleLibrary.module;
        const StringPtr moduleId = loadedModule.getModuleInfo().getId();

        if (moduleId.assigned() && moduleId.getLength() > 0)
        {
            auto iter = std::find_if(
                libraries.begin(),
                libraries.end(),
                [&moduleId](const ModuleLibrary& lib)
                {
                    const StringPtr addedModuleId = lib.module.getModuleInfo().getId();
                    return addedModuleId.assigned() &&
                           addedModuleId.getLength() > 0 &&
                           addedModuleId == moduleId;
                }
            );
            if (iter != libraries.end())
            {
                if (const auto existingPath = iter->path; existingPath.assigned())
                {
                    if (existingPath != path)
                    {
                        return DAQ_MAKE_ERROR_INFO(OPENDAQ_ERR_ALREADYEXISTS,
                                                   fmt::format(R"(Module with id "{}" was already loaded and added from path "{}". Reject loading module from "{}")", moduleId, existingPath, path));
                    }
                }
                else
                {
                    return DAQ_MAKE_ERROR_INFO(OPENDAQ_ERR_ALREADYEXISTS,
                                               fmt::format(R"(Module with id "{}" was already loaded from memory. Reject loading module from path "{}")", moduleId, path));
                }
            }
        }

        if (const auto violation = CheckLinkedStreamingTypes(loadedModule); !violation.empty())
        {
            return DAQ_MAKE_ERROR_INFO(
                OPENDAQ_ERR_MODULE_LOAD_FAILED,
                fmt::format(R"(Module with id "{}" rejected: {}. Reject loading module from "{}")", moduleId, violation, path));
        }
        logUnspecifiedDeviceKinds(loadedModule);

        if (authenticatedModulesOnly && moduleId.assigned() && moduleId.getLength() > 0)
        {
            moduleKeys.set(moduleId, moduleKey);
        }

        if (module != nullptr)
            *module = moduleLibrary.module.addRefAndReturn();

        if (auto version = loadedModule.getModuleInfo().getVersionInfo(); version.assigned())
        {
            LOG_I("Loaded & added module [v{}.{}.{} \"{}\"] from \"{}\".",
                  version.getMajor(),
                  version.getMinor(),
                  version.getPatch(),
                  loadedModule.getModuleInfo().getName(),
                  path);
        }
        else
        {
            LOG_W("Loaded & added module UNKNOWN VERSION of \"{}\" from \"{}\".", loadedModule.getModuleInfo().getName(), path);
        }
        if (!moduleId.assigned() || moduleId.getLength() == 0)
        {
            LOG_W("Empty or missing module id of \"{}\" from \"{}\".", loadedModule.getModuleInfo().getName(), path);
        }
        printAvailableTypes(loadedModule);

        libraries.push_back(std::move(moduleLibrary));
    }
    catch (const daq::DaqException& e)
    {
        return DAQ_MAKE_ERROR_INFO(e.getErrCode(), fmt::format(R"(Error loading module "{}": {} [{:#x}])", path, e.what(), e.getErrCode()));
    }
    catch (const std::exception& e)
    {
        return DAQ_MAKE_ERROR_INFO(OPENDAQ_ERR_GENERALERROR, fmt::format(R"(Error loading module "{}": {})", path, e.what()));
    }
    catch (...)
    {
        return DAQ_MAKE_ERROR_INFO(OPENDAQ_ERR_GENERALERROR, fmt::format(R"(Unknown error occurred loading module "{}")", path));
    }

    return OPENDAQ_SUCCESS;
}

ErrCode ModuleManagerImpl::setAuthenticatedOnly(Bool authOnly)
{
    authenticatedModulesOnly = authOnly;
    return OPENDAQ_SUCCESS;
}

ErrCode ModuleManagerImpl::setModuleAuthenticator(IModuleAuthenticator* authenticator)
{
    OPENDAQ_PARAM_NOT_NULL(authenticator);

    moduleAuthenticator = authenticator;
    return OPENDAQ_SUCCESS;
}

ErrCode ModuleManagerImpl::getVendorKeys(IDict** vendorKeys)
{
    OPENDAQ_PARAM_NOT_NULL(vendorKeys);

    DictPtr<IString, IString> dict(moduleKeys);
    *vendorKeys = dict.detach();

    return OPENDAQ_SUCCESS;
}

void ModuleManagerImpl::checkNetworkSettings(ListPtr<IDeviceInfo>& list)
{
    std::vector<std::string> addresses;
    for (const auto& deviceInfo : list)
        for (const auto& cap : deviceInfo.getServerCapabilities())
            if (cap.getConnectionType() == "TCP/IP")
                for (const auto& info : cap.getAddressInfo())
                    if (info.getType() == "IPv4" || info.getType() == "IPv6")
                        addresses.push_back(info.getAddress().toStdString());

    setAddressesReachable(icmpPing(addresses, 1, std::chrono::seconds(1)), list);
}

void ModuleManagerImpl::setAddressesReachable(const std::map<std::string, AddressReachabilityStatus>& addr, ListPtr<IDeviceInfo>& info)
{
    for (const auto& devInfo : info)
    {
        for (const auto& cap : devInfo.getServerCapabilities())
        {
            const auto addressInfos = cap.getAddressInfo();
            for (const auto& addressInfo : addressInfos)
            {
                if (addressInfo.getType() != "IPv4" && addressInfo.getType() != "IPv6")
                    continue;

                const auto it = addr.find(addressInfo.getAddress().toStdString());
                const auto reachability = it != addr.end() ? it->second : AddressReachabilityStatus::Unknown;
                addressInfo.asPtr<IAddressInfoPrivate>(true).setReachabilityStatusPrivate(reachability);
            }

            // Prefer Reachable, else the first Unknown; addresses are listed IPv4 first, so IPv4 stays primary without ICMP.
            StringPtr unknownConnectionString;
            bool primarySet = false;
            for (const auto& addressInfo : addressInfos)
            {
                auto reachability = addressInfo.getReachabilityStatus();
                if (reachability == AddressReachabilityStatus::Reachable)
                {
                    cap.asPtr<IServerCapabilityConfig>(true).setConnectionString(addressInfo.getConnectionString());
                    primarySet = true;
                    break;
                }
                if (reachability == AddressReachabilityStatus::Unknown && !unknownConnectionString.assigned())
                    unknownConnectionString = addressInfo.getConnectionString();
            }
            if (!primarySet && unknownConnectionString.assigned())
                cap.asPtr<IServerCapabilityConfig>(true).setConnectionString(unknownConnectionString);
        }
    }
}

ErrCode ModuleManagerImpl::getAvailableDevices(IList** availableDevices)
{
    OPENDAQ_PARAM_NOT_NULL(availableDevices);
    auto availableDevicesPtr = List<IDeviceInfo>();

    using AsyncEnumerationResult = std::future<ListPtr<IDeviceInfo>>;
    std::vector<std::pair<AsyncEnumerationResult, ModulePtr>> enumerationResults;

    // runs in parallel with getting available devices from modules
    std::future<DictPtr<IString, IDeviceInfo>> devicesWithIpModSupportAsyncResult = std::async([this]
    {
        return this->discoverDevicesWithIpModification();
    });

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
                  module.getModuleInfo().getName(),
                  e.what()
            )
        }
    }

    auto devicesWithIpModSupport = devicesWithIpModSupportAsyncResult.get();
    auto groupedDevices = Dict<IString, IDeviceInfo>();
    for (auto& [futureResult, module] : enumerationResults)
    {
        ListPtr<IDeviceInfo> moduleAvailableDevices;
        try
        {
            moduleAvailableDevices = futureResult.get();
        }
        catch (NotImplementedException&)
        {
            LOG_I("{}: GetAvailableDevices not implemented", module.getModuleInfo().getName())
        }
        catch (const std::exception& e)
        {
            LOG_W("{}: GetAvailableDevices failed: {}", module.getModuleInfo().getName(), e.what())
        }

        if (!moduleAvailableDevices.assigned())
            continue;

        for (const auto& deviceInfo : moduleAvailableDevices)
        {
            const auto deviceType = deviceInfo.getDeviceType();
            if (deviceType.assigned() && deviceType.getDeviceKind() == DeviceKind::Root)
                continue;

            StringPtr manufacturer = deviceInfo.getManufacturer();
            StringPtr serialNumber = deviceInfo.getSerialNumber();

            // Group devices that have manufacturer, serial number and at least 1 server capability,
            // the rest use their connection string as key.
            if (manufacturer.getLength() == 0 || serialNumber.getLength() == 0 || !deviceInfo.getServerCapabilities().getCount())
            {
                groupedDevices.set(deviceInfo.getConnectionString(), deviceInfo);
            }
            else
            {
                StringPtr id = "daq://" + manufacturer + "_" + serialNumber;

                if (groupedDevices.hasKey(id))
                {
                    DeviceInfoConfigPtr value = groupedDevices.get(id);
                    DeviceInfoInternalPtr valueInternal = value;
                    for (const auto & capability : deviceInfo.getServerCapabilities())
                        if (!value.hasServerCapability(capability.getProtocolId()))
                            valueInternal.addServerCapability(capability);
                }
                else
                {
                    if (devicesWithIpModSupport.hasKey(id))
                    {
                        DeviceInfoConfigPtr value = devicesWithIpModSupport.get(id);
                        DeviceInfoInternalPtr valueInternal = value;
                        for (const auto & capability : deviceInfo.getServerCapabilities())
                            if (!value.hasServerCapability(capability.getProtocolId()))
                                valueInternal.addServerCapability(capability);
                        value.asPtr<IPropertyObjectProtected>().setProtectedPropertyValue("connectionString", id);
                        groupedDevices.set(id, value);
                    }
                    else
                    {
                        deviceInfo.asPtr<IPropertyObjectProtected>().setProtectedPropertyValue("connectionString", id);
                        groupedDevices.set(id, deviceInfo);
                    }
                }
            }
        }
    }

    for (const auto & [_, deviceInfo] : groupedDevices)
        availableDevicesPtr.pushBack(deviceInfo);

    try
    {
        checkNetworkSettings(availableDevicesPtr);
    }
    catch(const std::exception& e)
    {
        LOG_W("Failed to check for network settings: {}", e.what());
        // ignore all errors
    }

    for (const auto& dev : availableDevicesPtr)
    {
        if (!dev.isFrozen())
            dev.freeze();
    }

    availableDevicesGroup = groupedDevices;

    auto visibleDevices = List<IDeviceInfo>();
    for (const auto & [_, deviceInfo]: groupedDevices)
    {
        bool deviceHidden = false;
        if (deviceInfo.hasProperty("hidden"))
            deviceHidden = deviceInfo.getPropertyValue("hidden");

        if (!deviceHidden)
            visibleDevices.pushBack(deviceInfo);
    }

    *availableDevices = visibleDevices.detach();
    lastScanTime = std::chrono::steady_clock::now();
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
            LOG_I("{}: GetAvailableDeviceTypes not implemented", module.getModuleInfo().getName())
        }
        catch ([[maybe_unused]] const std::exception& e)
        {
            LOG_W("{}: GetAvailableDeviceTypes failed: {}", module.getModuleInfo().getName(), e.what())
        }

        if (!moduleDeviceTypes.assigned())
            continue;

        for (const auto& [id, type] : moduleDeviceTypes)
            availableTypes.set(id, type);
    }

    *deviceTypes = availableTypes.detach();
    return OPENDAQ_SUCCESS;
}

AuthenticationConfigPtr ModuleManagerImpl::resolveDefaultAuthenticationConfig(const AuthenticationConfigPtr& authenticationConfig, const ComponentTypePtr& componentType) const
{
    if (authenticationConfig.assigned())
        return authenticationConfig;
    if (!componentType.asPtr<IComponentTypePrivate>().getSupportedAuthenticationMethods().getCount())
        return nullptr;
    return componentType.createAuthenticationConfig();
}

DictPtr<IString, IString> ModuleManagerImpl::resolveCredentials(const AuthenticationConfigPtr& authenticationConfig, const CredentialRequestPtr& credentialRequest) const
{
    // A field-less method (e.g. "Anonymous") needs no credentials at all - no provider consultation, no
    // supplied-credential check, regardless of whether one is registered.
    if (authenticationConfig.getSelectedAuthenticationMethod().getFields().getCount() == 0)
        return Dict<IString, IString>();

    const DictPtr<IString, IString> suppliedCredential = authenticationConfig.getSuppliedCredential();

    if (suppliedCredential.assigned() && suppliedCredential.getCount() > 0)
    {
        // Already has a non-empty value for every field the method requires (filled in by the caller), so it
        // is used directly as the credential, no provider asked to obtain anything.
        return suppliedCredential;
    }

    if (!credentialProvider.assigned())
    {
        DAQ_THROW_EXCEPTION(CredentialsRequiredException, "Authentication credentials are required but no credential provider is registered");
    }

    const DictPtr<IString, IString> credentials = credentialProvider.requestCredentials(credentialRequest);
    if (!credentialRequest.getAuthenticationMethod().asPtr<IAuthenticationMethodInternal>().acceptsCredentials(credentials))
    {
        DAQ_THROW_EXCEPTION(CredentialsRequiredException, "Credential provider returned credentials missing one or more required fields");
    }

    return credentials;
}

ErrCode ModuleManagerImpl::createDevice(IDevice** device, IString* connectionString, IComponent* parent, IPropertyObject* config)
{
    OPENDAQ_PARAM_NOT_NULL(connectionString);
    OPENDAQ_PARAM_NOT_NULL(device);
    *device = nullptr;

    DevicePtr devicePtr;
    const ErrCode errCode = wrapHandlerReturn(this, &ModuleManagerImpl::onCreateDevice, devicePtr, connectionString, parent, config);
    OPENDAQ_RETURN_IF_FAILED(
        errCode, fmt::format("Failed to create device from connection string '{}' and config", StringPtr::Borrow(connectionString)));

    *device = devicePtr.detach();
    return errCode;
}

DevicePtr ModuleManagerImpl::onCreateDevice(const StringPtr& connectionString,
                                            const ComponentPtr& parent,
                                            const PropertyObjectPtr& inputConfig)
{
    // Temporary bridge (see `ExtractAuthenticationConfig`): a caller that wants an authenticated connection
    // smuggles an `IAuthenticationConfig` onto the top-level `config` it passes in, rather than a dedicated
    // parameter.
    const AuthenticationConfigPtr authenticationConfig = ExtractAuthenticationConfig(inputConfig);

    // Parse the connection string
    const auto [pureConnectionString, connectionStringOptions] = SplitConnectionStringAndOptions(connectionString);
    auto connectionStringPtr = String(pureConnectionString);
    if (connectionStringPtr.getLength() == 0)
    {
        DAQ_THROW_EXCEPTION(ArgumentNullException, "Connection string is not set or empty");
    }

    const bool asChild = parent.assigned();
    const std::string prefix = getPrefixFromConnectionString(connectionStringPtr);
    const bool useSmartConnection = (prefix == "daq");

    DictPtr<IString, IDeviceType> deviceTypes;
    checkErrorInfo(getAvailableDeviceTypes(&deviceTypes));
    DictPtr<IString, IStreamingType> streamingTypes;
    checkErrorInfo(getAvailableStreamingTypes(&streamingTypes));

    // Scan for devices
    const auto inputAddDeviceConfig = inputConfig.asPtrOrNull<IAddDeviceConfig>(true);
    const bool scanForDevices = inputAddDeviceConfig.assigned() ? static_cast<bool>(inputAddDeviceConfig.getScanOnAdd())
                                                                : static_cast<bool>(getLegacyScanOnAdd(inputConfig, scanOnAdd));
    if (scanForDevices)
    {
        auto lock = std::lock_guard(availableDevicesSearchSync);
        const auto currentTime = std::chrono::steady_clock::now();
        if (!availableDevicesGroup.assigned() || currentTime - lastScanTime > rescanTimer)
            checkErrorInfo(getAvailableDevices(&ListPtr<IDeviceInfo>()));
    }

    // Find the discovered device and its capabilities
    const auto findDiscoveredDeviceInfo = [this](const StringPtr& protocolConnectionString) -> DeviceInfoPtr
    {
        if (!availableDevicesGroup.assigned())
            return nullptr;
        if (availableDevicesGroup.hasKey(protocolConnectionString))
            return availableDevicesGroup.get(protocolConnectionString);

        const auto isConnectionString = [&protocolConnectionString](const StringPtr& str)
        {
            return str == protocolConnectionString;
        };
        for (const auto& [_, info] : availableDevicesGroup)
        {
            for (const auto& capability : info.getServerCapabilities())
            {
                const ListPtr<IString> connectionStrings = capability.getConnectionStrings();
                if (std::any_of(connectionStrings.begin(), connectionStrings.end(), isConnectionString))
                    return info;
            }
        }
        return nullptr;
    };

    DeviceInfoPtr discoveredDeviceInfo;
    ListPtr<IServerCapability> capabilities = List<IServerCapability>();
    if (useSmartConnection)
    {
        discoveredDeviceInfo = getSmartConnectionDeviceInfo(connectionStringPtr);
        capabilities = discoveredDeviceInfo.getServerCapabilities();
        if (capabilities.getCount() == 0)
        {
            DAQ_THROW_EXCEPTION(
                NotFoundException, R"(Device with connection string "{}" has no available server capabilities)", connectionStringPtr);
        }
    }
    else if (const auto info = findDiscoveredDeviceInfo(connectionStringPtr); info.assigned())
        capabilities = info.getServerCapabilities();

    // Turn the input into an add device config. No code below reads the input config.
    const auto selectDeviceType = [&]() -> DeviceTypePtr
    {
        if (!useSmartConnection)
        {
            const auto deviceType = FindTypeByPrefix<DeviceTypePtr>(deviceTypes, prefix);
            if (!deviceType.assigned())
            {
                DAQ_THROW_EXCEPTION(NotFoundException, R"(No loaded device type for the prefix "{}")", prefix);
            }
            return deviceType;
        }

        const auto capability = SelectServerCapability(capabilities, deviceTypes, asChild);
        if (!capability.assigned())
        {
            DAQ_THROW_EXCEPTION(NotFoundException,
                                R"(Device with connection string "{}" has no server capability with a loaded device type)",
                                connectionStringPtr);
        }
        return FindTypeByPrefix<DeviceTypePtr>(deviceTypes, getServerCapabilityPrefix(capability));
    };

    const auto toAddDeviceConfig = [&]() -> AddDeviceConfigPtr
    {
        if (inputAddDeviceConfig.assigned())
            return inputAddDeviceConfig.asPtr<IAddDeviceConfigPrivate>().bind(deviceTypes, streamingTypes);

        const auto deviceType = selectDeviceType();
        if (inputConfig.assigned())
            return fromLegacyAddDeviceConfig(inputConfig, deviceType, streamingTypes, scanOnAdd, loggerComponent);
        return createAddDeviceConfigForType(deviceType, capabilities, streamingTypes);
    };

    const AddDeviceConfigPtr addDeviceConfig = toAddDeviceConfig();

    // Check the device type and resolve the address
    const auto resolveConnectionString = [&](const AddDeviceConfigPtr& config) -> StringPtr
    {
        const auto deviceType = config.getDeviceType();
        const StringPtr typePrefix = deviceType.getConnectionStringPrefix();
        if (!useSmartConnection)
        {
            if (prefix != typePrefix)
            {
                DAQ_THROW_EXCEPTION(InvalidParameterException,
                                    R"(Prefix "{}" differs from the prefix "{}" of the device type "{}")",
                                    prefix,
                                    typePrefix,
                                    deviceType.getId());
            }
            return connectionStringPtr;
        }

        const auto hasTypePrefix = [&typePrefix](const ServerCapabilityPtr& capability)
        {
            return getServerCapabilityPrefix(capability) == typePrefix;
        };
        const auto it = std::find_if(capabilities.begin(), capabilities.end(), hasTypePrefix);
        if (it == capabilities.end())
        {
            DAQ_THROW_EXCEPTION(NotFoundException,
                                R"(Device with connection string "{}" has no server capability with the prefix "{}")",
                                connectionStringPtr,
                                typePrefix);
        }
        return resolveSmartConnectionString(*it, config.getPrimaryAddressType());
    };

    const DeviceTypePtr deviceType = addDeviceConfig.getDeviceType();
    checkErrorInfo(CheckKindAllowed(deviceType, asChild));
    connectionStringPtr = resolveConnectionString(addDeviceConfig);

    // Connect through the module that has the device type
    PropertyObjectPtr deviceTypeConfig = addDeviceConfig.getTypeConfig();
    PopulateDeviceTypeConfigFromConnStrOptions(deviceTypeConfig, connectionStringOptions);

    // The module connects by TransportOptionConfigProperty and PortConfigProperty. An IAddDeviceConfig input or a
    // created config hands over its own selection; a legacy input keeps what its type config names. The selection is
    // read from the caller's config: binding takes the loaded type's options, while the caller's config lists the
    // options and ports the discovered server announces, so its port is the one to connect at.
    if (addDeviceConfig.getTransportOptions().getCount() > 0)
    {
        AddTransportSelectionProperties(deviceTypeConfig, addDeviceConfig.getTransportOptions());
        if (inputAddDeviceConfig.assigned() || !inputConfig.assigned())
        {
            const auto selection = inputAddDeviceConfig.assigned() && inputAddDeviceConfig.getTransportOptions().getCount() > 0
                                       ? inputAddDeviceConfig
                                       : addDeviceConfig;
            const IntegerPtr port = selection.getPort();
            deviceTypeConfig.setPropertyValue(TransportOptionConfigProperty, selection.getSelectedTransportOption().getName());
            deviceTypeConfig.setPropertyValue(PortConfigProperty, port);
            connectionStringPtr = ReplaceConnectionStringPort(connectionStringPtr, port);
        }
        else
        {
            connectionStringPtr = applySelectedTransportPort(
                connectionStringPtr, findDiscoveredCapability(connectionStringPtr, discoveredDeviceInfo), deviceTypeConfig);
        }
        checkErrorInfo(CheckSelectedTransportOption(deviceType, deviceTypeConfig));
    }

    const DevicePtr device = connectDevice(connectionStringPtr, parent, deviceType, deviceTypeConfig, authenticationConfig);
    if (device.assigned())
    {
        onCompleteCapabilities(device, discoveredDeviceInfo);
        if (const auto& componentPrivate = device.asPtrOrNull<IComponentPrivate>(true); componentPrivate.assigned())
            componentPrivate.setComponentConfig(addDeviceConfig);
    }
    return device;
}

DevicePtr ModuleManagerImpl::connectDevice(const StringPtr& connectionStringPtr,
                                           const ComponentPtr& parent,
                                           const DeviceTypePtr& deviceType,
                                           const PropertyObjectPtr& deviceTypeConfig,
                                           const AuthenticationConfigPtr& authenticationConfig)
{
    const bool authenticationConfigSupplied = authenticationConfig.assigned();

    const auto hasDeviceType = [&deviceType](const ModulePtr& module)
    {
        DictPtr<IString, IDeviceType> types;
        const ErrCode err = module->getAvailableDeviceTypes(&types);
        if (err == OPENDAQ_ERR_NOTIMPLEMENTED)
            daqClearErrorInfo();
        else
            checkErrorInfo(err);

        return types.assigned() && types.hasKey(deviceType.getId());
    };

    for (const auto& library : libraries)
    {
        if (!hasDeviceType(library.module))
            continue;

        const bool authenticationSupported = deviceType.asPtr<IComponentTypePrivate>().getSupportedAuthenticationMethods().getCount() > 0;

        if (authenticationConfigSupplied && !authenticationSupported)
        {
            LOG_W("Device with given connection string '{}' does not support authentication; creating it without authentication.", connectionStringPtr);
        }

        // Any supported authentication method - field-less (e.g. "Anonymous") included - goes through
        // `createAuthenticatedDevice`; `createDevice` is only for a type with none supported at all.
        const AuthenticationConfigPtr resolvedAuthConfig =
            authenticationSupported ? resolveDefaultAuthenticationConfig(authenticationConfig, deviceType) : nullptr;

        DevicePtr device;
        DictPtr<IString, IString> credentials;
        ErrCode err;
        if (authenticationSupported)
        {
            const auto credentialRequest = CredentialRequestBuilder()
                                               .setConnectionString(library.module.getCanonicalConnectionString(connectionStringPtr))
                                               .setAuthenticationMethod(resolvedAuthConfig.getSelectedAuthenticationMethod())
                                               .setComponentType(deviceType)
                                               .build();
            credentials = resolveCredentials(resolvedAuthConfig, credentialRequest);

            auto loginParameters = LoginParametersBuilder()
                                       .setAuthenticationMethod(resolvedAuthConfig.getSelectedAuthenticationMethod())
                                       .setCredentials(credentials)
                                       .build();
            err = library.module->createAuthenticatedDevice(&device, connectionStringPtr, parent, deviceTypeConfig, loginParameters);
        }
        else
        {
            err = library.module->createDevice(&device, connectionStringPtr, parent, deviceTypeConfig);
        }

        checkErrorInfo(err);

        if (device.assigned())
        {
            ModuleInfoPtr moduleInfo;
            checkErrorInfo(library.module->getModuleInfo(&moduleInfo));

            if (auto info = device.getInfo(); info.assigned())
            {
                auto deviceInfoType = info.getDeviceType();
                if (deviceInfoType.assigned())
                    deviceInfoType.asPtr<IComponentTypePrivate>().setModuleInfo(moduleInfo);
            }

            if (authenticationSupported)
            {
                if (const auto& componentPrivate = device.asPtrOrNull<IComponentPrivate>(true); componentPrivate.assigned())
                    componentPrivate.setAuthenticationConfig(resolvedAuthConfig);

                // TODO: cache `credentials` with the device's own resolved manufacturer/serial number/model here when a caching
                // mechanism (e.g. a vault) is ready.
            }
        }

        return device;
    }
    DAQ_THROW_EXCEPTION(NotFoundException, "Device with given connection string '{}' and config is not available", connectionStringPtr);
}

ErrCode ModuleManagerImpl::createDevices(IDict** devices, IDict* connectionArgs, IComponent* parent, IDict* errCodes, IDict* errorInfos)
{
    OPENDAQ_PARAM_NOT_NULL(devices);
    OPENDAQ_PARAM_NOT_NULL(connectionArgs);

    DictPtr<IString, IPropertyObject> connectionArgsDictPtr = DictPtr<IString, IPropertyObject>::Borrow(connectionArgs);
    DictPtr<IString, IInteger> errCodesDictPtr = DictPtr<IString, IInteger>::Borrow(errCodes);
    DictPtr<IString, IErrorInfo> errorInfosDictPtr = DictPtr<IString, IErrorInfo>::Borrow(errorInfos);

    std::atomic<SizeT> createdDevicesCount = 0;
    std::mutex errorResultSync;

    if (connectionArgsDictPtr.getCount() == 0)
        return DAQ_MAKE_ERROR_INFO(OPENDAQ_ERR_INVALIDPARAMETER, "None connection arguments provided");

    auto saveErrCode = [&errCodesDictPtr, &errorResultSync](const StringPtr& connectionString, ErrCode errCode)
    {
        if (errCodesDictPtr.assigned())
        {
            std::scoped_lock lock(errorResultSync);
            errCodesDictPtr[connectionString] = errCode;
        }
    };
    auto saveErrorInfo = [&errorInfosDictPtr, &errorResultSync](const StringPtr& connectionString)
    {
        if (errorInfosDictPtr.assigned())
        {
            ObjectPtr<IErrorInfo> errorInfo;
            daqGetErrorInfo(&errorInfo);

            std::scoped_lock lock(errorResultSync);
            errorInfosDictPtr[connectionString] = errorInfo;
        }
        daqClearErrorInfo();
    };

    using AsyncDeviceCreationResult = std::future<DevicePtr>;
    std::vector<std::pair<AsyncDeviceCreationResult, StringPtr>> deviceCreationResults;
    auto devicesDictPtr = Dict<IString, IDevice>();

    // init error infos
    for (const auto& [connectionString, _] : connectionArgsDictPtr)
        if (errorInfosDictPtr.assigned())
            errorInfosDictPtr[connectionString] = nullptr;

    for (const auto& [connectionString, config] : connectionArgsDictPtr)
    {
        devicesDictPtr[connectionString] = nullptr;
        try
        {
            // Parallelize the process of each device creation as it may be time-consuming
            AsyncDeviceCreationResult deviceObjFuture =
                std::async([&, this, connectionString = connectionString, config = config]()
                           {
                               DevicePtr device;
                               LOG_D("Run create device \"{}\" asynchronously", connectionString)
                               ErrCode errCode = this->createDevice(&device, connectionString, parent, config);

                               // Preserve error information before making any smart pointer calls that will overwrite it
                               if (OPENDAQ_FAILED(errCode))
                                   saveErrorInfo(connectionString);
                               else
                                   ++createdDevicesCount;
                               saveErrCode(connectionString, errCode);

                               return device;
                           });
            deviceCreationResults.emplace_back(std::move(deviceObjFuture), connectionString);
        }
        catch (const std::exception& e)
        {
            LOG_E("Failed to run create device \"{}\" asynchronously: {}", connectionString, e.what())
            ErrCode errCode = DAQ_ERROR_FROM_STD_EXCEPTION(e, this->getThisAsBaseObject(), OPENDAQ_ERR_GENERALERROR);
            saveErrorInfo(connectionString);
            saveErrCode(connectionString, errCode);
        }
    }

    for (auto& [futureResult, connectionString] : deviceCreationResults)
    {
        LOG_D("Getting async create device \"{}\" result ...", connectionString);
        devicesDictPtr[connectionString] = futureResult.get();
    }
    *devices = devicesDictPtr.detach();

    if (createdDevicesCount == connectionArgsDictPtr.getCount())
        return OPENDAQ_SUCCESS;
    else if (createdDevicesCount == 0)
        return DAQ_MAKE_ERROR_INFO(OPENDAQ_ERR_GENERALERROR, "No devices were created");
    else
        return OPENDAQ_PARTIAL_SUCCESS;
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
            LOG_I("{}: GetAvailableFunctionBlockTypes not implemented", module.getModuleInfo().getName())
        }
        catch ([[maybe_unused]] const std::exception& e)
        {
            LOG_W("{}: GetAvailableFunctionBlockTypes failed: {}", module.getModuleInfo().getName(), e.what())
        }

        if (!types.assigned())
            continue;

        for (const auto& [id, type] : types)
            availableTypes.set(id, type);
    }

    *functionBlockTypes = availableTypes.detach();
    return OPENDAQ_SUCCESS;
}

StringPtr ModuleManagerImpl::ConvertIfOldIdFB(const StringPtr& id)
{
    if (id == "ref_fb_module_classifier")
        return "RefFBModuleClassifier";
    if (id == "ref_fb_module_fft")
        return "RefFBModuleFFT";
    if (id == "ref_fb_module_power")
        return "RefFBModulePower";
    if (id == "ref_fb_module_renderer")
        return "RefFBModuleRenderer";
    if (id == "ref_fb_module_scaling")
        return "RefFBModuleScaling";
    if (id == "ref_fb_module_statistics")
        return "RefFBModuleStatistics";
    if (id == "ref_fb_module_trigger")
        return "RefFBModuleTrigger";
    if (id == "audio_device_module_wav_writer")
        return "AudioDeviceModuleWavWriter";
    return id;
}

StringPtr ModuleManagerImpl::ConvertIfOldIdProtocol(const StringPtr& id)
{
    if (id == "opendaq_native_config")
        return "OpenDAQNativeConfiguration";
    if (id == "opendaq_opcua_config")
        return "OpenDAQOPCUAConfiguration";
    if (id == "opendaq_native_streaming")
        return "OpenDAQNativeStreaming";
    if (id == "opendaq_lt_streaming")
        return "OpenDAQLTStreaming";
    if (id == "openDAQ LT Streaming")
        return "OpenDAQLTStreaming";
    if (id == "openDAQ Native Streaming")
        return "OpenDAQNativeStreaming";
    if (id == "openDAQ OpcUa")
        return "OpenDAQOPCUA";
    return id;
}

void ModuleManagerImpl::PopulateDeviceTypeConfigFromConnStrOptions(
    PropertyObjectPtr& deviceTypeConfig,
    const tsl::ordered_map<std::string, ObjectPtr<IBaseObject>>& options)
{
    for (const auto& item: options)
    {
        if (deviceTypeConfig.hasProperty(item.first))
            deviceTypeConfig.setPropertyValue(item.first, item.second);
    }
}

ErrCode ModuleManagerImpl::createFunctionBlock(IFunctionBlock** functionBlock, IString* id, IComponent* parent, IPropertyObject* config, IString* localId)
{
    OPENDAQ_PARAM_NOT_NULL(functionBlock);
    OPENDAQ_PARAM_NOT_NULL(id);

    const StringPtr typeId = ConvertIfOldIdFB(StringPtr::Borrow(id));

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
            LOG_I("{}: GetAvailableFunctionBlockTypes not implemented", module.getModuleInfo().getName())
        }
        catch ([[maybe_unused]] const std::exception& e)
        {
            LOG_W("{}: GetAvailableFunctionBlockTypes failed: {}", module.getModuleInfo().getName(), e.what())
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

        auto err = module->createFunctionBlock(functionBlock, typeId, parent, String(localIdStr), config);
        OPENDAQ_RETURN_IF_FAILED(err);

        FunctionBlockPtr fbPtr = FunctionBlockPtr::Borrow(*functionBlock);
        if (fbPtr.assigned())
        {
            ModuleInfoPtr moduleInfo;
            err = library.module->getModuleInfo(&moduleInfo);
            OPENDAQ_RETURN_IF_FAILED(err);

            auto fbType = fbPtr.getFunctionBlockType();
            if (fbType.assigned())
                fbType.asPtr<IComponentTypePrivate>().setModuleInfo(moduleInfo);
        }

        return OPENDAQ_SUCCESS;
    }

    return DAQ_MAKE_ERROR_INFO(
        OPENDAQ_ERR_NOTFOUND,
        fmt::format(R"(Function block with given uid and config is not available [{}])", typeId)
    );
}

ErrCode ModuleManagerImpl::createStreaming(IStreaming** streaming,
                                           IString* connectionString,
                                           IPropertyObject* config,
                                           IDevice* owner)
{
    OPENDAQ_PARAM_NOT_NULL(connectionString);
    OPENDAQ_PARAM_NOT_NULL(streaming);

    // Temporary bridge (see `ExtractAuthenticationConfig`): a caller that wants an authenticated connection
    // smuggles an `IAuthenticationConfig` onto `config`, rather than a dedicated parameter.
    const AuthenticationConfigPtr authenticationConfig = ExtractAuthenticationConfig(PropertyObjectPtr::Borrow(config));

    StreamingPtr streamingPtr;
    const ErrCode errCode = wrapHandlerReturn(
        this, &ModuleManagerImpl::onCreateStreaming, streamingPtr, connectionString, config, authenticationConfig, DevicePtr::Borrow(owner));
    OPENDAQ_RETURN_IF_FAILED(errCode);

    *streaming = streamingPtr.detach();
    return errCode;
}

ErrCode ModuleManagerImpl::getAvailableStreamingTypes(IDict** streamingTypes)
{
    OPENDAQ_PARAM_NOT_NULL(streamingTypes);

    auto availableTypes = Dict<IString, IStreamingType>();

    for (const auto& library : libraries)
    {
        const auto module = library.module;

        DictPtr<IString, IStreamingType> types;
        try
        {
            types = module.getAvailableStreamingTypes();
        }
        catch (const NotImplementedException&)
        {
            LOG_I("{}: GetAvailableStreamingTypes not implemented", module.getModuleInfo().getName())
        }
        catch ([[maybe_unused]] const std::exception& e)
        {
            LOG_W("{}: GetAvailableStreamingTypes failed: {}", module.getModuleInfo().getName(), e.what())
        }

        if (!types.assigned())
            continue;

        for (const auto& [id, type] : types)
            availableTypes.set(id, type);
    }

    *streamingTypes = availableTypes.detach();
    return OPENDAQ_SUCCESS;
}

ErrCode ModuleManagerImpl::createDefaultAddDeviceConfig(IPropertyObject** defaultConfig)
{
    OPENDAQ_PARAM_NOT_NULL(defaultConfig);

    DictPtr<IString, IDeviceType> deviceTypes;
    ErrCode err = getAvailableDeviceTypes(&deviceTypes);
    OPENDAQ_RETURN_IF_FAILED(err);

    DictPtr<IString, IStreamingType> streamingTypes;
    err = getAvailableStreamingTypes(&streamingTypes);
    OPENDAQ_RETURN_IF_FAILED(err);

    return daqTry([&] { *defaultConfig = createLegacyAddDeviceConfig(deviceTypes, streamingTypes, scanOnAdd).detach(); });
}

ErrCode ModuleManagerImpl::createServer(IServer** server, IString* serverTypeId, IDevice* rootDevice, IPropertyObject* serverConfig)
{
    OPENDAQ_PARAM_NOT_NULL(serverTypeId);
    OPENDAQ_PARAM_NOT_NULL(server);
    OPENDAQ_PARAM_NOT_NULL(rootDevice);

    auto typeId = ConvertIfOldIdProtocol(StringPtr::Borrow(serverTypeId));

    for (const auto& library : libraries)
    {
        const auto module = library.module;
        DictPtr<IString, IServerType> serverTypes;
        try
        {
            serverTypes = module.getAvailableServerTypes();
        }
        catch (NotImplementedException&)
        {
            serverTypes = nullptr;
        }

        if (!serverTypes.assigned())
            continue;

        for (const auto& [id, serverType] : serverTypes)
        {
            if (id == typeId)
            {
                auto createdServer = module.createServer(typeId, rootDevice, serverConfig);

                *server = createdServer.detach();
                return OPENDAQ_SUCCESS;
            }
        }
    }

    return DAQ_MAKE_ERROR_INFO(OPENDAQ_ERR_NOTFOUND);
}

ErrCode ModuleManagerImpl::changeIpConfig(IString* iface, IString* manufacturer, IString* serialNumber, IPropertyObject* config)
{
    return discoveryClient.applyIpConfiguration(manufacturer, serialNumber, iface, config);
}

ErrCode ModuleManagerImpl::requestIpConfig(IString* iface, IString* manufacturer, IString* serialNumber, IPropertyObject** config)
{
    OPENDAQ_PARAM_NOT_NULL(config);

    PropertyObjectPtr ipConfig;
    auto errCode = discoveryClient.requestIpConfiguration(manufacturer, serialNumber, iface, ipConfig);
    *config = ipConfig.detach();

    return errCode;
}

ErrCode ModuleManagerImpl::completeDeviceCapabilities(IDevice* device)
{
    OPENDAQ_PARAM_NOT_NULL(device);

    DevicePtr devicePtr = DevicePtr::Borrow(device);

    const ErrCode errCode = wrapHandler(this, &ModuleManagerImpl::onCompleteCapabilities, device, nullptr);
    OPENDAQ_RETURN_IF_FAILED(errCode);
    return errCode;
}

ErrCode ModuleManagerImpl::getDiscoveryInfo(IDeviceInfo** deviceInfo, IString* manufacturer, IString* serialNumber)
{
    OPENDAQ_PARAM_NOT_NULL(deviceInfo);
    OPENDAQ_PARAM_NOT_NULL(manufacturer);
    OPENDAQ_PARAM_NOT_NULL(serialNumber);

    StringPtr manufacturerPtr = StringPtr::Borrow(manufacturer);
    StringPtr serialNumberPtr = StringPtr::Borrow(serialNumber);

    if (!availableDevicesGroup.assigned())
    {
        if (!scanOnAdd)
            return OPENDAQ_NOTFOUND;

        auto lock = std::lock_guard(availableDevicesSearchSync);
        const auto errCode = getAvailableDevices(&ListPtr<IDeviceInfo>());
        OPENDAQ_RETURN_IF_FAILED(errCode, "Failed getting available devices");
    }

    for (const auto& [_, info] : availableDevicesGroup)
    {
        if (info.getManufacturer() == manufacturerPtr && info.getSerialNumber() == serialNumberPtr)
        {
            *deviceInfo = info.addRefAndReturn();
            return OPENDAQ_SUCCESS;
        }
    }
    return OPENDAQ_NOTFOUND;
}

ErrCode ModuleManagerImpl::createAddDeviceConfig(IAddDeviceConfig** config, IDeviceInfo* info, IString* prefix)
{
    OPENDAQ_PARAM_NOT_NULL(config);
    OPENDAQ_PARAM_NOT_NULL(info);

    return daqTry([&]()
    {
        const auto infoPtr = DeviceInfoPtr::Borrow(info);
        const StringPtr prefixPtr = (prefix != nullptr) ? StringPtr(prefix) : StringPtr("");

        DictPtr<IString, IDeviceType> deviceTypes;
        checkErrorInfo(getAvailableDeviceTypes(&deviceTypes));
        DictPtr<IString, IStreamingType> streamingTypes;
        checkErrorInfo(getAvailableStreamingTypes(&streamingTypes));

        const auto deviceType = chooseDeviceType(infoPtr, prefixPtr, deviceTypes);
        const auto capabilities = infoPtr.getServerCapabilities();
        auto addDeviceConfig = createAddDeviceConfigForType(deviceType, capabilities, streamingTypes);

        // A device info that announces no options names the port to connect to in its connection string, if any
        if (!AnnouncedTransportOptions(capabilities, deviceType.getConnectionStringPrefix()).assigned() &&
            addDeviceConfig.getTransportOptions().getCount() > 0)
        {
            if (const auto port = GetConnectionStringPort(infoPtr.getConnectionString()); port.has_value())
                addDeviceConfig.setPort(Integer(port.value()));
        }

        *config = addDeviceConfig.detach();
    });
}

// The discovered capability whose connection strings include the given one - of the device found with a smart
// connection string, or of any discovered device
ServerCapabilityPtr ModuleManagerImpl::findDiscoveredCapability(const StringPtr& connectionString,
                                                                const DeviceInfoPtr& discoveredDeviceInfo) const
{
    const auto findIn = [&connectionString](const DeviceInfoPtr& info) -> ServerCapabilityPtr
    {
        for (const auto& capability : info.getServerCapabilities())
        {
            for (const auto& capabilityConnectionString : capability.getConnectionStrings())
            {
                if (capabilityConnectionString == connectionString)
                    return capability;
            }

            for (const auto& addressInfo : capability.getAddressInfo())
            {
                if (addressInfo.getConnectionString() == connectionString)
                    return capability;
            }
        }

        return nullptr;
    };

    if (discoveredDeviceInfo.assigned())
        return findIn(discoveredDeviceInfo);

    if (!availableDevicesGroup.assigned())
        return nullptr;

    for (const auto& [_, info] : availableDevicesGroup)
    {
        if (const auto capability = findIn(info); capability.assigned())
            return capability;
    }

    return nullptr;
}

// A connection string taken from discovery carries the port of the capability's default transport option. When the
// type config selects another option the capability offers, the port the server announces for that option is used
// instead: in the connection string, and as the Port of the type config unless that one was set explicitly.
StringPtr ModuleManagerImpl::applySelectedTransportPort(const StringPtr& connectionString,
                                                        const ServerCapabilityPtr& discoveredCapability,
                                                        const PropertyObjectPtr& deviceTypeConfig) const
{
    if (!discoveredCapability.assigned())
        return connectionString;

    const StringPtr selected = GetSelectedTransportOption(deviceTypeConfig);
    const auto options = discoveredCapability.getTransportOptions();
    if (!selected.assigned() || !options.assigned() || options.getCount() == 0)
        return connectionString;

    const auto option = FindTransportOption(options, selected);
    if (!option.assigned())
    {
        LOG_W("The discovered capability [{}] of {} does not offer the transport option '{}' selected in the type config, "
              "connecting as is",
              discoveredCapability.getProtocolId(),
              connectionString,
              selected);
        return connectionString;
    }

    const Int port = option.getPort();
    if (deviceTypeConfig.hasProperty(PortConfigProperty))
    {
        const auto portProperty = deviceTypeConfig.getProperty(PortConfigProperty);
        if (deviceTypeConfig.getPropertyValue(PortConfigProperty) == portProperty.getDefaultValue())
            deviceTypeConfig.setPropertyValue(PortConfigProperty, port);
    }

    return ReplaceConnectionStringPort(connectionString, port);
}

AddDeviceConfigPtr ModuleManagerImpl::createAddDeviceConfigForType(const DeviceTypePtr& deviceType,
                                                                   const ListPtr<IServerCapability>& capabilities,
                                                                   const DictPtr<IString, IStreamingType>& streamingTypes) const
{
    const auto candidates = (capabilities.getCount() > 0) ? getAdvertisedStreamingTypes(capabilities, streamingTypes) : streamingTypes;
    const auto listedTypes = getListableStreamingTypes(candidates, loggerComponent);

    // The config lists the transport options the server of the device announces instead of the ones of the types
    const auto transportOptions = AnnouncedTransportOptions(capabilities, deviceType.getConnectionStringPrefix());
    auto streamingTransportOptions = Dict<IString, IList>();
    for (const auto& [id, streamingType] : listedTypes)
    {
        if (const auto announced = AnnouncedTransportOptions(capabilities, streamingType.getConnectionStringPrefix()); announced.assigned())
            streamingTransportOptions.set(id, announced);
    }

    AddDeviceConfigPtr addDeviceConfig = createWithImplementation<IAddDeviceConfig, AddDeviceConfigImpl>(
        deviceType, listedTypes, scanOnAdd, transportOptions, streamingTransportOptions);

    const auto options = addDeviceConfig.getStreamingOptions();
    if (!options.assigned())
        return addDeviceConfig;

    const auto defaultType = selectStreamingType(listedTypes, deviceType);
    if (defaultType.assigned())
        options.setStreamingType(defaultType.getId());

    return addDeviceConfig;
}

ErrCode ModuleManagerImpl::getScanOnAdd(Bool* scanOnAdd)
{
    OPENDAQ_PARAM_NOT_NULL(scanOnAdd);

    *scanOnAdd = this->scanOnAdd;
    return OPENDAQ_SUCCESS;
}

DeviceTypePtr ModuleManagerImpl::chooseDeviceType(const DeviceInfoPtr& info,
                                                  const StringPtr& prefix,
                                                  const DictPtr<IString, IDeviceType>& deviceTypes) const
{
    const auto findAddableDeviceType = [&deviceTypes](const StringPtr& typePrefix)
    {
        const auto deviceType = FindTypeByPrefix<DeviceTypePtr>(deviceTypes, typePrefix);
        if (!deviceType.assigned())
            DAQ_THROW_EXCEPTION(NotFoundException, R"(No loaded device type for the prefix "{}")", typePrefix);

        checkErrorInfo(CheckKindAllowed(deviceType, true));
        return deviceType;
    };

    const auto capabilities = info.getServerCapabilities();
    if (capabilities.getCount() == 0)
    {
        const std::string connectionStringPrefix = getPrefixFromConnectionString(info.getConnectionString());
        if (prefix.getLength() > 0 && prefix != connectionStringPrefix)
        {
            DAQ_THROW_EXCEPTION(InvalidParameterException,
                                R"(Prefix "{}" differs from the prefix "{}" of the device connection string)",
                                prefix,
                                connectionStringPrefix);
        }

        return findAddableDeviceType(connectionStringPrefix);
    }

    if (prefix.getLength() > 0)
    {
        if (std::none_of(capabilities.begin(),
                         capabilities.end(),
                         [&prefix](const ServerCapabilityPtr& capability) { return getServerCapabilityPrefix(capability) == prefix; }))
            DAQ_THROW_EXCEPTION(NotFoundException, R"(The device has no server capability with the prefix "{}")", prefix);

        return findAddableDeviceType(prefix);
    }

    const auto selected = SelectServerCapability(capabilities, deviceTypes, true);
    if (selected.assigned())
        return FindTypeByPrefix<DeviceTypePtr>(deviceTypes, getServerCapabilityPrefix(selected));

    for (const auto& capability : capabilities)
    {
        const auto deviceType = FindTypeByPrefix<DeviceTypePtr>(deviceTypes, getServerCapabilityPrefix(capability));
        if (deviceType.assigned())
            checkErrorInfo(CheckKindAllowed(deviceType, true));
    }

    DAQ_THROW_EXCEPTION(NotFoundException, "No loaded device type for any server capability of the device");
}

ServerCapabilityPtr ModuleManagerImpl::SelectServerCapability(const ListPtr<IServerCapability>& capabilities,
                                                              const DictPtr<IString, IDeviceType>& deviceTypes,
                                                              bool asChild)
{
    ServerCapabilityPtr selected;
    bool selectedMirrored = false;
    for (const auto& capability : capabilities)
    {
        const auto deviceType = FindTypeByPrefix<DeviceTypePtr>(deviceTypes, getServerCapabilityPrefix(capability));
        if (!deviceType.assigned() || !IsKindAllowed(deviceType.getDeviceKind(), asChild))
            continue;

        if (capability.getProtocolId() == "OpenDAQNativeConfiguration")
            return capability;

        const bool mirrored = (deviceType.getDeviceKind() == DeviceKind::Mirrored);
        if (!selected.assigned() || (mirrored && !selectedMirrored))
        {
            selected = capability;
            selectedMirrored = mirrored;
        }
    }

    return selected;
}

DeviceInfoPtr ModuleManagerImpl::getSmartConnectionDeviceInfo(const StringPtr& inputConnectionString) const
{
    if (!availableDevicesGroup.assigned())
        DAQ_THROW_EXCEPTION(NotFoundException, "Device scan has not yet been initiated.");

    if (availableDevicesGroup.hasKey(inputConnectionString))
        return availableDevicesGroup.get(inputConnectionString);
    DAQ_THROW_EXCEPTION(NotFoundException, "Device with connection string \"{}\" not found", inputConnectionString);
}

DeviceInfoPtr ModuleManagerImpl::getDiscoveredDeviceInfo(const DeviceInfoPtr& deviceInfo) const
{
    auto serialNumber = deviceInfo.getSerialNumber();
    auto manufacturer = deviceInfo.getManufacturer();

    if (!serialNumber.getLength() || !manufacturer.getLength())
        return nullptr;

    if (!availableDevicesGroup.assigned())
        return nullptr;

    DeviceInfoPtr localInfo;
    for (const auto & [_, info] : availableDevicesGroup)
    {
        if (manufacturer == info.getManufacturer() && serialNumber == info.getSerialNumber())
        {
            if (info.getServerCapabilities().getCount())
                return info;

            localInfo = info;
        }
    }

    return localInfo;
}

StringPtr ModuleManagerImpl::resolveSmartConnectionString(const ServerCapabilityPtr& selectedCapability,
                                                          AddressType primaryAddressType) const
{
    const auto addressInfos = sortAddressInfos(selectedCapability.getAddressInfo(), primaryAddressType);
    if (addressInfos.empty())
        return selectedCapability.getConnectionString();

    const auto& selected = addressInfos.front();
    const auto connectionString = selected.getConnectionString();
    if (selected.getReachabilityStatus() == AddressReachabilityStatus::Unreachable)
    {
        LOG_W("Trying to connect to an unreachable address {}. No reachable/unknown addresses were available.", connectionString);
    }

    return connectionString;
}

StreamingPtr ModuleManagerImpl::onCreateStreaming(const StringPtr& connectionString,
                                                  const PropertyObjectPtr& config,
                                                  const AuthenticationConfigPtr& authenticationConfig,
                                                  const DevicePtr& owner) const
{
    const AddStreamingConfigPtr addStreamingConfig = config.asPtrOrNull<IAddStreamingConfig>(true);
    const StreamingTypePtr configType = addStreamingConfig.assigned() ? addStreamingConfig.getStreamingType() : nullptr;
    const std::string prefix = getPrefixFromConnectionString(connectionString);
    if (configType.assigned() && configType.getConnectionStringPrefix() != prefix)
    {
        DAQ_THROW_EXCEPTION(InvalidParameterException,
                            R"(Prefix "{}" differs from the prefix "{}" of the streaming type "{}")",
                            prefix,
                            configType.getConnectionStringPrefix(),
                            configType.getId());
    }

    const PropertyObjectPtr inputTypeConfig = addStreamingConfig.assigned() ? addStreamingConfig.getTypeConfig() : config;
    PropertyObjectPtr typeConfig;
    if (inputTypeConfig.assigned())
        checkErrorInfo(inputTypeConfig.asPtr<IPropertyObjectInternal>()->clone(&typeConfig));

    StreamingPtr streaming;

    for (const auto& library : libraries)
    {
        const auto module = library.module;

        DictPtr<IString, IStreamingType> types;
        const ErrCode errCode = module->getAvailableStreamingTypes(&types);
        if (OPENDAQ_FAILED(errCode))
            daqClearErrorInfo();
        if (!types.assigned())
            continue;

        const StreamingTypePtr streamingType = configType.assigned()
                                                   ? (types.hasKey(configType.getId()) ? types.get(configType.getId()) : nullptr)
                                                   : FindTypeByPrefix<StreamingTypePtr>(types, prefix);
        if (!streamingType.assigned())
            continue;

        PropertyObjectPtr streamingTypeConfig =
            addStreamingConfig.assigned() ? typeConfig : fromLegacyAddStreamingConfig(typeConfig, streamingType);

        // The module connects by the selection properties; a config without them gets the default option
        if (const auto options = ComponentTypeTransportOptions(streamingType); options.getCount() > 0)
        {
            if (!streamingTypeConfig.assigned())
                streamingTypeConfig = streamingType.createDefaultConfig();
            AddTransportSelectionProperties(streamingTypeConfig, options);
        }

        checkErrorInfo(CheckSelectedTransportOption(streamingType, streamingTypeConfig));

        if (authenticationConfig.assigned() && !streamingType.asPtr<IComponentTypePrivate>().getSupportedAuthenticationMethods().getCount())
        {
            LOG_W("Streaming with given connection string '{}' does not support authentication; creating it without authentication.", connectionString);
        }

        const AuthenticationConfigPtr resolvedAuthConfig = resolveDefaultAuthenticationConfig(authenticationConfig, streamingType);
        // Any supported authentication method - field-less (e.g. "Anonymous") included - goes through
        // `createAuthenticatedStreaming`; `createStreaming` is only for a type with none supported at all.
        const bool authenticated = resolvedAuthConfig.assigned();

        try
        {
            if (authenticated)
            {
                StringPtr manufacturer, serialNumber, model, displayName;
                if (owner.assigned())
                {
                    if (const DeviceInfoPtr ownerInfo = owner.getInfo(); ownerInfo.assigned())
                    {
                        manufacturer = ownerInfo.getManufacturer();
                        serialNumber = ownerInfo.getSerialNumber();
                        model = ownerInfo.getModel();
                        displayName = ownerInfo.getName();
                    }
                }

                const auto credentialRequest = CredentialRequestBuilder()
                                                   .setConnectionString(module.getCanonicalConnectionString(connectionString))
                                                   .setManufacturer(manufacturer)
                                                   .setSerialNumber(serialNumber)
                                                   .setModel(model)
                                                   .setDisplayName(displayName)
                                                   .setAuthenticationMethod(resolvedAuthConfig.getSelectedAuthenticationMethod())
                                                   .setComponentType(streamingType)
                                                   .build();
                const DictPtr<IString, IString> credentials = resolveCredentials(resolvedAuthConfig, credentialRequest);

                const auto loginParameters = LoginParametersBuilder()
                                                 .setAuthenticationMethod(resolvedAuthConfig.getSelectedAuthenticationMethod())
                                                 .setCredentials(credentials)
                                                 .build();
                streaming = module.createAuthenticatedStreaming(connectionString, streamingTypeConfig, loginParameters);

                // TODO: cache `credentials` keyed by `credentialRequest` here when a caching mechanism (e.g. a vault) is ready.
            }
            else
            {
                streaming = module.createStreaming(connectionString, streamingTypeConfig);
            }
        }
        catch ([[maybe_unused]] const std::exception& e)
        {
            LOG_E("{}: createStreaming failed: {}", module.getModuleInfo().getName(), e.what())
            throw;
        }

        break;
    }

    return streaming;
}

void ModuleManagerImpl::completeServerCapabilities(const DevicePtr& device) const
{
    const auto connectedDeviceInfo = device.getInfo();
    const auto configConnectionInfo = connectedDeviceInfo.getConfigurationConnectionInfo();
    if (!configConnectionInfo.assigned() || configConnectionInfo.getProtocolType() == ProtocolType::Unknown)
        return;

    const auto source = configConnectionInfo;
    const auto targetCaps = connectedDeviceInfo.getServerCapabilities();

    for (const auto& target : targetCaps)
    {
        for (const auto& library : libraries)
        {
            const auto module = library.module;
            try
            {
                if (module.completeServerCapability(source, target))
                    break;
            }
            catch (const NotImplementedException&)
            {
                LOG_D("{}: completeServerCapability not implemented", module.getModuleInfo().getName());
            }
            catch ([[maybe_unused]] const std::exception& e)
            {
                LOG_W("{}: completeServerCapability failed: {}", module.getModuleInfo().getName(), e.what());
            }
        }
    }

    const auto addressInfo = source.getAddressInfo();
    if (!addressInfo.assigned() || !addressInfo.getCount())
        return;

    const auto connectionType = source.getConnectionType();
    const auto address = addressInfo[0].getAddress();
    const auto addressType = addressInfo[0].getType();

    for (const auto& target : targetCaps)
    {
        if (target.getConnectionType() != connectionType)
            continue;

        const auto targetAddressInfo = target.getAddressInfo();
        if (!targetAddressInfo.assigned() || !targetAddressInfo.getCount())
            continue;

        for (const auto& info : targetAddressInfo)
        {
            if (address == info.getAddress() && addressType == info.getType())
            {
                info.asPtr<IAddressInfoPrivate>(true).setReachabilityStatusPrivate(AddressReachabilityStatus::Reachable);
            }
        }
    }

    for (const auto& target: targetCaps)
        target.freeze();
}

DictPtr<IString, IDeviceInfo> ModuleManagerImpl::discoverDevicesWithIpModification()
{
    auto result = Dict<IString, IDeviceInfo>();
    for (const auto& device : discoveryClient.discoverMdnsDevices())
    {
        auto [smartConnString, deviceInfo] = populateDiscoveredDevice(device);
        if (smartConnString.assigned() && deviceInfo.assigned())
            result[smartConnString] = deviceInfo;
    }

    return result;
}

std::pair<StringPtr, DeviceInfoPtr> ModuleManagerImpl::populateDiscoveredDevice(const discovery::MdnsDiscoveredDevice& discoveredDevice)
{
    auto deviceInfo = DeviceInfo("");
    PropertyObjectPtr info = deviceInfo;
    discovery::DiscoveryClient::populateDiscoveredInfoProperties(info, discoveredDevice, ConnectedClientInfo());

    StringPtr manufacturer = deviceInfo.getManufacturer();
    StringPtr serialNumber = deviceInfo.getSerialNumber();

    // Filter-out devices that don't have manufacturer, serial number and at least one advertised network interface
    if (manufacturer.getLength() != 0 && serialNumber.getLength() != 0 && deviceInfo.hasProperty("interfaces"))
    {
        StringPtr interfacesString = deviceInfo.getPropertyValue("interfaces");
        const auto thisPtr = this->borrowPtr<BaseObjectPtr>();

        std::string interfaceName;
        std::stringstream ss(interfacesString.toStdString());
        while (std::getline(ss, interfaceName, ';'))
        {
            if (!interfaceName.empty())
            {
                const auto networkInterface = NetworkInterface(interfaceName, manufacturer, serialNumber, thisPtr);
                deviceInfo.asPtr<IDeviceInfoInternal>(true).addNetworkInteface(interfaceName, networkInterface);
            }
        }

        if (deviceInfo.getNetworkInterfaces().getCount() > 0)
        {
            StringPtr id = "daq://" + manufacturer + "_" + serialNumber;
            return {id, deviceInfo};
        }
    }

    return {nullptr, nullptr};
}

void ModuleManagerImpl::onCompleteCapabilities(const DevicePtr& device, const DeviceInfoPtr& discoveredDeviceInfo)
{
    ReplaceSubDeviceOldProtocolIds(device);
    mergeDiscoveryAndDeviceCapabilities(device,
                                        discoveredDeviceInfo.assigned()
                                            ? discoveredDeviceInfo
                                            : getDiscoveredDeviceInfo(device.getInfo()));
    completeServerCapabilities(device);
}

std::string ModuleManagerImpl::getPrefixFromConnectionString(std::string connectionString) const
{
    try
    {
        return connectionString.substr(0, connectionString.find("://"));
    }
    catch(...)
    {
        LOG_W("Connection string has no prefix denoted by the \"://\" delimiter")
    }

    return "";
}

std::pair<std::string, tsl::ordered_map<std::string, BaseObjectPtr>> ModuleManagerImpl::SplitConnectionStringAndOptions(
    const std::string& connectionString)
{
    std::vector<std::string> strs1;
    boost::split(strs1, connectionString, boost::is_any_of("?"));

    if (strs1.size() == 1)
        return std::make_pair(strs1[0], tsl::ordered_map<std::string, BaseObjectPtr> {});

    if (strs1.size() != 2)
        DAQ_THROW_EXCEPTION(InvalidParameterException, "Invalid connection string");

    std::vector<std::string> options;
    boost::split(options, strs1[1], boost::is_any_of("&"));

    tsl::ordered_map<std::string, BaseObjectPtr> optionsMap;
    for (const auto& option: options)
    {
        std::vector<std::string> keyAndValue;
        boost::split(keyAndValue, option, boost::is_any_of("="));
        if (keyAndValue.size() != 2)
            DAQ_THROW_EXCEPTION(InvalidParameterException, "Invalid connection string");

        BaseObjectPtr value = EvalValue(keyAndValue[1]);
        optionsMap.insert({keyAndValue[0], value});
    }

    return std::make_pair(strs1[0], optionsMap);
}

void ModuleManagerImpl::ReplaceSubDeviceOldProtocolIds(const DevicePtr& device)
{
    using namespace search;
    auto devices = device.getDevices(Recursive(Any()));
    devices.pushBack(device);

    for (const auto& dev : devices)
    {
        const auto devInfo = dev.getInfo();
        const auto devInfoInternal = devInfo.asPtr<IDeviceInfoInternal>();

        for (const auto& cap : devInfo.getServerCapabilities())
        {
            const auto newCap = ReplaceOldProtocolIds(cap);
            devInfoInternal.removeServerCapability(cap.getProtocolId());
            devInfoInternal.addServerCapability(newCap);
        }
    }
}

ServerCapabilityPtr ModuleManagerImpl::ReplaceOldProtocolIds(const ServerCapabilityPtr& cap)
{
    const auto oldProtocolId = cap.getProtocolId();
    const auto newProtocolId = ConvertIfOldIdProtocol(oldProtocolId);

    if (oldProtocolId == newProtocolId)
        return cap;

    auto newCap = ServerCapability(newProtocolId, cap.getProtocolName(), cap.getProtocolType());

    for (const auto& prop : cap.getAllProperties())
    {
        const auto name = prop.getName();

        if (name == "protocolId")
            continue;

        const auto val = cap.getPropertyValue(name);
        const bool hasProp = newCap.hasProperty(name);

        if (val == prop.getDefaultValue() && hasProp)
            continue;

        if (hasProp)
            newCap.asPtr<IPropertyObjectProtected>().setProtectedPropertyValue(name, val);
        else
            newCap.addProperty(prop.asPtr<IPropertyInternal>().clone());
    }

    newCap.freeze();
    return newCap.detach();
}

ServerCapabilityPtr ModuleManagerImpl::MergeDiscoveryAndDeviceCapability(const ServerCapabilityPtr& discoveryCap,
                                                                         const ServerCapabilityPtr& deviceCap)
{
    ServerCapabilityConfigPtr merged = deviceCap.asPtr<IPropertyObjectInternal>(true).clone();

    for (const auto& prop : discoveryCap.getAllProperties())
    {
        const auto name = prop.getName();

        if (!merged.hasProperty(name))
        {
            merged.addProperty(prop.asPtr<IPropertyInternal>(true).clone());
            continue;
        }

        const auto val = discoveryCap.getPropertyValue(name);
        if (val != prop.getDefaultValue())
        {
            merged.asPtr<IPropertyObjectProtected>(true).setProtectedPropertyValue(name, val);
        }
    }

    return merged.detach();
}

void ModuleManagerImpl::mergeDiscoveryAndDeviceCapabilities(const DevicePtr& device, const DeviceInfoPtr& discoveredDeviceInfo) const
{
    const auto connectedDeviceInfo = device.getInfo();
    const auto connectedDeviceInfoInternal = connectedDeviceInfo.asPtr<IDeviceInfoInternal>();

    if (discoveredDeviceInfo.assigned())
    {
        // Replaces the default fields of capabilities retrieved from the config/structure protocol
        // if a better alternative is available from the discovery results
        for (const auto& capability : discoveredDeviceInfo.getServerCapabilities())
        {
            const auto capId = capability.getProtocolId();
            ServerCapabilityPtr capPtr = capability;

            if (connectedDeviceInfo.hasServerCapability(capId))
            {
                try
                {
                    capPtr = MergeDiscoveryAndDeviceCapability(capability, connectedDeviceInfo.getServerCapability(capId));
                }
                catch ([[maybe_unused]] const std::exception& e)
                {
                    capPtr = capability;
                    LOG_W("{}: Failed to merge discovery and device server capability with ID {}: {}", discoveredDeviceInfo.getName(), capId, e.what())
                }

                connectedDeviceInfoInternal.removeServerCapability(capability.getProtocolId());
            }

            connectedDeviceInfoInternal.addServerCapability(capPtr);
        }
    }
}

std::string ModuleManagerImpl::CheckLinkedStreamingTypes(const ModulePtr& module)
{
    DictPtr<IString, IStreamingType> streamingTypes;
    try
    {
        streamingTypes = module.getAvailableStreamingTypes();
    }
    catch (...)
    {
        // A module whose streaming types cannot be listed has no links to check
        return {};
    }

    if (!streamingTypes.assigned())
        return {};

    DictPtr<IString, IDeviceType> deviceTypes;
    StringPtr currentId;
    try
    {
        for (const auto& [streamingTypeId, streamingType] : streamingTypes)
        {
            currentId = streamingTypeId;
            const DeviceTypePtr linkedDeviceType = streamingType.getLinkedDeviceType();
            if (!linkedDeviceType.assigned())
                continue;

            const StringPtr linkedId = linkedDeviceType.getId();

            if (!deviceTypes.assigned())
            {
                try
                {
                    deviceTypes = module.getAvailableDeviceTypes();
                }
                catch (const DaqException& e)
                {
                    return fmt::format(
                        R"(streaming type "{}" links to device type "{}", but the module's device types cannot be listed: {} [{:#x}])",
                        streamingTypeId,
                        linkedId,
                        e.what(),
                        e.getErrCode());
                }
                if (!deviceTypes.assigned())
                    deviceTypes = Dict<IString, IDeviceType>();
            }

            if (!deviceTypes.hasKey(linkedId))
                return fmt::format(
                    R"(streaming type "{}" links to device type "{}", which the module does not provide)", streamingTypeId, linkedId);
        }
    }
    catch (const std::exception& e)
    {
        return fmt::format(R"(checking the link of streaming type "{}" failed: {})",
                           currentId.assigned() ? currentId.toStdString() : std::string{},
                           e.what());
    }

    return {};
}

void ModuleManagerImpl::logUnspecifiedDeviceKinds(const ModulePtr& module) const
{
    if (!loggerComponent.assigned())
        return;

    DictPtr<IString, IDeviceType> deviceTypes;
    try
    {
        deviceTypes = module.getAvailableDeviceTypes();
    }
    catch (...)
    {
        return;
    }

    if (!deviceTypes.assigned())
        return;

    for (const auto& [deviceTypeId, deviceType] : deviceTypes)
    {
        if (deviceType.getDeviceKind() == DeviceKind::Unspecified)
            LOG_I(R"({}: Device type "{}" declares no device kind.)", module.getModuleInfo().getName(), deviceTypeId);
    }
}

void GetModulesPath(std::vector<fs::path>& modulesPath, const LoggerComponentPtr& loggerComponent, std::string searchFolder)
{
    if (searchFolder == "[[none]]")
    {
        LOGP_D("Search folder ignored");
        return;
    }

    if (searchFolder.empty())
    {
        searchFolder = boost::dll::program_location().parent_path().string();
        LOG_D("search folder empty, moduler search folder set to program path {}", searchFolder)
    }

    std::error_code errCode;
    if (!fs::exists(searchFolder, errCode))
        DAQ_THROW_EXCEPTION(InvalidParameterException, fmt::format(R"(The specified path "{}" does not exist.)", searchFolder));

    if (!fs::is_directory(searchFolder, errCode))
        DAQ_THROW_EXCEPTION(InvalidParameterException, fmt::format(R"(The specified path "{}" is not a folder.)", searchFolder));

    [[maybe_unused]]
    Finally onExit([workingDir = fs::current_path()]
    {
        fs::current_path(workingDir);
    });

    fs::current_path(searchFolder);
    fs::recursive_directory_iterator dirIterator(fs::current_path());

    const auto endIter = fs::recursive_directory_iterator();
    while (dirIterator != endIter)
    {
        fs::directory_entry entry = *dirIterator++;

        if (!is_regular_file(entry, errCode))
            continue;

        const fs::path& entryPath = entry.path();
        const auto filename = entryPath.filename().u8string();

        if (boost::algorithm::ends_with(filename, OPENDAQ_MODULE_SUFFIX))
            modulesPath.push_back(entryPath);
    }
}


template <typename Functor>
void ModuleManagerImpl::printComponentTypes(Functor func, const std::string& kind)
{
    try
    {
        DictPtr<IString, IComponentType> componentTypes = func();
        if (componentTypes.getCount() > 0)
        {
            for (const auto& [id, type] : componentTypes)
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

void ModuleManagerImpl::printAvailableTypes(const ModulePtr& module)
{
    printComponentTypes([&module]{ return module.getAvailableDeviceTypes(); }, "DEV");
    printComponentTypes([&module]{ return module.getAvailableFunctionBlockTypes(); }, "FB");
    printComponentTypes([&module]{ return module.getAvailableServerTypes(); }, "SRV");
}

static std::string GetMessageFromLibraryErrCode(std::error_code libraryErrCode)
{
#if defined(__linux__) || defined(linux) || defined(__linux)
    // boost does not propagate `dlopen()` error messages
    return dlerror();
#else
    return libraryErrCode.message();
#endif
}

ModuleLibrary loadModuleInternal(const LoggerComponentPtr& loggerComponent, const fs::path& path, IContext* context, Bool safeLoadingMode)
{
    LOG_T("Loading module \"{}\".", path.string());

    // The statics a module library builds live as long as the library does.
    [[maybe_unused]] const UntrackedAllocations untracked;

    std::error_code libraryErrCode;
    boost::dll::shared_library moduleLibrary(path, libraryErrCode, safeLoadingMode ? boost::dll::load_mode::rtld_now : boost::dll::load_mode::default_mode);

    if (libraryErrCode)
    {
        DAQ_THROW_EXCEPTION(ModuleLoadFailedException,
                            "Module \"{}\" failed to load. Error: {} [{}]",
                            path.string(),
                            libraryErrCode.value(),
                            GetMessageFromLibraryErrCode(libraryErrCode)
        );
    }

    if (moduleLibrary.has(getCoreVersionMetadataFunc))
    {
        using GetCoreVersionMetadataFunc = ErrCode (*)(unsigned int*, unsigned int*, unsigned int*, IString**, IString**, IString**);
        GetCoreVersionMetadataFunc getMetadata = moduleLibrary.get<ErrCode(unsigned int*, unsigned int*, unsigned int*, IString**, IString**, IString**)>(getCoreVersionMetadataFunc);

        unsigned int major, minor, patch;
        StringPtr branch, sha;

        ErrCode errCode = getMetadata(&major, &minor, &patch, &branch, &sha, nullptr);

        if (OPENDAQ_SUCCEEDED(errCode))
        {
            std::string inModuleMetadataAsString =
                fmt::format(R"([[ 'major': '{}'; 'minor': '{}'; 'patch': '{}'; 'branch': '{}'; 'sha': '{}'; ]])",
                            major,
                            minor,
                            patch,
                            branch,
                            sha);

            StringPtr logMsg;
            errCode = checkModuleVersionCompatibility(major, minor, patch, branch, sha, nullptr, &logMsg);
            if (OPENDAQ_FAILED(errCode))
            {
                DAQ_EXTEND_ERROR_INFO(
                    errCode,
                    OPENDAQ_ERR_MODULE_INCOMPATIBLE_DEPENDENCIES,
                    fmt::format(R"(Module "{}" failed dependencies check via version metadata: {})",
                                path.string(),
                                logMsg.assigned() ? logMsg : ""
                    )
                );
                checkErrorInfo(OPENDAQ_ERR_MODULE_INCOMPATIBLE_DEPENDENCIES);
            }
            else if (errCode == OPENDAQ_PARTIAL_SUCCESS && logMsg.assigned())
            {
                LOG_W("Module \'{}\' SDK core version metadata checking result: {}", path.string(), logMsg);
            }
            LOG_I("Loaded module \'{}\' was built with SDK version: \"{}\"", path.string(), inModuleMetadataAsString);
        }
        else
        {
            LOG_W("Module \'{}\' failed to get SDK core version metadata: {}", path.string(), getErrorInfoMessage(errCode));
            daqClearErrorInfo();
        }
    }
    else
    {
        LOG_W("Module \"{}\" does not provide SDK core version metadata", path.string());
    }

    if (moduleLibrary.has(checkDependenciesFunc))
    {
        using CheckDependenciesFunc = ErrCode (*)(IString**);
        CheckDependenciesFunc checkDeps = moduleLibrary.get<ErrCode(IString**)>(checkDependenciesFunc);

        LOG_T("Checking dependencies of \"{}\" via \"{}\" ", path.string(), checkDependenciesFunc);

        StringPtr logMsg;
        const ErrCode errCode = checkDeps(&logMsg);
        if (OPENDAQ_FAILED(errCode))
        {
            DAQ_EXTEND_ERROR_INFO(
                errCode,
                OPENDAQ_ERR_MODULE_INCOMPATIBLE_DEPENDENCIES,
                fmt::format(R"(Module "{}" failed dependencies check via "{}": {})",
                            path.string(),
                            checkDependenciesFunc,
                            logMsg.assigned() ? logMsg : ""
                )
            );
            checkErrorInfo(OPENDAQ_ERR_MODULE_INCOMPATIBLE_DEPENDENCIES);
        }
        else if (errCode == OPENDAQ_PARTIAL_SUCCESS && logMsg.assigned())
        {
            LOG_W("Module \"{}\" check dependencies warning: {}", path.string(), logMsg);
        }
    }

    if (!moduleLibrary.has(createModuleFactory))
    {
        LOG_T("Module \"{}\" has no exported module factory.", path.string());
        DAQ_THROW_EXCEPTION(ModuleNoEntryPointException, fmt::format(R"(Module "{}" has no exported module factory.)", path.string()));
    }

    using ModuleFactory = ErrCode(IModule**, IContext*);
    ModuleFactory* factory = moduleLibrary.get<ModuleFactory>(createModuleFactory);

    LOG_T("Creating module from \"{}\".", path.string());

    ModulePtr module;
    const ErrCode errCode = factory(&module, context);
    if (OPENDAQ_FAILED(errCode))
    {
        LOG_T("Failed creating module from \"{}\"", path.string());
        DAQ_EXTEND_ERROR_INFO(errCode, OPENDAQ_ERR_MODULE_ENTRY_POINT_FAILED, fmt::format(R"(Library "{}" failed to create a Module.)", path.string()));
        checkErrorInfo(OPENDAQ_ERR_MODULE_ENTRY_POINT_FAILED);
    }

    return { std::move(moduleLibrary), module, String(path.string()) };
}

OPENDAQ_DEFINE_CLASS_FACTORY(LIBRARY_FACTORY, ModuleManager,
    IString*, path, ICredentialProvider*, credentialProvider)

OPENDAQ_DEFINE_CLASS_FACTORY_WITH_INTERFACE_AND_CREATEFUNC(
    LIBRARY_FACTORY, ModuleManager,
    IModuleManager, createModuleManagerMultiplePaths,
    IList*, paths, ICredentialProvider*, credentialProvider)

END_NAMESPACE_OPENDAQ
