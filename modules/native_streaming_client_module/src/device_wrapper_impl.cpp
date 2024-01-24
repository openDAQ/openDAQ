#include <native_streaming_client_module/device_wrapper_impl.h>

BEGIN_NAMESPACE_OPENDAQ_NATIVE_STREAMING_CLIENT_MODULE

DeviceWrapperImpl::DeviceWrapperImpl()
{
}

DeviceWrapperImpl::~DeviceWrapperImpl()
{
}

// IDevice

ErrCode DeviceWrapperImpl::getDomain(IDeviceDomain** deviceDomain)
{
    return wrappedDevice->getDomain(deviceDomain);
}

ErrCode DeviceWrapperImpl::getInputsOutputsFolder(IFolder** inputsOutputsFolder)
{
    return wrappedDevice->getInputsOutputsFolder(inputsOutputsFolder);
}

ErrCode DeviceWrapperImpl::getCustomComponents(IList** customFolders)
{
    return wrappedDevice->getCustomComponents(customFolders);
}

ErrCode DeviceWrapperImpl::getSignals(IList** signals, ISearchFilter* searchFilter)
{
    return wrappedDevice->getSignals(signals, searchFilter);
}

ErrCode DeviceWrapperImpl::getSignalsRecursive(IList** signals, ISearchFilter* searchFilter)
{
    return wrappedDevice->getSignalsRecursive(signals, searchFilter);
}

ErrCode DeviceWrapperImpl::getAvailableDevices(IList** availableDevices)
{
    return wrappedDevice->getAvailableDevices(availableDevices);
}

ErrCode DeviceWrapperImpl::getAvailableDeviceTypes(IDict** deviceTypes)
{
    return wrappedDevice->getAvailableDeviceTypes(deviceTypes);
}

ErrCode DeviceWrapperImpl::addDevice(IDevice** device, IString* connectionString, IPropertyObject* config)
{
    return wrappedDevice->addDevice(device, connectionString, config);
}

ErrCode DeviceWrapperImpl::removeDevice(IDevice* device)
{
    return wrappedDevice->removeDevice(device);
}

ErrCode DeviceWrapperImpl::getDevices(IList** devices, ISearchFilter* searchFilter)
{
    return wrappedDevice->getDevices(devices, searchFilter);
}

ErrCode DeviceWrapperImpl::getAvailableFunctionBlockTypes(IDict** functionBlockTypes)
{
    return wrappedDevice->getAvailableFunctionBlockTypes(functionBlockTypes);
}

ErrCode DeviceWrapperImpl::addFunctionBlock(IFunctionBlock** functionBlock, IString* typeId, IPropertyObject* config)
{
    return wrappedDevice->addFunctionBlock(functionBlock, typeId, config);
}

ErrCode DeviceWrapperImpl::removeFunctionBlock(IFunctionBlock* functionBlock)
{
    return wrappedDevice->removeFunctionBlock(functionBlock);
}

ErrCode DeviceWrapperImpl::getFunctionBlocks(IList** functionBlocks, ISearchFilter* searchFilter)
{
    return wrappedDevice->getFunctionBlocks(functionBlocks, searchFilter);
}

ErrCode DeviceWrapperImpl::getChannels(IList** channels, ISearchFilter* searchFilter)
{
    return wrappedDevice->getChannels(channels, searchFilter);
}

ErrCode DeviceWrapperImpl::getChannelsRecursive(IList** channels, ISearchFilter* searchFilter)
{
    return wrappedDevice->getChannelsRecursive(channels, searchFilter);
}

ErrCode DeviceWrapperImpl::saveConfiguration(IString** configuration)
{
    return wrappedDevice->saveConfiguration(configuration);
}

ErrCode DeviceWrapperImpl::loadConfiguration(IString* configuration)
{
    return wrappedDevice->loadConfiguration(configuration);
}

// IDeviceDomain

ErrCode DeviceWrapperImpl::getTickResolution(IRatio** resolution)
{
    return daqTry([this, &resolution] {
        return wrappedDevice.asPtr<IDeviceDomain>()->getTickResolution(resolution);
    });
}

ErrCode DeviceWrapperImpl::getTicksSinceOrigin(uint64_t* ticks)
{
    return daqTry([this, &ticks] {
        return wrappedDevice.asPtr<IDeviceDomain>()->getTicksSinceOrigin(ticks);
    });
}

ErrCode DeviceWrapperImpl::getOrigin(IString** origin)
{
    return daqTry([this, &origin] {
        return wrappedDevice.asPtr<IDeviceDomain>()->getOrigin(origin);
    });
}

