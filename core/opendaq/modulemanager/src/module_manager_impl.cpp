#include <coreobjects/property_factory.h>
#include <opendaq/module_manager_impl.h>
#include <opendaq/module_manager_ptr.h>
#include <opendaq/custom_log.h>
#include <opendaq/module_ptr.h>
#include <opendaq/module_manager_exceptions.h>
#include <opendaq/module_library.h>
#include <boost/dll/runtime_symbol_info.hpp>
#include <opendaq/orphaned_modules.h>
#include <opendaq/device_info_config_ptr.h>
#include <opendaq/device_info_internal_ptr.h>
#include <coretypes/validation.h>
#include <opendaq/device_private.h>
#include <string>
#include <future>
#include <boost/algorithm/string/predicate.hpp>
#include <opendaq/search_filter_factory.h>
#include <coretypes/validation.h>
#include <opendaq/server_capability_config_ptr.h>
#include <ping/icmp_ping.h>
#include <coreobjects/property_object_factory.h>
#include <coreobjects/property_factory.h>
#include <opendaq/mirrored_signal_config_ptr.h>
#include <optional>
#include <map>

BEGIN_NAMESPACE_OPENDAQ

static OrphanedModules orphanedModules;

static constexpr char createModuleFactory[] = "createModule";
static constexpr char checkDependenciesFunc[] = "checkDependencies";

static std::vector<ModuleLibrary> enumerateModules(const LoggerComponentPtr& loggerComponent, std::string searchFolder, IContext* context);

