#include <opendaq_module_template/device_template.h>

BEGIN_NAMESPACE_OPENDAQ_TEMPLATES

void DeviceTemplate::initDevices(const FolderConfigPtr& /*devicesFolder*/)
{
}

void DeviceTemplate::initSyncComponent(const SyncComponentPrivatePtr& /*syncComponent*/)
{
}

void DeviceTemplate::initIOFolder(const IoFolderConfigPtr& /*ioFolder*/)
{
}

void DeviceTemplate::initCustomComponents()
{
}

DeviceDomainPtr DeviceTemplate::initDeviceDomain()
{
    return {};
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

DevicePtr DeviceTemplate::getDevice() const
{
    return componentImpl->objPtr;
}

IoFolderConfigPtr DeviceTemplate::createAndAddIOFolder(const std::string& folderId, const IoFolderConfigPtr& parent) const
{
    LOG_T("Creating and adding IO folder with id {}", folderId)
	return componentImpl->addIoFolder(folderId, parent);
}

void DeviceTemplate::setDeviceDomain(const DeviceDomainPtr& deviceDomain) const
{
    LOG_T("Setting device domain")
    componentImpl->setDeviceDomain(deviceDomain);
}

uint64_t DeviceTemplateHooks::onGetTicksSinceOrigin()
{
    return 0;
}

bool DeviceTemplateHooks::allowAddDevicesFromModules()
{
    return device->allowAddDevicesFromModules();
}

bool DeviceTemplateHooks::allowAddFunctionBlocksFromModules()
{
    return device->allowAddFunctionBlocksFromModules();
}


DeviceInfoPtr DeviceTemplateHooks::onGetInfo()
{
    return info;
}

END_NAMESPACE_OPENDAQ_TEMPLATES
