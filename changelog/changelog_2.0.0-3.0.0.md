# 2024-04-12

## Description

- Implement BlockReaderStatus

```diff
+ [interface] IBlockReaderStatus
+ [function] IBlockReaderStatus::getReadSamples(SizeT* readSamples)
+ [factory] BlockReaderStatusPtr BlockReaderStatus(const EventPacketPtr& packet = nullptr, Bool valid = true, SizeT readSamples = 0)
```

# 2024-03-29

## Description

- Grouping discovered deivces with IServerCapability
- Replacing IStreamingInfo with IServerCapability

```diff
- [interface] IStreamingInfo
- [function] IStreamingInfo::getPrimaryAddress(IString** address)
- [function] IStreamingInfo::getProtocolId(IString** protocolId)

- [interface] IStreamingInfoConfig
- [function] IStreamingInfoConfig::setPrimaryAddress(IString* address)
- [factory] StreamingInfoConfigPtr StreamingInfo(const StringPtr& protocolId)

+ [interface] IServerCapability
+ [function] IServerCapabilityConfig::getConnectionString(IString** connectionString)
+ [function] IServerCapabilityConfig::getConnectionStrings(IList** connectionStrings)
+ [function] IServerCapability::getProtocolName(IString** protocolName)
+ [function] IServerCapability::getProtocolType(IEnumeration** type)
+ [function] IServerCapability::getConnectionType(IString** type)
+ [function] IServerCapability::getCoreEventsEnabled(Bool* enabled)

+ [interface] IServerCapabilityConfig
+ [function] IServerCapabilityConfig::setConnectionString(IString* connectionString)
+ [function] IServerCapabilityConfig::addConnectionString(IString* connectionString)
+ [function] IServerCapabilityConfig::setProtocolName(IString* protocolName)
+ [function] IServerCapabilityConfig::setProtocolType(IString* type)
+ [function] IServerCapabilityConfig::setConnectionType(IString* type)
+ [function] IServerCapabilityConfig::setCoreEventsEnabled(Bool enabled)
+ [function] IServerCapabilityConfig::addProperty(IProperty* property)
+ [factory] ServerCapabilityConfigPtr ServerCapability(const StringPtr& protocolName, ProtocolType protocolType)

-m [function] IModule::acceptsStreamingConnectionParameters(Bool* accepted, IString* connectionString, IPropertyObject* config = nullptr)
+m [function] IModule::acceptsStreamingConnectionParameters(Bool* accepted, IString* connectionString, IPropertyObject* config = nullptr)
-m [function] IModule::createStreaming(IStreaming** streaming, IString* connectionString, IPropertyObject* config)
+m [function] IModule::createStreaming(IStreaming** streaming, IString* connectionString, IPropertyObject* config)

+ [function] IDeviceInfo::getServerCapabilities(IList** serverCapabilities)
+ [interface] IDeviceInfoInternal
+ [function] IDeviceInfoInternal::addServerCapability(IServerCapability* serverCapability)
+ [function] IDeviceInfoInternal::removeServerCapability(IString* protocolId)
+ [function] IDeviceInfoInternal::clearServerStreamingCapabilities()

- [function] IDevicePrivate::addStreamingOption(IStreamingInfo* info)
- [function] IDevicePrivate::removeStreamingOption(IString* protocolId)
- [function] IDevicePrivate::getStreamingOptions(IList** streamingOptions)

-m [function] IModule::acceptsStreamingConnectionParameters(Bool* accepted, IString* connectionString, IStreamingInfo* config = nullptr)
+m [function] IModule::acceptsStreamingConnectionParameters(Bool* accepted, IString* connectionString, IServerCapability* capability = nullptr)
-m [function] IModule::createStreaming(IStreaming** streaming, IString* connectionString, IStreamingInfo* config)
+m [function] IModule::createStreaming(IStreaming** streaming, IString* connectionString, IServerCapability* capability)
```

# 2024-03-26

## Description

- Implement multi reader builder

```diff
+ [interface] IMultiReaderBuilder
+ [interface] IMultiReaderBuilder::build(IMultiReader** multiReader)
+ [interface] IMultiReaderBuilder::addSignal(ISignal* signal)
+ [interface] IMultiReaderBuilder::addInputPort(IInputPort* port)
+ [interface] IMultiReaderBuilder::getSourceComponents(IList** ports)
+ [interface] IMultiReaderBuilder::setValueReadType(SampleType type)
+ [interface] IMultiReaderBuilder::getValueReadType(SampleType* type)
+ [interface] IMultiReaderBuilder::setDomainReadType(SampleType type)
+ [interface] IMultiReaderBuilder::getDomainReadType(SampleType* type)
+ [interface] IMultiReaderBuilder::setReadMode(ReadMode mode)
+ [interface] IMultiReaderBuilder::getReadMode(ReadMode* mode)
+ [interface] IMultiReaderBuilder::setReadTimeoutType(ReadTimeoutType type)
+ [interface] IMultiReaderBuilder::getReadTimeoutType(ReadTimeoutType* type)
+ [interface] IMultiReaderBuilder::setRequiredCommonSampleRate(Int sampleRate)
+ [interface] IMultiReaderBuilder::getRequiredCommonSampleRate(Int* sampleRate)
+ [interface] IMultiReaderBuilder::setStartOnFullUnitOfDomain(Bool enabled)
+ [interface] IMultiReaderBuilder::getStartOnFullUnitOfDomain(Bool* enabled)
+ [factory] MultiReaderBuilderPtr MultiReaderBuilder()

+ [factory] MultiReaderPtr MultiReaderFromBuilder(const MultiReaderBuilderPtr& builder)
```

# 2024-03-26

## Description

- Fix outdated data descriptor before subscription

