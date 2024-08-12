#include <opendaq_module_template/device_template.h>

BEGIN_NAMESPACE_OPENDAQ

void DeviceTemplate::handleConfig(const PropertyObjectPtr& /*config*/)
{
}

void DeviceTemplate::handleOptions(const DictPtr<IString, IBaseObject>& /*options*/)
{
}

void DeviceTemplate::initSignals(const FolderConfigPtr& /*signalsFolder*/)
{
}

void DeviceTemplate::initDevices(const FolderConfigPtr& /*devicesFolder*/)
{
}

void DeviceTemplate::initFunctionBlocks(const FolderConfigPtr& /*fbFolder*/)
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

void DeviceTemplate::start()
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

BaseObjectPtr DeviceTemplate::onPropertyWrite(const StringPtr& /*propertyName*/, const PropertyPtr& /*property*/, const BaseObjectPtr& /*value*/)
{
    return nullptr;
}

BaseObjectPtr DeviceTemplate::onPropertyRead(const StringPtr& /*propertyName*/, const PropertyPtr& /*property*/, const BaseObjectPtr& /*value*/)
{
    return nullptr;
}

bool DeviceTemplate::allowAddDevicesFromModules()
{
    return false;
}

bool DeviceTemplate::allowAddFunctionBlocksFromModules()
{
    return false;
}

uint64_t DeviceTemplateHooks::onGetTicksSinceOrigin()
{
    return 0;
}

void DeviceTemplateHooks::registerCallbacks(const PropertyObjectPtr& obj)
{
    for (const auto& prop : obj.getAllProperties())
    {
        obj.getOnPropertyValueWrite(prop.getName()) +=
            [this](const PropertyObjectPtr&, const PropertyValueEventArgsPtr& args)
            {
                const auto prop = args.getProperty();
                const auto val = device->onPropertyWrite(prop.getName(), prop, args.getValue());
                if (val.assigned())
                    args.setValue(val);
            };

        obj.getOnPropertyValueRead(prop.getName()) +=
            [this](const PropertyObjectPtr&, const PropertyValueEventArgsPtr& args)
            {
                const auto prop = args.getProperty();
                const auto val = device->onPropertyRead(prop.getName(), prop, args.getValue());
                if (val.assigned())
                    args.setValue(val);
            };

        if (prop.getValueType() == ctObject)
            registerCallbacks(obj.getPropertyValue(prop.getName()));
    }
}

DevicePtr DeviceTemplate::getDevice() const
{
    return deviceImpl->objPtr;
}

void DeviceTemplate::removeComponentWithId(const FolderConfigPtr& parentFolder, const std::string& componentId) const
{
    if (parentFolder.assigned())
        throw NotAssignedException{"Parent component is not assigned."};
    if (componentId.empty())
        throw NotAssignedException{"Missing component removal target local ID."};

    if(parentFolder.hasItem(componentId))
        parentFolder.removeItemWithLocalId(componentId);
    else
        LOG_W("Component with id {} not found in parent folder", componentId);

    LOG_T("Component with id {} removed from parent folder", componentId)
}

void DeviceTemplate::removeComponent(const FolderConfigPtr& parentFolder, const ComponentPtr& component) const
{
    if (parentFolder.assigned())
        throw NotAssignedException{"Parent component is not assigned."};
    if (component.assigned())
        throw NotAssignedException{"Component slated for removal is not assigned."};

    if(parentFolder.hasItem(component.getLocalId()))
        parentFolder.removeItem(component);
    else
        LOG_W("Component with id {} not found in parent folder", component.getLocalId());

    LOG_T("Component with id {} removed from parent folder", component.getLocalId())
}

IoFolderConfigPtr DeviceTemplate::createAndAddIOFolder(const std::string& folderId, const IoFolderConfigPtr& parent) const
{
    LOG_T("Creating and adding IO folder with id {}", folderId)
	return deviceImpl->addIoFolder(folderId, parent);
}

void DeviceTemplate::setDeviceDomain(const DeviceDomainPtr& deviceDomain) const
{
    LOG_T("Setting device domain")
    deviceImpl->setDeviceDomain(deviceDomain);
}

void DeviceTemplateHooks::onObjectReady()
{
    GenericDevice<IDevice>::onObjectReady();
    if (initialized)
        return;

    initialized = true;
}

DeviceInfoPtr DeviceTemplateHooks::onGetInfo()
{
    return info;
}

END_NAMESPACE_OPENDAQ