ModuleManagerImpl::ModuleManagerImpl(const BaseObjectPtr& path)
    : modulesLoaded(false)
    , work(ioContext.get_executor())
{
    if (const StringPtr pathStr = path.asPtrOrNull<IString>(); pathStr.assigned())
    {
        paths.push_back(pathStr.toStdString());
    }
    else if (const ListPtr<IString> pathList = path.asPtrOrNull<IList>(); pathList.assigned())
    {
        paths.insert(paths.end(), pathList.begin(), pathList.end());
    }

    std::size_t numThreads = 2;
    pool.reserve(numThreads);
    
    for (std::size_t i = 0; i < numThreads; ++i)
    {
        pool.emplace_back([this]
        {
            ioContext.run();
        });
    }

    if (paths.empty())
        throw InvalidParameterException{"No valid paths provided!"};
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

    work.reset();
    ioContext.stop();
    for (auto& thread : pool)
    {
        if (thread.joinable())
            thread.join();
    }
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

struct DevicePing
{
    DeviceInfoPtr info;
    std::shared_ptr<IcmpPing> ping;
};

void ModuleManagerImpl::checkNetworkSettings(ListPtr<IDeviceInfo>& list)
{
    using namespace std::chrono_literals;

    std::vector<DevicePing> statuses;

    for (auto deviceInfo : list)
    {
        if (!deviceInfo.hasProperty("ipv4Address"))
        {
            continue;
        }

        auto icmp = IcmpPing::Create(ioContext, logger);
        IcmpPing& ping = *icmp;

        std::string deviceIp = deviceInfo.getPropertyValue("ipv4Address");

        ping.setMaxHops(1);
        ping.start(boost::asio::ip::make_address_v4(deviceIp));
        if (ping.waitSend())
        {
            deviceInfo.addProperty(BoolProperty("canPing", true));
            continue;
        }

        statuses.push_back({deviceInfo, icmp});

        LOG_T("No replies received yet: waiting 1s\n");
    }

    if (!statuses.empty())
    {
        std::this_thread::sleep_for(1s);

        for (const auto& [info, ping] : statuses)
        {
            ping->stop();
            info->addProperty(BoolProperty("canPing", ping->getNumReplies() > 0));
        }
        statuses.clear();
    }
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
            StringPtr manufacturer = deviceInfo.getManufacturer();
            StringPtr serialNumber = deviceInfo.getSerialNumber();

            if (manufacturer.getLength() == 0 ||
                serialNumber.getLength() == 0 ||
                deviceInfo.getServerCapabilities().getCount() == 0)
            {
                groupedDevices.set(deviceInfo.getConnectionString(), deviceInfo);
            }
            else
            {
                StringPtr id = "daq://" + manufacturer + "_" + serialNumber;

                if (groupedDevices.hasKey(id))
                {
                    DeviceInfoInternalPtr value = groupedDevices.get(id);
                    for (const auto & capability : deviceInfo.getServerCapabilities())
                        if (!value.hasServerCapability(capability.getProtocolId()))
                            value.addServerCapability(capability);
                }
                else
                {
                    deviceInfo.asPtr<IDeviceInfoConfig>().setConnectionString(id);
                    groupedDevices.set(id, deviceInfo);
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

    *availableDevices = availableDevicesPtr.detach();

    availableDevicesGroup = groupedDevices;
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
    OPENDAQ_PARAM_NOT_NULL(connectionString);
    OPENDAQ_PARAM_NOT_NULL(device);
    *device = nullptr;

    auto connectionStringPtr = StringPtr::Borrow(connectionString);
    if (!connectionStringPtr.assigned() || connectionStringPtr.getLength() == 0)
        return this->makeErrorInfo(OPENDAQ_ERR_ARGUMENT_NULL, "Connection string is not set or empty");

    bool useSmartConnection = (connectionStringPtr.toStdString().find("daq://") == 0);
    auto configPtr = PropertyObjectPtr::Borrow(config);

    DeviceInfoPtr deviceInfo;
    
    if (useSmartConnection)
    {
        if (!availableDevicesGroup.assigned())
        {
            auto errCode = getAvailableDevices(&ListPtr<IDeviceInfo>());
            if (OPENDAQ_FAILED(errCode))
                return this->makeErrorInfo(errCode, "Failed getting available devices");
        }
        
        if (availableDevicesGroup.hasKey(connectionStringPtr))
            deviceInfo = availableDevicesGroup.get(connectionStringPtr);
        
        if (!deviceInfo.assigned())
        {
            return this->makeErrorInfo(OPENDAQ_ERR_NOTFOUND, fmt::format("Device with connection string \"{}\" not found", connectionStringPtr));
        }

        const auto capabilities = deviceInfo.getServerCapabilities();
        if (capabilities.getCount() == 0)
        {
            return this->makeErrorInfo(OPENDAQ_ERR_NOTFOUND, fmt::format("Device with connection string \"{}\" has no availble server capabilites", connectionStringPtr));
        }

        ServerCapabilityPtr selectedCapability = capabilities[0];
        auto selectedPriority = getServerCapabilityPriority(selectedCapability);

        for (const auto & capability : capabilities)
        {
            const auto priority = getServerCapabilityPriority(capability);
            if (priority  > selectedPriority)
            {
                selectedCapability = capability;
                selectedPriority = priority;
            }
        }

        if (selectedCapability.assigned())
            connectionStringPtr = selectedCapability.getConnectionString();
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
                    for (const auto & connection: capability.getConnectionStrings())
                    {
                        if (connection == connectionStringPtr)
                        {
                            deviceInfo = info;
                            break;
                        }
                    }
                }
                if (deviceInfo.assigned())
                    break;
            }
        }
    }
    
    for (const auto& library : libraries)
    {
        const auto module = library.module;

        bool accepted{};
        try
        {
            accepted = module.acceptsConnectionParameters(connectionStringPtr,
                                                          useSmartConnection ? nullptr : configPtr);
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
            auto errCode = module->createDevice(device, connectionStringPtr, parent, useSmartConnection ? nullptr : configPtr);

            if (OPENDAQ_FAILED(errCode))
                return errCode;

            auto devicePtr = DevicePtr::Borrow(*device); 
            if (devicePtr.assigned() && deviceInfo.assigned() && devicePtr.getInfo().assigned())
            {
                auto deviceInfoConfig = devicePtr.getInfo().asPtr<IDeviceInfoInternal>();
                for (const auto & capability : deviceInfo.getServerCapabilities())
                {
                    if (deviceInfoConfig.hasServerCapability(capability.getProtocolId()))
                        deviceInfoConfig.removeServerCapability(capability.getProtocolId());
                    deviceInfoConfig.addServerCapability(capability);
                }
            }

            // automatically skips streaming connection for local and pseudo (streaming) devices
            // FIXME: filters out native config devices
            auto mirroredDeviceConfigPtr = devicePtr.asPtrOrNull<IMirroredDeviceConfig>();
            if (mirroredDeviceConfigPtr.assigned() &&
                connectionStringPtr.toStdString().find("daq.nd://") != 0)
            {
                configureStreamings(mirroredDeviceConfigPtr,
                                    useSmartConnection ? configPtr : nullptr);
            }

            return errCode;
        }
    }

    return this->makeErrorInfo(
        OPENDAQ_ERR_NOTFOUND,
        "Device with given connection string and config is not available [{}]",
        connectionString
    );
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
        OPENDAQ_ERR_NOTFOUND,
        fmt::format(R"(Function block with given uid and config is not available [{}])", typeId)
    );
}

