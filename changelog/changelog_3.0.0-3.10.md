# Fix Property Values Written Before Validation

## Description

- Fixed the issue where property values were written before validation.

## Required integration changes

**Callback Attachment for Property Write**  
To attach a callback to the property object for handling property writes, use the following pattern:

```cpp
propObj.getOnPropertyValueWrite(propName) += (PropertyObjectPtr& obj, PropertyValueEventArgsPtr& arg)
{
    // Your logic goes here
};
```

**Additional Notes**:

- If the property is being updated, the method `getPropertyValue` returns the intermediate value, which can still be overridden in further updates or aborted.
- If the property is being updated, the method `IPropertyValueEventArgs::getOldValue` inside the callback returns the old property value.

**Example**:

```cpp
// propObj has a property prop1 with default value 100
// Setting a new property value to 345
propObj.getOnPropertyValueWrite("prop1") += [](PropertyObjectPtr& obj, PropertyValueEventArgsPtr& arg)
{
    assert(obj.getPropertyValue("prop1") == 345); // Value to set
    assert(arg.getValue() == 345);                // Value to set
    assert(arg.getOldValue() == 100);             // Old value
};
propObj.setPropertyValue("prop1", 345);
```

**Ignoring the New Value for a Property**:

- To ignore setting the new value for a property, **throw an exception** in the callback. This cancels the property update and propagates the exception to the `setPropertyValue` call.
- **Alternatively, skip the update** using the following pattern to revert to the old value without throwing an exception:

```cpp
propObj.getOnPropertyValueWrite("prop1") += [](PropertyObjectPtr& /* obj */, PropertyValueEventArgsPtr& arg)
{
    // Restore by throwing an exception
    if ((Int)arg.getValue() < 0)
        throw OutOfRangeException("prop1 value is negative");
};
```

```cpp
propObj.getOnPropertyValueWrite("prop1") += [](PropertyObjectPtr& /* obj */, PropertyValueEventArgsPtr& arg)
{
    // Restore value without throwing an exception
    if ((Int)arg.getValue() < 0)
        arg.setValue(arg.getOldValue());
};
```

**Handling Flaky Property Updates**:
When defining a callback for a property (e.g., `prop1`), if you update another property (`prop2`) within the callback and then throw an exception, the changes to `prop1` will be rolled back automatically, while `prop2` will have a new value:

```cpp
// prop1 has a default value of 0
// prop2 has a default value of 0
propObj.getOnPropertyValueWrite("prop1") += [](PropertyObjectPtr& obj, PropertyValueEventArgsPtr& arg)
{
    obj.setPropertyValue("prop2", arg.getValue()); // Update prop2
    if ((int)arg.getValue() < 0) // Validate prop1's value
        throw OutOfRangeException("prop1 value is negative"); // Abort prop1 update
};

try
{
    propObj.setPropertyValue("prop1", -1); // This will throw an exception
}
catch (...)
{
    // Exception is handled here
}

// Ensure property values remain unchanged
assert(propObj.getPropertyValue("prop1") == 0);  // prop1 retains its original value
assert(propObj.getPropertyValue("prop2") == -1); // prop2 has a new value
```

## API changes

```diff
+ [function] IPropertyValueEventArgs::getOldValue(IBaseObject** value)
-m[factory] PropertyValueEventArgsPtr PropertyValueEventArgs(const PropertyPtr& propChanged, const BaseObjectPtr& newValue, PropertyEventType changeType, Bool isUpdating)
+m[factory] PropertyValueEventArgsPtr PropertyValueEventArgs(const PropertyPtr& propChanged, const BaseObjectPtr& newValue, const BaseObjectPtr& oldValue, PropertyEventType changeType, Bool isUpdating)
```

---

# Separate Container for Connection Status

> [!CAUTION]
> Breaks binary compatibility

## Description

- Introduces a separate container (accessible per device) for connection statuses.
- Introduces a new core event type `"ConnectionStatusChanged"` to notify when the configuration or streaming connection status of the device changes.
  - **Limitations**: Streaming connection statuses are neither serialized nor propagated through the native configuration protocol.
- DAQ Context always has `TypeManager` assigned.
- `ConnectionStatusType` is added to the `TypeManager` by default.
- Makes connection status accessible directly from the `Streaming` object.

## Required integration changes

- Introduces a new mechanism for retrieving the connection status of the device.

## API changes

```diff
+ [interface] IConnectionStatusContainerPrivate : public IBaseObject
+ [function] IConnectionStatusContainerPrivate::addConfigurationConnectionStatus(IString* connectionString, IEnumeration* initialValue)
+ [function] IConnectionStatusContainerPrivate::addStreamingConnectionStatus(IString* connectionString, IEnumeration* initialValue, IStreaming* streamingObject)
+ [function] IConnectionStatusContainerPrivate::removeStreamingConnectionStatus(IString* connectionString)
+ [function] IConnectionStatusContainerPrivate::updateConnectionStatus(IString* connectionString, IEnumeration* value, IStreaming* streamingObject)

+ [function] IDevice::getConnectionStatusContainer(IComponentStatusContainer** statusContainer)

+ [function] IStreaming::getConnectionStatus(IEnumeration** connectionStatus)
```

---

# Separate IModuleInfo Interface for Module Name, ID, and Version

> [!CAUTION]
> Breaks binary compatibility

## Description

- Deletes the fields `id`, `name`, and `versionInfo` from the `IModule` interface and adds them to the new `IModuleInfo` interface, which is a new field in the `IModule` interface.
- `IModuleInfo` field is also added to the `IComponentType` interface.
- Works over Native.
- `ComponentType` is moved from `coreobjects` to `opendaq`.

## API changes

```diff
+ [interface] IModuleInfo : public IBaseObject
+ [function] IModuleInfo::getVersionInfo(IVersionInfo** version)
+ [function] IModuleInfo::getName(IVersionInfo** version)
+ [function] IModuleInfo::getId(IVersionInfo** version)
+ [interface] IComponentTypePrivate : public IBaseObject
+ [function] IComponentTypePrivate::setModuleInfo(IModuleInfo* info)
+ [function] IComponentType::getModuleInfo(IModuleInfo** info)
+ [function] IModule::getModuleInfo(IModuleInfo** info)
- [function] IModule::getVersionInfo(IVersionInfo** version)
- [function] IModule::getName(IVersionInfo** version)
- [function] IModule::getId(IVersionInfo** version)
```