```diff
-m [function] Bool IMirroredSignalPrivate::triggerEvent(const EventPacketPtr& eventPacket)
+m [function] ErrCode IMirroredSignalPrivate::triggerEvent(IEventPacket* eventPacket, Bool* forward)
-m [function] ErrCode IMirroredSignalPrivate::INTERFACE_FUNC addStreamingSource(const StreamingPtr& streaming)
+m [function] ErrCode IMirroredSignalPrivate::addStreamingSource(IStreaming* streaming)
-m [function] ErrCode IMirroredSignalPrivate::removeStreamingSource(const StringPtr& streamingConnectionString)
+m [function] ErrCode IMirroredSignalPrivate::removeStreamingSource(IString* streamingConnectionString)
-m [function] void IMirroredSignalPrivate::subscribeCompleted(const StringPtr& streamingConnectionString)
+m [function] ErrCode IMirroredSignalPrivate::subscribeCompleted(IString* streamingConnectionString)
-m [function] void IMirroredSignalPrivate::unsubscribeCompleted(const StringPtr& streamingConnectionString)
+m [function] ErrCode IMirroredSignalPrivate::unsubscribeCompleted(IString* streamingConnectionString)
+ [function] ErrCode IMirroredSignalPrivate::getMirroredDataDescriptor(IDataDescriptor** descriptor)
+ [function] ErrCode IMirroredSignalPrivate::setMirroredDataDescriptor(IDataDescriptor* descriptor)
+ [function] ErrCode IMirroredSignalPrivate::getMirroredDomainSignal(IMirroredSignalConfig** domainSignals)
+ [function] ErrCode IMirroredSignalPrivate::setMirroredDomainSignal(IMirroredSignalConfig* domainSignal)
```

# 2024-04-25

## Description

- Constant rule rework in openDAQ core and support for native streaming

```diff
-m [factory] DataRulePtr ConstantDataRule(const NumberPtr& value)
+m [factory] DataRulePtr ConstantDataRule()
+  [factory] template <class T> DataPacketPtr ConstantDataPacketWithDomain(const DataPacketPtr& domainPacket, const DataDescriptorPtr& descriptor, uint64_t sampleCount, T initialValue, const std::vector<ConstantPosAndValue<T>>& otherValues = {})
-m [factory] DataPacketPtr DataPacketWithExternalMemory(const DataPacketPtr& domainPacket, const DataDescriptorPtr& descriptor, uint64_t sampleCount, void* data, const DeleterPtr& deleter, NumberPtr offset = nullptr)
+m [factory] DataPacketPtr DataPacketWithExternalMemory(const DataPacketPtr& domainPacket, const DataDescriptorPtr& descriptor, uint64_t sampleCount, void* data, const DeleterPtr& deleter, NumberPtr offset = nullptr, SizeT bufferSize = std::numeric_limits<SizeT>::max())
```

# 2024-03-22

## Description

- Add support for multiple module search paths in ModuleManager

```diff
+ [factory] ModuleManagerPtr ModuleManagerMultiplePaths(const ListPtr<IString>& paths)

+ [function] IInstanceBuilder::addModulePath(IString* path)
+ [function] IInstanceBuilder::getModulePathsList(IList** paths)
```

# 2024-03-21

## Description

- Move create fb/device logic to module manager
- Move client device logic for adding fbs/devices to device_impl

```diff
+ [interface] IModuleManagerUtils
+ [function] IModuleManagerUtils::getAvailableDevices(IList** availableDevices)
+ [function] IModuleManagerUtils::getAvailableDeviceTypes(IDict** deviceTypes)
+ [function] IModuleManagerUtils::createDevice(IDevice** device, IString* connectionString, IComponent* parent, IPropertyObject* config = nullptr)
+ [function] IModuleManagerUtils::getAvailableFunctionBlockTypes(IDict** functionBlockTypes)
+ [function] IModuleManagerUtils::createFunctionBlock(IFunctionBlock** functionBlock, IString* id, IComponent* parent, IPropertyObject* config = nullptr, IString* localId = nullptr)
-m [factory] DevicePtr Client(const ContextPtr& context, const StringPtr& localId, const DeviceInfoPtr& defaultDeviceInfo = nullptr)
+m [factory] DevicePtr Client(const ContextPtr& context, const StringPtr& localId, const DeviceInfoPtr& defaultDeviceInfo = nullptr, const ComponentPtr& parent = nullptr)
```

# 2024-03-11

## Description

- Make DeviceDomain a standalone object in DeviceImpl
  - Devices no longer override the DeviceDomain getters, but instead set it via the protected `setDeviceDomain` method
  - Changing the DeviceDomain triggers a core event
- Move getTicksSinceOrigin to IDevice

```diff
+ [factory] DeviceDomainPtr DeviceDomain(const RatioPtr& tickResolution, const StringPtr& origin, const UnitPtr& unit)
- [function] IDeviceDomain::getTicksSinceOrigin(UInt* ticks)
+ [function] IDevice::getTicksSinceOrigin(UInt* ticks)
+ [factory] CoreEventArgsPtr CoreEventArgsDeviceDomainChanged(const DeviceDomainPtr& deviceDomain)
```

# 2024-03-11

## Description

- Update native transport protocol - initiate streaming only upon client request;
- New protocol message type added: PAYLOAD_TYPE_STREAMING_PROTOCOL_INIT_REQUEST = 11

# 2024-03-08

## Description

- Propagate server IUpdatable::Update to native protocol clients
- Properly handle nested property objects in native protocol

```diff
+ [function] IDeserializer::callCustomProc(IProcedure* customDeserialize, IString* serialized)
+ [function] ISerializedObject::isRoot(Bool* isRoot)
+ [function] IUpdatable::updateEnded()
```

# 2024-02-26

## Description

- Integrating config provider options in modules

```diff
+ [function] IModule::getId(IString** id)
```

# 2024-02-26

## Description

- Streaming framework refactoring

