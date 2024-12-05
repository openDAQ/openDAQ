# 04.12.2024
##
- Add "Any read/write" events to property object.
- These events are triggered whenever any property value is read/written.

```
+ [function] IPropertyObject::getOnAnyPropertyValueWrite(IEvent** event)
+ [function] IPropertyObject::getOnAnyPropertyValueRead(IEvent** event)
```

# 28.11.2024:
## Description
- Introduces separate container accessible per device for connection statuses
- Introduces new core event type "ConnectionStatusChanged" to notify when configuration or streaming connection status of the device changed
    limitations: streaming connections statuses are neither serialized nor propagated through the native configuration protocol
- daq Context always has TypeManager assigned
- "ConnectionStatusType" added to TypeManager by default
- Makes connection status accessible directly from Streaming object

## Required integration changes
- Breaks binary compatibility
- Introduces new mechanism for retrieving connection status of the device

```
+ [interface] IConnectionStatusContainerPrivate : public IBaseObject
+ [function] IConnectionStatusContainerPrivate::addConfigurationConnectionStatus(IString* connectionString, IEnumeration* initialValue)
+ [function] IConnectionStatusContainerPrivate::addStreamingConnectionStatus(IString* connectionString, IEnumeration* initialValue, IStreaming* streamingObject)
+ [function] IConnectionStatusContainerPrivate::removeStreamingConnectionStatus(IString* connectionString)
+ [function] IConnectionStatusContainerPrivate::updateConnectionStatus(IString* connectionString, IEnumeration* value, IStreaming* streamingObject)

+ [function] IDevice::getConnectionStatusContainer(IComponentStatusContainer** statusContainer)

+ [function] IStreaming::getConnectionStatus(IEnumeration** connectionStatus)
```

# 25.11.2024
## Description
- Delete the fields `id`, `name`, and `versionInfo` from the `IModule` interface and add them to the new `IModuleInfo` interface, which is a new field in the `IModule` interface
- `IModuleInfo` field is also added to `IComponentType` interface
- Works over Native
- Component Type is moved from `coreobjects` to `opendaq`

## Required integration changes
- Breaks binary compatibility

```
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

# 20.11.2024
## Description
- Support for different client types over the native configuration protocol has been introduced.
- A ClientType selection property has been added to the general configuration object of the IDevice::addDevice method. Supported values are Control(0), Exclusive Control(1), and View Only(2).
- An ExclusiveControlDropOthers boolean property has been added to the general configuration object of the IDevice::addDevice method.
- The native configuration protocol has been updated to version 7.

## Required integration changes
- None. The default client type is Control(0), which is backward-compatible with previous protocol versions.

# 19.11.2024
## Description
- Raise minimum required config protocol version to 6 for the following device lockign metods: IDevice::lock(), IDevice::unlock(), IDevice::isLocked(Bool* isLockedOut)

# 12.11.2024:
## Description
- Improved component updates.
- Removed generation of local ID for client device in instance.
- Fixed connecting signals to input ports on the server to which the client is connecting during configuration loading.
## Required integration changes:
- An instance with the client device as the root device no longer generates a unique ID but uses `openDAQDevice` as the ID.
- If a developer encountered issues restoring signal connections from the client to the server during configuration loading, they need to create a new configuration and save it. Otherwise, everything should work fine.

```
+ [function] ISignalPrivate::getSignalSerializeId(IString** serializeId)
```

# 08.11.2024
## Description
- Add support for forcefully unlocking a device over native config protocol
- Native config protocol bumped to version 6
- Add user lock object to serialized device

## Required integration changes
- Breaks binary compatibility

```
+ [function] IDevicePrivate::forceUnlock()

+ [function] IAuthenticationProvider::findUser(IString* username, IUser** userOut)

