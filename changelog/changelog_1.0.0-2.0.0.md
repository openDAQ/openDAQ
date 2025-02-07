# Multi-reader Support for Reading Raw Signal Values

## Description

Added multi-reader support for reading raw signal values (no scaling or conversion).

## API changes

```diff
-m enum class ReadMode { Raw, Scaled }
+m enum class ReadMode { Unscaled, Scaled, RawValue }
```

---

# New IPropertyInternal Method

## Description

Added a new method to `IPropertyInternal`.

## API changes

```diff
+m  [function] IPropertyInternal::getValueTypeUnresolved(CoreType* coreType)
```

---

# Removed ConfigurationMode from SDK

## Description

Removed `ConfigurationMode` from the SDK as it was no longer used.

## API changes

```diff
-m  [function] IDeserializer::update(IUpdatable* updatable, ConfigurationMode mode, IString* serialized)
+m  [function] IDeserializer::update(IUpdatable* updatable, IString* serialized)
-m  [function] IUpdatable::update(ConfigurationMode mode, ISerializedObject* update)
+m  [function] IUpdatable::update(ISerializedObject* update)
```

---

# openDAQ 2.0.0 – Large Implementation with Struct Core Type

_(openDAQ Package version: 2.0.0)_

## Description

Major changes implementing the Struct core type and enabling object creation through the builder pattern.

**Main highlights**:

- Struct core type implementation, with `StructType` objects to facilitate creation.
- A new `TypeManager` was introduced, replacing the old property object class manager.
- Property object classes now inherit `IType` and are registered with the Type Manager.
- Several objects implement the `IStruct` interface (`Unit`, `ArgumentInfo`, `DataDescriptor`, etc.).
- Moved from “freezable” config objects to a builder pattern (builder objects with `build()` methods).
- Builder objects have a `RTGen` flag `returnSelf` (currently C++ only).
- Added OPC UA implementation for generic struct transfer (currently supports known Struct types only).

## API changes