ErrCode DeviceWrapperImpl::getUnit(IUnit** unit)
{
    return daqTry([this, &unit] {
        return wrappedDevice.asPtr<IDeviceDomain>()->getUnit(unit);
    });
}

// IComponent

ErrCode DeviceWrapperImpl::getContext(IContext** context)
{
    return wrappedDevice->getContext(context);
}

ErrCode DeviceWrapperImpl::getLocalId(IString** localId)
{
    return wrappedDevice->getLocalId(localId);
}

ErrCode DeviceWrapperImpl::getGlobalId(IString** globalId)
{
    return wrappedDevice->getGlobalId(globalId);
}

ErrCode DeviceWrapperImpl::getActive(Bool* active)
{
    return wrappedDevice->getActive(active);
}

ErrCode DeviceWrapperImpl::setActive(Bool active)
{
    return wrappedDevice->setActive(active);
}

ErrCode INTERFACE_FUNC DeviceWrapperImpl::getParent(IComponent** parent)
{
    return wrappedDevice->getParent(parent);
}

ErrCode DeviceWrapperImpl::getName(IString** name)
{
    return wrappedDevice->getName(name);
}

ErrCode DeviceWrapperImpl::setName(IString* name)
{
    return wrappedDevice->setName(name);
}

ErrCode DeviceWrapperImpl::getDescription(IString** description)
{
    return wrappedDevice->getDescription(description);
}

ErrCode DeviceWrapperImpl::setDescription(IString* description)
{
    return wrappedDevice->setDescription(description);
}

ErrCode DeviceWrapperImpl::getTags(ITags** tags)
{
    return wrappedDevice->getTags(tags);
}

ErrCode DeviceWrapperImpl::getVisible(Bool* visible)
{
    return wrappedDevice->getVisible(visible);
}

ErrCode DeviceWrapperImpl::setVisible(Bool visible)
{
    return wrappedDevice->setVisible(visible);
}

ErrCode DeviceWrapperImpl::getLockedAttributes(IList** attributes)
{
    return wrappedDevice->getLockedAttributes(attributes);
}

ErrCode DeviceWrapperImpl::getOnComponentCoreEvent(IEvent** event)
{
    return wrappedDevice->getOnComponentCoreEvent(event);
}

// IFolder

ErrCode DeviceWrapperImpl::getItems(IList** items, ISearchFilter* searchFilter)
{
    return wrappedDevice->getItems(items, searchFilter);
}

ErrCode DeviceWrapperImpl::getItem(IString* localId, IComponent** item)
{
    return wrappedDevice->getItem(localId, item);
}

ErrCode INTERFACE_FUNC DeviceWrapperImpl::isEmpty(Bool* empty)
{
    return wrappedDevice->isEmpty(empty);
}

ErrCode DeviceWrapperImpl::hasItem(IString* localId, Bool* value)
{
    return wrappedDevice->hasItem(localId, value);
}

// IPropertyObject

ErrCode DeviceWrapperImpl::getClassName(IString** className)
{
    return wrappedDevice->getClassName(className);
}

ErrCode DeviceWrapperImpl::setPropertyValue(IString* propertyName, IBaseObject* value)
{
    return wrappedDevice->setPropertyValue(propertyName, value);
}

ErrCode DeviceWrapperImpl::getPropertyValue(IString* propertyName, IBaseObject** value)
{
    return wrappedDevice->getPropertyValue(propertyName, value);
}

ErrCode DeviceWrapperImpl::getPropertySelectionValue(IString* propertyName, IBaseObject** value)
{
    return wrappedDevice->getPropertySelectionValue(propertyName, value);
}

ErrCode DeviceWrapperImpl::clearPropertyValue(IString* propertyName)
{
    return wrappedDevice->clearPropertyValue(propertyName);
}

ErrCode DeviceWrapperImpl::getProperty(IString* propertyName, IProperty** property)
{
    return wrappedDevice->getProperty(propertyName, property);
}

ErrCode DeviceWrapperImpl::addProperty(IProperty* property)
{
    return wrappedDevice->addProperty(property);
}

ErrCode DeviceWrapperImpl::removeProperty(IString* propertyName)
{
    return wrappedDevice->removeProperty(propertyName);
}

ErrCode DeviceWrapperImpl::getVisibleProperties(IList** properties)
{
    return wrappedDevice->getVisibleProperties(properties);
}

