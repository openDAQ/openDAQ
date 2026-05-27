# Changes from 3.30 to 3.40

## Features

- [#914](https://github.com/openDAQ/openDAQ/pull/914) Implementing time delay function block. Fix overflow while calculating timestamp in renderer
- [#886](https://github.com/openDAQ/openDAQ/pull/886) Adds simulator device module.
- [#975](https://github.com/openDAQ/openDAQ/pull/975) Add `DevelopmentVersionInfo` for more detailed version information (tweak, branch and hash).
- [#981](https://github.com/openDAQ/openDAQ/pull/981) Component active state now considers parent's active state. `getActive()` returns `active && parentActive`. Add posibility to generate c and python bindings on MacOs
- [#1000](https://github.com/openDAQ/openDAQ/pull/1000) Added a mode to the CSV recorder that allows data from multiple same-rate signals to be written to a single file.
- [#1018](https://github.com/openDAQ/openDAQ/pull/1018) Optimized multireader synchronization for linear data rule domain signals by avoiding iteration through timestamps.
- [#1034](https://github.com/openDAQ/openDAQ/pull/1034) Add string support for data packets and signals
- [#1036](https://github.com/openDAQ/openDAQ/pull/1036) Allow creation of empty multi reader, add/remove inputs dyinamically. Mark inputs as invalid to ignore sync failures from specific ports.
- [#1037](https://github.com/openDAQ/openDAQ/pull/1037) Function blocks and devices of which type is not in their parent's `getAvailableFunctionBlock/DeviceTypes` output can no longer be removed.
- [#1074](https://github.com/openDAQ/openDAQ/pull/1074) Add public attribute to input port. Non-public input ports are not available one client side.
- [#1079](https://github.com/openDAQ/openDAQ/pull/1079) Add support for parallel RPC calls in native configuration protocol server
- [#1081](https://github.com/openDAQ/openDAQ/pull/1081) Pre-parsing of a JSON setup that allows users to select from a set of actions to be performed when loading devices (load, remove, remap...)
- [#1088](https://github.com/openDAQ/openDAQ/pull/1088) Enable bundling of sent core events within the native configuration protocol
- [#1097](https://github.com/openDAQ/openDAQ/pull/1097) Implement value based selection list
- [#1124](https://github.com/openDAQ/openDAQ/pull/1124) Implement clearPropertyValues() for property objects
- [#1137](https://github.com/openDAQ/openDAQ/pull/1137) Add `IInputPort::acceptsSignals` to check multiple signals at once, using a single RPC call via native protocol.
- [#1056](https://github.com/openDAQ/openDAQ/pull/1056) implement attempting to reconnect with all available addresses#1056
- [#1157](https://github.com/openDAQ/openDAQ/pull/1157) Devices default to `OperationModeType::SafeOperation` when added to folder or set as root. Add virtual method to make it customizable.
- [#1153](https://github.com/openDAQ/openDAQ/pull/1153) Enable and disable discovery for openDAQ Server via native.
- [#1178](https://github.com/openDAQ/openDAQ/pull/1178) Calling begin/end update only on the root component while save/load
- [#1179](https://github.com/openDAQ/openDAQ/pull/1179) Reduce component config size
- [#1166](https://github.com/openDAQ/openDAQ/pull/1166) Add `IUpdateParameters::setRemoveUnusedDevices` to optionally remove devices not specified in the loaded configuration.

## Python

- [#980](https://github.com/openDAQ/openDAQ/pull/980) Update add device with configuration dialog in Python GUI demo app.
- [#986](https://github.com/openDAQ/openDAQ/pull/986) Make Struct Properties editable in Python GUI demo application
- [#987](https://github.com/openDAQ/openDAQ/pull/987) Make Enumeration Properties editable in Python GUI demo application
- [#991](https://github.com/openDAQ/openDAQ/pull/991) Add Function Block configuration to Python GUI demo app
- [#1007](https://github.com/openDAQ/openDAQ/pull/1007) Add a missing error popup when adding a Function Block fails in Python GUI demo app.
- [#1029](https://github.com/openDAQ/openDAQ/pull/1029) Fix python binding for iterators to enable list comprehensions.
- [#1035](https://github.com/openDAQ/openDAQ/pull/1035) Fix showing description metadata of properties in Python GUI app
- [#1081](https://github.com/openDAQ/openDAQ/pull/1081) Adds device load options to the Python GUI.
- [#1142](https://github.com/openDAQ/openDAQ/pull/1142) Improve properties treeview UX in Python GUI demo app: float formatting, method Run buttons and cleaner object display.

## Bug fixes

- [#1214](https://github.com/openDAQ/openDAQ/pull/1214) Fixes the order in which packets are enqueued in sendPackets. They are now always correctly enqueued front-to-back.
- [#1213](https://github.com/openDAQ/openDAQ/pull/1213) Forward device locked state core event in native client.
- [#1205](https://github.com/openDAQ/openDAQ/pull/1205) Fix raw socket access exception on Linux/macOS for non-root users
- [#1200](https://github.com/openDAQ/openDAQ/pull/1200) Recreate binary data packets into `BinaryDataPacketImpl` objects on client side of native streaming protocol.
- [#1185](https://github.com/openDAQ/openDAQ/pull/1185) Fix re-entrant property update crash in endApplyUpdate
- [#1158](https://github.com/openDAQ/openDAQ/pull/1158) Skip logging for identical component statuses.
- [#1150](https://github.com/openDAQ/openDAQ/pull/1150) Serialize public flag for input ports
- [#1149](https://github.com/openDAQ/openDAQ/pull/1149) Return error code instead of throwing exceptions from module info.
- [#1146](https://github.com/openDAQ/openDAQ/pull/1146) Use sender addresses if device does not provide A or AAAA records
- [#1143](https://github.com/openDAQ/openDAQ/pull/1143) Fix uncaught exception when closing renderer window
- [#1130](https://github.com/openDAQ/openDAQ/pull/1130) Fix active rework issue with older devices
- [#1122](https://github.com/openDAQ/openDAQ/pull/1122) Revert endUpdate nested property application order back to bottom-up; Batch update values on target child object when using dot notation
- [#1116](https://github.com/openDAQ/openDAQ/pull/1116) Fix set/get for dynamically added object properties via Native Client
- [#1093](https://github.com/openDAQ/openDAQ/pull/1093) Device info - serialize only editable properties
- [#1108](https://github.com/openDAQ/openDAQ/pull/1108) Add a patch for pybind11 to avoid leaking in python bindings.
- [#1104](https://github.com/openDAQ/openDAQ/pull/1104) Fix path configuration option getting ignored by Parquet and CSV recorders in some cases.
- [#1103](https://github.com/openDAQ/openDAQ/pull/1103) Reject loading of duplicating modules by Id and path
- [#1098](https://github.com/openDAQ/openDAQ/pull/1098) Fix setting active streaming source for InputPort
- [#1095](https://github.com/openDAQ/openDAQ/pull/1095) Update imported delegate.hpp and fix cpp 20 build incompatibilities
- [#1086](https://github.com/openDAQ/openDAQ/pull/1086) Mirrored device type is not serialised for native usage
- [#1062](https://github.com/openDAQ/openDAQ/pull/1062) Fix nested PropertyObject update order in beginUpdate/endUpdate
- [#1054](https://github.com/openDAQ/openDAQ/pull/1054) Add validation default value items type for List And Dict Property
- [#1077](https://github.com/openDAQ/openDAQ/pull/1077) Transfer default add-device config from server to client via Native protocol
- [#1015](https://github.com/openDAQ/openDAQ/pull/1015) IPropertyObject::hasProperty returns false instead of throwing not found error if the parent property does not exists
- [#1046](https://github.com/openDAQ/openDAQ/pull/1046) Prevent updating locked attributes during the update process
- [#1052](https://github.com/openDAQ/openDAQ/pull/1052) Renderer FB labels take into account reference domain offset from ReferenceDomainInfo.
- [#1054](https://github.com/openDAQ/openDAQ/pull/1054) Add validation default value items type for List And Dict Property
- [#1059](https://github.com/openDAQ/openDAQ/pull/1059) Prevent post scaling in descriptors with vector/matrix dimensions.
- [#1206](https://github.com/openDAQ/openDAQ/pull/1206) Fix setting operation mode recursively to apply to hidden child devices, not only visible ones.
- [#1197](https://github.com/openDAQ/openDAQ/pull/1197) Propagate parent active state on configuration update.
- [#1177](https://github.com/openDAQ/openDAQ/pull/1177) Reset missing attributes to their default values during update.
- [#1169](https://github.com/openDAQ/openDAQ/pull/1169) Fix active not being set via native protocol when parent active is false.
- [#1160](https://github.com/openDAQ/openDAQ/pull/1160) Align behaviour between the ObjectProperty and ObjectPropertyBuilder factories.
- [#1111](https://github.com/openDAQ/openDAQ/pull/1111) Reset last value calculation values on unsubscribe completion.
- [#1040](https://github.com/openDAQ/openDAQ/pull/1040) Check for cyclic references when connecting input ports; cyclic connections are now rejected.

## Misc

- [#1216](https://github.com/openDAQ/openDAQ/pull/1216) Speeds up comparison between end sentinel when iterating over openDAQ list and dictionary objects
- [#1120](https://github.com/openDAQ/openDAQ/pull/1120) Change reachability status to a sparse selection property type.
- [#1171](https://github.com/openDAQ/openDAQ/pull/1171) MDNS discovery ratelimiting
- [#1125](https://github.com/openDAQ/openDAQ/pull/1125) Removing all function blocks before load 
- [#1109](https://github.com/openDAQ/openDAQ/pull/1109) New check version dependencies mechanism for modules
- [#1090](https://github.com/openDAQ/openDAQ/pull/1090) Reduce unnecessary RPC calls and signal updates
- [#1049](https://github.com/openDAQ/openDAQ/pull/1049) Extract LT and OpcUa modules to remote repos
- [#1051](https://github.com/openDAQ/openDAQ/pull/1051) Removes the FB wrapper implementation as it was never used.
- [#979](https://github.com/openDAQ/openDAQ/pull/979) Relocate and install dependency management cmake helpers, install daq::test_utils and rename "bb" to "daq" in testultils/daq_memcheck_listener.h
- [#974](https://github.com/openDAQ/openDAQ/pull/974) Reorganizes the audio device implementation for greater clarity. Adds automatic device/FB version info setting.
- [#967](https://github.com/openDAQ/openDAQ/pull/967), [#994](https://github.com/openDAQ/openDAQ/pull/994), [#993](https://github.com/openDAQ/openDAQ/pull/993), [#995](https://github.com/openDAQ/openDAQ/pull/995), [#1001](https://github.com/openDAQ/openDAQ/pull/1001), [1025](https://github.com/openDAQ/openDAQ/pull/1025), [#1026](https://github.com/openDAQ/openDAQ/pull/1026), [#1020](https://github.com/openDAQ/openDAQ/pull/1020) Update external libraries:
    - gtest from 1.12.1 to 1.17.0
    - date from 3.0.1 to 3.0.4
    - pybind11 from 2.13.1 to 3.0.1
    - sfml from 3.0.1 to 3.0.2
    - spdlog from 1.13.0.0 to 1.16.0
    - tsl-ordered-map from 1.0.0 to 1.2.0
    - mimalloc from 2.1.1 to 3.0.11
    - fmt from 10.2.1 to 12.1.0
    - xxhash from 0.8.1 to 0.8.3
    - taskflow from 3.5.0 to 3.11.0
    - miniaudio from 0.11.11 to 0.11.23
    - rapidjson from unknown to latest master
    - thrift from 0.20.0 to 0.22.0
    - arrow from 20.0.0 to 22.0.0

## Required application changes

### [#1051](https://github.com/openDAQ/openDAQ/pull/1051) Removed function block wrapper

The IFunctionBlockWrapper interface was removed as it was never used. Similarly, the base implementation headers were removed. The wrapper objects should no longer be used.

### [#1125](https://github.com/openDAQ/openDAQ/pull/1125) Removing all function blocks before load

On load configuration, all non-static function blocks will be removed and recreated if they are in the load config

## Required module changes

### [#1214](https://github.com/openDAQ/openDAQ/pull/1214) Fix packet enqueue order in sendPackets

`ISignalConfig::sendPackets` had an issue where packets were sometimes enqueued front-to-back (first packet in list is enqueued first) and sometimes back-to-front. This inconsistency was addressed to always enqueue packets front-to-back.

Module implementations where `sendPackets` is used should check whether this change in behaviour reversed the order in which packets are enqueued and adjust the packet list order accordingly.

### [#1037](https://github.com/openDAQ/openDAQ/pull/1037) Mandatory device types
To enable static components, devices must include the Device Type in their Device Info objects. Modules set the value within the onGetInfo overriding method by calling IDeviceInfoConfig::setDeviceType().

## Interface API changes
- **2** new interfaces, 
- **1** interface removed, 
- **54** functions added.
- **16** functions removed.

### New interfaces

#### `IDevelopmentVersionInfo`
```diff
+ createDevelopmentVersionInfo(IDevelopmentVersionInfo** obj, SizeT major, SizeT minor, SizeT patch, SizeT tweak, IString* branch, IString* hash)
+ IDevelopmentVersionInfo::getMajor(SizeT* major)
+ IDevelopmentVersionInfo::getMinor(SizeT* minor)
+ IDevelopmentVersionInfo::getPatch(SizeT* patch)
+ IDevelopmentVersionInfo::getTweak(SizeT* tweak)
+ IDevelopmentVersionInfo::getBranchName(IString** branchName)
+ IDevelopmentVersionInfo::getHashDigest(IString** hash)
```

#### `IDeviceUpdateOptions`
```diff
+ createDeviceUpdateOptions(IDeviceUpdateOptions** obj, IString* setupString)
+ IDeviceUpdateOptions::getLocalId(IString** localId)
+ IDeviceUpdateOptions::getManufacturer(IString** manufacturer)
+ IDeviceUpdateOptions::getSerialNumber(IString** serialNumber)
+ IDeviceUpdateOptions::getConnectionString(IString** connectionString)
+ IDeviceUpdateOptions::setNewManufacturer(IString* manufacturer)
+ IDeviceUpdateOptions::getNewManufacturer(IString** manufacturer)
+ IDeviceUpdateOptions::setNewSerialNumber(IString* serialNumber)
+ IDeviceUpdateOptions::getNewSerialNumber(IString** serialNumber)
+ IDeviceUpdateOptions::setNewConnectionString(IString* connectionString)
+ IDeviceUpdateOptions::getNewConnectionString(IString** connectionString)
+ IDeviceUpdateOptions::getUpdateMode(DeviceUpdateMode* mode)
+ IDeviceUpdateOptions::setUpdateMode(DeviceUpdateMode mode)
+ IDeviceUpdateOptions::getChildDeviceOptions(IList** childDeviceOptions)
```

### Removed interfaces

#### `IFunctionBlockWrapper`
```diff
- createFunctionBlockWrapper(IFunctionBlock** obj, IFunctionBlock* functionBlock, Bool includeInputPortsByDefault, Bool includeSignalsByDefault, Bool includePropertiesByDefault, Bool includeFunctionBlocksByDefault)
- IFunctionBlockWrapper::includeInputPort(IString* inputPortName)
- IFunctionBlockWrapper::excludeInputPort(IString* inputPortName)
- IFunctionBlockWrapper::includeSignal(IString* signalLocalId)
- IFunctionBlockWrapper::excludeSignal(IString* signalLocalId)
- IFunctionBlockWrapper::includeProperty(IString* propertyName)
- IFunctionBlockWrapper::excludeProperty(IString* propertyName)
- IFunctionBlockWrapper::includeFunctionBlock(IString* functionBlockLocalId)
- IFunctionBlockWrapper::excludeFunctionBlock(IString* functionBlockLocalId)
- IFunctionBlockWrapper::setPropertyCoercer(IString* propertyName, ICoercer* coercer)
- IFunctionBlockWrapper::setPropertyValidator(IString* propertyName, IValidator* validator)
- IFunctionBlockWrapper::setPropertySelectionValues(IString* propertyName, IList* enumValues)
- IFunctionBlockWrapper::getWrappedFunctionBlock(IFunctionBlock** functionBlock)
```

### Modified interfaces

#### `IProperty`
```diff
+ IProperty::getPropertyType(PropertyType* type)
```

#### `IPropertyBuilder`
```diff
+ IPropertyBuilder::getIsIntegerValueSelection(Bool* isIntegerValueSelection)
+ IPropertyBuilder::setIsIntegerValueSelection(Bool isIntegerValueSelection)
```

#### `IPropertyObject`
```diff
+ IPropertyObject::clearPropertyValues()
+ IPropertyObject::setPropertySelectionValue(IString* propertyName, IBaseObject* value)
```

#### `IPropertyObjectProtected`
```diff
+ IPropertyObjectProtected::clearProtectedPropertyValues()
+ IPropertyObjectProtected::setProtectedPropertySelectionValue(IString* propertyName, IBaseObject* value)
```

#### `IComponent`
```diff
+ IComponent::getLocalActive(Bool* localActive)
+ IComponent::getParentActive(Bool* parentActive)
```

#### `IComponentPrivate`
```diff
+ IComponentPrivate::setParentActive(Bool parentActive, Bool onUpdate)
```

#### `IComponentUpdateContext`
```diff
+ IComponentUpdateContext::addDeviceRemapping(IString* originalDeviceId, IString* newDeviceId)
+ IComponentUpdateContext::getDeviceUpdateOptionsWithLocalIdOrNull(IString* localId, IDeviceUpdateOptions** options)
+ IComponentUpdateContext::getInternalState(IDict** state)
+ IComponentUpdateContext::getUpdateParameters(IUpdateParameters** updateParameters)
+ IComponentUpdateContext::overrideState(IComponentUpdateContext* updateContext)
+ IComponentUpdateContext::remapInputPortConnections()
+ IComponentUpdateContext::setRootComponent(IComponent* baseComponent)
- IComponentUpdateContext::getReAddDevicesEnabled(Bool* enabled)
```

#### `IInputPort`
```diff
+ IInputPort::acceptsSignals(IList* signals, IList** accepts)
+ IInputPort::getPublic(Bool* isPublic)
+ IInputPort::setPublic(Bool isPublic)
```

#### `IInputPortConfig`
```diff
+ IInputPortConfig::getListener(IInputPortNotifications** port)
```

#### `IMirroredDevice`
```diff
+ IMirroredDevice::getMirroredDeviceType(IDeviceType** type)
```

#### `IMirroredDeviceConfig`
```diff
+ IMirroredDeviceConfig::setMirroredDeviceType(IDeviceType* type)
```

#### `IModuleAuthenticator`
```diff
+ IModuleAuthenticator::setLogger(ILogger* logger)
```

#### `IMultiReader`
```diff
+ IMultiReader::addInput(IComponent* input)
+ IMultiReader::getInputUsed(IString* id, Bool* isUsed)
+ IMultiReader::removeInput(IString* id)
+ IMultiReader::setInputUsed(IString* id, Bool isUsed)
```

#### `IServer`
```diff
+ IServer::disableDiscovery()
```

#### `IUpdateParameters`
```diff
+ IUpdateParameters::getDeviceUpdateOptions(IDeviceUpdateOptions** options)
+ IUpdateParameters::getRemoveUnusedDevices(Bool* remove)
+ IUpdateParameters::setDeviceUpdateOptions(IDeviceUpdateOptions* options)
+ IUpdateParameters::setRemoveUnusedDevices(Bool remove)
- IUpdateParameters::getReAddDevicesEnabled(Bool* enabled)
- IUpdateParameters::setReAddDevicesEnabled(Bool enabled)
```