---

# Support for Different Client Types Over Native Protocol

## Description

- Support for different client types over the native configuration protocol has been introduced.
- A `ClientType` selection property has been added to the general configuration object of the `IDevice::addDevice` method.  
  Supported values are: `Control(0)`, `Exclusive Control(1)`, `View Only(2)`.
- An `ExclusiveControlDropOthers` boolean property has been added to the general configuration object of the `IDevice::addDevice` method.
- The native configuration protocol has been updated to version 7.

## Required integration changes

- None. The default client type is `Control(0)`, which is backward-compatible with previous protocol versions.

---

# Minimum Config Protocol Version 6 for Locking Methods

## Description

- Raised minimum required config protocol version to 6 for the following device locking methods:
  - `IDevice::lock()`
  - `IDevice::unlock()`
  - `IDevice::isLocked(Bool* isLockedOut)`

---

# Improved Component Updates and Removed Local Client ID Generation

## Description

- Improved component updates.
- Removed generation of local ID for the client device in the instance.
- Fixed connecting signals to input ports on the server to which the client is connecting during configuration loading.

## Required integration changes

- An instance with the client device as the root device no longer generates a unique ID but uses `openDAQDevice` as the ID.
- If a developer previously encountered issues restoring signal connections from the client to the server during configuration loading, they must create a new configuration and save it. Otherwise, everything should work as expected.

## API changes

```diff
+ [function] ISignalPrivate::getSignalSerializeId(IString** serializeId)
```

---

# Forceful Device Unlocking Over Native Config Protocol

> [!CAUTION]
> Breaks binary compatibility

## Description

- Adds support for forcefully unlocking a device over the native config protocol.
- Native config protocol bumped to version 6.
- Adds user lock object to the serialized device.

## API changes

```diff
+ [function] IDevicePrivate::forceUnlock()

+ [function] IAuthenticationProvider::findUser(IString* username, IUser** userOut)

+ [interface] IUserLock : public IBaseObject
+ [function] IUserLock::lock(IUser* user = nullptr)
+ [function] IUserLock::unlock(IUser* user = nullptr)
+ [function] IUserLock::forceUnlock()
+ [function] IUserLock::isLocked(Bool* isLockedOut)
```

---

# Thread Synchronization Mechanism in Property Objects

## Description

- Implements thread synchronization mechanism in property objects.
- Provides a new internal method to allow for recursive locking in `onWrite`/`onRead` events via `GenericPropertyObjectImpl::getRecursiveConfigLock()`.
- A standard lock guard can be acquired via `GenericPropertyObjectImpl::getAcquisitionLock()`.
- The `sync` mutex previously available in `ComponentImpl` was moved up to `GenericPropertyObjectImpl`.

## Required integration changes

- The `sync` mutex available in the `GenericPropertyObjectImpl` should no longer be locked in `onWrite`/`onRead` events. If needed due to existing code patterns, use the `getRecursiveConfigLock` method instead. See the reference device implementations for guidance.
- Current device/function block implementations that lock the `sync` mutex during events will deadlock.

## API changes

```diff
+ [function] ICoercer::coerceNoLock(IBaseObject* propObj, IBaseObject* value, IBaseObject** result)

+ [function] IValidator::validateNoLock(IBaseObject* propObj, IBaseObject* value)

+ [function] IEvalValue::getResultNoLock(IBaseObject** obj)

+ [function] IPropertyInternal::getValueTypeNoLock(CoreType* type)
+ [function] IPropertyInternal::getKeyTypeNoLock(CoreType* type)
+ [function] IPropertyInternal::getItemTypeNoLock(CoreType* type)
+ [function] IPropertyInternal::getDescriptionNoLock(IString** description)
+ [function] IPropertyInternal::getUnitNoLock(IUnit** unit)
+ [function] IPropertyInternal::getMinValueNoLock(INumber** min)
+ [function] IPropertyInternal::getMaxValueNoLock(INumber** max)
+ [function] IPropertyInternal::getDefaultValueNoLock(IBaseObject** value)
+ [function] IPropertyInternal::getSuggestedValuesNoLock(IList** values)
+ [function] IPropertyInternal::getVisibleNoLock(Bool* visible)
+ [function] IPropertyInternal::getReadOnlyNoLock(Bool* readOnly)
+ [function] IPropertyInternal::getSelectionValuesNoLock(IBaseObject** values)
+ [function] IPropertyInternal::getReferencedPropertyNoLock(IProperty** propertyEval)
+ [function] IPropertyInternal::getIsReferencedNoLock(Bool* isReferenced)
+ [function] IPropertyInternal::getValidatorNoLock(IValidator** validator)
+ [function] IPropertyInternal::getCoercerNoLock(ICoercer** coercer)
+ [function] IPropertyInternal::getCallableInfoNoLock(ICallableInfo** callable)
+ [function] IPropertyInternal::getStructTypeNoLock(IStructType** structType)

+ [function] IPropertyObjectInternal::checkForReferencesNoLock(IProperty* property, Bool* isReferenced)
+ [function] IPropertyObjectInternal::getPropertyValueNoLock(IString* name, IBaseObject** value)
+ [function] IPropertyObjectInternal::getPropertySelectionValueNoLock(IString* name, IBaseObject** value)
+ [function] IPropertyObjectInternal::setPropertyValueNoLock(IString* name, IBaseObject* value)
```

---

# Readers Skipped Events by Default in Python

## Description

- `BlockReader`, `TailReader`, and `StreamReader` now skip events by default in Python.

## Required integration changes

- In Python, you must explicitly set `skip_events=False` if you do not want to skip events in `BlockReader`, `TailReader`, and `StreamReader`.

---

# Support Creating Readers with Connected Input Ports

## Description

- Support creating readers with a connected input port.

## Required integration changes

- None.

## API changes

```diff
+ [interface] IConnectionInternal : public IBaseObject
+ [function] IConnectionInternal::enqueueLastDescriptor()
```

---

# View Server Protocol Version in Server Capabilities

> [!CAUTION]
> Breaks binary compatibility

## Description

- Adds a way to view the server protocol version in server capabilities.
- Fixes the path issue in `ConfigurationConnectionInfo` of device info:
  - If the connection to the device included a path, ensures the path is correctly reflected in the `ConfigurationConnectionInfo`.
- For native configuration, updates the `ConfigurationConnectionInfo` of device info to display the actual protocol version used in communication between the server and client.
- Native client default config property `ProtocolVersion` is set to the latest supported protocol version.