+ [interface] IUserLock : public IBaseObject
+ [function] IUserLock::lock(IUser* user = nullptr)
+ [function] IUserLock::unlock(IUser* user = nullptr)
+ [function] IUserLock::forceUnlock()
+ [function] IUserLock::isLocked(Bool* isLockedOut)
```

# 29.10.2024:
## Description
- Implement thread synchronization mechanism in property objects
- Provides new internal method to allow for recursive locking in onWrite/onRead events via `GenericPropertyObjectImpl::getRecursiveConfigLock()`
- A standard lock guard can be acquired via `GenericPropertyObjectImpl::getAcquisitionLock()`
- The `sync` mutex previously available in `ComponentImpl` was moved up to `GenericPropertyObjectImpl`

## Required integration changes:
- The `sync` mutex available in the `GenericPropertyObjectImpl` should no longer be locked in `onWrite`/`onRead` events. If needed due to used programming patterns, the `getRecursiveConfigLock` method should be used instead. See the reference device implementations for guidance.
- Current device/function block implementations that lock the `sync` mutex during events will deadlock.

```
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

# 8.11.2024
## Description
- Block Reader, Tail Reader and Stream Reader now skip events by default in Python

## Required integration changes
- In Python, you need to explicitly set `skip_events=False` if you don't want to skip the events in Block, Tail and Stream Reader

# 29.10.2024

## Description
- Support creating readers with connected input port

## Required integration changes
- None

```
+ [interface] IConnectionInternal : public IBaseObject
+ [function] IConnectionInternal::enqueueLastDescriptor()
```

# 24.10.2024

## Description
- Add a way to view the server protocol version in server capabilities.
- Fix the path issue in ConfigurationConnectionInfo of device info: If the connection to the device included a path, ensure the path is correctly reflected in the ConfigurationConnectionInfo.
- For native configuration, update the ConfigurationConnectionInfo of device info to display the actual protocol version used in communication between the server and client.
- Native client default config property `ProtocolVersion` is set to the latest support protocol version

##  Required integration changes
- Breaks binary compatibility

```
+ [function] IServerCapability::getProtocolVersion(IString** version)
+ [function] IServerCapabilityConfig::setProtocolVersion(IString* version)
```

# 24.10.2024

## Description
- Implement log file info interface

## Required integration changes:
- Breaks binary compatibility

```
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

# 21.10.2024

## Description
- Introduce a new Sample Type "Null"
- Replace nullptr with a Data Descriptor having SampleType::Null in the "DATA_DESCRIPTOR_CHANGED" event packet when a signal's descriptor is not assigned
- Enable resetting the signal's Data Descriptor to nullptr

## Required integration changes:
- In the "DATA_DESCRIPTOR_CHANGED" event packet, the parameters "DataDescriptor" and "DomainDataDescriptor" are set to nullptr only if the corresponding descriptors have not changed.
  If the signal descriptor is not assigned, they are set to a DataDescriptor object with the "Null" sample type.

```
+ [factory] DataDescriptorPtr NullDataDescriptor()
```

# 11.10.2024

## Description
- Add methods in function block to add/remove nested fb

##  Required integration changes
- Breaks binary compatibility
- For function blocks that contain nested function blocks, developers should override the method 
    `FunctionBlockPtr onAddFunctionBlock(const StringPtr& typeId, const PropertyObjectPtr& config)` as this method is used during the loadConfiguration process. 
    Additionally, developers may optionally override the methods `DictPtr<IString, IFunctionBlockType> onGetAvailableFunctionBlockTypes()` 
    and `void onRemoveFunctionBlock(const FunctionBlockPtr& functionBlock)`.
    
    Examples of this can be found in the mock function block (`core/opendaq/opendaq/mocks/include/opendaq/mock/mock_fb.h`) 
    or the statistics function block (`modules/ref_fb_module/include/ref_fb_module/statistics_fb_impl.h`).

```
+ [function] IFunctionBlock::getAvailableFunctionBlockTypes(IDict** functionBlockTypes)
+ [function] IFunctionBlock::addFunctionBlock(IFunctionBlock** functionBlock, IString* typeId, IPropertyObject* config = nullptr)
+ [function] IFunctionBlock::removeFunctionBlock(IFunctionBlock* functionBlock)
```

# 10.10.2024

## Description
- Restoring the device while loading the configuration.
- Add update parameters to set a flag indicating whether to use the existing device or recreate a new one

## Required integration changes
- Breaks binary compatibility

```
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

