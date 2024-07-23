#include <opendaq/client_impl.h>
#include <opendaq/device_info_factory.h>
#include <boost/algorithm/string.hpp>
#include <future>

BEGIN_NAMESPACE_OPENDAQ

ClientImpl::ClientImpl(const ContextPtr ctx, const StringPtr& localId, const DeviceInfoPtr& deviceInfo, const ComponentPtr& parent)
    : Device(ctx, parent, localId)
    , manager(this->context.assigned() ? this->context.getModuleManager() : nullptr)
    , logger(ctx.getLogger())
    , loggerComponent( this->logger.assigned()
                          ? this->logger.getOrAddComponent("Client")
                          : throw ArgumentNullException("Logger must not be null"))
{
    if (deviceInfo.assigned())
        this->deviceInfo = deviceInfo;
    else
        this->deviceInfo = DeviceInfo("", "OpenDAQClient");
    this->deviceInfo.freeze();
    this->name = "OpenDAQClient";
    this->isRootDevice = true;
}

DeviceInfoPtr ClientImpl::onGetInfo()
{
    return this->deviceInfo;
}

bool ClientImpl::allowAddDevicesFromModules()
{
    return true;
}

bool ClientImpl::allowAddFunctionBlocksFromModules()
{
    return true;
}

OPENDAQ_DEFINE_CLASS_FACTORY_WITH_INTERFACE_AND_CREATEFUNC(
    LIBRARY_FACTORY,
    Client,
    IDevice,
    createClient,
    IContext*,
    ctx,
    IString*,
    localId,
    IDeviceInfo*,
    defaultDeviceInfo,
    IComponent*,
    parent
    )

END_NAMESPACE_OPENDAQ