## API changes

```diff
+ [function] IServerCapability::getProtocolVersion(IString** version)
+ [function] IServerCapabilityConfig::setProtocolVersion(IString* version)
```

---

# Implements Log File Info Interface

> [!CAUTION]
> Breaks binary compatibility

## Description

- Implements the log file info interface.

## API changes

```diff
+ [interface] ILogFileInfoBuilder : public IBaseObject
+ [function] ILogFileInfoBuilder::build(ILogFileInfo** logFileInfo)
+ [function] ILogFileInfoBuilder::getLocalPath(IString** localPath)
+ [function] ILogFileInfoBuilder::setLocalPath(IString* localPath)
+ [function] ILogFileInfoBuilder::getName(IString** name)
+ [function] ILogFileInfoBuilder::setName(IString* name)
+ [function] ILogFileInfoBuilder::getId(IString** id)
+ [function] ILogFileInfoBuilder::setId(IString* id)
+ [function] ILogFileInfoBuilder::getDescription(IString** description)
+ [function] ILogFileInfoBuilder::setDescription(IString* description)
+ [function] ILogFileInfoBuilder::getEncoding(IString** encoding)
+ [function] ILogFileInfoBuilder::setEncoding(IString* encoding)
+ [function] ILogFileInfoBuilder::getSize(SizeT* size)
+ [function] ILogFileInfoBuilder::setSize(SizeT size)
+ [function] ILogFileInfoBuilder::getLastModified(IString** lastModified)
+ [function] ILogFileInfoBuilder::setLastModified(IString* lastModified)
+ [factory] LogFileInfoBuilderPtr::LogFileInfoBuilder()

+ [interface] ILogFileInfo : public IBaseObject
+ [function] ILogFileInfo::getId(IString** id)
+ [function] ILogFileInfo::getLocalPath(IString** localPath)
+ [function] ILogFileInfo::getName(IString** name)
+ [function] ILogFileInfo::getDescription(IString** description)
+ [function] ILogFileInfo::getSize(SizeT* size)
+ [function] ILogFileInfo::getEncoding(IString** encoding)
+ [function] ILogFileInfo::getLastModified(IString** lastModified)

+ [function] IDevice::getLogFileInfos(IList** logFileInfos)
+ [function] IDevice::getLog(IString** log, IString* id, Int size = -1, Int offset = 0)
```

---

# Null Sample Type for Unassigned Signals

## Description

- Introduces a new Sample Type `"Null"`.
- Replaces `nullptr` with a `DataDescriptor` having `SampleType::Null` in the `"DATA_DESCRIPTOR_CHANGED"` event packet when a signal's descriptor is not assigned.
- Enables resetting the signal's Data Descriptor to `nullptr`.

## Required integration changes

- In the `"DATA_DESCRIPTOR_CHANGED"` event packet, the parameters `"DataDescriptor"` and `"DomainDataDescriptor"` are set to `nullptr` only if the corresponding descriptors have not changed.
- If the signal descriptor is not assigned, they are set to a `DataDescriptor` object with the `"Null"` sample type.

## API changes

```diff
+ [factory] DataDescriptorPtr NullDataDescriptor()
```

---

# Adding Nested Function Block Methods

> [!CAUTION]
> Breaks binary compatibility

## Description

- Adds methods in the function block to add/remove nested function blocks.

## Required integration changes

- For function blocks containing nested function blocks, developers should override the method  
  `FunctionBlockPtr onAddFunctionBlock(const StringPtr& typeId, const PropertyObjectPtr& config)`  
  as it is used during the `loadConfiguration` process.
- Optionally, override:
  - `DictPtr<IString, IFunctionBlockType> onGetAvailableFunctionBlockTypes()`
  - `void onRemoveFunctionBlock(const FunctionBlockPtr& functionBlock)`

## API changes

```diff
+ [function] IFunctionBlock::getAvailableFunctionBlockTypes(IDict** functionBlockTypes)
+ [function] IFunctionBlock::addFunctionBlock(IFunctionBlock** functionBlock, IString* typeId, IPropertyObject* config = nullptr)
+ [function] IFunctionBlock::removeFunctionBlock(IFunctionBlock* functionBlock)
```

---

# Restoring Device on Configuration Load

## Description

- Restores the device while loading the configuration.
- Adds update parameters to set a flag indicating whether to use the existing device or recreate a new one.

## API changes

```diff
-m[function] IUpdatable::update(ISerializedObject* update)
+m[function] IUpdatable::update(ISerializedObject* update, IBaseObject* config)

-m[function] IDeserializer::update(IUpdatable * updatable, IString * serialized)
+m[function] IDeserializer::update(IUpdatable* updatable, IString* serialized, IBaseObject* config)

+ [function] IComponentUpdateContext::getReAddDevicesEnabled(Bool* enabled)

+ [interface] IUpdateParameters: public IPropertyObject
+ [function] IUpdateParameters::getReAddDevicesEnabled(Bool* enabled)
+ [function] IUpdateParameters::setReAddDevicesEnabled(Bool enabled)
+ [factory] UpdateParametersPtr UpdateParameters()

-m[function] IDevice::loadConfiguration(IString* configuration)
+m[function] IDevice::loadConfiguration(IString* configuration, IUpdateParameters* config = nullptr)
```

---

# minReadCount Option in MultiReader

## Description

- Adds a `minReadCount` option to `MultiReader`. Default = 1. The reader will not read fewer than `minReadCount` samples. If there are fewer than `minReadCount` samples in the queue and an event arrives after those samples, it will discard the samples and return the event.

## Required integration changes

- None.

## API changes

```diff
+ [function] IMultiReaderBuilder::setMinReadCount(SizeT minReadCount)
+ [function] IMultiReaderBuilder::getMinReadCount(SizeT* minReadCount)
-m [factory] MultiReaderPtr MultiReaderEx(const ListPtr<ISignal>& signals, SampleType valueReadType, SampleType domainReadType, ReadMode mode = ReadMode::Scaled, ReadTimeoutType timeoutType = ReadTimeoutType::All, Int requiredCommonSampleRate = -1, bool startOnFullUnitOfDomain = false)
+m [factory] MultiReaderPtr MultiReaderEx(const ListPtr<ISignal>& signals, SampleType valueReadType, SampleType domainReadType, ReadMode mode = ReadMode::Scaled, ReadTimeoutType timeoutType = ReadTimeoutType::All, Int requiredCommonSampleRate = -1, bool startOnFullUnitOfDomain = false, SizeT minReadCount = 1)
```

