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

ListPtr<ILogFileInfo> DeviceTemplate::getLogFileInfos()
{
    return {};
}

StringPtr DeviceTemplate::getLog(const StringPtr& /*id*/, Int /*size*/, Int /*offset*/)
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
    ComponentPtr parent;
    checkErrorInfo(this->componentImpl->getParent(&parent));
    if (!parent.assigned())
        return true;

    return false;
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
    return templateImpl->getTicksSinceOrigin();
}

ListPtr<ILogFileInfo> DeviceTemplateHooks::onGetLogFileInfos()
{
    return templateImpl->getLogFileInfos();
}

StringPtr DeviceTemplateHooks::onGetLog(const StringPtr& id, Int size, Int offset)
{
    return templateImpl->getLog(id, size, offset);
}

bool DeviceTemplateHooks::allowAddDevicesFromModules()
{
    return templateImpl->allowAddDevicesFromModules();
}

bool DeviceTemplateHooks::allowAddFunctionBlocksFromModules()
{
    return templateImpl->allowAddFunctionBlocksFromModules();
}

void DeviceTemplateHooks::removed()
{
    templateImpl->removed();
    templateImpl.reset();
    GenericDevice::removed();
}

DeviceInfoPtr DeviceTemplateHooks::onGetInfo()
{
    return info;
}

END_NAMESPACE_OPENDAQ_TEMPLATES
