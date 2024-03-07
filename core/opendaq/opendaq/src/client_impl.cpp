#include <opendaq/client_impl.h>
#include <opendaq/custom_log.h>
#include <opendaq/device_info_factory.h>
#include <opendaq/create_device.h>
#include <boost/algorithm/string.hpp>
#include <future>

BEGIN_NAMESPACE_OPENDAQ

ClientImpl::ClientImpl(const ContextPtr ctx, const StringPtr& localId, const DeviceInfoPtr& deviceInfo)
    : DeviceBase<>(ctx, nullptr, localId)
    , manager(this->context.assigned() ? this->context.getModuleManager() : nullptr)
    , logger(ctx.getLogger())
    , loggerComponent( this->logger.assigned()
                          ? this->logger.getOrAddComponent("Client")
                          : throw ArgumentNullException("Logger must not be null"))
{
    if (deviceInfo.assigned())
        this->deviceInfo = deviceInfo;
    else
        this->deviceInfo = DeviceInfo("", "daq_client");
    this->deviceInfo.freeze();
    this->isRootDevice = true;
}

DeviceInfoPtr ClientImpl::onGetInfo()
{
    return this->deviceInfo;
}

ListPtr<IDeviceInfo> ClientImpl::onGetAvailableDevices()
{
    std::scoped_lock lock(sync);
    auto availableDevices = List<IDeviceInfo>();
    groupedDevices = Dict<IString, IDeviceInfo>();

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
        {
            StringPtr id = "daq://" + deviceInfo.getManufacturer() + "_" + deviceInfo.getSerialNumber();
            if (groupedDevices.hasKey(id))
            {
                DeviceInfoPtr value = groupedDevices.get(id);
                for (const auto & capability : deviceInfo.getDeviceCapabilities())
                    value.asPtr<IDeviceInfoConfig>().addDeviceCapability(capability);
            }
            else
            {
                if (deviceInfo.getDeviceCapabilities().getCount() == 0)
                {
                    deviceInfo.asPtr<IDeviceInfoConfig>().addDeviceCapability(DeviceCapability(deviceInfo.getConnectionString()));
                }

                deviceInfo.asPtr<IDeviceInfoConfig>().setConnectionString(id);
                groupedDevices.set(id, deviceInfo);
            }
            availableDevices.pushBack(deviceInfo);
        }
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
    std::unique_lock lock(sync);
    if (!groupedDevices.assigned())
    {
        lock.unlock();
        onGetAvailableDevices();
        lock.lock();
    }

    StringPtr internalConnectionString = connectionString;
    DeviceInfoPtr deviceInfo;
    
    if (connectionString.toStdString().find("daq://") == 0)
    {
        if (groupedDevices.hasKey(connectionString))
            deviceInfo = groupedDevices.get(connectionString);
        
        if (!deviceInfo.assigned())
            throw NotFoundException(fmt::format("device with connection string \"{}\" not found", connectionString));

        if (deviceInfo.getDeviceCapabilities().getCount() == 0)
            throw NotFoundException(fmt::format("device with connection string \"{}\" has now available protocols", connectionString));

        if (deviceInfo.getDeviceCapabilities().getCount() == 1)
        {
            internalConnectionString = deviceInfo.getDeviceCapabilities()[0].getConnectionString();
        }
        else
        {
            auto findProtocol = [] (const ListPtr<IDeviceCapability>& capabilities, const StringPtr& protocolName) -> DeviceCapabilityPtr
            {
                for (const auto & capability : capabilities)
                {
                    if (capability.getProtocolName() == protocolName)
                        return capability;
                }
                return nullptr; 
            };

            auto capability = findProtocol(deviceInfo.getDeviceCapabilities(), "openDAQ Native Streaming");
            if (!capability.assigned())
                capability = findProtocol(deviceInfo.getDeviceCapabilities(), "openDAQ OpcUa");
            else if (!capability.assigned())
                capability = deviceInfo.getDeviceCapabilities()[0];

            internalConnectionString = capability.getConnectionString();
        }
    } 
    else
    {
        for (const auto & [_, info] : groupedDevices)
        {
            for(const auto & capability : info.getDeviceCapabilities())
            {
                if (capability.getConnectionString() == connectionString)
                {
                    deviceInfo = info;
                    break;
                }
            }
        }
    }

    auto device = detail::createDevice(internalConnectionString, config, devices, manager, loggerComponent, deviceInfo);
    devices.addItem(device);

    return device;
}

void ClientImpl::onRemoveDevice(const DevicePtr& device)
{
    this->devices.removeItem(device);
}

OPENDAQ_DEFINE_CLASS_FACTORY_WITH_INTERFACE_AND_CREATEFUNC(
    LIBRARY_FACTORY, Client, IDevice, createClient,
    IContext*, ctx,
    IString*, localId,
    IDeviceInfo*, defaultDeviceInfo
)

END_NAMESPACE_OPENDAQ