```diff
+ [factory] StructTypePtr ComplexNumberStructType()

-m [function] IRatio::simplify()
+m [function] IRatio::simplify(IRatio** simplifiedRatio)
+ [factory] StructTypePtr RatioStructType()

+ [interface] ISimpleType : public IType
+ [factory] ObjectPtr<ISimpleType> SimpleType(CoreType coreType)

+ [interface] IStruct : public IBaseObject
+ [function] IStruct::getStructType(IStructType** type)
+ [function] IStruct::getFieldNames(IList** names)
+ [function] IStruct::getFieldValues(IList** values)
+ [function] IStruct::get(IString* name, IBaseObject** field)
+ [function] IStruct::getAsDictionary(IDict** dictionary)
+ [function] IStruct::hasField(IString* name, Bool* contains)
+ [factory] StructPtr Struct(const StringPtr& name, const DictPtr<IString, IBaseObject>& fields, const TypeManagerPtr& typeManager)

+ [interface] IStructType : public IType
+ [function] IStructType::getFieldNames(IList** names)
+ [function] IStructType::getFieldDefaultValues(IList** defaultValues)
+ [function] IStructType::getFieldTypes(IList** types)
+ [factory] StructTypePtr StructType(const StringPtr& name, const ListPtr<IString>& fieldNames, const ListPtr<IBaseObject>& defaultValues, const ListPtr<IType>& fieldTypes)
+ [factory] StructTypePtr StructType(const StringPtr& name, const ListPtr<IString>& fieldNames, const ListPtr<IType>& fieldTypes)

+ [interface] IType : public IBaseObject
+ [function] IType::getName(IString** typeName)

+ [interface] ITypeManager : public IBaseObject
+ [function] ITypeManager::addType(IType* type)
+ [function] ITypeManager::removeType(IString* typeName)
+ [function] ITypeManager::getType(IString* typeName, IType** type)
+ [function] ITypeManager::getTypes(IList** types)
+ [function] ITypeManager::hasType(IString* typeName, Bool* hasType)
+ [factory] TypeManagerPtr TypeManager()

+ [factory] StructTypePtr ArgumentInfoStructType()
+ [factory] StructTypePtr CallableInfoStructType()

+ [function] IProperty::getStructType(IStructType** structType)

-m [interface] IPropertyConfig : public IProperty
+m [interface] IPropertyBuilder : public IBaseObject
+ [function] IPropertyBuilder::setOnPropertyValueWrite(IEvent* event)
+ [function] IPropertyBuilder::setOnPropertyValueRead(IEvent* event)
+ [function] IPropertyBuilder::build(IProperty** property)

-m [factory] PropertyConfigPtr BoolProperty(const StringPtr& name, const BooleanPtr& defaultValue, const BooleanPtr& visible = true)
+m [factory] PropertyPtr BoolProperty(const StringPtr& name, const BooleanPtr& defaultValue, const BooleanPtr& visible = true)
-m [factory] PropertyConfigPtr IntProperty(const StringPtr& name, const IntegerPtr& defaultValue, const BooleanPtr& visible = true)
+m [factory] PropertyPtr IntProperty(const StringPtr& name, const IntegerPtr& defaultValue, const BooleanPtr& visible = true)
-m [factory] PropertyConfigPtr FloatProperty(const StringPtr& name, const FloatPtr& defaultValue, const BooleanPtr& visible = true)
+m [factory] PropertyPtr FloatProperty(const StringPtr& name, const FloatPtr& defaultValue, const BooleanPtr& visible = true)
-m [factory] PropertyConfigPtr StringProperty(const StringPtr& name, const StringPtr& defaultValue, const BooleanPtr& visible = true)
+m [factory] PropertyPtr StringProperty(const StringPtr& name, const StringPtr& defaultValue, const BooleanPtr& visible = true)
-m [factory] PropertyConfigPtr ListProperty(const StringPtr& name, const ListPtr<IBaseObject>& defaultValue, const BooleanPtr& visible = true)
+m [factory] PropertyPtr ListProperty(const StringPtr& name, const ListPtr<IBaseObject>& defaultValue, const BooleanPtr& visible = true)
-m [factory] PropertyConfigPtr DictProperty(const StringPtr& name, const DictPtr<IBaseObject, IBaseObject>& defaultValue, const BooleanPtr& visible = true)
+m [factory] PropertyPtr DictProperty(const StringPtr& name, const DictPtr<IBaseObject, IBaseObject>& defaultValue, const BooleanPtr& visible = true)
-m [factory] PropertyConfigPtr RatioProperty(const StringPtr& name, const RatioPtr& defaultValue, const BooleanPtr& visible = true)
+m [factory] PropertyPtr RatioProperty(const StringPtr& name, const RatioPtr& defaultValue, const BooleanPtr& visible = true)
-m [factory] PropertyConfigPtr ObjectProperty(const StringPtr& name, const PropertyObjectPtr& defaultValue = nullptr)
+m [factory] PropertyPtr ObjectProperty(const StringPtr& name, const PropertyObjectPtr& defaultValue = nullptr)
-m [factory] PropertyConfigPtr FunctionProperty(const StringPtr& name, const CallableInfoPtr& callableInfo, const BooleanPtr& visible = true)
+m [factory] PropertyPtr FunctionProperty(const StringPtr& name, const CallableInfoPtr& callableInfo, const BooleanPtr& visible = true)
-m [factory] PropertyConfigPtr ReferenceProperty(const StringPtr& name, const EvalValuePtr& referencedPropertyEval)
+m [factory] PropertyPtr ReferenceProperty(const StringPtr& name, const EvalValuePtr& referencedPropertyEval)
-m [factory] PropertyConfigPtr SelectionProperty(const StringPtr& name, const ListPtr<IBaseObject>& selectionValues, const IntegerPtr& defaultValue, const BooleanPtr& visible = true)
+m [factory] PropertyPtr SelectionProperty(const StringPtr& name, const ListPtr<IBaseObject>& selectionValues, const IntegerPtr& defaultValue, const BooleanPtr& visible = true)
-m [factory] PropertyConfigPtr SparseSelectionProperty(const StringPtr& name, const DictPtr<Int, IBaseObject>& selectionValues, const IntegerPtr& defaultValue, const BooleanPtr& visible = true)
+m [factory] PropertyPtr SparseSelectionProperty(const StringPtr& name, const DictPtr<Int, IBaseObject>& selectionValues, const IntegerPtr& defaultValue, const BooleanPtr& visible = true)
+m [factory] PropertyPtr StructProperty(const StringPtr& name, const StructPtr& defaultValue, const BooleanPtr& visible = true)
+m [factory] PropertyPtr PropertyFromBuildParams(const DictPtr<IString, IBaseObject>& buildParams)

-m [factory] PropertyConfigPtr Property(const StringPtr& name)
+m [factory] PropertyBuilderPtr BoolPropertyBuilder(const StringPtr& name, const BooleanPtr& defaultValue)
+ [factory] PropertyBuilderPtr PropertyBuilder(const StringPtr& name)
+ [factory] PropertyBuilderPtr IntPropertyBuilder(const StringPtr& name, const IntegerPtr& defaultValue)
+ [factory] PropertyBuilderPtr FloatPropertyBuilder(const StringPtr& name, const FloatPtr& defaultValue)
+ [factory] PropertyBuilderPtr StringPropertyBuilder(const StringPtr& name, const StringPtr& defaultValue)
+ [factory] PropertyBuilderPtr ListPropertyBuilder(const StringPtr& name, const ListPtr<IBaseObject>& defaultValue)
+ [factory] PropertyBuilderPtr DictPropertyBuilder(const StringPtr& name, const DictPtr<IBaseObject, IBaseObject>& defaultValue)
+ [factory] PropertyBuilderPtr RatioPropertyBuilder(const StringPtr& name, const RatioPtr& defaultValue)
+ [factory] PropertyBuilderPtr ObjectPropertyBuilder(const StringPtr& name, const PropertyObjectPtr& defaultValue = nullptr)
+ [factory] PropertyBuilderPtr FunctionPropertyBuilder(const StringPtr& name, const CallableInfoPtr& callableInfo)
+ [factory] PropertyBuilderPtr ReferencePropertyBuilder(const StringPtr& name, const EvalValuePtr& referencedPropertyEval)
+ [factory] PropertyBuilderPtr SelectionPropertyBuilder(const StringPtr& name, const ListPtr<IBaseObject>& selectionValues, const IntegerPtr& defaultValue)
+ [factory] PropertyBuilderPtr SparseSelectionPropertyBuilder(const StringPtr& name, const DictPtr<Int, IBaseObject>& selectionValues, const IntegerPtr& defaultValue)
+ [factory] PropertyBuilderPtr StructPropertyBuilder(const StringPtr& name, const StructPtr& defaultValue)

- [function] IPropertyInternal::setOnPropertyValueWrite(IEvent* event)
- [function] IPropertyInternal::getOnPropertyValueWrite(IEvent** event)

-m [interface] IPropertyObjectClass : public IBaseObject
+m [interface] IPropertyObjectClass : public IType
- [function] IPropertyObjectClass::getName(IString** className);

-m [interface] IPropertyObjectClassConfig : public IPropertyObjectClass
+m [interface] IPropertyObjectClassBuilder : public IBaseObject
+ [function] IPropertyObjectClassBuilder::build(IPropertyObjectClass** propertyObjectClass)

-m [factory] PropertyObjectClassConfigPtr PropertyObjectClass(const StringPtr& name)
+m [factory] PropertyObjectClassBuilderPtr PropertyObjectClassBuilder(const StringPtr& name)
-m [factory] PropertyObjectClassConfigPtr PropertyObjectClass(const PropertyObjectClassManagerPtr& manager, const StringPtr& name)
+m [factory]PropertyObjectClassBuilderPtr PropertyObjectClassBuilder(const TypeManagerPtr& manager, const StringPtr& name)
+ [factory] PropertyObjectClassPtr PropertyObjectClassFromBuildParams(const DictPtr<IString, IBaseObject>& buildParams)

- [interface] IPropertyObjectClassManager

-m [factory] PropertyObjectPtr PropertyObject(const PropertyObjectClassManagerPtr& manager, const StringPtr& className)
+m [factory] PropertyObjectPtr PropertyObject(const TypeManagerPtr& manager, const StringPtr& className)

-m [interface] IUnitConfig : public IUnit
-m [interface] IUnitBuilder: public IBaseObject
+ [function] IUnitBuilder::build(IUnit** unit)
+ [factory] UnitBuilderPtr UnitBuilder()
+ [factory] StructTypePtr UnitStructType()
-m [factory] UnitConfigPtr UnitCopy(const UnitPtr& unit)
+m [factory] UnitBuilderPtr UnitBuilderCopy(const UnitPtr& unit)

-m [function] IContext::getPropertyObjectClassManager(IPropertyObjectClassManager** manager)
+m [function] IContext::getTypeManager(ITypeManager** manager)
-m [factory]ContextPtr Context(const SchedulerPtr& scheduler, const LoggerPtr& logger, const PropertyObjectClassManagerPtr& propertyObjectClassManager, const TypeManagerPtr& typeManager, const ModuleManagerPtr& moduleManager)
+m [factory]ContextPtr Context(const SchedulerPtr& scheduler, const LoggerPtr& logger, const TypeManagerPtr& propertyObjectClassManager, const TypeManagerPtr& typeManager, const ModuleManagerPtr& moduleManager)

+m [factory] StructTypePtr DeviceTypeStructType()
+m [factory] StructTypePtr FunctionBlockTypeStructType()
+m [factory] StructTypePtr ServerTypeStructType()
-m [factory] ServerTypeConfigPtr ServerType(const StringPtr& id, const StringPtr& name, const StringPtr& description, const FunctionPtr& createDefaultConfigCallback = nullptr)
+m [factory] ServerTypePtr ServerType(const StringPtr& id, const StringPtr& name, const StringPtr& description, const FunctionPtr& createDefaultConfigCallback = nullptr)

-m [interface] IDataDescriptorConfig : public IDataDescriptor
+m [interface] IDataDescriptorBuilder : public IBaseObject
+ [function] IDataDescriptorBuilder::build(IDataDescriptor** dataDescriptor)
-m [factory] DataDescriptorConfigPtr DataDescriptor(SampleType sampleType)
+m [factory] DataDescriptorBuilderPtr DataDescriptorBuilder()
-m [factory] DataDescriptorConfigPtr DataDescriptorCopy(const DataDescriptorPtr& dataDescriptor)
+m [factory] DataDescriptorBuilderPtr DataDescriptorBuilderCopy(const DataDescriptorPtr& dataDescriptor)
+ [factory] DataDescriptorPtr DataDescriptor(const DictPtr<IString, IBaseObject>& descriptorParams)
+ [factory] StructTypePtr DataDescriptorStructType()

-m [interface] IDataRuleConfig : public IDataRule
+m [interface] IDataRuleBuilder : public IBaseObject
+ [function] IDataRuleBuilder::addParameter(IString* name, IBaseObject* parameter)
+ [function] IDataRuleBuilder::removeParameter(IString* name)
+ [function] IDataRuleBuilder::build(IDataRule** dataRule)
-m [factory] DataRuleConfigPtr DataRule()
+m [factory] DataRuleBuilderPtr DataRuleBuilder()
-m [factory] DataRuleConfigPtr DataRuleCopy(const DataRulePtr& rule)
+m [factory] DataRuleBuilderPtr DataRuleBuilderCopy(const DataRulePtr& rule)
+ [factory] DataRulePtr DataRule(DataRuleType type, DictPtr<IString, IBaseObject>& parameters)
+ [factory] StructTypePtr DataRuleStructType()

-m [interface] IDimensionConfig : public IDimension
+m [interface] IDimensionBuilder : public IBaseObject
+ [function] IDimensionBuilder::build(IDimensionBuilder** dimension)
-m [factory] DimensionConfigPtr Dimension()
+m [factory] DimensionBuilderPtr DimensionBuilder()
-m [factory] DimensionConfigPtr DimensionCopy(const DimensionPtr& dimension)
+m [factory] DimensionBuilderPtr DimensionBuilderCopy(const DimensionPtr& dimension)
+ [factory] StructTypePtr DimensionStructType()

-m [interface] IDimensionRuleConfig : public IDimensionRule
+m [interface] IDimensionRuleBuilder : public IBaseObject
+ [function] IDimensionRule::addParameter(IString* name, IBaseObject* parameter)
+ [function] IDimensionRule::removeParameter(IString* name)
+ [function] IDimensionRule::build(IDimensionRule** dimensionRule)
-m [factory] DimensionRuleConfigPtr DimensionRule()
+m [factory] DimensionRuleBuilderPtr DimensionRuleBuilder()
-m [factory] DimensionRuleConfigPtr DimensionRuleCopy(const DimensionRulePtr& rule)
+m [factory] DimensionRuleBuilderPtr DimensionRuleBuilderCopy(const DimensionRulePtr& rule)
+ [factory] DimensionRulePtr DimensionRule(DimensionRuleType type, const DictPtr<IString, IBaseObject> parameters)
+ [factory] StructTypePtr DimensionRuleStructType()

+ [factory] StructTypePtr RangeStructType()

-m [interface] IScalingConfig : public IScaling
+m [interface] IScalingBuilder : public IBaseObject
+ [function] IScalingBuilder::addParameter(IString* name, IBaseObject* parameter)
+ [function] IScalingBuilder::removeParameter(IString* name)
+ [function] IScalingBuilder::build(IScalingBuilder** scaling)
-m [factory] ScalingConfigPtr Scaling()
+m [factory] ScalingBuilderPtr ScalingBuilder()
-m [factory] ScalingConfigPtr ScalingCopy(const ScalingPtr& scaling)
+m [factory] ScalingBuilderPtr ScalingBuilderCopy(const ScalingPtr& scaling)
+ [factory] ScalingPtr Scaling(SampleType inputDataType, ScaledSampleType outputDataType, ScalingType scalingType, const DictPtr<IString, IBaseObject>& params)
+ [factory] StructTypePtr ScalingStructType()
```