```diff
- [function] IMirroredSignalPrivate::assignDomainSignal(const SignalPtr& domainSignal);
- [function] IMirroredSignalPrivate::hasMatchingId(const StringPtr& signalId);

-m [function] IStreamingPrivate::subscribeSignal(const MirroredSignalConfigPtr& signal);
+m [function] IStreamingPrivate::subscribeSignal(const StringPtr& signalRemoteId, const StringPtr& domainSignalRemoteId);

-m [function] IStreamingPrivate::unsubscribeSignal(const MirroredSignalConfigPtr& signal);
+m [function] IStreamingPrivate::unsubscribeSignal(const StringPtr& signalRemoteId, const StringPtr& domainSignalRemoteId);

-m [function] IStreamingPrivate::createDataDescriptorChangedEventPacket(const MirroredSignalConfigPtr& signal);
+m [function] IStreamingPrivate::createDataDescriptorChangedEventPacket(const StringPtr& signalRemoteId);

+ [function] IStreamingPrivate::detachRemovedSignal(const StringPtr& signalRemoteId);
```

# 2024-02-25

## Description

- Readers return IReaderStatus
- Support creating readers with IInputPort
- Add callback on available packet for reader

```diff
+ [interface] IReaderStatus : public IBaseObject
+ [function] IReaderStatus::getReadStatus(ReadStatus* status)
+ [function] IReaderStatus::getEventPacket(IEventPacket** packet)
+ [function] IReaderStatus::getValid(Bool* valid)
+ [factory] ReaderStatusPtr ReaderStatus(const EventPacketPtr& packet = nullptr, Bool convertable = true)

-m [function] IBlockReader::read(void* blocks, SizeT* count, SizeT timeoutMs = 0)
+m [function] IBlockReader::read(void* blocks, SizeT* count, SizeT timeoutMs = 0, IReaderStatus** status = nullptr)
-m [function] IBlockReader::readWithDomain(void* dataBlocks, void* domainBlocks, SizeT* count, SizeT timeoutMs = 0)
+m [function] IBlockReader::readWithDomain(void* dataBlocks, void* domainBlocks, SizeT* count, SizeT timeoutMs = 0, IReaderStatus** status = nullptr)

-m [function] IMultiReader::read(void* samples, SizeT* count, SizeT timeoutMs = 0)
+m [function] IMultiReader::read(void* samples, SizeT* count, SizeT timeoutMs = 0, IReaderStatus** status = nullptr)
-m [function] IMultiReader::readWithDomain(void* samples, void* domain, SizeT* count, SizeT timeoutMs = 0)
+m [function] IMultiReader::readWithDomain(void* samples, void* domain, SizeT* count, SizeT timeoutMs = 0, IReaderStatus** status = nullptr)

-m [function] IStreamReader::read(void* samples, SizeT* count, SizeT timeoutMs = 0)
+m [function] IStreamReader::read(void* samples, SizeT* count, SizeT timeoutMs = 0, IReaderStatus** status = nullptr)
-m [function] IStreamReader::readWithDomain(void* samples, void* domain, SizeT* count, SizeT timeoutMs = 0)
+m [function] IStreamReader::readWithDomain(void* samples, void* domain, SizeT* count, SizeT timeoutMs = 0, IReaderStatus** status = nullptr)

-m [function] IStreamReader::read(void* values, SizeT* count)
+m [function] IStreamReader::read(void* values, SizeT* count, IReaderStatus** status = nullptr)
-m [function] IStreamReader::readWithDomain(void* values, void* domain, SizeT* count)
+m [function] IStreamReader::readWithDomain(void* values, void* domain, SizeT* count, IReaderStatus** status = nullptr)

+ [factory] PacketReaderPtr PacketReaderFromPort(InputPortConfigPtr port)
+ [factory] StreamReaderPtr StreamReaderFromPort(InputPortConfigPtr port, SampleType valueReadType, SampleType domainReadType, ReadMode mode, ReadTimeoutType timeoutType)
+ [factory] TailReaderPtr TailReaderFromPort(InputPortConfigPtr port, SizeT historySize, SampleType valueReadType, SampleType domainReadType, ReadMode mode)
+ [factory] BlockReaderPtr BlockReaderFromPort(InputPortConfigPtr port, SizeT blockSize, SampleType valueReadType, SampleType domainReadType, ReadMode mode)
+ [factory] MultiReaderPtr MultiReaderFromPort(ListPtr<IInputPortConfig> ports, SampleType valueReadType, SampleType domainReadType, ReadMode mode = ReadMode::Scaled, ReadTimeoutType timeoutType)

+ [function] IReader::setOnDataAvailable(IProcedure* callback)
- [function] IReader::setOnDescriptorChanged(IFunction* callback)
- [function] IReaderConfig::getOnDescriptorChanged(IFunction** callback)

+ [function] MultiReaderPtr::setOnDescriptorChanged(IFunction* callback)
+ [function] MultiReaderPtr::getOnDescriptorChanged(IFunction** callback)

- [factory] inline BlockReaderPtr BlockReaderFromExisting(const BlockReaderPtr& invalidatedReader, SampleType valueReadType, SampleType domainReadType)
+ [factory] inline BlockReaderPtr BlockReaderFromExisting(const BlockReaderPtr& invalidatedReader, SizeT blockSize, SampleType valueReadType, SampleType domainReadType)
```

# 2024-02-25

## Description

- rename search filter search::SearchId to search::InterfaceId
- add local ID search filter

```diff
-m [factory] SearchFilterPtr SearchId(IntfID intfId)
+m [factory] SearchFilterPtr InterfaceId(const IntfID& intfId)
+ [factory] SearchFilterPtr LocalId(const StringPtr& localId)
```

# 2024-02-12

## Description

- Add module implementations for device based on the native config protocol