---

# Concurrent Config Connections Limit Over Native Server

## Description

- Enables concurrent config connections limit for the native server using `"MaxAllowedConfigConnections"` server config property.
- Introduces a new `PacketBuffer` type `ConnectionRejected` in the native configuration protocol.
- Native config protocol bumped to version 3.

---

# Property Value Write/Read Events Over Native Protocol

## Description

- Bugfix where `onPropertyValueWrite`/`onPropertyValueRead` events were available on the native protocol client but were not fully supported.
- These property object events were disabled to reduce the probability of misuse.
- `CoreEvents` should be used instead of `onWrite`/`onRead` events where needed.

---

# Support for Device Locking Over Native Protocol

> [!CAUTION]
> Breaks binary compatibility

## Description

- Adds support for device locking over the native config protocol.

## API changes

```diff
+ [function] IDevice::lock()
+ [function] IDevice::unlock()
+ [function] IDevice::isLocked(Bool* locked)

+ [function] IAuthenticationProvider::authenticateAnonymous(IUser** userOut)
```

---

# Manually Dropping Data Packets in MultiReader

## Description

- Enables `MultiReader` to be manually set inactive to drop data packets.
- Adds validation of unit parameters for `power-reader` function block input voltage signal.

## Required integration changes

- The unit symbol should be set to `"V"` in the unit object assigned for the voltage input signal descriptor of the `RefFBModulePowerReader` function block.

## API changes

```diff
+ [function] IMultiReader::setActive(Bool isActive)
+ [function] IMultiReader::getActive(Bool* isActive)
```

---

# Native Protocol Client-to-Device Streaming

## Description

- Enables client-to-device streaming within the Native protocol.
- Introduces a new `PacketBuffer` type `NoReplyRpc` in the native configuration protocol.
- Native config protocol bumped to version 2.

---

# Adding openDAQ Servers Under Device

> [!CAUTION]
> Breaks binary compatibility

## Description

- Enables `openDAQ` servers to be added to the component tree under the device.

## API changes

```diff
-m[interface] IServer : public IBaseObject
+m[interface] IServer : public IFolder
+ [function] IServer::getSignals(IList** signals, ISearchFilter* searchFilter = nullptr)

- [function] IInstance::addServer(IString* serverTypeId, IPropertyObject* serverConfig, IServer** server)
- [function] IInstance::removeServer(IServer* server)
- [function] IInstance::getServers(IList** servers)

+ [function] IDevice::addServer(IString* typeId, IPropertyObject* config, IServer** server)
+ [function] IDevice::removeServer(IServer* server)
+ [function] IDevice::getServers(IList** servers)

+ [function] IModuleManagerUtils::createServer(IServer** server, IString* serverTypeId, IDevice* rootDevice, IPropertyObject* serverConfig = nullptr)
```

---

# MultiReader Returns Events on First Read

## Description

- `MultiReader` returns events on the first read.
- Sets the default `skipEvents` for `BlockReader` to `false`.

## Required integration changes

- By default, creating a `BlockReader` with a signal had `skipEvents = true`. Now, `skipEvents` is set to `false`.
- `MultiReader` does not lose the first connection event packet. With the first read, `MultiReader` now returns the event packets received by signal connection.

---

# Improves Save/Load Mechanism for Input Port Connections

## Description

- Improves the save/load mechanism for restoring input port connections.

## API changes

```diff
+ [interface] IComponentUpdateContext : public IBaseObject
+ [function] IComponentUpdateContext::setInputPortConnection(IString* parentId, IString* portId, IString* signalId)
+ [function] IComponentUpdateContext::getInputPortConnections(IString* parentId, IDict** connections)
+ [function] IComponentUpdateContext::removeInputPortConnection(IString* parentId)
+ [function] IComponentUpdateContext::getRootComponent(IComponent** rootComponent)
+ [function] IComponentUpdateContext::getSignal(IString* parentId, IString* portId, ISignal** signal)
+ [function] IComponentUpdateContext::setSignalDependency(IString* signalId, IString* parentId)

-m[function] IUpdatable::updateEnded()
+m[function] IUpdatable::updateEnded(IBaseObject* context)
+ [function] IUpdatable::updateInternal(ISerializedObject* update, IBaseObject* context)
```

---

# OPENDAQ_ERR_CONNECTION_LOST and ConnectionLostException

## Description

- Adds `OPENDAQ_ERR_CONNECTION_LOST` error code and `ConnectionLostException` exception type.

## Required integration changes

- None. Disconnection errors can now be identified by a specific error type.

---

# Reference Domain Info for Syncing Signals

## Description

