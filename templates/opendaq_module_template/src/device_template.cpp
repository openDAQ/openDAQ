#include <opendaq_module_template/device_template.h>

BEGIN_NAMESPACE_OPENDAQ

void DeviceTemplate::configureSignals(const FolderConfigPtr& /*signalsFolder*/)
{
}

void DeviceTemplate::configureDevices(const FolderConfigPtr& /*devicesFolder*/)
{
}

void DeviceTemplate::configureFunctionBlocks(const FolderConfigPtr& /*fbFolder*/)
{
}

void DeviceTemplate::configureIOComponents(const IoFolderConfigPtr& /*ioFolder*/)
{
}

void DeviceTemplate::configureCustomComponents()
{
}

DeviceDomainPtr DeviceTemplate::getDeviceDomain()
{
    return nullptr;
}

uint64_t DeviceTemplate::getTicksSinceOrigin()
{
    return 0;
}

bool DeviceTemplate::allowAddDevicesFromModules()
{
    return false;
}

bool DeviceTemplate::allowAddFunctionBlocksFromModules()
{
    return false;
}

uint64_t DeviceTemplate::onGetTicksSinceOrigin()
{
    return 0;
}

void DeviceTemplate::onObjectReady()
{
    GenericDevice<IDevice>::onObjectReady();
    if (initialized)
        return;

    initialized = true;
}

DeviceInfoPtr DeviceTemplate::onGetInfo()
{
    return info;
}

END_NAMESPACE_OPENDAQ