```diff
+ [function] IMirroredSignalPrivate::assignDomainSignal(const SignalPtr& domainSignal);
```

# 2024-02-06

## Description

- Support configuring instance builder from config provider

```diff
-m [factory] ContextPtr Context(const SchedulerPtr& scheduler, const LoggerPtr& logger, const TypeManagerPtr& typeManager, const ModuleManagerPtr& moduleManager)
+m [factory] ContextPtr Context(const SchedulerPtr& scheduler, const LoggerPtr& logger, const TypeManagerPtr& typeManager, const ModuleManagerPtr& moduleManager, const DictPtr<IString, IBaseObject> options)

+ [function] IInstanceBuilder::addConfigProvider(IConfigProvider* configProvider)
+ [function] IInstanceBuilder::getOptions(IDict** options)

+ [interface] IConfigProvider: public IBaseObject
+ [function] IConfigProvider::populateOptions(IDict* options)

+ [factory] ConfigProviderPtr JsonConfigProvider(const StringPtr& filename)
+ [factory] ConfigProviderPtr EnvConfigProvider()
+ [factory] ConfigProviderPtr CmdLineArgsConfigProvider(const ListPtr<IString>& args)
```

# 2024-02-05

## Description

- Add core event support to config client

```diff
-m [function] IInstance::findComponent(IComponent* component, IString* id, IComponent** outComponent)
+m [function] IComponent::findComponent(IString* id, IComponent** outComponent)
+ [function] ITags::set(IList* tags) = 0;
```

# 2024-02-05

## Description

- Add Component status implementations
- Add method getIntValue for Enumeration object

```diff
+ [function] IEnumeration::getIntValue(Int* value)

+ [function] IComponent::getStatusContainer(IComponentStatus** statusContainer)

+ [interface] IComponentStatusContainerPrivate : public IBaseObject
+ [function] IComponentStatusContainerPrivate::addStatus(IString* name, IEnumeration* initialValue)
+ [function] IComponentStatusContainerPrivate::setStatus(IString* name, IEnumeration* value)

+ [interface] IComponentStatusContainer : public IBaseObject
+ [function] IComponentStatusContainer::getStatus(IString* name, IEnumeration** value)
+ [function] IComponentStatusContainer::getStatuses(IDict** statuses)
+ [function] IComponentStatusContainer::getOnStatusChanged(IEvent** event)
+ [factory] ComponentStatusContainerPtr ComponentStatusContainer()
```

# 2024-01-23

## Description

- Add debug logger sink for testing purpose

```diff
+ [interface] ILastMessageLoggerSinkPrivate : public IBaseObject
+ [function] ILastMessageLoggerSinkPrivate::getLastMessage(IString** lastMessage)
+ [function] ILastMessageLoggerSinkPrivate::waitForMessage(SizeT timeoutMs, Bool* success)
+ [factory] LoggerSinkPtr LastMessageLoggerSink()
```

# 2024-02-01

```diff
+m [function] IMultiReader::skipSamples
```

# 2024-01-23

## Description

- Change ITagsConfig to ITagsPrivate and remove inheritance
- Add TagsChanged core event
- Make Tags changeable after creation (Tags are no longer freezable)

```diff
-m [interface] ITagsConfig : public ITags
+m [interface] ITagsPrivate : public IBaseObject
- [factory] inline TagsConfigPtr TagsCopy(TagsPtr tags)
-m [factory] inline TagsConfigPtr Tags()
+m [factory] inline TagsPtr Tags()
-m [function] IComponent::getTags(ITagsConfig** tags)
-m [function] IComponent::getTags(ITags** tags)
```

# 2024-01-22

## Description

- Support for external deserialization factory

```diff
-m [function] IDeserializer::deserialize(IString* serialized, IBaseObject* context, IBaseObject** object)
+m [function] IDeserializer::deserialize(IString* serialized, IBaseObject* context, IFunction* factoryCallback, IBaseObject** object)
-m [function] ISerializedObject::readList(IString* key, IBaseObject* context, IList** list)
+m [function] ISerializedObject::readList(IString* key, IBaseObject* context, IFunction* factoryCallback, IList** list)
-m [function] ISerializedObject::readObject(IString* key, IBaseObject* context, IBaseObject** obj)
+m [function] ISerializedObject::readObject(IString* key, IBaseObject* context, IFunction* factoryCallback, IBaseObject** obj)
-m [function] ISerializedList::readObject(IBaseObject* context, IBaseObject** obj)
+m [function] ISerializedList::readObject(IBaseObject* context, IFunction* factoryCallback, IBaseObject** obj)
-m [function] ISerializedList::readList(IBaseObject* context, IBaseObject** obj)
+m [function] ISerializedList::readList(IBaseObject* context, IFunction* factoryCallback, IBaseObject** obj)
+  [function] IUpdatable::serializeForUpdate(ISerializer* serializer)
+  [interface] IComponentDeserializeContext: public IBaseObject
+  [interface] IDeserializeComponent: public IBaseObject
```

# 2024-01-16

## Description

- Nested object-type property enhancement
  - Object-type property default values are now frozen when added to Property objects
  - Said default values are cloned as local values that can be modified by users
  - Property object core events now contain the path to the property if the property is contained within a nested property object