---

# Unscaled/Raw Mode Option in Readers

## Description

Added unscaled (raw) mode option to readers.

## API changes

```diff
+  [function] ISampleReader::getReadMode(ReadMode* mode);
+  [factory] StreamReaderPtr StreamReader(SignalPtr signal, ReadMode mode, ReadTimeoutType timeoutType = ReadTimeoutType::All)
-m [factory] StreamReaderPtr StreamReader(SignalPtr signal, SampleType valueReadType, SampleType domainReadType, ReadTimeoutType timeoutType = ReadTimeoutType::All)
+m [factory] StreamReaderPtr StreamReader(SignalPtr signal, SampleType valueReadType, SampleType domainReadType, ReadMode mode = ReadMode::Scaled, ReadTimeoutType timeoutType = ReadTimeoutType::All
-m [factory] StreamReaderPtr StreamReader(SignalPtr signal, ReadTimeoutType timeoutType = ReadTimeoutType::All)
+m [factory] StreamReaderPtr StreamReader(SignalPtr signal, ReadMode mode = ReadMode::Scaled, ReadTimeoutType timeoutType = ReadTimeoutType::All)
-m [factory] TailReaderPtr TailReader(SignalPtr signal, SizeT historySize, SampleType valueReadType, SampleType domainReadType)
+m [factory] TailReaderPtr TailReader(SignalPtr signal, SizeT historySize, SampleType valueReadType, SampleType domainReadType, ReadMode mode = ReadMode::Scaled)
-m [factory] TailReaderPtr TailReader(SignalPtr signal, SizeT historySize)
+m [factory] TailReaderPtr TailReader(SignalPtr signal, SizeT historySize, ReadMode mode = ReadMode::Scaled)
-m [factory] TailReaderPtr TailReader(SignalPtr signal, SizeT historySize)
+m [factory] TailReaderPtr TailReader(SignalPtr signal, SizeT historySize, ReadMode mode = ReadMode::Scaled)
-m [factory] BlockReaderPtr BlockReader(SignalPtr signal, SizeT blockSize, SampleType valueReadType, SampleType domainReadType)
+m [factory] BlockReaderPtr BlockReader(SignalPtr signal, SizeT blockSize, SampleType valueReadType, SampleType domainReadType, ReadMode mode = ReadMode::Scaled)
-m [factory] BlockReaderPtr BlockReader(SignalPtr signal, SizeT blockSize)
+m [factory] BlockReaderPtr BlockReader(SignalPtr signal, SizeT blockSize, ReadMode mode = ReadMode::Scaled)
-m [factory] MultiReaderPtr MultiReader(const ListPtr<ISignal>& signals, SampleType valueReadType, SampleType domainReadType, ReadTimeoutType timeoutType = ReadTimeoutType::All)
+m [factory] MultiReaderPtr MultiReader(const ListPtr<ISignal>& signals, SampleType valueReadType, SampleType domainReadType, ReadMode mode = ReadMode::Scaled, ReadTimeoutType timeoutType = ReadTimeoutType::All)
+  [factory] MultiReaderPtr MultiReader(ListPtr<ISignal> signals, ReadMode mode, ReadTimeoutType timeoutType = ReadTimeoutType::All)
```