ErrCode DeviceWrapperImpl::getAllProperties(IList** properties)
{
    return wrappedDevice->getAllProperties(properties);
}

ErrCode DeviceWrapperImpl::setPropertyOrder(IList* orderedPropertyNames)
{
    return wrappedDevice->setPropertyOrder(orderedPropertyNames);
}

ErrCode DeviceWrapperImpl::getOnPropertyValueWrite(IString* propertyName, IEvent** event)
{
    return wrappedDevice->getOnPropertyValueWrite(propertyName, event);
}

ErrCode DeviceWrapperImpl::getOnPropertyValueRead(IString* propertyName, IEvent** event)
{
    return wrappedDevice->getOnPropertyValueRead(propertyName, event);
}

ErrCode DeviceWrapperImpl::beginUpdate()
{
    return wrappedDevice->beginUpdate();
}

ErrCode DeviceWrapperImpl::endUpdate()
{
    return wrappedDevice->endUpdate();
}

ErrCode DeviceWrapperImpl::getOnEndUpdate(IEvent** event)
{
    return wrappedDevice->endUpdate();
}

ErrCode DeviceWrapperImpl::hasProperty(IString* propertyName, Bool* hasProperty)
{
    return wrappedDevice->hasProperty(propertyName, hasProperty);
}

// ISerializable

ErrCode DeviceWrapperImpl::serialize(ISerializer* serializer)
{
    return daqTry([this, &serializer] {
        return wrappedDevice.asPtr<ISerializable>(true)->serialize(serializer);
    });
}

ErrCode DeviceWrapperImpl::getSerializeId(ConstCharPtr* id) const
{
    return daqTry([this, &id] {
        return wrappedDevice.asPtr<ISerializable>(true)->getSerializeId(id);
    });
}

// IOwnable

ErrCode DeviceWrapperImpl::setOwner(IPropertyObject* owner)
{
    return daqTry([this, &owner] {
        return wrappedDevice.asPtr<IOwnable>(true)->setOwner(owner);
    });
}

// IFreezable

ErrCode DeviceWrapperImpl::freeze()
{
    return daqTry([this] {
        return wrappedDevice.asPtr<IFreezable>(true)->freeze();
    });
}

ErrCode DeviceWrapperImpl::isFrozen(Bool* isFrozen) const
{
    return daqTry([this, &isFrozen] {
        return wrappedDevice.asPtr<IFreezable>(true)->isFrozen(isFrozen);
    });
}

// IUpdatable

ErrCode DeviceWrapperImpl::update(ISerializedObject* update)
{
    return daqTry([this, &update] {
        return wrappedDevice.asPtr<IUpdatable>(true)->update(update);
    });
}

ErrCode DeviceWrapperImpl::serializeForUpdate(ISerializer* serializer)
{
    return daqTry([this, &serializer] {
        return wrappedDevice.asPtr<IUpdatable>(true)->serializeForUpdate(serializer);
    });
}

// IPropertyObjectProtected

ErrCode DeviceWrapperImpl::setProtectedPropertyValue(IString* propertyName, IBaseObject* value)
{
    return daqTry([this, &propertyName, &value] {
        return wrappedDevice.asPtr<IPropertyObjectProtected>(true)->setProtectedPropertyValue(propertyName, value);
    });
}

ErrCode DeviceWrapperImpl::clearProtectedPropertyValue(IString* propertyName)
{
    return daqTry([this, &propertyName] {
        return wrappedDevice.asPtr<IPropertyObjectProtected>(true)->clearProtectedPropertyValue(propertyName);
    });
}

// IPropertyObjectInternal

ErrCode DeviceWrapperImpl::checkForReferences(IProperty* property, Bool* isReferenced)
{
    return daqTry([this, &property, &isReferenced] {
        return wrappedDevice.asPtr<IPropertyObjectInternal>(true)->checkForReferences(property, isReferenced);
    });
}

ErrCode DeviceWrapperImpl::enableCoreEventTrigger()
{
    return daqTry([this] {
        return wrappedDevice.asPtr<IPropertyObjectInternal>(true)->enableCoreEventTrigger();
    });
}

ErrCode DeviceWrapperImpl::disableCoreEventTrigger()
{
    return daqTry([this] {
        return wrappedDevice.asPtr<IPropertyObjectInternal>(true)->disableCoreEventTrigger();
    });
}

ErrCode DeviceWrapperImpl::getCoreEventTrigger(IProcedure** trigger)
{
    return daqTry([this, &trigger] {
        return wrappedDevice.asPtr<IPropertyObjectInternal>(true)->getCoreEventTrigger(trigger);
    });
}