- `ReferenceDomainInfo` was added as an interface that provides additional information about the reference domain:
  - **Reference Domain ID** (Signals with the same Reference Domain ID share a common synchronization source and can be read together)
  - **Reference Domain Offset** (which must be added to the domain values of the Signal for them to be equal to that of the sync source)
  - **Reference Time Source** (which is used to determine if two signals with different Domain IDs can be read together); possible values are: 
      - [Tai](https://en.wikipedia.org/wiki/International_Atomic_Time)
      - [Gps](https://en.wikipedia.org/wiki/Global_Positioning_System#Timekeeping)
      - [Utc](https://en.wikipedia.org/wiki/Coordinated_Universal_Time)
      - Unknown
  - **Uses Offset**
- A builder is available for creating `ReferenceDomainInfo`.
- `ReferenceDomainInfo` is part of two interfaces:
  - `DeviceDomain`
  - `DataDescriptor`
- Currently, `ReferenceDomainInfo` is only supported over Native (not over OPC UA or LT Streaming protocols), causing two data descriptor changed events when combining supported and unsupported protocols for configuration or streaming.

## Required integration changes

- None, but users are encouraged to use `ReferenceDomainInfo`.

## API changes

```diff
+ [interface] IReferenceDomainInfo : public IBaseObject
+ [function] IReferenceDomainInfo::getReferenceDomainId(IString** referenceDomainId)
+ [function] IReferenceDomainInfo::getReferenceDomainOffset(IInteger** referenceDomainOffset)
+ [function] IReferenceDomainInfo::getReferenceTimeSource(TimeSource* referenceTimeSource)
+ [function] IReferenceDomainInfo::getUsesOffset(UsesOffset* usesOffset)

+ [interface] IReferenceDomainInfoBuilder : public IBaseObject
+ [function] IReferenceDomainInfoBuilder::build(IReferenceDomainInfo** referenceDomainInfo)
+ [function] IReferenceDomainInfoBuilder::setReferenceDomainId(IString** referenceDomainId)
+ [function] IReferenceDomainInfoBuilder::setReferenceDomainOffset(IInteger** referenceDomainOffset)
+ [function] IReferenceDomainInfoBuilder::setReferenceTimeSource(TimeSource* referenceTimeSource)
+ [function] IReferenceDomainInfoBuilder::setUsesOffset(UsesOffset* usesOffset)
+ [function] IReferenceDomainInfoBuilder::getReferenceDomainId(IString** referenceDomainId)
+ [function] IReferenceDomainInfoBuilder::getReferenceDomainOffset(IInteger** referenceDomainOffset)
+ [function] IReferenceDomainInfoBuilder::getReferenceTimeSource(TimeSource* referenceTimeSource)
+ [function] IReferenceDomainInfoBuilder::getUsesOffset(UsesOffset* usesOffset)

+ [factory] ReferenceDomainInfoBuilderPtr ReferenceDomainInfoBuilder()
+ [factory] ReferenceDomainInfoBuilderPtr ReferenceDomainInfoBuilderCopy(const ReferenceDomainInfoPtr& referenceDomainInfo)
+ [factory] ReferenceDomainInfoPtr ReferenceDomainInfoFromBuilder(const ReferenceDomainInfoBuilderPtr& builder)

+ [function] IDeviceDomain::getReferenceDomainInfo(IReferenceDomainInfo** referenceDomainInfo)
-m [factory] DeviceDomainPtr DeviceDomain(const RatioPtr& tickResolution, const StringPtr& origin, const UnitPtr& unit)
+m [factory] DeviceDomainPtr DeviceDomain(const RatioPtr& tickResolution, const StringPtr& origin, const UnitPtr& unit, const ReferenceDomainInfoPtr& referenceDomainInfo = nullptr)

+ [function] IDataDescriptor::getReferenceDomainInfo(IReferenceDomainInfo** referenceDomainInfo)
+ [function] IDataDescriptorBuilder::setReferenceDomainInfo(IReferenceDomainInfo* referenceDomainInfo)
+ [function] IDataDescriptorBuilder::getReferenceDomainInfo(IReferenceDomainInfo** referenceDomainInfo)
```

---

# Reworked IProperty onPropertyValueWrite/Read

## Description

- Changed logic of `IProperty::getOnPropertyValue` events.
- Now returns the owner's event when called, if the owner is assigned.
- A new function is available to get the class's event. This function is used internally to trigger class value change events.

## API changes

```diff
+ [function] IEvent::getSubscribers(IList** subscribers)
+ [function] IPropertyInternal::getClassOnPropertyValueWriteEvent(IEvent** event)
+ [function] IPropertyInternal::getClassOnPropertyValueReadEvent(IEvent** event)
```

---

# Sync Component Integration and ChildProperty Expressions

## Description

- Integration of the Sync Component.
- Populating eval expressions with `%ChildProperty:PropertyNames` to get the list of child property names, where the ChildProperty is an Object-type property.

## Required integration changes

- Each device now has a `sync` component, visible in default components as `Synchronization`.
- To set `Mode` selection values or `Status.State`, developers can set custom values for the property `ModeOptions` or `Status.StateOptions`.
- The Sync component replaces the dummy property object in the ref device.

## API changes

```diff
+ [interface] ISyncComponent : public IComponent
+ [function] ISyncComponent::getSyncLocked(Bool* synchronizationLocked)
+ [function] ISyncComponent::getSelectedSource(Int* selectedSource)
+ [function] ISyncComponent::setSelectedSource(Int selectedSource)
+ [function] ISyncComponent::getInterfaces(IDict** interfaces)

+ [interface] ISyncComponentPrivate : public IBaseObject
+ [function] ISyncComponentPrivate::setSyncLocked(Bool synchronizationLocked)
+ [function] ISyncComponentPrivate::addInterface(IPropertyObject* syncInterface)
+ [function] ISyncComponentPrivate::removeInterface(IString* syncInterfaceName)

+ [factory] SyncComponentPtr SyncComponent(const ContextPtr& context, const ComponentPtr& parent, const StringPtr& localId)
```

---

# IProperty getValue/setValue Methods

> [!CAUTION]
> Breaks binary compatibility

## Description

- Adds `getValue`/`setValue` methods to `IProperty`.
- Helpers to provide easier access to the property's value when iterating through an object's properties.

## API changes

```diff
+ [function] IProperty::getValue(IBaseObject** value)
+ [function] IProperty::setValue(IBaseObject* value)
```

---

# Add getUpdating() to IPropertyObject

## Description

- Adds a method to `IPropertyObject` to detect begin/end update status.

## API changes

```diff
+ [function] IPropertyObject::getUpdating(Bool* updating)
```

---

# Add User Context to JSON Serializer

## Description

- Adds user context to the JSON serializer.

## API changes

```diff
+ [function] ISerializer::getUser(IBaseObject** user)
+ [function] ISerializer::setUser(IBaseObject* user)
```

---

# Reader Builders for All Readers

## Description

- Reader improvement.
- Implements `reader builder` for all Readers.
- Populates connection methods.

## Required integration changes

1. It is disallowed to create a reader with an input port that is already connected to a signal. You must change the order of creation:

   - create port
   - create reader
   - connect signal to port

   Otherwise, an exception will be thrown.

2. On the first read, the reader returns the first event packet.
3. To read data without interruption on an event packet, create the reader via the builder with `setSkipEvents(true)`.
4. `reader::getAvailableSamples` returns available samples up to:

   - the next event packet if `skipEvents == false`
   - the next gap packet if `skipEvents == true`

   Use `IReader::getEmpty(Bool* empty)` to check if there is a data packet or an event packet to read.

## API changes

```diff
+ [function] IReader::getEmpty(Bool* empty)

+ [function] IBlockReaderBuilder::setSkipEvents(Bool skipEvents)
+ [function] IBlockReaderBuilder::getSkipEvents(Bool* skipEvents)

-m [function] IBlockReader::read(void* blocks, SizeT* count, SizeT timeoutMs = 0, IReaderStatus** status = nullptr)
+m [function] IBlockReader::read(void* blocks, SizeT* count, SizeT timeoutMs = 0, IBlockReaderStatus** status = nullptr)
-m [function] IBlockReader::readWithDomain(void* dataBlocks, void* domainBlocks, SizeT* count, SizeT timeoutMs = 0, IReaderStatus** status = nullptr)
+m [function] IBlockReader::readWithDomain(void* dataBlocks, void* domainBlocks, SizeT* count, SizeT timeoutMs = 0, IBlockReaderStatus** status = nullptr)

-m [function] IMultiReader::read(void* samples, SizeT* count, SizeT timeoutMs = 0, IReaderStatus** status = nullptr)
+m [function] IMultiReader::read(void* samples, SizeT* count, SizeT timeoutMs = 0, IMultiReaderStatus** status = nullptr)
-m [function] IMultiReader::readWithDomain(void* samples, void* domain, SizeT* count, SizeT timeoutMs = 0, IReaderStatus** status = nullptr)
+m [function] IMultiReader::readWithDomain(void* samples, void* domain, SizeT* count, SizeT timeoutMs = 0, IMultiReaderStatus** status = nullptr)
-m [function] IMultiReader::skipSamples(SizeT* count, IReaderStatus** status)
+m [function] IMultiReader::skipSamples(SizeT* count, IMultiReaderStatus** status)

-m [function] ITailReader::read(void* values, SizeT* count, IReaderStatus** status = nullptr)
+m [function] ITailReader::read(void* values, SizeT* count, ITailReaderStatus** status = nullptr)
-m [function] ITailReader::readWithDomain(void* values, void* domain, SizeT* count, IReaderStatus** status = nullptr)
+m [function] ITailReader::readWithDomain(void* values, void* domain, SizeT* count, ITailReaderStatus** status = nullptr)

m [function] IReaderStatus::getOffset(INumber** offset)

-m [factory] ReaderStatusPtr ReaderStatus(const EventPacketPtr& packet = nullptr, Bool valid = true)
+m [factory] ReaderStatusPtr ReaderStatus(const EventPacketPtr& packet = nullptr, Bool valid = true, const NumberPtr& offset = 0)
-m [factory] BlockReaderStatusPtr BlockReaderStatus(const EventPacketPtr& packet = nullptr, Bool valid = true, SizeT readSamples = 0)
+m [factory] BlockReaderStatusPtr BlockReaderStatus(const EventPacketPtr& packet = nullptr, Bool valid = true, const NumberPtr& offset = 0, SizeT readSamples = 0)
-m [factory] TailReaderStatusPtr TailReaderStatus(const EventPacketPtr& packet = nullptr, Bool valid = true, Bool sufficientHistory = true)
+m [factory] TailReaderStatusPtr TailReaderStatus(const EventPacketPtr& packet = nullptr, Bool valid = true, const NumberPtr& offset = 0, Bool sufficientHistory = true)
-m [factory] MultiReaderStatusPtr MultiReaderStatus(const DictPtr<ISignal, IEventPacket>& eventPackets = nullptr, Bool valid = true)
+m [factory] MultiReaderStatusPtr MultiReaderStatus(const EventPacketPtr& mainDescriptor, const DictPtr<IString, IEventPacket>& eventPackets = nullptr, Bool valid = true, const NumberPtr& offset = 0)

+ [factory] StreamReaderBuilderPtr StreamReaderBuilder()
+ [factory] StreamReaderPtr StreamReaderFromBuilder(const StreamReaderBuilderPtr& builder)
+ [factory] TailReaderBuilderPtr TailReaderBuilder()
+ [factory] TailReaderPtr TailReaderFromBuilder(const TailReaderBuilderPtr& builder)

-m [factory] StreamReaderPtr TailReaderFromExisting(TailReaderPtr invalidatedReader, SizeT historySize)
+m [factory] TailReaderPtr TailReaderFromExisting(TailReaderPtr invalidatedReader, SizeT historySize)

+ [interface] IStreamReaderBuilder : public IBaseObject
+ [function] IStreamReaderBuilder::build(IStreamReader** streamReader)
+ [function] IStreamReaderBuilder::setSignal(ISignal* signal)
+ [function] IStreamReaderBuilder::getSignal(ISignal** signal)
+ [function] IStreamReaderBuilder::setInputPort(IInputPort* port)
+ [function] IStreamReaderBuilder::getInputPort(IInputPort** port)
+ [function] IStreamReaderBuilder::setValueReadType(SampleType type)
+ [function] IStreamReaderBuilder::getValueReadType(SampleType* type)
+ [function] IStreamReaderBuilder::setDomainReadType(SampleType type)
+ [function] IStreamReaderBuilder::getDomainReadType(SampleType* type)
+ [function] IStreamReaderBuilder::setReadMode(ReadMode mode)
+ [function] IStreamReaderBuilder::getReadMode(ReadMode* mode)
+ [function] IStreamReaderBuilder::setReadTimeoutType(ReadTimeoutType type)
+ [function] IStreamReaderBuilder::getReadTimeoutType(ReadTimeoutType* type)
+ [function] IStreamReaderBuilder::setSkipEvents(Bool skipEvents)
+ [function] IStreamReaderBuilder::getSkipEvents(Bool* skipEvents)

+ [interface] ITailReaderBuilder : public IBaseObject
+ [function] ITailReaderBuilder::build(ITailReader** tailReader)
+ [function] ITailReaderBuilder::setSignal(ISignal* signal)
+ [function] ITailReaderBuilder::getSignal(ISignal** signal)
+ [function] ITailReaderBuilder::setInputPort(IInputPort* port)
+ [function] ITailReaderBuilder::getInputPort(IInputPort** port)
+ [function] ITailReaderBuilder::setValueReadType(SampleType type)
+ [function] ITailReaderBuilder::getValueReadType(SampleType* type)
+ [function] ITailReaderBuilder::setDomainReadType(SampleType type)
+ [function] ITailReaderBuilder::getDomainReadType(SampleType* type)
+ [function] ITailReaderBuilder::setReadMode(ReadMode mode)
+ [function] ITailReaderBuilder::getReadMode(ReadMode* mode)
+ [function] ITailReaderBuilder::setHistorySize(SizeT historySize)
+ [function] ITailReaderBuilder::getHistorySize(SizeT* historySize)
+ [function] ITailReaderBuilder::setSkipEvents(Bool skipEvents)
+ [function] ITailReaderBuilder::getSkipEvents(Bool* skipEvents)

-m [function] TimeReader::readWithDomain(void* values, std::chrono::system_clock::time_point* domain, daq::SizeT* count, daq::SizeT timeoutMs = 0, IReaderStatus** status = nullptr) const
+m [function] TimeReader::readWithDomain(void* values, std::chrono::system_clock::time_point* domain, daq::SizeT* count, daq::SizeT timeoutMs = 0, IReaderStatusType** status = nullptr)

+ [function] IConnection::getSamplesUntilNextEventPacket(SizeT* samples)
+ [function] IConnection::getSamplesUntilNextGapPacket(SizeT* samples)
+ [function] IConnection::hasEventPacket(Bool* hasEventPacket)
+ [function] IConnection::hasGapPacket(Bool* hasGapPacket)
```

---

# Standardizing Various IDs and Names to PascalCase

## Description

- Standardizes various IDs and names to PascalCase:
  - **Component IDs**:
    - Reference Device
    - Reference Function Blocks (backwards compatible): Classifier, FFT, Power, Renderer, Scaling, Statistics, Trigger
    - AudioDeviceModuleWavWriter
    - Reference device IO components (AI, CAN, RefCh)
  - **Component names**:
    - Default client device
  - **Type IDs**:
    - Reference modules
    - Streaming/config clients (backwards compatible)
    - Server modules
    - MiniAudio
  - **Type names**:
    - Reference modules
    - Streaming/config clients
    - Server modules
  - **Server capability protocol ID** (backwards compatible)
  - **Server capability protocol name**
  - **Struct Type field names**
  - **Other** strings
- Old IDs can still be used when adding new objects to a device (for backwards compatibility), but direct string comparisons may fail if they rely on old IDs.

## Required integration changes

- Generally none, except where an integration depends on changed strings in some way. If relying on string comparison to old IDs, update them accordingly.

---

# AddressType and AddressReachability in ServerCapability

## Description

- Adds `addressType` and `addressReachabilityStatus` to `serverCapability`.
- Allows easier identification of what address is used, and checks if the device is available.
- Reachability is currently only available for IPv4.
- `"canPing"` and `"ipv4Address"` properties have been removed from discovered device info.

## Required integration changes

- If a client application relied on `"canPing"` and `"ipv4Address"` on device info, it should now check the `ServerCapability` fields `"AddressType"` in conjunction with `"Addresses"`.
- `"canPing"` has been replaced with `AddressReachabilityInfo`. Non-IPv4 addresses are currently labeled as `"Unknown"` in terms of reachability.

## API changes

```diff
+ [function] IServerCapability::getAddressTypes(IList** addressTypes)
+ [function] IServerCapability::getAddressReachabilityStatus(IList** addressReachability)
+ [function] IServerCapabilityConfig::addAddressType(IString* addressType)
+ [function] IServerCapabilityConfig::addAddressReachabilityStatus(AddressReachabilityStatus addressReachability)
+ [function] IServerCapabilityConfig::setAddressReachabilityStatus(IList* addressReachability)
```

---

# Mandatory Connection String Prefix and Removed Accept Methods

## Description

- Makes the device connection string prefix mandatory.
- Removes `"accepts connection string"` methods from module.

## Required integration changes

- `onGetAvailableDeviceTypes()` override in modules must add a `"prefix"` to its device type.
- `onAcceptsConnectionParameters` and `onAcceptsStreamingConnectionParameters` overrides should be removed from modules.
- Connection strings must now always start with a prefix, followed by `"://"`.

## API changes

```diff
-m [factory] inline DeviceTypePtr DeviceType(const StringPtr& id, const StringPtr& name, const StringPtr& description, const PropertyObjectPtr& defaultConfig = PropertyObject())
+m [factory] inline DeviceTypePtr DeviceType(const StringPtr& id, const StringPtr& name, const StringPtr& description, const StringPtr& prefix, const PropertyObjectPtr& defaultConfig = PropertyObject())
- [function] IModule::acceptsConnectionParameters(Bool* accepted, IString* connectionString, IPropertyObject* config = nullptr)
- [function] IModule::acceptsStreamingConnectionParameters(Bool* accepted, IString* connectionString, IPropertyObject* config = nullptr)
```

---

# New IDeviceInfo hasServerCapability

## API changes

```diff
- [function] IDeviceInfoInternal::hasServerCapability(IString* protocolId, Bool* hasCapability)
+ [function] IDeviceInfo::hasServerCapability(IString* protocolId, Bool* hasCapability)
+ [function] IDeviceInfo::getServerCapability(IString* protocolId, IServerCapability** capability)
```

---

# Discovery Service Renamed to Discovery Server

## API changes

```diff
-m [function] IInstanceBuilder::addDiscoveryService
+m [function] IInstanceBuilder::addDiscoveryServer
-m [function] IInstanceBuilder::getDiscoveryServices
+m [function] IInstanceBuilder::getDiscoveryServers
```

---

# StreamingType and ComponentType as Builder

## Description

- Adds `StreamingType` object.
- Changes `ComponentType` objects to a builder pattern.
- Provides default `add-device` config object containing config of all modules.

## API changes

```diff
+ [function] IDevice::createDefaultAddDeviceConfig(IPropertyObject** defaultConfig)
+ [function] IDeviceType::getConnectionStringPrefix(IString** prefix) = 0;
+ [interface] IComponentTypeBuilder : public IBaseObject
+ [interface] IServerType : public IComponentType
+ [factory] ComponentTypeBuilderPtr StreamingTypeBuilder()
+ [factory] ComponentTypeBuilderPtr DeviceTypeBuilder()
+ [factory] ComponentTypeBuilderPtr FunctionBlockTypeBuilder()
+ [factory] ComponentTypeBuilderPtr ServerTypeBuilder()
+ [function] createDefaultAddDeviceConfig(IPropertyObject** defaultConfig)
+ [function] IModule::getAvailableStreamingTypes(IDict** streamingTypes)
+ [function] IModuleManagerUtils::getAvailableStreamingTypes(IDict** streamingTypes)
+ [function] IModuleManagerUtils::createDefaultAddDeviceConfig(IDict** streamingTypes)
```

---

# Reading Client Connection Info in deviceInfo

## Description

- Supports reading the client's connection info in the `deviceInfo`.

## API changes

```diff
+ [function] IDeviceInfo::getConfigurationConnectionInfo(IServerCapability** connectionInfo)
```

---

# Servers Discovered by mDNS

## Description

- Supports servers being discovered by mDNS.

## API changes

```diff
+ [interface] IServer::getId(IString** serverId)
+ [interface] IServer::enableDiscovery();

+ [interface] IDiscoveryServer
+ [function] IDiscoveryServer::registerService(IString* id, IPropertyObject* config, IDeviceInfo* deviceInfo);
+ [function] IDiscoveryServer::unregisterService(IString* id)
+ [factory] DiscoveryServerPtr MdnsDiscoveryServer(const LoggerPtr& logger)

+ [function] IInstanceBuilder::getDiscoveryServices(IList** services)
+ [function] IInstanceBuilder::addDiscoveryService(IString* serviceName)

+ [function] Context::getDiscoveryServers(IDict** services);
-m [factory] ContextPtr Context(const SchedulerPtr& scheduler, const LoggerPtr& logger, const TypeManagerPtr& typeManager, const ModuleManagerPtr& moduleManager, const AuthenticationProviderPtr& authenticationProvider, const DictPtr<IString, IBaseObject> options = Dict<IString, IBaseObject>())
+m [factory] ContextPtr Context(const SchedulerPtr& scheduler, const LoggerPtr& logger, const TypeManagerPtr& typeManager, const ModuleManagerPtr& moduleManager, const AuthenticationProviderPtr& authenticationProvider, const DictPtr<IString, IBaseObject> options = Dict<IString, IBaseObject>(), const DictPtr<IString, IDiscoveryServer> discoveryServices = Dict<IString, IDiscoveryServer>())
```

---

# Manual Device Streaming Connection After addDevice

## Description

- Adds ability to manually connect to streaming for a device after the device is added.
- Creates connection string from `ServerCapability` via modules.

## API changes

```diff
+ [function] IDevice::addStreaming(IStreaming** streaming, IString* connectionString, IPropertyObject* config = nullptr)
+ [function] IModuleManagerUtils::createStreaming(IStreaming** streaming, IString* connectionString, IPropertyObject* config = nullptr)

-m [function] IModule::createStreaming(IStreaming** streaming, IString* connectionString, IPropertyObject* config)
+m [function] IModule::createStreaming(IStreaming** streaming, IString* connectionString, IPropertyObject* config = nullptr)

+ [function] IModule::createConnectionString(IString** connectionString, IServerCapability* serverCapability)
```

---

# Send/Dequeue Multiple Packets

## Description

- Adds functions for sending and dequeueing multiple packets.
- Adds functions with a _steal reference_ behavior for sending packets.

## API changes

```diff
+ [function] ErrCode ISignalConfig::sendPackets(IList* packets)
+ [function] ErrCode ISignalConfig::sendPacketAndStealRef(IPacket* packet)
+ [function] ErrCode ISignalConfig::sendPacketsAndStealRef(IList* packets)

+ [function] ErrCode IConnection::enqueueAndStealRef(IPacket* packet)
+ [function] ErrCode IConnection::enqueueMultiple(IList* packets)
+ [function] ErrCode IConnection::enqueueMultipleAndStealRef(IList* packets)
+ [function] ErrCode IConnection::dequeueAll(IList** packets)()
```

---

# Producing Gap Packets on Request

## Description

- Produces gap packets on request.

## API changes

```diff
-m [factory] InputPortConfigPtr InputPort(const ContextPtr& context, const ComponentPtr& parent, const StringPtr& localId)
+m [factory] InputPortConfigPtr InputPort(const ContextPtr& context, const ComponentPtr& parent, const StringPtr& localId, bool gapChecking = false)
+ [function] InputPortConfig::getGapCheckingEnabled(Bool* gapCheckingEnabled);
+ [factory] EventPacketPtr ImplicitDomainGapDetectedEventPacket(const NumberPtr& diff)
+ [packet] IMPLICIT_DOMAIN_GAP_DETECTED
```

---

# Cloning a Property Object to Create a Default Config

## Description

- Clones a property object to create a default config from a type.

## API changes

```diff
-m [factory] DeviceTypePtr DeviceType(const StringPtr& id, const StringPtr& name, const StringPtr& description, const FunctionPtr& createDefaultConfigCallback = nullptr)
+m [factory] DeviceTypePtr DeviceType(const StringPtr& id, const StringPtr& name, const StringPtr& description, const PropertyObjectPtr& defaultConfig = PropertyObject())
-m [factory] FunctionBlockTypePtr FunctionBlockType(const StringPtr& id, const StringPtr& name, const StringPtr& description, const FunctionPtr& createDefaultConfigCallback = nullptr)
+m [factory] FunctionBlockTypePtr FunctionBlockType(const StringPtr& id, const StringPtr& name, const StringPtr& description, const PropertyObjectPtr& defaultConfig = PropertyObject())
-m [factory] ServerTypePtr ServerType(const StringPtr& id, const StringPtr& name, const StringPtr& description, const FunctionPtr& createDefaultConfigCallback = nullptr)
+m [factory] ServerTypePtr ServerType(const StringPtr& id, const StringPtr& name, const StringPtr& description, const PropertyObjectPtr& defaultConfig = PropertyObject())
```

---

# Mirrored Device Base Implementation

## Description

- Adds a mirrored device base implementation as a general approach to manage streaming sources for configuration-enabled devices.

## API changes

```diff
+ [interface] IMirroredDevice
+ [function] IMirroredDevice::getStreamingSources(IList** streamingSources)

+ [interface] IMirroredDeviceConfig
+ [function] IMirroredDeviceConfig::addStreamingSource(IStreaming* streamingSource)
+ [function] IMirroredDeviceConfig::removeStreamingSource(IString* streamingConnectionString)
```

---

# Add Addresses in ServerCapability

## Description

- Adds addresses in `ServerCapability`.

## API changes

```diff
+ [function] IServerCapabilityConfig::addAddress(IString* address)
+ [function] IServerCapability::getAddresses(IList** addresses)
```

---

# Fix Delphi Keyword Clashes

## Description

- Fixes reserved keyword clashes with Delphi bindings.

## API changes

```diff
- [function] IPermissionsBuilder::set(StringPtr groupId, PermissionMaskBuilderPtr permissions)
+ [function] IPermissionsBuilder::assign(StringPtr groupId, PermissionMaskBuilderPtr permissions)
```