```diff
+ [function] IPropertyObjectInternal::clone(IPropertyObject** cloned)
+ [function] IPropertyObjectInternal::setPath(IString* path)
-m [factory] CoreEventArgsPtr CoreEventArgsPropertyValueChanged(const PropertyObjectPtr& propOwner, const StringPtr& propName, const BaseObjectPtr& value)
+m [factory] CoreEventArgsPtr CoreEventArgsPropertyValueChanged(const PropertyObjectPtr& propOwner, const StringPtr& propName, const BaseObjectPtr& value, const StringPtr& path)
-m [factory] CoreEventArgsPtr CoreEventArgsPropertyObjectUpdateEnd(const PropertyObjectPtr& propOwner, const DictPtr<IString, IBaseObject>& updatedProperties)
+m [factory] CoreEventArgsPtr CoreEventArgsPropertyObjectUpdateEnd(const PropertyObjectPtr& propOwner, const DictPtr<IString, IBaseObject>& updatedProperties, const StringPtr& path)
-m [factory] CoreEventArgsPtr CoreEventArgsPropertyAdded(const PropertyObjectPtr& propOwner, const PropertyPtr& prop)
+m [factory] CoreEventArgsPtr CoreEventArgsPropertyAdded(const PropertyObjectPtr& propOwner, const PropertyPtr& prop, const StringPtr& path)
-m [factory] CoreEventArgsPtr CoreEventArgsPropertyRemoved(const PropertyObjectPtr& propOwner, const StringPtr& propName)
+m [factory] CoreEventArgsPtr CoreEventArgsPropertyRemoved(const PropertyObjectPtr& propOwner, const StringPtr& propName, const StringPtr& path)
```

# 2024-01-12

## Description

- Search filters, visible flag, component attributes
  - Add SearchFilter that allows for more granular component search on tree traversal methods (getItems, getSignals...)
    - Add visible flag to Component
    - Default component getters were modified to return only components with visible==true
    - PropertyChanged event packets were removed
    - ComponentModified core event was changed to AttributeModified
    - Attribute lock was added to components preventing changes to locked attributes
    - Name and Description are no longer properties, but component attributes
    - Add per-component core event triggers

```diff
+ [interface] ISearchFilter : public IBaseObject
+ [function] ISearchFilter::acceptsComponent(IComponent* component, Bool* accepts)
+ [function] ISearchFilter::visitChildren(IComponent* component, Bool* visit)
+ [factory] SearchFilterPtr search::Visible()
+ [factory] SearchFilterPtr search::RequireTags(const ListPtr<IString>& requiredTags)
+ [factory] SearchFilterPtr search::ExcludeTags(const ListPtr<IString>& excludedTags)
+ [factory] SearchFilterPtr search::SearchId(const IntfID searchId)
+ [factory] SearchFilterPtr search::Any()
+ [factory] SearchFilterPtr search::And(const SearchFilterPtr& left, const SearchFilterPtr& right)
+ [factory] SearchFilterPtr search::Or(const SearchFilterPtr& left, const SearchFilterPtr& right)
+ [factory] SearchFilterPtr search::Not(const SearchFilterPtr& filter)
+ [factory] SearchFilterPtr search::Custom(const FunctionPtr& acceptsFunction, const FunctionPtr& visitFunction)
+ [factory] SearchFilterPtr search::Recursive(const SearchFilterPtr& filter)

+ [interface] IRecursiveSearch : public IBaseObject

+ [function] IComponent::getVisible(Bool* visible)
+ [function] IComponent::setVisible(Bool visible)
+ [function] IComponent::getLockedAttributes(IList** attributes)
+ [function] IComponent::getOnComponentCoreEvent(IEvent** event)
-m [factory] ComponentPtr Component(const ContextPtr& context, const ComponentPtr& parent, const StringPtr& localId, ComponentStandardProps propertyMode = ComponentStandardProps::Add)
+m [factory] ComponentPtr Component(const ContextPtr& context, const ComponentPtr& parent, const StringPtr& localId)

+ [interface] IComponentPrivate : public IBaseObject
+ [function] IComponentPrivate::lockAttributes(IList* attributes)
+ [function] IComponentPrivate::lockAllAttributes()
+ [function] IComponentPrivate::unlockAttributes(IList* attributes)
+ [function] IComponentPrivate::unlockAllAttributes()
+ [function] IComponentPrivate::triggerComponentCoreEvent(ICoreEventArgs* args)

-m [function] IFolder::getItems(IList** items)
+m [function] IFolder::getItems(IList** items, ISearchFilter* searchFilter = nullptr)
-m [factory] FolderConfigPtr Folder(const ContextPtr& context, const ComponentPtr& parent, const StringPtr& localId, const ComponentStandardProps propertyMode = ComponentStandardProps::Add)
+m [factory] FolderConfigPtr Folder(const ContextPtr& context, const ComponentPtr& parent, const StringPtr& localId)

-m [factory] IoFolderConfigPtr IoFolder(const ContextPtr& context, const ComponentPtr& parent, const StringPtr& localId, const ComponentStandardProps propertyMode = ComponentStandardProps::Add)
+m [factory] IoFolderConfigPtr IoFolder(const ContextPtr& context, const ComponentPtr& parent, const StringPtr& localId)

-m [function] IFunctionBlock::getFunctionBlocks(IList** functionBlocks)
+m [function] IFunctionBlock::getFunctionBlocks(IList** functionBlocks, ISearchFilter* searchFilter = nullptr)
-m [function] IFunctionBlock::getInputPorts(IList** ports)
+m [function] IFunctionBlock::getInputPorts(IList** ports, ISearchFilter* searchFilter = nullptr)
-m [function] IFunctionBlock::getSignals(IList** signals)
+m [function] IFunctionBlock::getSignals(IList** signals, ISearchFilter* searchFilter = nullptr)
-m [function] IFunctionBlock::getSignalsRecursive(IList** signals)
+m [function] IFunctionBlock::getSignalsRecursive(IList** signals, ISearchFilter* searchFilter = nullptr)

-m [function] IDevice::getFunctionBlocks(IList** functionBlocks)
+m [function] IDevice::getFunctionBlocks(IList** functionBlocks, ISearchFilter* searchFilter = nullptr)
-m [function] IDevice::getChannels(IList** channels)
+m [function] IDevice::getChannels(IList** channels, ISearchFilter* searchFilter = nullptr)
-m [function] IDevice::getChannelsRecursive(IList** channels)
+m [function] IDevice::getChannelsRecursive(IList** channels, ISearchFilter* searchFilter = nullptr)
-m [function] IDevice::getSignals(IList** signals)
+m [function] IDevice::getSignals(IList** signals, ISearchFilter* searchFilter = nullptr)
-m [function] IDevice::getSignalsRecursive(IList** signals)
+m [function] IDevice::getSignalsRecursive(IList** signals, ISearchFilter* searchFilter = nullptr)
-m [function] IDevice::getDevices(IList** devices)
+m [function] IDevice::getDevices(IList** devices, ISearchFilter* searchFilter = nullptr)

- [factory] EventPacketPtr PropertyChangedEventPacket(const StringPtr& name, const BaseObjectPtr& value)

-m [factory] CoreEventArgsPtr CoreEventArgsComponentModified(const DictPtr<IString, IBaseObject>& modifiedAttributes)
+m [factory] CoreEventArgsPtr CoreEventArgsAttributeChanged(const StringPtr& attributeName, const BaseObjectPtr& attributeValue)
```