---

# Module Manager in Context & Nested FunctionBlock

## Description

- `ModuleManager` is now accessible within `Context`.
- A new `createAndAddNestedFunctionBlock` method was added to `SignalContainerImpl` for easy creation of nested Function Blocks using loaded modules.
- Some general API improvements.

## API changes

```diff
+ [function] FunctionBlockPtr GenericSignalContainerImpl::createAndAddNestedFunctionBlock(const StringPtr& typeId, const StringPtr& localId, const PropertyObjectPtr& config = nullptr)
+ [function] GenericSignalContainerImpl::createAndAddNestedFunctionBlock(const StringPtr& typeId, const StringPtr& localId, const PropertyObjectPtr& config = nullptr)
-m [function] void GenericSignalContainerImpl::addFB(const FunctionBlockPtr& functionBlock)
+m [function] void GenericSignalContainerImpl::addNestedFunctionBlock(const FunctionBlockPtr& functionBlock)
-m [function] void GenericSignalContainerImpl::removeFB(const FunctionBlockPtr& functionBlock)
+m [function] void GenericSignalContainerImpl::removeNestedFunctionBlock(const FunctionBlockPtr& functionBlock)

+ [function] IContext::getModuleManager(IBaseObject** manager)
-m [factory] ContextPtr Context(const SchedulerPtr& scheduler, const LoggerPtr& logger, const PropertyObjectClassManagerPtr& propertyObjectClassManager)
+m [factory] ContextPtr Context(const SchedulerPtr& scheduler, const LoggerPtr& logger, const PropertyObjectClassManagerPtr& propertyObjectClassManager, const ModuleManager& moduleManager)
+ [interface] IContextInternal : public IBaseObject
+ [function] IContextInternal::moveModuleManager(IModuleManager** manager)

-m [function] IModule::createFunctionBlock(IString* id, IComponent* parent, IString* localId, IFunctionBlock** functionBlock)
+m [function] IModule::createFunctionBlock(IFunctionBlock** functionBlock, IString* id, IComponent* parent, IString* localId, IPropertyObject* config = nullptr)
-m [function] IModule::createServer(IString* serverTypeId, IPropertyObject* serverConfig, IDevice* rootDevice, IServer** server)
+m [function] IModule::createServer(IServer** server, IString* serverTypeId, IDevice* rootDevice, IPropertyObject* config = nullptr);
-m [function] createModule(IModule** module, IContext* context, IModule* manager)
+m [function] createModule(IModule** module, IContext* context)

+ [function] IModuleManager::loadModules(IContext* context)
-m [factory] ModuleManagerPtr ModuleManager(const StringPtr& searchPath, ContextPtr context)
+m [factory] ModuleManagerPtr ModuleManager(const StringPtr& searchPath)

-m [factory] InstancePtr Instance(const ContextPtr& context, const ModuleManagerPtr& moduleManager, const StringPtr& localId)
+m [factory] InstancePtr InstanceCustom(const ContextPtr& context, const StringPtr& localId)
-m [factory] DevicePtr Client(const ContextPtr& context, const ModuleManagerPtr& moduleManager, const StringPtr& localId)
+m [factory] DevicePtr Client(const ContextPtr& context, const StringPtr& localId)
```