uint16_t ModuleManagerImpl::getServerCapabilityPriority(const ServerCapabilityPtr& cap)
{
    const std::string nativeId = "opendaq_native_config";
    if (cap.getProtocolId() == nativeId)
        return 42;

    switch (cap.getProtocolType())
    {
        case ProtocolType::Unknown:
            return 0;
        case ProtocolType::Streaming:
            return 1;
        case ProtocolType::Configuration:
            return 2;
        case ProtocolType::ConfigurationAndStreaming:
            return 3;
    }

    return 0;
}

ErrCode ModuleManagerImpl::registerDiscoveryDevice(IString* serverId, IPropertyObject* config)
{
    OPENDAQ_PARAM_NOT_NULL(serverId);
    OPENDAQ_PARAM_NOT_NULL(config);
    discoveryServer.registerDevice(serverId, config);
    return OPENDAQ_SUCCESS;
}

ErrCode ModuleManagerImpl::removeDiscoveryDevice(IString* serverId)
{
    OPENDAQ_PARAM_NOT_NULL(serverId);
    discoveryServer.removeDevice(serverId);
    return OPENDAQ_SUCCESS;
}

PropertyObjectPtr ModuleManagerImpl::populateStreamingConfig(const PropertyObjectPtr& streamingConfig)
{
    PropertyObjectPtr resultConfig = streamingConfig.assigned() ? streamingConfig : PropertyObject();

    if (!resultConfig.hasProperty("StreamingConnectionHeuristic"))
    {
        const auto streamingConnectionHeuristicProp =  SelectionProperty("StreamingConnectionHeuristic",
                                                                        List<IString>("MinConnections",
                                                                                      "MinHops",
                                                                                      "Fallbacks",
                                                                                      "NotConnected"),
                                                                        0);
        resultConfig.addProperty(streamingConnectionHeuristicProp);
    }

    if (!resultConfig.hasProperty("PrioritizedStreamingProtocols"))
    {
        auto prioritizedStreamingProtocols = List<IString>("opendaq_native_streaming", "opendaq_lt_streaming");
        resultConfig.addProperty(ListProperty("PrioritizedStreamingProtocols", prioritizedStreamingProtocols));
    }

    return resultConfig;
}

ListPtr<IMirroredDeviceConfig> ModuleManagerImpl::getAllDevicesRecursively(const MirroredDeviceConfigPtr& device)
{
    auto result = List<IMirroredDeviceConfig>();

    auto childDevices = device.getDevices();
    for (const auto& childDevice : childDevices)
    {
        auto subDevices = getAllDevicesRecursively(childDevice);
        for (const auto& subDevice : subDevices)
        {
            result.pushBack(subDevice);
        }
    }

    result.pushBack(device);

    return result;
}