# 2024-01-11

## Description

- Add Enumeration Core type implementations

```diff
+ [interface] IEnumerationType : public IType
+ [function] IEnumerationType::getEnumeratorNames(IList** names)
+ [function] IEnumerationType::getAsDictionary(IDict** dictionary)
+ [function] IEnumerationType::getEnumeratorIntValue(IString* name, Int* value)
+ [function] IEnumerationType::getCount(SizeT* count)
+ [factory] EnumerationTypePtr EnumerationType(const StringPtr& typeName, const ListPtr<IString>& enumeratorNames, const Int firstEnumeratorIntValue)
+ [factory] EnumerationTypePtr EnumerationTypeWithValues(const StringPtr& typeName, const DictPtr<IString, IInteger>& enumerators)

+ [interface] IEnumeration : public IBaseObject
+ [function] IEnumeration::getEnumerationType(IEnumerationType** type)
+ [function] IEnumeration::getValue(IString** value)
+ [factory] EnumerationPtr Enumeration(const StringPtr& typeName, const StringPtr& value, const TypeManagerPtr& typeManager)
```

# 2024-01-03

## Description

- Add method getLastValue in ISignal

```diff
+ [function] ISignal::getLastValue(IBaseObject** value)
+ [function] ISignalPrivate::enableKeepLastValue(Bool enabled)
```

# 2023-12-18

## Description

- Builder pattern implementation for IInstance

```diff
+ [interface] IInstanceBuilder : public IBaseObject
+ [function] IInstanceBuilder::build(IInstance** instance)
+ [function] IInstanceBuilder::setLogger(ILogger* logger)
+ [function] IInstanceBuilder::setLogger(ILogger* logger)
+ [function] IInstanceBuilder::getLogger(ILogger** logger)
+ [function] IInstanceBuilder::setGlobalLogLevel(LogLevel logLevel)
+ [function] IInstanceBuilder::getGlobalLogLevel(LogLevel* logLevel)
+ [function] IInstanceBuilder::setComponentLogLevel(IString* component, LogLevel logLevel)
+ [function] IInstanceBuilder::setSinkLogLevel(ILoggerSink* sink, LogLevel logLevel)
+ [function] IInstanceBuilder::setModulePath(IString* path)
+ [function] IInstanceBuilder::setModuleManager(IModuleManager* moduleManager)
+ [function] IInstanceBuilder::getModuleManager(IModuleManager** moduleManager)
+ [function] IInstanceBuilder::setSchedulerWorkerNum(SizeT numWorkers)
+ [function] IInstanceBuilder::setScheduler(IScheduler* scheduler)
+ [function] IInstanceBuilder::getScheduler(IScheduler** scheduler)
+ [function] IInstanceBuilder::setOption(IString* option, IBaseObject* value)
+ [function] IInstanceBuilder::getOptions(IDict** options)
+ [function] IInstanceBuilder::setRootDevice(IDevice* rootDevice)
+ [function] IInstanceBuilder::getRootDevice(IDevice** rootDevice)
+ [function] IInstanceBuilder::setDefaultRootDeviceLocalId(IString* localId)
+ [function] IInstanceBuilder::getDefaultRootDeviceLocalId(IString** localId)
+ [function] IInstanceBuilder::setDefaultRootDeviceInfo(IDeviceInfo* deviceInfo)
+ [function] IInstanceBuilder::getDefaultRootDeviceInfo(IDeviceInfo** deviceInfo)
+ [factory] InstanceBuilderPtr InstanceBuilder()

-m [factory] DevicePtr Client(const ContextPtr& context, const StringPtr& localId)
+m [factory] DevicePtr Client(const ContextPtr& context, const StringPtr& localId, const DeviceInfoPtr& defaultDeviceInfo)
```

# 2023-12-13

## Description

- Add Core event to Context object that triggers on core changes
- Add automatic triggers for Property Object changes
  - Add/Remove property, Value changed, Update ended

```diff
+ [interface] ICoreEventArgs : public IEventArgs
+ [function] ICoreEventArgs::getParameters(IDict** parameters)
+ [factory] CoreEventArgsPtr CoreEventArgs(Int id, const DictPtr<IString, IBaseObject>& parameters)
+ [factory] CoreEventArgsPtr CoreEventArgsPropertyValueChanged(const StringPtr& propName, const BaseObjectPtr& value)
+ [factory] CoreEventArgsPtr CoreEventArgsUpdateEnd(const DictPtr<IString, IBaseObject>& updatedProperties)
+ [factory] CoreEventArgsPtr CoreEventArgsPropertyAdded(const PropertyPtr& prop)
+ [factory] CoreEventArgsPtr CoreEventArgsPropertyRemoved(const StringPtr& propName)

+ [function] IContext::getOnCoreEvent(IEvent** event)
```

