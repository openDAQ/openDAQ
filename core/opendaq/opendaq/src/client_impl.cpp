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
    onSetDeviceInfo();
    this->isRootDevice = true;
}

void ClientImpl::onSetDeviceInfo()
{
    if (!deviceInfo.getName().assigned() || deviceInfo.getName().getLength() == 0)
        deviceInfo.asPtr<IDeviceInfoConfig>().setName("daq_client");
    deviceInfo.freeze();
}

ListPtr<IDeviceInfo> ClientImpl::onGetAvailableDevices()
{
    std::scoped_lock lock(sync);
    return manager.getAvailableDevices().detach();
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
    auto device = manager.getDevice(connectionString, config, devices, loggerComponent);
    if (device.assigned())
        devices.addItem(device);
    return device.addRefAndReturn();
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
