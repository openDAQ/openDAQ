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
#include <boost/algorithm/string.hpp>
#include <opendaq/search_filter_factory.h>
#include <coretypes/validation.h>
#include <opendaq/server_capability_config_ptr.h>
#include <opendaq/icmp_ping.h>
#include <coreobjects/property_object_factory.h>
#include <coreobjects/property_factory.h>
#include <opendaq/mirrored_signal_config_ptr.h>
#include <optional>
#include <map>
#include <opendaq/device_info_factory.h>
#include <opendaq/address_info_private_ptr.h>
#include <coreobjects/property_object_protected_ptr.h>
#include <coreobjects/property_internal_ptr.h>
#include <coreobjects/property_object_internal.h>
#include <coreobjects/eval_value_factory.h>

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

    return daqTry([&]()
    {
        for (const auto& path: paths)
        {
            try
            {
                auto localLibraries = enumerateModules(loggerComponent, path, context);
                libraries.insert(libraries.end(), localLibraries.begin(), localLibraries.end());                
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
        modulesLoaded = true;
        return OPENDAQ_SUCCESS;
    });
}

struct DevicePing
{
    std::string address;
    std::shared_ptr<IcmpPing> ping;
};

void ModuleManagerImpl::checkNetworkSettings(ListPtr<IDeviceInfo>& list)
{
    using namespace std::chrono_literals;

    std::vector<DevicePing> statuses;
    std::map<std::string, bool> ipv4Addresses;
    std::map<std::string, bool> ipv6Addresses;

    for (auto deviceInfo : list)
    {
        for (const auto& cap : deviceInfo.getServerCapabilities())
        {
            if (cap.getConnectionType() != "TCP/IP")
                continue;

            const auto addressInfos = cap.getAddressInfo();
            for (const auto& info : addressInfos)
            {
                const auto address = info.getAddress();
                const auto addressType = info.getType();
                if (addressType == "IPv4")
                    ipv4Addresses.emplace(address.toStdString(), false);
                else if (addressType == "IPv6")
                    ipv6Addresses.emplace(address.toStdString(), false);
            }
        }
    }

    for (auto& address : ipv4Addresses)
    {
        const auto icmp = IcmpPing::Create(ioContext, logger);
        IcmpPing& ping = *icmp;

        ping.setMaxHops(1);
        ping.start(boost::asio::ip::make_address_v4(address.first));
        if (ping.waitSend())
        {
            address.second = true;
            continue;
        }

        statuses.push_back({address.first, icmp});
    }

    if (!statuses.empty())
    {
        LOG_T("Missing ping replies: waiting 1s\n");
        std::this_thread::sleep_for(1s);

        for (const auto& [address, ping] : statuses)
        {
            ping->stop();
            ipv4Addresses[address] = ping->getNumReplies() > 0;
        }
        statuses.clear();
    }

    setAddressesReachable(ipv4Addresses, "IPv4", list);

    // TODO: Check ipv6 for reachability
}

void ModuleManagerImpl::setAddressesReachable(const std::map<std::string, bool>& addr, const std::string& type, ListPtr<IDeviceInfo>& info)
{
    for (const auto& devInfo : info)
    {
        for (const auto& cap : devInfo.getServerCapabilities())
        {
            const auto addressInfos = cap.getAddressInfo();
            for (const auto& addressInfo : addressInfos)
            {
                const auto address = addressInfo.getAddress();
                const auto addressType = addressInfo.getType();

                if (addr.count(address) && addressType == type)
                {
                    AddressReachabilityStatus reachability = addr.at(address)
                                                                 ? AddressReachabilityStatus::Reachable
                                                                 : AddressReachabilityStatus::Unreachable;
                    addressInfo.asPtr<IAddressInfoPrivate>().setReachabilityStatusPrivate(reachability);

                    if (reachability == AddressReachabilityStatus::Unreachable && cap.getConnectionString() == addressInfo.getConnectionString())
                    {
                        for (const auto& addressInfoInner : cap.getAddressInfo())
                        {
                            if (addressInfoInner == addressInfo)
                                continue;

                            if (addressInfoInner.getReachabilityStatus() != AddressReachabilityStatus::Unreachable)
                            {
                                cap.asPtr<IServerCapabilityConfig>().setConnectionString(addressInfoInner.getConnectionString());
                                break;
                            }
                        }
                    }

                    break;
                }
            }
        }
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

            // Group devices that have manufacturer, serial number and at least 1 server capability,
            // the rest use their connection string as key.
            if (manufacturer.getLength() == 0 || serialNumber.getLength() == 0)
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
                else if (deviceInfo.getServerCapabilities().getCount())
                {
                    deviceInfo.asPtr<IDeviceInfoConfig>().setConnectionString(id);
                    groupedDevices.set(id, deviceInfo);
                }
                else
                {
                    groupedDevices.set(deviceInfo.getConnectionString(), deviceInfo);
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

    try
    {
        const auto [pureConnectionString, connectionStringOptions] = splitConnectionStringAndOptions(StringPtr::Borrow(connectionString));
        auto connectionStringPtr = String(pureConnectionString);

        if (!connectionStringPtr.assigned() || connectionStringPtr.getLength() == 0)
            return this->makeErrorInfo(OPENDAQ_ERR_ARGUMENT_NULL, "Connection string is not set or empty");

        // Scan for devices if not yet done so
        // TODO: Should we re-scan after a timeout?
        if (!availableDevicesGroup.assigned())
        {
            const auto errCode = getAvailableDevices(&ListPtr<IDeviceInfo>());
            if (OPENDAQ_FAILED(errCode))
                return this->makeErrorInfo(errCode, "Failed getting available devices");
        }

        // Connection strings with the "daq" prefix automatically choose the best method of connection
        const bool useSmartConnection = connectionStringPtr.toStdString().find("daq://") == 0;
        const auto discoveredDeviceInfo = getDiscoveredDeviceInfo(connectionStringPtr, useSmartConnection);
        if (useSmartConnection)
            connectionStringPtr = resolveSmartConnectionString(connectionStringPtr, discoveredDeviceInfo, config, loggerComponent);

        for (const auto& library : libraries)
        {
            const auto deviceType = getDeviceTypeFromConnectionString(connectionStringPtr, library.module);
            
            // Check if module can create device with given connection string
            if (!deviceType.assigned())
                continue;

            const auto devConfig = populateDeviceConfig(config, deviceType, connectionStringOptions);
            const auto err = library.module->createDevice(device, connectionStringPtr, parent, devConfig);
            checkErrorInfo(err);

            const auto devicePtr = DevicePtr::Borrow(*device);
            if (devicePtr.assigned() && devicePtr.getInfo().assigned())
            {
                replaceSubDeviceOldProtocolIds(devicePtr);
                mergeDiscoveryAndDeviceCapabilities(devicePtr, discoveredDeviceInfo);
                completeServerCapabilities(devicePtr);

                // automatically skips streaming connection for local and pseudo (streaming) devices
                if (const auto mirroredDeviceConfigPtr = devicePtr.asPtrOrNull<IMirroredDeviceConfig>(true); mirroredDeviceConfigPtr.assigned())
                    configureStreamings(mirroredDeviceConfigPtr, config);
            }

            return err;
        }
    }
    catch (const DaqException& e)
    {
        return errorFromException(e);
    }
    catch (const std::exception& e)
    {
        return makeErrorInfo(OPENDAQ_ERR_GENERALERROR, e.what(), nullptr);
    }
    catch (...)
    {
        return OPENDAQ_ERR_GENERALERROR;
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

StringPtr ModuleManagerImpl::convertIfOldIdFB(const StringPtr& id)
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

StringPtr ModuleManagerImpl::convertIfOldIdProtocol(const StringPtr& id)
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

void ModuleManagerImpl::populateDeviceConfigFromConnStrOptions(const PropertyObjectPtr& devConfig,
    const tsl::ordered_map<std::string, ObjectPtr<IBaseObject>>& options)
{
    for (const auto& item: options)
    {
        if (devConfig.hasProperty(item.first))
            devConfig.setPropertyValue(item.first, item.second);
    }
}

PropertyObjectPtr ModuleManagerImpl::populateDeviceConfig(const PropertyObjectPtr& config,
                                                          const DeviceTypePtr& deviceType,
                                                          const tsl::ordered_map<std::string, ObjectPtr<IBaseObject>>& connStrOptions)
{
    PropertyObjectPtr configClone;
    if (config.assigned())
        checkErrorInfo(config.asPtr<IPropertyObjectInternal>()->clone(&configClone));
    else
        configClone = deviceType.createDefaultConfig();

    const bool isDefaultAddDeviceConfigRes = isDefaultAddDeviceConfig(configClone);
    const PropertyObjectPtr generalConfig = isDefaultAddDeviceConfigRes ? configClone.getPropertyValue("General").asPtr<IPropertyObject>() : PropertyObject();
    PropertyObjectPtr devConfig = isDefaultAddDeviceConfigRes ? configClone.getPropertyValue("Device").asPtr<IPropertyObject>() : configClone;

    const StringPtr id = deviceType.getId();
    if (!devConfig.assigned())
        devConfig = deviceType.createDefaultConfig();

    if (isDefaultAddDeviceConfigRes && devConfig.hasProperty(id))
    {
        devConfig = devConfig.getPropertyValue(id);
        copyGeneralProperties(generalConfig, devConfig);
    }

    if (!connStrOptions.empty())
        populateDeviceConfigFromConnStrOptions(devConfig, connStrOptions);

    return devConfig;
}

ErrCode ModuleManagerImpl::createFunctionBlock(IFunctionBlock** functionBlock, IString* id, IComponent* parent, IPropertyObject* config, IString* localId)
{
    OPENDAQ_PARAM_NOT_NULL(functionBlock);
    OPENDAQ_PARAM_NOT_NULL(id);

    const StringPtr typeId = convertIfOldIdFB(StringPtr::Borrow(id));

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

ErrCode ModuleManagerImpl::createStreaming(IStreaming** streaming, IString* connectionString, IPropertyObject* config)
{
    OPENDAQ_PARAM_NOT_NULL(connectionString);
    OPENDAQ_PARAM_NOT_NULL(streaming);

    StreamingPtr streamingPtr;
    const ErrCode errCode = wrapHandlerReturn(this, &ModuleManagerImpl::onCreateStreaming, streamingPtr, connectionString, config);

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
            LOG_I("{}: GetAvailableStreamingTypes not implemented", module.getName())
        }
        catch ([[maybe_unused]] const std::exception& e)
        {
            LOG_W("{}: GetAvailableStreamingTypes failed: {}", module.getName(), e.what())
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
    if (OPENDAQ_FAILED(err))
        return err;
    
    DictPtr<IString, IStreamingType> streamingTypes;
    err = getAvailableStreamingTypes(&streamingTypes);
    if (OPENDAQ_FAILED(err))
        return err;

    auto config = PropertyObject();
    
    auto deviceConfig = PropertyObject();
    auto streamingConfig = PropertyObject();
    auto generalConfig = PropertyObject();

    for (auto const& [key, typeObj] : deviceTypes)
        deviceConfig.addProperty(ObjectProperty(key, typeObj.createDefaultConfig()));
    
    for (auto const& [key, typeObj] : streamingTypes)
        streamingConfig.addProperty(ObjectProperty(key, typeObj.createDefaultConfig()));

    config.addProperty(ObjectProperty("Device", deviceConfig.detach()));
    config.addProperty(ObjectProperty("Streaming", streamingConfig.detach()));
    config.addProperty(ObjectProperty("General", createGeneralConfig().detach()));

    *defaultConfig = config.detach();
    return OPENDAQ_SUCCESS;
}

ErrCode ModuleManagerImpl::createServer(IServer** server, IString* serverTypeId, IDevice* rootDevice, IPropertyObject* serverConfig)
{
    OPENDAQ_PARAM_NOT_NULL(serverTypeId);
    OPENDAQ_PARAM_NOT_NULL(server);
    OPENDAQ_PARAM_NOT_NULL(rootDevice);

    auto typeId = convertIfOldIdProtocol(toStdString(serverTypeId));

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

    return OPENDAQ_ERR_NOTFOUND;
}

uint16_t ModuleManagerImpl::getServerCapabilityPriority(const ServerCapabilityPtr& cap)
{
    const std::string nativeId = "OpenDAQNativeConfiguration";
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

DeviceInfoPtr ModuleManagerImpl::getDiscoveredDeviceInfo(const StringPtr& inputConnectionString, bool useSmartConnection) const
{
    if (!availableDevicesGroup.assigned())
        throw NotFoundException("Device scan has not yet been initiated.");

    if (useSmartConnection)
    {
        if (availableDevicesGroup.hasKey(inputConnectionString))
            return availableDevicesGroup.get(inputConnectionString);
        throw NotFoundException(fmt::format("Device with connection string \"{}\" not found", inputConnectionString));
    }

    for (const auto & [_, info] : availableDevicesGroup)
    {
        if (info.getServerCapabilities().getCount() == 0)
        {
            if (info.getConnectionString() == inputConnectionString)
                return info;
        }
        else
        {
            for(const auto & capability : info.getServerCapabilities())
            {
                for (const auto & connection: capability.getConnectionStrings())
                {
                    if (connection == inputConnectionString)
                        return info;
                }
            }
        }
    }

    return nullptr;
}

StringPtr ModuleManagerImpl::resolveSmartConnectionString(const StringPtr& inputConnectionString,
                                                          const DeviceInfoPtr& discoveredDeviceInfo,
                                                          const PropertyObjectPtr& config,
                                                          const LoggerComponentPtr& loggerComponent)
{
    const auto capabilities = discoveredDeviceInfo.getServerCapabilities();
    if (capabilities.getCount() == 0)
        throw NotFoundException(fmt::format("Device with connection string \"{}\" has no available server capabilities", inputConnectionString));

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

    const PropertyObjectPtr generalConfig = isDefaultAddDeviceConfig(config)
                                                ? config.getPropertyValue("General").asPtr<IPropertyObject>()
                                                : populateGeneralConfig(config);
    const StringPtr primaryAddessType = generalConfig.getPropertyValue("PrimaryAddressType");
    if (isValidConnectionAddressType(primaryAddessType))
    {
        for (const auto& addrInfo : selectedCapability.getAddressInfo())
        {
            if (addrInfo.getType() == primaryAddessType)
                return addrInfo.getConnectionString();
        }
        LOG_W("Selected server capability of device with connection string \"{}\" does not provide any addresses of primary {} type",
              inputConnectionString,
              primaryAddessType);
    }
    return selectedCapability.getConnectionString();
}

DeviceTypePtr ModuleManagerImpl::getDeviceTypeFromConnectionString(const StringPtr& connectionString, const ModulePtr& module) const
{
    const std::string prefix = getPrefixFromConnectionString(connectionString);

    DictPtr<IString, IDeviceType> types;
    const ErrCode err = module->getAvailableDeviceTypes(&types);
    if (err != OPENDAQ_ERR_NOTIMPLEMENTED && OPENDAQ_FAILED(err))
        throwExceptionFromErrorCode(err);

    if (!types.assigned())
        return nullptr;

    for (auto const& [typeId, type] : types)
    {
        if (type.getConnectionStringPrefix()== prefix)
            return type;
    }

    return nullptr;
}

PropertyObjectPtr ModuleManagerImpl::populateGeneralConfig(const PropertyObjectPtr& config)
{
    auto defConfig = createGeneralConfig();
    if (!config.assigned())
        return defConfig;

    for (const auto& prop : defConfig.getAllProperties())
    {
        const auto name = prop.getName();
        if (config.hasProperty(name))
            defConfig.setPropertyValue(name, config.getPropertyValue(name));
    }

    return defConfig;
}

ListPtr<IMirroredDeviceConfig> ModuleManagerImpl::getAllDevicesRecursively(const MirroredDeviceConfigPtr& device)
{
    auto result = List<IMirroredDeviceConfig>();

    const auto childDevices = device.getDevices();
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

AddressInfoPtr ModuleManagerImpl::getDeviceConnectionAddress(const DevicePtr& device)
{
    const auto info = device.getInfo();
    const auto configConnection = info.getConfigurationConnectionInfo();
    const auto deviceInfoConnectionString = info.getConnectionString();

    if (!configConnection.assigned())
        return nullptr;

    const auto deviceConnectionString =
        (deviceInfoConnectionString.assigned() && deviceInfoConnectionString.getLength())
            ? deviceInfoConnectionString
            : configConnection.getConnectionString();

    for (const auto& addressInfo : configConnection.getAddressInfo())
    {
        if (deviceConnectionString == addressInfo.getConnectionString())
            return addressInfo;
    }

    return nullptr;
}

AddressInfoPtr ModuleManagerImpl::findStreamingAddress(const ListPtr<IAddressInfo>& availableAddresses,
                                                       const AddressInfoPtr& deviceConnectionAddress,
                                                       const StringPtr& primaryAddressType)
{
    if (isValidConnectionAddressType(primaryAddressType)) // restrict by connection address type
    {
        // Attempt to reuse the address of device connection if it meets type constraints
        if (deviceConnectionAddress.assigned() && deviceConnectionAddress.getType() == primaryAddressType)
        {
            for (const auto& addressInfo : availableAddresses)
                if (addressInfo.getAddress() == deviceConnectionAddress.getAddress())
                    return addressInfo;
        }

        // If the device connection address is unavailable for streaming, search for any address matching type constraints
        for (const auto& addressInfo : availableAddresses)
        {
            if (addressInfo.getType() == primaryAddressType)
                return addressInfo;
        }
        LOG_W("Server streaming capability does not provide any addresses of primary {} type", primaryAddressType);
    }

    // Attempt to reuse the address of device connection
    for (const auto& addressInfo : availableAddresses)
    {
        if (addressInfo.getAddress() == deviceConnectionAddress.getAddress())
            return addressInfo;
    }

    return nullptr;
}

bool ModuleManagerImpl::isValidConnectionAddressType(const StringPtr& connectionAddressType)
{
    return connectionAddressType == "IPv4" || connectionAddressType == "IPv6";
}

void ModuleManagerImpl::attachStreamingsToDevice(const MirroredDeviceConfigPtr& device,
                                                 const PropertyObjectPtr& generalConfig,
                                                 const PropertyObjectPtr& config,
                                                 const AddressInfoPtr& deviceConnectionAddress)
{
    auto deviceInfo = device.getInfo();
    auto signals = device.getSignals(search::Recursive(search::Any()));
    
    const ListPtr<IString> prioritizedStreamingProtocols = generalConfig.getPropertyValue("PrioritizedStreamingProtocols");
    const ListPtr<IString> allowedStreamingProtocols = generalConfig.getPropertyValue("AllowedStreamingProtocols");
    const StringPtr primaryAddessType = generalConfig.getPropertyValue("PrimaryAddressType");

    // protocol Id as a key, protocol priority as a value
    std::unordered_set<std::string> allowedProtocolsSet;
    for (SizeT index = 0; index < allowedStreamingProtocols.getCount(); ++index)
    {
        allowedProtocolsSet.insert(allowedStreamingProtocols[index]);
    }

    // protocol Id as a key, protocol priority as a value
    std::map<StringPtr, SizeT> prioritizedProtocolsMap;
    for (SizeT index = 0; index < prioritizedStreamingProtocols.getCount(); ++index)
    {
        prioritizedProtocolsMap.insert({prioritizedStreamingProtocols[index], index});
    }

    // protocol priority as a key, streaming source as a value
    std::map<SizeT, StreamingPtr> prioritizedStreamingSourcesMap;

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

        if (!allowedProtocolsSet.empty() && !allowedProtocolsSet.count(protocolId))
            continue;

        const auto protocolIt = prioritizedProtocolsMap.find(protocolId);
        if (protocolIt == prioritizedProtocolsMap.end())
            continue;
        const SizeT protocolPriority = protocolIt->second;

        StreamingPtr streaming;
        const auto streamingAddress = findStreamingAddress(cap.getAddressInfo(), deviceConnectionAddress, primaryAddessType);
        StringPtr connectionString = streamingAddress.assigned() ? streamingAddress.getConnectionString() : cap.getConnectionString();

        if (!connectionString.assigned())
            continue;

        wrapHandlerReturn(this, &ModuleManagerImpl::onCreateStreaming, streaming, connectionString, config);
        if (!streaming.assigned())
            continue;

        prioritizedStreamingSourcesMap.insert_or_assign(protocolPriority, streaming);
    }

    // add streaming sources ordered by protocol priority
    for (const auto& [_, streaming] : prioritizedStreamingSourcesMap)
    {
        device.addStreamingSource(streaming);
        LOG_I("Device {} added streaming connection {}", device.getGlobalId(), streaming.getConnectionString());

        streaming.addSignals(signals);
        streaming.setActive(true);
    }

    // set the streaming source with the highest priority as active for device signals
    if (!prioritizedStreamingSourcesMap.empty())
    {
        for (const auto& signal : signals)
        {
            if (!signal.getPublic())
                continue;

            MirroredSignalConfigPtr mirroredSignalConfigPtr = signal.template asPtr<IMirroredSignalConfig>();
            if (!mirroredSignalConfigPtr.getActiveStreamingSource().assigned())
            {
                auto signalStreamingSources = mirroredSignalConfigPtr.getStreamingSources();
                for (const auto& [_, streaming] : prioritizedStreamingSourcesMap)
                {
                    auto connectionString = streaming.getConnectionString();

                    auto it = std::find(signalStreamingSources.begin(), signalStreamingSources.end(), connectionString);
                    if (it != signalStreamingSources.end())
                    {
                        mirroredSignalConfigPtr.setActiveStreamingSource(connectionString);
                        break;
                    }
                }
            }
        }
    }
}

void ModuleManagerImpl::configureStreamings(const MirroredDeviceConfigPtr& topDevice, const PropertyObjectPtr& config)
{
    PropertyObjectPtr generalConfig;
    PropertyObjectPtr addDeviceConfig;

    // Get the address used for device connection
    const auto deviceConnectionAddress = getDeviceConnectionAddress(topDevice);

    if (isDefaultAddDeviceConfig(config))
    {
        addDeviceConfig = config;
        generalConfig = config.getPropertyValue("General");
    }
    else
    {
        generalConfig = populateGeneralConfig(config);
    }

    const StringPtr streamingHeuristic = generalConfig.getPropertySelectionValue("StreamingConnectionHeuristic");
    const bool automaticallyConnectStreaming = generalConfig.getPropertyValue("AutomaticallyConnectStreaming");
    if (!automaticallyConnectStreaming)
        return;

    if (streamingHeuristic == "MinConnections")
    {
        attachStreamingsToDevice(topDevice, generalConfig, addDeviceConfig, deviceConnectionAddress);
    }
    else if (streamingHeuristic == "MinHops")
    {
        // The order of handling nested devices is important since we need to establish streaming connections
        // for the leaf devices first. The custom function is used to get the list of sub-devices
        // recursively, because using the recursive search filter does not guarantee the required order
        const auto allDevicesInTree = getAllDevicesRecursively(topDevice);
        for (const auto& device : allDevicesInTree)
        {
            attachStreamingsToDevice(device, generalConfig, addDeviceConfig, deviceConnectionAddress);
        }
    }
}

StreamingPtr ModuleManagerImpl::onCreateStreaming(const StringPtr& connectionString, const PropertyObjectPtr& config) const
{
    const bool isDefaultDeviceConfig = isDefaultAddDeviceConfig(config);
    StreamingPtr streaming = nullptr;
    PropertyObjectPtr generalConfig = isDefaultDeviceConfig ? config.getPropertyValue("General").asPtr<IPropertyObject>() : PropertyObject();
    PropertyObjectPtr streamingConfig = isDefaultDeviceConfig ? config.getPropertyValue("Streaming").asPtr<IPropertyObject>() : config;

    for (const auto& library : libraries)
    {
        const auto module = library.module;
    
        const std::string prefix = getPrefixFromConnectionString(connectionString);
        DictPtr<IString, IStreamingType> types;
        module->getAvailableStreamingTypes(&types);
        if (!types.assigned())
            continue;

        StringPtr id;
        for (auto const& [typeId, type] : types)
        {
            if (type.getConnectionStringPrefix() == prefix)
            {
                id = typeId;
                break;
            }
        }

        if (!id.assigned())
            continue;

        if (isDefaultDeviceConfig)
        {
            if (streamingConfig.hasProperty(id))
            {
                streamingConfig = streamingConfig.getPropertyValue(id);
                copyGeneralProperties(generalConfig, streamingConfig);
            }
            else
            {
                streamingConfig = nullptr;
            }
        }

        try
        {
            streaming = module.createStreaming(connectionString, streamingConfig);
        }
        catch ([[maybe_unused]] const std::exception& e)
        {
            LOG_E("{}: createStreaming failed: {}", module.getName(), e.what())
            throw;
        }
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
                LOG_D("{}: completeServerCapability not implemented", module.getName());
            }
            catch ([[maybe_unused]] const std::exception& e)
            {
                LOG_W("{}: completeServerCapability failed: {}", module.getName(), e.what());
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
                info.asPtr<IAddressInfoPrivate>().setReachabilityStatusPrivate(AddressReachabilityStatus::Reachable);
            }
        }
    }
}

PropertyObjectPtr ModuleManagerImpl::createGeneralConfig()
{
    auto obj = PropertyObject();

    const auto streamingConnectionHeuristicProp =  SelectionProperty("StreamingConnectionHeuristic",
                                                                    List<IString>("MinConnections",
                                                                                  "MinHops",
                                                                                  "NotConnected"),
                                                                    0);
    obj.addProperty(streamingConnectionHeuristicProp);

    auto prioritizedStreamingProtocols = List<IString>("OpenDAQNativeStreaming", "OpenDAQLTStreaming");
    obj.addProperty(ListProperty("PrioritizedStreamingProtocols", prioritizedStreamingProtocols));

    obj.addProperty(ListProperty("AllowedStreamingProtocols", List<IString>()));

    obj.addProperty(BoolProperty("AutomaticallyConnectStreaming", true));

    obj.addProperty(StringProperty("Username", ""));
    obj.addProperty(StringProperty("Password", ""));

    obj.addProperty(
        StringPropertyBuilder("PrimaryAddressType", "")
            .setDescription("Specifies the primary address type for establishing configuration and streaming protocols connections "
                            "while using smart connection string with \"daq://\" prefix. "
                            "Acceptable values are \"IPv4\" or \"IPv6\"; if left blank, any address type may be used. "
                            "If no addresses of the specified type are available, the first available address of the alternate type will be used.")
            .build()
    );

    return obj.detach();
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

std::pair<std::string, tsl::ordered_map<std::string, BaseObjectPtr>> ModuleManagerImpl::splitConnectionStringAndOptions(
    const std::string& connectionString)
{
    std::vector<std::string> strs1;
    boost::split(strs1, connectionString, boost::is_any_of("?"));

    if (strs1.size() == 1)
        return std::make_pair(strs1[0], tsl::ordered_map<std::string, BaseObjectPtr> {});

    if (strs1.size() != 2)
        throw InvalidParameterException("Invalid connection string");

    std::vector<std::string> options;
    boost::split(options, strs1[1], boost::is_any_of("&"));

    tsl::ordered_map<std::string, BaseObjectPtr> optionsMap;
    for (const auto& option: options)
    {
        std::vector<std::string> keyAndValue;
        boost::split(keyAndValue, option, boost::is_any_of("="));
        if (keyAndValue.size() != 2)
            throw InvalidParameterException("Invalid connection string");

        BaseObjectPtr value = EvalValue(keyAndValue[1]);
        optionsMap.insert({keyAndValue[0], value});
    }

    return std::make_pair(strs1[0], optionsMap);
}

void ModuleManagerImpl::replaceSubDeviceOldProtocolIds(const DevicePtr& device)
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
            const auto newCap = replaceOldProtocolIds(cap);
            devInfoInternal.removeServerCapability(cap.getProtocolId());
            devInfoInternal.addServerCapability(newCap);
        }
    }
}

ServerCapabilityPtr ModuleManagerImpl::replaceOldProtocolIds(const ServerCapabilityPtr& cap)
{
    const auto oldProtocolId = cap.getProtocolId();
    const auto newProtocolId = convertIfOldIdProtocol(oldProtocolId);

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

ServerCapabilityPtr ModuleManagerImpl::mergeDiscoveryAndDeviceCapability(const ServerCapabilityPtr& discoveryCap,
                                                                         const ServerCapabilityPtr& deviceCap)
{
    auto merged = ServerCapability(deviceCap.getProtocolId(), deviceCap.getProtocolName(), deviceCap.getProtocolType());
    const auto caps = List<IServerCapability>(deviceCap, discoveryCap);

    for (const auto& cap : caps)
        for (const auto& prop : cap.getAllProperties())
        {
            const auto name = prop.getName();
            const auto val = cap.getPropertyValue(name);
            const bool hasProp = merged.hasProperty(name);
            
            if (val == prop.getDefaultValue() && hasProp)
                continue;

            if (hasProp)
                merged.asPtr<IPropertyObjectProtected>().setProtectedPropertyValue(name, val);
            else
                merged.addProperty(prop.asPtr<IPropertyInternal>(true).clone());
        }

    merged.freeze();
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
                    capPtr = mergeDiscoveryAndDeviceCapability(capability, connectedDeviceInfo.getServerCapability(capId));
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

void ModuleManagerImpl::copyGeneralProperties(const PropertyObjectPtr& general, const PropertyObjectPtr& tartgetObj)
{
    if (!general.assigned())
        return;

    for (const auto& prop : general.getAllProperties())
    {
        const auto name = prop.getName();
        const auto value = general.getPropertyValue(name);
        const auto defaultValue = prop.getDefaultValue();

        if (tartgetObj.hasProperty(name) && tartgetObj.getPropertyValue(name) == defaultValue)
            tartgetObj.setPropertyValue(name, value);
    }
}

bool ModuleManagerImpl::isDefaultAddDeviceConfig(const PropertyObjectPtr& config)
{
    if (!config.assigned())
        return false;
    return config.hasProperty("General") && config.hasProperty("Streaming") && config.hasProperty("Device");
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

    auto loadPath = fs::absolute(searchFolder).string();
    LOG_I("Loading modules from '{}'", fs::absolute(searchFolder).string())

    std::vector<ModuleLibrary> moduleDrivers;
    fs::recursive_directory_iterator dirIterator(searchFolder);

    [[maybe_unused]]
    Finally onExit([workingDir = fs::current_path()]()
    {
        fs::current_path(workingDir);
    });

    fs::current_path(searchFolder);
    auto currPath = fs::current_path().string();

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
    auto currDir = fs::current_path();
    auto relativePath = fs::proximate(path).string();
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