# 4.10.2024

## Description
- Adds min read count option to multi reader. Default = 1. Reader will not read less that "min read count". If there are less
than "min read count" samples in the queue and there's an event after those samples, it will discard the samples and return event.

## Required integration changes 
- None

```
+ [function] IMultiReaderBuilder::setMinReadCount(SizeT minReadCount)
+ [function] IMultiReaderBuilder::getMinReadCount(SizeT* minReadCount)
-m [factory] MultiReaderPtr MultiReaderEx(const ListPtr<ISignal>& signals, SampleType valueReadType, SampleType domainReadType, ReadMode mode = ReadMode::Scaled, ReadTimeoutType timeoutType = ReadTimeoutType::All, Int requiredCommonSampleRate = -1, bool startOnFullUnitOfDomain = false)
+m [factory] MultiReaderPtr MultiReaderEx(const ListPtr<ISignal>& signals, SampleType valueReadType, SampleType domainReadType, ReadMode mode = ReadMode::Scaled, ReadTimeoutType timeoutType = ReadTimeoutType::All, Int requiredCommonSampleRate = -1, bool startOnFullUnitOfDomain = false,  SizeT minReadCount = 1)
```

# 03.10.2024

## Description
- Enable concurrent config connections limit for native server using "MaxAllowedConfigConnections" server config property
- Introduce a new PacketBuffer type ConnectionRejected in the native configuration protocol
- Native config protocol bumped to version 3

# 03.10.2024

## Description
- Bugfix where onPropertyValueWrite/Read events were available on the native protocol client, but were not fully supported.
- Said property object events were disabled to reduce probability of misuse.
- CoreEvents should be used instead of onWrite/Read events where needed.

# 03.10.2024

## Description
- Add support for device locking over native config protocol

## Required integration changes
- Breaks binary compatibility
```
+ [function] IDevice::lock()
+ [function] IDevice::unlock()
+ [function] IDevice::isLocked(Bool* locked)

+ [function] IAuthenticationProvider::authenticateAnonymous(IUser** userOut)
```
# 23.09.2024

## Description
- Enable multireader to be manually set inactive to drop data packets
- Add validation of unit parameters for power-reader function block input voltage signal
## Required integration changes 
- Unit symbol should be set to "V" within the unit object assigned for voltage input signal descriptor of "RefFBModulePowerReader" function block
```
+ [function] IMultiReader::setActive(Bool isActive)
+ [function] IMultiReader::getActive(Bool* isActive)
```
# 11.09.2024

## Description
- Enable client-to-device streaming feature within the Native protocol
- Introduce a new PacketBuffer type NoReplyRpc in the native configuration protocol
- Native config protocol bumped to version 2

# 11.09.2024

## Description
- Enable openDAQ servers to be added to the component tree under the device

## Required integration changes
- Breaks binary compatibility
```
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
# 28.08.2024

## Description
- Multi reader returns events on first read
- Set default skip event for block reader to false
## Required integration changes
- By default creating block reader with signal had skip events true. Now skip events set to false
- Multi reader is not losing the first connection event packet. With first read, multi reader now returns event packets which were recieved by signal connection

# 28.08.2024

## Description
- Improving save/load mechanism for restoring input ports connection
```
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
# 26.08.2024

## Description
- Add OPENDAQ_ERR_CONNECTION_LOST error code and ConnectionLostException exception type.

## Required integration changes
- None, however, disconnection errors can now be identified by a specific error type.

#  21.08.2024