---

# Removed Context from IInstance

## Description

The `Context` was removed from the `IInstance` interface.

## API changes

```diff
- [function] IInstance::getContext(IContext** context)
```

---

# Delphi Bindings Fixes & Enhancements

## Description

- Various fixes and improvements for Delphi bindings.
- Removed empty `Property()` factory that now requires at least a name.
- Changed return type of `XyzProperty()` factories to `IPropertyConfig` instead of base `IProperty`.
- Removed `ILoggerComponent::logMessage` overload (unused and violates code conventions).
- Changed `SourceLocation::line` variable from `int` to the fixed-width type `daq::Int`.
- Updated `IAllocator::allocate` and `IAllocator::free`.

## API changes

```diff
-m [factory] Property()
+m [factory] Property(name)
-  [factory] PropertyWithName(name)
-  [function] ILoggerComponent.logMessage(ConstCharPtr msg, LogLevel level)
```

---

# OPC UA Model Update

## Description

Updated to the latest OPC UA model. `Signal` and `InputPort` properties are not visible over OPC UA.  
`IComponent` methods are accessible via OPC UA for all openDAQ components.  
`UserName` and `location` moved from `DeviceInfo` into `Device` properties.

## API changes

```diff
- [function] IDeviceInfo::getUserName(IString** userName)
- [function] IDeviceInfo::getLocation(IString** location)
- [function] IDeviceInfoConfig::setLocation(IString* location)
- [function] IDeviceInfo::setUserName(IString* userName)
```