# 2023-12-07

## Description

- Add Struct Builder for generic struct creation

```diff
+ [interface] IStructBuilder
+ [function] IStructBuilder::build(IStruct** struct_)
+ [function] IStructBuilder::getStructType(IStructType** type)
+ [function] IStructBuilder::getFieldNames(IList** names)
+ [function] IStructBuilder::setFieldValues(IList* values)
+ [function] IStructBuilder::getFieldValues(IList** values)
+ [function] IStructBuilder::set(IString* name, IBaseObject* field)
+ [function] IStructBuilder::get(IString* name, IBaseObject** field)
+ [function] IStructBuilder::hasField(IString* name, Bool* contains)
+ [function] IStructBuilder::getAsDictionary(IDict** dictionary)
+ [factory] StructPtr StructFromBuilder(const StructBuilderPtr& builder)
+ [factory] StructBuilderPtr StructBuilder(const StringPtr& name, const TypeManagerPtr& typeManager)
+ [factory] StructBuilderPtr StructBuilder(const StructPtr& struct_)
```

# 2023-12-07

## Description

- Add subscribe/unsubscribe completion acknowledgement Events

```diff
+ [function] IMirroredSignalConfig::getOnSubscribeComplete(IEvent** event)
+ [function] IMirroredSignalConfig::getOnUnsubscribeComplete(IEvent** event)

+ [function] IMirroredSignalPrivate::subscribeCompleted(const StringPtr& streamingConnectionString)
+ [function] IMirroredSignalPrivate::unsubscribeCompleted(const StringPtr& streamingConnectionString)

+ [interface] ISubscriptionEventArgs : IEventArgs
+ [function] ISubscriptionEventArgs::getStreamingConnectionString(IString** streamingConnectionString)
+ [function] ISubscriptionEventArgs::getSubscriptionEventType(SubscriptionEventType* type)

+ [factory] SubscriptionEventArgsPtr SubscriptionEventArgs(const StringPtr& streamingConnectionString, SubscriptionEventType eventType)
```

# 2023-12-05

## Description

- Expose Origin Epoch and StartOffset the MultiReader aligned all the read signals to
- Rework how interface inheritance and queryInterface work to hopefully prevent ICEs and out-of-heap-space errors

```diff
+ [function] IMultiReader::getTickResolution(IRatio** resolution)
+ [function] IMultiReader::getOrigin(IString** origin)
+ [function] IMultiReader::getOffset(void* domainStart)
```

# 2023-11-28

## Description

- Builder pattern improvement
  - Expand builder classes with getters
  - Support creating objects from builder

```diff
+ [function] IPropertyBuilder::getValueType(CoreType* type)
+ [function] IPropertyBuilder::getName(IString** name)
+ [function] IPropertyBuilder::getDescription(IString** description)
+ [function] IPropertyBuilder::getUnit(IUnit** unit)
+ [function] IPropertyBuilder::getMinValue(INumber** min)
+ [function] IPropertyBuilder::getMaxValue(INumber** max)
+ [function] IPropertyBuilder::getDefaultValue(IBaseObject** value)
+ [function] IPropertyBuilder::getSuggestedValues(IList** values)
+ [function] IPropertyBuilder::getVisible(IBoolean** visible)
+ [function] IPropertyBuilder::getReadOnly(IBoolean** readOnly)
+ [function] IPropertyBuilder::getSelectionValues(IBaseObject** values)
+ [function] IPropertyBuilder::getReferencedProperty(IEvalValue** propertyEval)
+ [function] IPropertyBuilder::getValidator(IValidator** validator)
+ [function] IPropertyBuilder::getCoercer(ICoercer** coercer)
+ [function] IPropertyBuilder::getCallableInfo(ICallableInfo** callable)
+ [function] IPropertyBuilder::getOnPropertyValueWrite(IEvent** event)
+ [function] IPropertyBuilder::getOnPropertyValueRead(IEvent** event)
- [factory] PropertyPtr PropertyFromBuildParams(const DictPtr<IString, IBaseObject>& buildParams)
+ [factory] PropertyPtr PropertyFromBuilder(const PropertyBuilderPtr& builder)

+ [function] IPropertyObjectClassBuilder::getName(IString** className)
+ [function] IPropertyObjectClassBuilder::getParentName(IString** parentName)
+ [function] IPropertyObjectClassBuilder::getProperties(IDict** properties)
+ [function] IPropertyObjectClassBuilder::getPropertyOrder(IList** orderedPropertyNames)
+ [function] IPropertyObjectClassBuilder::getManager(ITypeManager** manager)
- [factory] PropertyObjectClassPtr PropertyObjectClassFromBuildParams(const DictPtr<IString, IBaseObject>& buildParams);
+ [factory] PropertyObjectClassPtr PropertyObjectClassFromBuilder(const PropertyObjectClassBuilderPtr& builder)

+ [function] IUnitBuilder::getId(Int* id)
+ [function] IUnitBuilder::getSymbol(IString** symbol)
+ [function] IUnitBuilder::getName(IString** name)
+ [function] IUnitBuilder::getQuantity(IString** quantity)
+ [factory] UnitPtr UnitFromBuilder(const UnitBuilderPtr& builder)

+ [function] IDataDescriptorBuilder::getName(IString** name)
+ [function] IDataDescriptorBuilder::getDimensions(IList** dimensions)
+ [function] IDataDescriptorBuilder::getSampleType(SampleType* sampleType)
+ [function] IDataDescriptorBuilder::getUnit(IUnit** unit)
+ [function] IDataDescriptorBuilder::getValueRange(IRange** range)
+ [function] IDataDescriptorBuilder::getRule(IDataRule** rule)
+ [function] IDataDescriptorBuilder::getOrigin(IString** origin)
+ [function] IDataDescriptorBuilder::getTickResolution(IRatio** tickResolution)
+ [function] IDataDescriptorBuilder::getPostScaling(IScaling** scaling)
+ [function] IDataDescriptorBuilder::getMetadata(IDict** metadata)
+ [function] IDataDescriptorBuilder::getStructFields(IList** structFields)
- [factory] DataDescriptorPtr DataDescriptor(const DictPtr<IString, IBaseObject>& descriptorParams)
+ [factory] DataDescriptorPtr DataDescriptorFromBuilder(const DataDescriptorBuilderPtr& builder)

+ [function] IDataRuleBuilder::getType(DataRuleType* type)
+ [function] IDataRuleBuilder::getParameters(IDict** parameters)
+ [factory] DataRulePtr DataRuleFromBuilder(const DataRuleBuilderPtr& builder)

+ [function] IDimensionBuilder::getName(IString** name)
+ [function] IDimensionBuilder::getUnit(IUnit** unit)
+ [function] IDimensionBuilder::getRule(IDimensionRule** rule)
+ [factory] DimensionPtr DimensionFromBuilder(const DimensionBuilderPtr& builder)

+ [function] IDimensionRuleBuilder::getType(DimensionRuleType* type)
+ [function] IDimensionRuleBuilder::getParameters(IDict** parameters)
+ [factory] DimensionRulePtr DimensionRuleFromBuilder(const DimensionRuleBuilderPtr& builder)

+ [function] IScalingBuilder::getInputDataType(SampleType* type)
+ [function] IScalingBuilder::getOutputDataType(ScaledSampleType* type)
+ [function] IScalingBuilder::getScalingType(ScalingType* type)
+ [function] IScalingBuilder::getParameters(IDict** parameters)
+ [factory] ScalingPtr ScalingFromBuilder(const ScalingBuilderPtr& builder)
```