void ModuleManagerImpl::attachStreamingsToDevice(const MirroredDeviceConfigPtr& device,
                                                 const ListPtr<IString>& prioritizedStreamingProtocols,
                                                 bool overrideActiveSourceForSignal)
{
    auto deviceInfo = device.getInfo();
    auto signals = device.getSignals(search::Recursive(search::Any()));
    std::optional<std::pair<StringPtr, SizeT>> primaryProtocolParams;

    // protocol Id as a key, protocol priority as a value
    std::map<StringPtr, SizeT> prioritizedProtocolsMap;
    const auto protocolsCount = prioritizedStreamingProtocols.getCount();
    for (SizeT index = 0; index < protocolsCount; ++index)
    {
        prioritizedProtocolsMap.insert({prioritizedStreamingProtocols[index], protocolsCount - index});
    }

    // connect via all allowed streaming protocols
    for (const auto& cap : device.getInfo().getServerCapabilities())
    {
        LOG_D("Device {} has capability: name [{}] id [{}] string [{}] prefix [{}]",
              device.getGlobalId(),
              cap.getProtocolName(),
              cap.getProtocolId(),
              cap.getConnectionString(),
              cap.getPrefix());

        const StringPtr protocolId = cap.getPropertyValue("protocolId");
        if (cap.getProtocolType() != ProtocolType::Streaming)
            continue;

        const auto it = prioritizedProtocolsMap.find(protocolId);
        if (it == prioritizedProtocolsMap.end())
            continue;
        const SizeT protocolPriority = it->second;

        const auto streaming = createStreaming(cap.getConnectionString(), cap);
        if (!streaming.assigned())
            continue;

        // keep track of protocol with the highest priority
        if (primaryProtocolParams.has_value())
        {
            const auto primaryProtocolPriority = primaryProtocolParams.value().second;

            if (protocolPriority > primaryProtocolPriority)
                primaryProtocolParams = {cap.getPrefix(), protocolPriority};
        }
        else
        {
            primaryProtocolParams = {cap.getPrefix(), protocolPriority};
        }

        LOG_I("Device {} added streaming connection {}", device.getGlobalId(), streaming.getConnectionString());
        device.addStreamingSource(streaming);

        streaming.addSignals(signals);
        streaming.setActive(true);
    }

    // set the streaming source with the highest priority as active for device signals
    if (primaryProtocolParams.has_value())
    {
        auto primaryProtocolPrefix = primaryProtocolParams.value().first.toStdString();
        for (const auto& streaming : device.getStreamingSources())
        {
            auto connectionString = streaming.getConnectionString();
            if (connectionString.toStdString().find(primaryProtocolPrefix) == 0)
            {
                for (const auto& signal : signals)
                {
                    if (!signal.getPublic())
                        continue;
                    MirroredSignalConfigPtr mirroredSignalConfigPtr = signal.template asPtr<IMirroredSignalConfig>();
                    if (!mirroredSignalConfigPtr.getActiveStreamingSource().assigned() || overrideActiveSourceForSignal)
                        mirroredSignalConfigPtr.setActiveStreamingSource(connectionString);
                }
            }
        }
    }
}

void ModuleManagerImpl::configureStreamings(MirroredDeviceConfigPtr& topDevice, const PropertyObjectPtr& config)
{
    auto streamingConfig = populateStreamingConfig(config);

    const StringPtr streamingHeuristic = streamingConfig.getPropertySelectionValue("StreamingConnectionHeuristic");
    const ListPtr<IString> prioritizedStreamingProtocols = streamingConfig.getPropertyValue("PrioritizedStreamingProtocols");

    if (streamingHeuristic == "MinConnections")
    {
        attachStreamingsToDevice(topDevice, prioritizedStreamingProtocols, false);
    }
    else if (streamingHeuristic == "MinHops" || streamingHeuristic == "Fallbacks")
    {
        // The order of handling nested devices is important since we need to establish streaming connections
        // for the leaf devices first. The custom function is used to get the list of sub-devices
        // recursively, because using the recursive search filter does not guarantee the required order
        auto allDevicesInTree = getAllDevicesRecursively(topDevice);
        for (const auto& device : allDevicesInTree)
        {
            attachStreamingsToDevice(device, prioritizedStreamingProtocols, (streamingHeuristic == "Fallbacks"));
        }
    }
}

StreamingPtr ModuleManagerImpl::createStreaming(const StringPtr& connectionString,
                                                const ServerCapabilityPtr& capability)
{
    StreamingPtr streaming = nullptr;
    for (const auto& library : libraries)
    {
        const auto module = library.module;
        bool accepted{};
        try
        {
            accepted = module.acceptsStreamingConnectionParameters(connectionString, capability);
        }
        catch(NotImplementedException&)
        {
            LOG_I("{}: acceptsStreamingConnectionParameters not implemented", module.getName())
            accepted = false;
        }
        catch(const std::exception& e)
        {
            LOG_W("{}: acceptsStreamingConnectionParameters failed: {}", module.getName(), e.what())
            accepted = false;
        }

        if (accepted)
        {
            streaming = module.createStreaming(connectionString, capability);
        }
    }
    return streaming;
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
