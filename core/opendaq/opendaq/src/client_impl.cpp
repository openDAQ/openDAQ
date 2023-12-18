#include <opendaq/client_impl.h>
#include <opendaq/custom_log.h>
#include <opendaq/device_info_factory.h>
#include <opendaq/create_device.h>
#include <boost/algorithm/string.hpp>
#include <future>

BEGIN_NAMESPACE_OPENDAQ

ClientImpl::ClientImpl(const ContextPtr ctx, const StringPtr& localId, const DeviceInfoPtr& deviceInfo)
    : DeviceBase<IClientPrivate>(ctx, nullptr, localId)
    , manager(this->context.assigned() ? this->context.getModuleManager() : nullptr)
    , logger(ctx.getLogger())
    , loggerComponent( this->logger.assigned()
                          ? this->logger.getOrAddComponent("Client")
                          : throw ArgumentNullException("Logger must not be null"))
    , rootDeviceSet(false)
{
    this->deviceInfo = deviceInfo.assigned() ? deviceInfo : DeviceInfo("", "daq_client");
    this->deviceInfo.freeze();
}

DeviceInfoPtr ClientImpl::onGetInfo()
{
    return this->deviceInfo;
}

DictPtr<IString, IFunctionBlockType> ClientImpl::onGetAvailableFunctionBlockTypes()
{
    std::scoped_lock lock(sync);
    auto availableTypes = Dict<IString, IFunctionBlockType>();

    for (const auto module : manager.getModules())
    {
        DictPtr<IString, IFunctionBlockType> moduleFbTypes;

        try
        {
            moduleFbTypes = module.getAvailableFunctionBlockTypes();
        }
        catch (NotImplementedException&)
        {
            LOG_I("{}: GetAvailableFunctionBlockTypes not implemented", module.getName())
        }
        catch (const std::exception& e)
        {
            LOG_W("{}: GetAvailableFunctionBlockTypes failed: {}", module.getName(), e.what())
        }

        if (!moduleFbTypes.assigned())
            continue;

        for (const auto& [id, type] : moduleFbTypes)
            availableTypes.set(id, type);
    }

    return availableTypes.detach();
}

ComponentPtr ClientImpl::getFunctionBlocksFolder()
{
    ComponentPtr functionBlocksFolder = functionBlocks;

    if (rootDeviceSet)
    {
        auto rootDevicePtr = rootDevice.getRef();
        if (rootDevicePtr.assigned() && rootDevicePtr.supportsInterface<IFolder>())
        {
            const auto folders = rootDevicePtr.asPtr<IFolder>(true);
            try
            {
                functionBlocksFolder = folders.getItem("FB");
            }
            catch (NotFoundException&)
            {
                LOGP_E("Root device has no function blocks folder")
            }
        }
    }

    return functionBlocksFolder;
}

FunctionBlockPtr ClientImpl::onAddFunctionBlock(const StringPtr& typeId, const PropertyObjectPtr& config)
{
    for (const auto module : manager.getModules())
    {
        DictPtr<IString, IFunctionBlockType> types;

        try
        {
            types = module.getAvailableFunctionBlockTypes();
        }
        catch (NotImplementedException&)
        {
            LOG_I("{}: GetAvailableFunctionBlockTypes not implemented", module.getName())
        }
        catch (const std::exception& e)
        {
            LOG_W("{}: GetAvailableFunctionBlockTypes failed: {}", module.getName(), e.what())
        }

        if (!types.assigned())
            continue;
        if (!types.hasKey(typeId))
            continue;

        const FolderConfigPtr fbFolder = getFunctionBlocksFolder();

        std::string localId;
        if (config.assigned() && config.hasProperty("LocalId"))
        {
            localId = static_cast<std::string>(config.getPropertyValue("LocalId"));

            std::vector<std::string> splitStr;
            boost::split(splitStr, localId, boost::is_any_of("_"));

            if (!splitStr.empty())
            {
                size_t cnt = std::stoi(*(splitStr.end() - 1)) + 1;

                if (!functionBlockCountMap.count(typeId))
                    functionBlockCountMap.insert(std::pair<std::string, size_t>(typeId, cnt));
                else
                    functionBlockCountMap[typeId] = std::max(cnt, functionBlockCountMap[typeId]);
            }
        }
        else
        {
            if (!functionBlockCountMap.count(typeId))
                functionBlockCountMap.insert(std::pair<std::string, size_t>(typeId, 0));

            localId = fmt::format("{}_{}", typeId, functionBlockCountMap[typeId]++);
        }

        auto fb = module.createFunctionBlock(typeId, fbFolder, localId, config);
        fbFolder.addItem(fb);
        return fb;
    }

    throw NotFoundException{"Function block with given uid is not available."};
}

void ClientImpl::onRemoveFunctionBlock(const FunctionBlockPtr& functionBlock)
{
    const FolderConfigPtr fbFolder = getFunctionBlocksFolder();
    fbFolder.removeItem(functionBlock);
}

ListPtr<IDeviceInfo> ClientImpl::onGetAvailableDevices()
{
    auto availableDevices = List<IDeviceInfo>();

    using AsyncEnumerationResult = std::future<ListPtr<IDeviceInfo>>;
    std::vector<std::pair<AsyncEnumerationResult, ModulePtr>> enumerationResults;

    for (const auto module : manager.getModules())
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
            availableDevices.pushBack(deviceInfo);
    }

    return availableDevices.detach();
}

DictPtr<IString, IDeviceType> ClientImpl::onGetAvailableDeviceTypes()
{
    std::scoped_lock lock(sync);
    auto availableTypes = Dict<IString, IDeviceType>();

    for (const auto module : manager.getModules())
    {
        DictPtr<IString, IDeviceType> moduleDeviceTypes;

        try
        {
            moduleDeviceTypes = module.getAvailableDeviceTypes();
        }
        catch (NotImplementedException&)
        {
            LOG_I("{}: GetAvailableDeviceTypes not implemented", module.getName())
        }
        catch (const std::exception& e)
        {
            LOG_W("{}: GetAvailableDeviceTypes failed: {}", module.getName(), e.what())
        }

        if (!moduleDeviceTypes.assigned())
            continue;

        for (const auto& [id, type] : moduleDeviceTypes)
            availableTypes.set(id, type);
    }

    return availableTypes.detach();
}

DevicePtr ClientImpl::onAddDevice(const StringPtr& connectionString, const PropertyObjectPtr& config)
{
    std::scoped_lock lock(sync);

    auto device = detail::createDevice(connectionString, config, devices, manager, loggerComponent);
    devices.addItem(device);

    return device;
}

void ClientImpl::onRemoveDevice(const DevicePtr& device)
{
    this->devices.removeItem(device);
}

ErrCode ClientImpl::setRootDevice(IComponent* rootDevice)
{
    if (rootDeviceSet)
        return makeErrorInfo(OPENDAQ_ERR_INVALIDSTATE, "Root device already set");

    this->rootDevice = rootDevice;
    rootDeviceSet = true;

    return OPENDAQ_SUCCESS;
}

OPENDAQ_DEFINE_CLASS_FACTORY_WITH_INTERFACE_AND_CREATEFUNC(
    LIBRARY_FACTORY, Client, IDevice, createClient,
    IContext*, ctx,
    IString*, localId,
    IDeviceInfo*, defaultDeviceInfo
)

END_NAMESPACE_OPENDAQ