## Description
- Reference Domain Info was added as an interface that gives additional information about the reference domain
- Reference Domain Info has getters for:
    - Reference Domain ID (Signals with the same Reference Domain ID share a common synchronization source and can be read together)
    - Reference Domain Offset (which must be added to the domain values of the Signal for them to be equal to that of the sync source)
    - Reference Time Source (which is used to determine if two signals with different Domain IDs can be read together); possible values are: 
        - [Tai](https://en.wikipedia.org/wiki/International_Atomic_Time)
        - [Gps](https://en.wikipedia.org/wiki/Global_Positioning_System#Timekeeping)
        - [Utc](https://en.wikipedia.org/wiki/Coordinated_Universal_Time)
        - Unknown
    - Uses Offset
- There is also a builder available for creating Reference Domain Info
- Reference Domain Info is a part of two interfaces:
    - Device Domain
    - Data Descriptor
- Reference Domain Info is currently only supported over Native, not over OPC UA or LT Streaming protocols (this will cause two data descriptor changed events to be sent when combining supported and unsupported protocols for configuration/streaming - for example OPC UA and Native streaming)

## Required integration changes
- None, however, users are encouraged to use Reference Domain Info
```
+ [interface] IReferenceDomainInfo : public IBaseObject
+ [function] IReferenceDomainInfo::getReferenceDomainId(IString** referenceDomainId)
+ [function] IReferenceDomainInfo::getReferenceDomainOffset(IInteger** referenceDomainOffset)
+ [function] IReferenceDomainInfo::getReferenceTimeSource(TimeSource* referenceTimeSource)
+ [function] IReferenceDomainInfo::getUsesOffset(UsesOffset * usesOffset)

+ [interface] IReferenceDomainInfoBuilder : public IBaseObject
+ [function] IReferenceDomainInfoBuilder::build(IReferenceDomainInfo** referenceDomainInfo)
+ [function] IReferenceDomainInfoBuilder::setReferenceDomainId(IString** referenceDomainId)
+ [function] IReferenceDomainInfoBuilder::setReferenceDomainOffset(IInteger** referenceDomainOffset)
+ [function] IReferenceDomainInfoBuilder::setReferenceTimeSource(TimeSource* referenceTimeSource)
+ [function] IReferenceDomainInfoBuilder::setUsesOffset(UsesOffset * usesOffset)
+ [function] IReferenceDomainInfoBuilder::getReferenceDomainId(IString** referenceDomainId)
+ [function] IReferenceDomainInfoBuilder::getReferenceDomainOffset(IInteger** referenceDomainOffset)
+ [function] IReferenceDomainInfoBuilder::getReferenceTimeSource(TimeSource* referenceTimeSource)
+ [function] IReferenceDomainInfoBuilder::getUsesOffset(UsesOffset * usesOffset)

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
# 12.08.2024

## Description
- Changed logic of IProperty::getOnPropertyValue events.
- Now returns the owner's event when called, if the owner is assigned.
- A new function is available to get the class's event. That function is used internally to trigger class value change events.
```
+ [function] IEvent::getSubscribers(IList** subscribers)
+ [function] IPropertyInternal::getClassOnPropertyValueWriteEvent(IEvent** event)
+ [function] IPropertyInternal::getClassOnPropertyValueReadEvent(IEvent** event)
```
# 12.08.2024

## Description
- Integration the Sync Component
- Populating eval expression with %ChildProperty:PropertyNames to get the list of child properties names where the ChildProperty is an Object-type property
## Required integration changes
- Each device now has a sync component, which is visible in default components as `Synchronization`.
- To set Mode selection values or Status.State, developer can set custom values for property ModeOptions or Status.StateOptions.
- Sync component is replacing dummy property object in the ref device.
```
+ [interface] ISyncComponent : public IComponent
+ [function] ISyncComponent::getSyncLocked(Bool* synchronizationLocked)
+ [function] ISyncComponent::getSelectedSource(Int* selectedSource)
+ [function] ISyncComponent::setSelectedSource(Int selectedSource)
+ [function] ISyncComponent::getInterfaces(IDict** interfaces)

+ [interface] ISyncComponentPrivate : public IBaseObject
+ [function] ISyncComponentPrivate::setSyncLocked(Bool synchronizationLocked);
+ [function] ISyncComponentPrivate::addInterface(IPropertyObject* syncInterface);
+ [function] ISyncComponentPrivate::removeInterface(IString* syncInterfaceName);

+ [factory] SyncComponentPtr SyncComponent(const ContextPtr& context, const ComponentPtr& parent, const StringPtr& localId)
```
# 08.08.2024

## Description
- Add get/setValue method to IProperty
- Helpers allowing easier access to the property's value when iterating through an object's properties
```
+ [function] IProperty::getValue(IBaseObject** value)
+ [function] IProperty::setValue(IBaseObject* value)
```
# 26.07.2024

## Description
- Add method to IPropertyObject to detect begin/end update status
## Required integration changes
- Breaks binary compatibility
```
+ [function] IPropertyObject::getUpdating(Bool* updating)
```
# 25.07.2024

## Description
- Add user context to json serializer
```
+ [function] ISerializer::getUser(IBaseObject** user)
+ [function] ISerializer::setUser(IBaseObject* user)
```
# 23.07.2024

## Description
- Reader improvement
- Implementing reader builder for all Readers
- Populate connection methods 
## Required integration changes
- Its refused to create reader with input port which connected to signal, so developer must change the order of creating reader: create port, create reader, connect signal to port. (Otherwise will be thrown an exception)
- In first read, reader returns first event packet
- to read data without interruption on event packet, developer can create reader with builder, with setSkipEvents(true)
- reader::getAvailableSamples returns available samples until event packet if skipEvents == false, or until gap packet if skipEvents == true. 
To check if reader has data to handle, was implemented method IReader::getEmptys(Bool* empty), which returns true if there is data packet to read or there is an event packet
```
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
+m [function] TimeReader::readWithDomain(void* values, std::chrono::system_clock::time_point* domain, daq::SizeT* count, daq::SizeT timeoutMs = 0, IReaderStatusType** status = nullptr) <- IReaderStatusType is type of internal reader

+ [function] IConnection::getSamplesUntilNextEventPacket(SizeT* samples)
+ [function] IConnection::getSamplesUntilNextGapPacket(SizeT* samples)
+ [function] IConnection::hasEventPacket(Bool* hasEventPacket)
+ [function] IConnection::hasGapPacket(Bool* hasGapPacket)
```
# 22.07.2024

## Description
- Standardize cases to PascalCase
    - Component IDs
        - Reference Device
        - Reference Function Blocks (backwards compatible)
            - Classifier
            - FFT
            - Power
            - Renderer
            - Scaling
            - Statistics
            - Trigger
        - AudioDeviceModuleWavWriter
        - Reference device IO components (AI, CAN, RefCh)
    - Component names
        - Default client device
    - Type IDs
        - Reference modules
        - Streaming/config clients (backwards compatible)
        - Server modules
        - MiniAudio
    - Type names
        - Reference modules
        - Streaming/config clients
        - Server modules
    - Server capability protocol ID (backwards compatible)
    - Server capability protocol name
    - Struct Type field names
    - Other

## Required integration changes
- Generally none, except for where integration depends upon changed strings listed above (in the description) in some way
- If relying on string comparison to hardcoded old IDs of things like FB, device, server types, or protocol IDs, those comparisons will need to be updated to match the new IDs, eg. a check like `if (fbType.getId() == "ref_fb_module_renderer")` will never be true
- Old IDs can still be used when adding new objects to a device via `addDevice`/`addFunctionBlock` or similar calls

# 10.07.2024

## Description
- Add address type and address reachability status to server capability
- Allows easier identification of what address is used, and checks if device is available
- Reachability is currently only available for ipv4
- "canPing" and "ipv4Address" properties have been removed from discovered device info

## Required integration changes
- If a client application has been relying on "canPing" and "ipv4Address" properties on device info, it should instead check the ServerCapability fields for "AddressType" in conjunction with "Addresses" to get the ipv4 address.
- "canPing" has been replaced with AddressReachabilityInfo on server capability and should be used instead. Non ipv4 addresses will for now be labelled as "Unknown" in terms of reachability
```
+ [function] IServerCapability::getAddressTypes(IList** addressTypes)
+ [function] IServerCapability::getAddressReachabilityStatus(IList** addressReachability)
+ [function] IServerCapabilityConfig::addAddressType(IString* addressType)
+ [function] IServerCapabilityConfig::addAddressReachabilityStatus(AddressReachabilityStatus addressReachability)
+ [function] IServerCapabilityConfig::setAddressReachabilityStatus(IList* addressReachability)
``` 
# 04.07.2024

## Description
- Make device connection string prefix mandatory
- Remove "accepts connection string" methods from module

## Required integration changes
- onGetAvailableDeviceTypes() override in modules now has to add a "prefix" to its device type
- onAcceptsConnectionParameters and onAcceptsStreamingConnectionParameters overrides should be removed from modules
- Connection strings must now always start with a prefix, followed by "://"
``` 
-m [factory] inline DeviceTypePtr DeviceType(const StringPtr& id, const StringPtr& name, const StringPtr& description, const PropertyObjectPtr& defaultConfig = PropertyObject())
+m [factory] inline DeviceTypePtr DeviceType(const StringPtr& id, const StringPtr& name, const StringPtr& description, const StringPtr& prefix, const PropertyObjectPtr& defaultConfig = PropertyObject())
- [function] IModule::acceptsConnectionParameters(Bool* accepted, IString* connectionString, IPropertyObject* config = nullptr)
- [function] IModule::acceptsStreamingConnectionParameters(Bool* accepted, IString* connectionString, IPropertyObject* config = nullptr)
```

# 21.06.2024
``` 
- [function] IDeviceInfoInternal::hasServerCapability(IString* protocolId, Bool* hasCapability)
+ [function] IDeviceInfo::hasServerCapability(IString* protocolId, Bool* hasCapability)
+ [function] IDeviceInfo::getServerCapability(IString* protocolId, IServerCapability** capability)
``` 
# 21.06.2024
``` 
-m [function] IInstanceBuilder::addDiscoveryService
+m [function] IInstanceBuilder::addDiscoveryServer
-m [function] IInstanceBuilder::getDiscoveryServices
+m [function] IInstanceBuilder::getDiscoveryServers
``` 
# 17.06.2024

## Description
- Add StreamingType object
- Change ComponentType objects to use builder pattern
- Provide default add-device config object containing config of all modules
``` 
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
# 30.05.2024

## Description
- Supporting reading client's connection info in the deviceInfo
```
+ [function] IDeviceInfo::getConfigurationConnectionInfo(IServerCapability** connectionInfo)
```
# 27.05.2024

## Description
- Supporting servers to be discovered by mDNS
``` 
+ [interface] IServer::getId(IString** serverId)
+ [interface] IServer::enableDiscovery();

+ [interface] IDiscoveryServer
+ [function] IDiscoveryServer::registerService(IString* id, IPropertyObject* config, IDeviceInfo* deviceInfo);
+ [function] IDiscoveryServer::unregisterService(IString* id)
+ [factory] DiscoveryServerPtr MdnsDiscoveryServer(const LoggerPtr& logger)

+ [function] IInstanceBuilder::getDiscoveryServices(IList** services)
+ [function] IInstanceBuilder::addDiscoveryService(IString* serviceName)

+ [function] Context::getDiscoveryServers(IDict** services);
-m [factory] ContextPtr Context(const SchedulerPtr& scheduler,
                           const LoggerPtr& logger,
                           const TypeManagerPtr& typeManager,
                           const ModuleManagerPtr& moduleManager,
                           const AuthenticationProviderPtr& authenticationProvider,
                           const DictPtr<IString, IBaseObject> options = Dict<IString, IBaseObject>())
+m [factory] ContextPtr Context(const SchedulerPtr& scheduler,
                           const LoggerPtr& logger,
                           const TypeManagerPtr& typeManager,
                           const ModuleManagerPtr& moduleManager,
                           const AuthenticationProviderPtr& authenticationProvider,
                           const DictPtr<IString, IBaseObject> options = Dict<IString, IBaseObject>(),
                           const DictPtr<IString, IDiscoveryServer> discoveryServices = Dict<IString, IDiscoveryServer>())
``` 
# 17.05.2024

## Description
- Add ability to manually connect to streaming for device after device added
- Create connection string from ServerCapability via modules
``` 
+ [function] IDevice::addStreaming(IStreaming** streaming, IString* connectionString, IPropertyObject* config = nullptr)
+ [function] IModuleManagerUtils::createStreaming(IStreaming** streaming, IString* connectionString, IPropertyObject* config = nullptr)

-m [function] IModule::createStreaming(IStreaming** streaming, IString* connectionString, IPropertyObject* config)
+m [function] IModule::createStreaming(IStreaming** streaming, IString* connectionString, IPropertyObject* config = nullptr)

+ [function] IModule::createConnectionString(IString** connectionString, IServerCapability* serverCapability)
``` 
# 16.05.2024

## Description
- Add functions for sending and dequeueing multiple packets
- Add functions with steal reference behaviour for sending packets
``` 
+ [function] ErrCode ISignalConfig::sendPackets(IList* packets)
+ [function] ErrCode ISignalConfig::sendPacketAndStealRef(IPacket* packet)
+ [function] ErrCode ISignalConfig::sendPacketsAndStealRef(IList* packets)

+ [function] ErrCode IConnection::enqueueAndStealRef(IPacket* packet)
+ [function] ErrCode IConnection::enqueueMultiple(IList* packets)
+ [function] ErrCode IConnection::enqueueMultipleAndStealRef(IList* packets)
+ [function] ErrCode IConnection::dequeueAll(IList** packets)()
``` 
# 26.04.2024

## Description
- Produce gap packets on request
``` 
-m [factory] InputPortConfigPtr InputPort(const ContextPtr& context, const ComponentPtr& parent, const StringPtr& localId)
+m [factory] InputPortConfigPtr InputPort(const ContextPtr& context, const ComponentPtr& parent, const StringPtr& localId, bool gapChecking = false)
+ [function] InputPortConfig::getGapCheckingEnabled(Bool* gapCheckingEnabled);
+ [factory] EventPacketPtr ImplicitDomainGapDetectedEventPacket(const NumberPtr& diff)
+ [packet] IMPLICIT_DOMAIN_GAP_DETECTED
``` 
# 26.04.2024

## Description
- Clone property object to create default config from type
``` 
-m [factory] DeviceTypePtr DeviceType(const StringPtr& id, const StringPtr& name, const StringPtr& description, const FunctionPtr& createDefaultConfigCallback = nullptr)
+m [factory] DeviceTypePtr DeviceType(const StringPtr& id, const StringPtr& name, const StringPtr& description, const PropertyObjectPtr& defaultConfig = PropertyObject())
-m [factory] FunctionBlockTypePtr FunctionBlockType(const StringPtr& id, const StringPtr& name, const StringPtr& description, const FunctionPtr& createDefaultConfigCallback = nullptr)
+m [factory] FunctionBlockTypePtr FunctionBlockType(const StringPtr& id, const StringPtr& name, const StringPtr& description, const PropertyObjectPtr& defaultConfig = PropertyObject())
-m [factory] ServerTypePtr ServerType(const StringPtr& id, const StringPtr& name, const StringPtr& description, const FunctionPtr& createDefaultConfigCallback = nullptr)
+m [factory] ServerTypePtr ServerType(const StringPtr& id, const StringPtr& name, const StringPtr& description, const PropertyObjectPtr& defaultConfig = PropertyObject())
``` 
# 25.04.2024

## Description
- Add mirrored device base implementation as a general approach to manage streaming sources for configuration enabled devices
``` 
+ [interface] IMirroredDevice
+ [function] IMirroredDevice::getStreamingSources(IList** streamingSources)

+ [interface] IMirroredDeviceConfig
+ [function] IMirroredDeviceConfig::addStreamingSource(IStreaming* streamingSource)
+ [function] IMirroredDeviceConfig::removeStreamingSource(IString* streamingConnectionString)
``` 
# 23.04.2024

## Description
- Adding addresses in ServerCapability
``` 
+ [function] IServerCapabilityConfig::addAddress(IString* address)
+ [function] IServerCapability::getAddresses(IList** addresses)
``` 
# 22.04.2024

## Description
- Fix reserved keyword clashes with Delphi bindings
``` 
- [function] IPermissionsBuilder::set(StringPtr groupId, PermissionMaskBuilderPtr permissions)
+ [function] IPermissionsBuilder::assign(StringPtr groupId, PermissionMaskBuilderPtr permissions)
``` 