# 2023-11-23

## Description

- setDescription, getDescription, and setName were moved to IComponent.
- OPC UA now uses DisplayName to set/get component names, and node Description to get/set component descriptions

```diff
+ [function] IComponent::setName(IString* name)
+ [function] IComponent::getDescription(IString** description)
+ [function] IComponent::setDescription(IString* description)

- [function] ISignal::setName(IString* name)
- [function] ISignal::getDescription(IString** description)
- [function] ISignal::setDescription(IString* description)

- [function] IFolderConfig::setName(IString* name)

-m [factory] ComponentPtr Component(const ContextPtr& context, const ComponentPtr& parent, const StringPtr& localId)
+m [factory] ComponentPtr Component(const ContextPtr& context, const ComponentPtr& parent, const StringPtr& localId, ComponentStandardProps propertyMode = ComponentStandardProps::Add)
-m [factory] FolderConfigPtr Folder(const ContextPtr& context, const ComponentPtr& parent, const StringPtr& localId)
+m [factory] FolderConfigPtr Folder(const ContextPtr& context, const ComponentPtr& parent, const StringPtr& localId, const ComponentStandardProps propertyMode = ComponentStandardProps::Add)
-m [factory] inline IoFolderConfigPtr IoFolder(const ContextPtr& context, const ComponentPtr& parent, const StringPtr& localId)
+m [factory] inline IoFolderConfigPtr IoFolder(const ContextPtr& context, const ComponentPtr& parent, const StringPtr& localId, const ComponentStandardProps propertyMode = ComponentStandardProps::Add)
```

# 2023-11-17

## Description

- Streaming framework changes:
  - ISignalRemote renamed to IMirroredSignalConfig
  - IMirroredSignalConfig inherits ISignalConfig
  - Streaming source management methods moved from ISignalConfig to IMirroredSignalConfig

```diff
- [function] ISignalConfig::getStreamingSources(IList** streamingConnectionStrings)
- [function] ISignalConfig::setActiveStreamingSource(IString* streamingConnectionString)
- [function] ISignalConfig::getActiveStreamingSource(IString** streamingConnectionString)
- [function] ISignalConfig::deactivateStreaming()

- [function] ISignalRemote::getRemoteId(IString** id)
- [function] ISignalRemote::triggerEvent(IEventPacket* eventPacket, Bool* forwardEvent)
- [function] ISignalRemote::addStreamingSource(IStreaming* streaming)
- [function] ISignalRemote::removeStreamingSource(IStreaming* streaming)

-m [interface] ISignalRemote : IBaseObject
+m [interface] IMirroredSignalConfig : ISignalConfig

+ [function] IMirroredSignalConfig::getRemoteId(IString** id)
+ [function] IMirroredSignalConfig::getStreamingSources(IList** streamingConnectionStrings)
+ [function] IMirroredSignalConfig::setActiveStreamingSource(IString* streamingConnectionString)
+ [function] IMirroredSignalConfig::getActiveStreamingSource(IString** streamingConnectionString)
+ [function] IMirroredSignalConfig::deactivateStreaming()

+ [interface] IMirroredSignalPrivate : IBaseObject
+ [function] Bool IMirroredSignalPrivate::triggerEvent(const EventPacketPtr& eventPacket)
+ [function] IMirroredSignalPrivate::addStreamingSource(const StreamingPtr& streaming)
+ [function] IMirroredSignalPrivate::removeStreamingSource(const StringPtr& streamingConnectionString)

+ [interface] IStreamingPrivate : IBaseObject
+ [function] IStreamingPrivate::subscribeSignal(const MirroredSignalConfigPtr& signal)
+ [function] IStreamingPrivate::unsubscribeSignal(const MirroredSignalConfigPtr& signal)
+ [function] EventPacketPtr IStreamingPrivate::createDataDescriptorChangedEventPacket(const MirroredSignalConfigPtr& signal)

+ [function] ISignal::getStreamed(Bool* streamed)
+ [function] ISignal::setStreamed(Bool streamed)
```
