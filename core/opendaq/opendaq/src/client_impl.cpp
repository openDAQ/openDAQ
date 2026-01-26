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
    this->name = "OpenDAQClient";
    this->deviceInfo = deviceInfo;

    auto syncComponentPrivate = this->syncComponent.asPtr<IComponentPrivate>(true);
    syncComponentPrivate.unlockAttributes(List<IString>("Visible"));
    this->syncComponent.setVisible(false);
    syncComponentPrivate.lockAttributes(List<IString>("Visible"));
}

DeviceInfoPtr ClientImpl::onGetInfo()
{
    auto info = DeviceInfo("daq.root://default_client", "OpenDAQClient");
    info.setDeviceType(DeviceTypeBuilder()
                           .setConnectionStringPrefix("daq.root")
                           .setId("OpenDAQClient")
                           .setName("OpenDAQClient")
                           .build());
    return info;
}

bool ClientImpl::allowAddDevicesFromModules()
{
    return true;
}

bool ClientImpl::allowAddFunctionBlocksFromModules()
{
    return true;
}

std::set<daq::OperationModeType> ClientImpl::onGetAvailableOperationModes() 
{ 
    return {daq::OperationModeType::Idle, daq::OperationModeType::Operation, daq::OperationModeType::SafeOperation}; 
}

OPENDAQ_DEFINE_CLASS_FACTORY_WITH_INTERFACE_AND_CREATEFUNC (
    LIBRARY_FACTORY, Client, IDevice, createClient,
    IContext*, ctx,
    IString*, localId,
    IDeviceInfo*, defaultDeviceInfo,
    IComponent*, parent
    )

END_NAMESPACE_OPENDAQ