ErrCode DeviceWrapperImpl::setCoreEventTrigger(IProcedure* trigger)
{
    return daqTry([this, &trigger] {
        return wrappedDevice.asPtr<IPropertyObjectInternal>(true)->setCoreEventTrigger(trigger);
    });
}

ErrCode DeviceWrapperImpl::clone(IPropertyObject** cloned)
{
    return daqTry([this, &cloned] {
        return wrappedDevice.asPtr<IPropertyObjectInternal>(true)->clone(cloned);
    });
}

ErrCode DeviceWrapperImpl::setPath(IString* path)
{
    return daqTry([this, &path] {
        return wrappedDevice.asPtr<IPropertyObjectInternal>(true)->setPath(path);
    });
}

// IRemovable

ErrCode DeviceWrapperImpl::remove()
{
    return daqTry([this] {
        return wrappedDevice.asPtr<IRemovable>(true)->remove();
    });
}

ErrCode DeviceWrapperImpl::isRemoved(Bool* removed)
{
    return daqTry([this, &removed] {
        return wrappedDevice.asPtr<IRemovable>(true)->isRemoved(removed);
    });
}

// IComponentPrivate

ErrCode DeviceWrapperImpl::lockAttributes(IList* attributes)
{
    return daqTry([this, &attributes] {
        return wrappedDevice.asPtr<IComponentPrivate>(true)->lockAttributes(attributes);
    });
}

ErrCode DeviceWrapperImpl::lockAllAttributes()
{
    return daqTry([this] {
        return wrappedDevice.asPtr<IComponentPrivate>(true)->lockAllAttributes();
    });
}

ErrCode DeviceWrapperImpl::unlockAttributes(IList* attributes)
{
    return daqTry([this, &attributes] {
        return wrappedDevice.asPtr<IComponentPrivate>(true)->unlockAttributes(attributes);
    });
}

ErrCode DeviceWrapperImpl::unlockAllAttributes()
{
    return daqTry([this] {
        return wrappedDevice.asPtr<IComponentPrivate>(true)->unlockAllAttributes();
    });
}

ErrCode DeviceWrapperImpl::triggerComponentCoreEvent(ICoreEventArgs* args)
{
    return daqTry([this, &args] {
        return wrappedDevice.asPtr<IComponentPrivate>(true)->triggerComponentCoreEvent(args);
    });
}

// IDeserializeComponent

ErrCode DeviceWrapperImpl::deserializeValues(ISerializedObject* serializedObject, IBaseObject* context, IFunction* callbackFactory)
{
    return daqTry([this, &serializedObject, &context, &callbackFactory] {
        return wrappedDevice.asPtr<IDeserializeComponent>(true)->deserializeValues(serializedObject, context, callbackFactory);
    });
}

ErrCode DeviceWrapperImpl::complete()
{
    return daqTry([this] {
        return wrappedDevice.asPtr<IDeserializeComponent>(true)->complete();
    });
}

ErrCode DeviceWrapperImpl::getDeserializedParameter(IString* parameter, IBaseObject** value)
{
    return daqTry([this, &parameter, &value] {
        return wrappedDevice.asPtr<IDeserializeComponent>(true)->getDeserializedParameter(parameter, value);
    });
}

ErrCode DeviceWrapperImpl::addStreamingOption(IStreamingInfo* info)
{
    return daqTry([this, &info] {
        return wrappedDevice.asPtr<IDevicePrivate>(true)->addStreamingOption(info);
    });
}

// IDevicePrivate

ErrCode DeviceWrapperImpl::removeStreamingOption(IString* protocolId)
{
    return daqTry([this, &protocolId] {
        return wrappedDevice.asPtr<IDevicePrivate>(true)->removeStreamingOption(protocolId);
    });
}

ErrCode DeviceWrapperImpl::getStreamingOptions(IList** streamingOptions)
{
    return daqTry([this, &streamingOptions] {
        return wrappedDevice.asPtr<IDevicePrivate>(true)->getStreamingOptions(streamingOptions);
    });
}

// IConfigClientObject

ErrCode DeviceWrapperImpl::getRemoteGlobalId(IString** remoteGlobalId)
{
    return daqTry([this, &remoteGlobalId] {
        return wrappedDevice.asPtr<IConfigClientObject>(true)->getRemoteGlobalId(remoteGlobalId);
    });
}

END_NAMESPACE_OPENDAQ_NATIVE_STREAMING_CLIENT_MODULE