---

# Added Name & Description to ISignal + PropertyChanged Event

Commit: ff6768d39a76b3b784994f6a17f1d730cb8be639

## Description

Introduced `Name` and `Description` as static and dynamic properties on `ISignal`. These were previously part of the signal descriptor.  
Added a new `PropertyChanged` event packet type sent to connected listeners when any property (e.g., `Name`) is changed on a signal.

## API changes

```diff
+ [function] ISignal::setName(IString* name)
+ [function] ISignal::setDescription(IString* name)
+ [function] ISignal::getDescription(IString** description)

+ [factory] inline EventPacketPtr PropertyChangedEventPacket(const StringPtr& name, const BaseObjectPtr& value)
```

---

# Removed ISignalDescriptor in Favor of IDataDescriptor

Commit: 70742e4554bbf6f13da11bc782ef7533d8d71795

## Description

Removed `ISignalDescriptor` and replaced it with `IDataDescriptor` throughout.  
`metadata` field is moved to `IDataDescriptor`.  
`Name` and `Description` are no longer part of the signal/data descriptor itself.

## API changes

```diff
-m [function] IDataPacket::getSignalDescriptor(ISignalDescriptor** descriptor)
+m [function] IDataPacket::getDescriptor(IDataDescriptor** descriptor)

+ [function] IDataDescriptor::getMetadata(IDict** metadata)
+ [function] IDataDescriptorConfig::getMetadata(IDict* metadata)

-m [factory] inline EventPacketPtr SignalDescriptorChangedEventPacket(SignalDescriptorPtr signalDescriptor, SignalDescriptorPtr domainSignalDescriptor)
+m [factory] inline EventPacketPtr DataDescriptorChangedEventPacket(const DataDescriptorPtr& dataDescriptor, const DataDescriptorPtr& domainDataDescriptor)

-m [function] ISignal::getDescriptor(ISignalDescriptor** descriptor)
+m [function] ISignal::getDescriptor(IDataDescriptor** descriptor)

-m [function] ISignalConfig::setDescriptor(ISignalDescriptor* descriptor)
+m [function] ISignalConfig::setDescriptor(IDataDescriptor* descriptor)

-m [factory] inline SignalConfigPtr SignalWithDescriptor(const ContextPtr& context, const SignalDescriptorPtr& descriptor, const DataDescriptorPtr& descriptor, const ComponentPtr& parent, const StringPtr& localId)
+m [factory] inline SignalConfigPtr SignalWithDescriptor(const ContextPtr& context, const DataDescriptorPtr& descriptor, const DataDescriptorPtr& descriptor, const ComponentPtr& parent, const StringPtr& localId)

-m [factory] inline DataPacketPtr BinaryDataPacket(const DataPacketPtr& domainPacket, const SignalDescriptorPtr& descriptor, const DataDescriptorPtr& descriptor, uint64_t sampleMemSize)
+m [factory] inline DataPacketPtr BinaryDataPacket(const DataPacketPtr& domainPacket, const DataDescriptorPtr& descriptor, const DataDescriptorPtr& descriptor, uint64_t sampleMemSize)

-m [factory] inline DataPacketPtr DataPacket(const SignalDescriptorPtr& descriptor, uint64_t sampleCount, NumberPtr offset = nullptr, const NumberPtr& offset = nullptr, AllocatorPtr allocator = nullptr)
+m [factory] inline DataPacketPtr DataPacket(const DataDescriptorPtr& descriptor, uint64_t sampleCount, NumberPtr offset = nullptr, const NumberPtr& offset = nullptr, AllocatorPtr allocator = nullptr)

-m [factory] inline DataPacketPtr DataPacketWithDomain(const DataPacketPtr& domainPacket, const SignalDescriptorPtr& descriptor, uint64_t sampleCount, NumberPtr offset = nullptr, AllocatorPtr allocator = nullptr)
+m [factory] inline DataPacketPtr DataPacketWithDomain(const DataPacketPtr& domainPacket, const DataDescriptorPtr& descriptor, uint64_t sampleCount, NumberPtr offset = nullptr, AllocatorPtr allocator = nullptr)
```

---

# IComponent, Device, and IInstance: Additional Changes

Commits: 7713cdbb0614b5c073a8d7eb3d834b62e9b1efb4 – 29ee6eb5ebdef33c23b1a7eed4b1e8a064cdb7eb

## API changes

```diff
+ [interface] IComponentType : public IBaseObject
+ [function] IComponentType::getId(IString** id)
+ [function] IComponentType::getName(IString** name)
+ [function] IComponentType::getDescription(IString** description)
+ [function] IComponentType::createDefaultConfig(IPropertyObject** defaultConfig)

+ [interface] IDeviceType : public IComponentType
+ [factory] inline DeviceTypePtr DeviceType(const StringPtr& id, const StringPtr& name, const StringPtr& description, const FunctionPtr& createDefaultConfigCallback = nullptr)

-m [interface] IFunctionBlockType : public IBaseObject
+m [interface] IFunctionBlockType : public IComponentType
- [function] IFunctionBlockType::getId(IString** id)
- [function] IFunctionBlockType::getName(IString** name)
- [function] IFunctionBlockType::getDescription(IString** description)
- [function] IFunctionBlockType::createDefaultConfig(IPropertyObject** defaultConfig)
-m [factory] FunctionBlockTypePtr FunctionBlockType(const StringPtr& id, const StringPtr& name, const StringPtr& description)
+m [factory] FunctionBlockTypePtr FunctionBlockType(const StringPtr& id, const StringPtr& name, const StringPtr& description, const FunctionPtr& createDefaultConfigCallback = nullptr)

-m [interface] IServerType : public IBaseObject
+m [interface] IServerType : public IComponentType
- [function] IServerType::getId(IString** id)
- [function] IServerType::getName(IString** name)
- [function] IServerType::getDescription(IString** description)
- [function] IServerType::createDefaultConfig(IPropertyObject** defaultConfig)
-m [factory] ServerTypeConfigPtr ServerType(const StringPtr& id, const StringPtr& name, const StringPtr& description)
+m [factory] ServerTypeConfigPtr ServerType(const StringPtr& id, const StringPtr& name, const StringPtr& description, const FunctionPtr& createDefaultConfigCallback = nullptr)

+ [function] IPropertyObjectProtected::clearProtectedPropertyValue(IString* propertyName)

-m [function] IDevice::getAvailableDevices(IDict** availableDevices)
+m [function] IDevice::getAvailableDevices(IList** availableDevices)
+ [function] IDevice::getAvailableDeviceTypes(IDict** deviceTypes)
-m [function] IDevice::getAvailableFunctionBlocks(IDict** functionBlockTypes)
+m [function] IDevice::getAvailableFunctionBlockTypes(IDict** functionBlockTypes)

+ [interface] IDevicePrivate : public IBaseObject
+ [function] IDevicePrivate::addStreamingOption(IStreamingInfo* info)
+ [function] IDevicePrivate::removeStreamingOption(IString* protocolId)
+ [function] IDevicePrivate::getStreamingOptions(IList** streamingOptions)

+ [function] IDeviceInfo::getDeviceType(IDeviceType** deviceType)
+ [function] IDeviceInfo::getDeviceRevision(IString** deviceRevision)
+ [function] IDeviceInfo::getAssetId(IString** id)
+ [function] IDeviceInfo::getMacAddress(IString** macAddress)
+ [function] IDeviceInfo::getParentMacAddress(IString** macAddress)
+ [function] IDeviceInfo::getPlatform(IString** platform)
+ [function] IDeviceInfo::getPosition(Int* position)
+ [function] IDeviceInfo::getSystemType(IString** type)
+ [function] IDeviceInfo::getSystemUuid(IString** uuid)
+ [function] IDeviceInfo::getLocation(IString** location)
+ [function] IDeviceInfo::getUserName(IString** userName)
+ [function] IDeviceInfo::getCustomInfoPropertyNames(IList** customInfoNames)

+ [function] IDeviceInfoConfig::setDeviceType(IDeviceType* deviceType)
+ [function] IDeviceInfoConfig::setDeviceRevision(IString* deviceRevision)
+ [function] IDeviceInfoConfig::setAssetId(IString* id);
+ [function] IDeviceInfoConfig::setMacAddress(IString* macAddress)
+ [function] IDeviceInfoConfig::setParentMacAddress(IString* macAddress)
+ [function] IDeviceInfoConfig::setPlatform(IString* platform)
+ [function] IDeviceInfoConfig::setPosition(Int position)
+ [function] IDeviceInfoConfig::setSystemType(IString* type)
+ [function] IDeviceInfoConfig::setSystemUuid(IString* uuid)
+ [function] IDeviceInfoConfig::setLocation(IString* location)
+ [function] IDeviceInfoConfig::setUserName(IString* userName)

-m [function] IDeviceInfo::getRevisionCounter(IString** revisionCounter)
+m [function] IDeviceInfo::getRevisionCounter(Int* revisionCounter)
-m [function] IDeviceInfoConfig::setRevisionCounter(IString* revisionCounter)
+m [function] IDeviceInfoConfig::setRevisionCounter(Int revisionCounter)

-m [function] IModule::getAvailableDevices(IDict** availableDevices)
+m [function] IModule::getAvailableDevices(IList** availableDevices)
+ [function] IModule::getAvailableDeviceTypes(IDict** deviceTypes)
-m [function] IModule::getAvailableFunctionBlocks(IDict** functionBlockTypes)
+m [function] IModule::getAvailableFunctionBlockTypes(IDict** functionBlockTypes)
-m [function] IModule::getAvailableServers(IDict** serverTypes)
+m [function] IModule::getAvailableServerTypes(IDict** serverTypes)
+ [function] IModule::acceptsStreamingConnectionParameters(Bool* accepted, IString* connectionString, IStreamingInfo* config = nullptr)
+ [function] IModule::createStreaming(IStreaming** streaming, IString* connectionString, IStreamingInfo* config)

-m [function] IInstance::setRootDevice(IDevice* rootDevice)
+m [function] IInstance::setRootDevice(IString* connectionString, IPropertyObject* config = nullptr)
-m [function] IInstance::getAvailableServers(IDict** serverTypes)
+m [function] IInstance::getAvailableServerTypes(IDict** serverTypes)

- [function] IServer::updateRootDevice(IDevice* rootDevice)

+ [function] ISignalConfig::getStreamingSources(IList** streamingConnectionStrings)
+ [function] ISignalConfig::setActiveStreamingSource(IString* streamingConnectionString)
+ [function] ISignalConfig::getActiveStreamingSource(IString** streamingConnectionString)
+ [function] ISignalConfig::deactivateStreaming()

+ [interface] ISignalRemote : public IBaseObject
+ [function] ISignalRemote::getRemoteId(IString** id) const
+ [function] ISignalRemote::triggerEvent(IEventPacket* eventPacket)
+ [function] ISignalRemote::addStreamingSource(IStreaming* streaming)
+ [function] ISignalRemote::removeStreamingSource(IStreaming* streaming)

+ [interface] IStreaming : public IBaseObject
+ [function] IStreaming::getActive(Bool* active)
+ [function] IStreaming::setActive(Bool active)
+ [function] IStreaming::addSignals(IList* signals)
+ [function] IStreaming::removeSignals(IList* signals)
+ [function] IStreaming::removeAllSignals()
+ [function] IStreaming::getConnectionString(IString** connectionString) const

+ [interface] IStreamingInfo : public IPropertyObject
+ [function] IStreamingInfo::getPrimaryAddress(IString** address)
+ [function] IStreamingInfo::getProtocolId(IString** protocolId)

+ [interface] IStreamingInfoConfig : public IStreamingInfo
+ [function] IStreamingInfoConfig::setPrimaryAddress(IString* address)
```
