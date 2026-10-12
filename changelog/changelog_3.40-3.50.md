# Changes from 3.40 to 3.50

## Features

- [#1242](https://github.com/openDAQ/openDAQ/pull/1242) Implement IContext::getRootDevice
- [#1244](https://github.com/openDAQ/openDAQ/pull/1244) Static objects and object pool
- [#1258](https://github.com/openDAQ/openDAQ/pull/1258) Make device info the nested device property
- [#1262](https://github.com/openDAQ/openDAQ/pull/1262) Add protocol group ID and security level to server capabilities. Streaming protocols sharing a group ID are treated as variants of one another, so only the most preferred source of each group is attached. A single server can now advertise multiple discovery services.
- [#1251](https://github.com/openDAQ/openDAQ/pull/1251) TLS encrypted channel for the LT streaming module. With `EnableTlsStreamingPort` the server serves the secure channel alongside the plaintext one, registering both the `OpenDAQLTStreaming` and `OpenDAQLTStreamingSecure` capabilities and advertising the `_streaming-lt._tcp` and `_streaming-lts._tcp` mDNS services. Mutual TLS is enabled by default. LT capabilities now carry the `LTStreaming` protocol group ID and a protocol security level (`0` plaintext / `10` secure), so a client ordering streaming protocols by security level prefers the secure channel on its own. The channel is opt-in: without `EnableTlsStreamingPort` the server behaves as before. It is opt-in at build time as well: see [#1278](https://github.com/openDAQ/openDAQ/pull/1278).
- [#1278](https://github.com/openDAQ/openDAQ/pull/1278) Building the TLS channel of the LT streaming modules is controlled by the new CMake option `OPENDAQ_ENABLE_WEBSOCKET_STREAMING_WITH_TLS`, off by default and on in the `full`, `package` and `simulator_package` presets. With it off the LT streaming modules are built without their OpenSSL dependency and the `daq.lts://` channel is absent. The option has no meaning for the legacy LT streaming modules (`DAQMODULES_LT_LEGACY_MODULES`) and is ignored there.
- [#1311](https://github.com/openDAQ/openDAQ/pull/1311) Add the `ScanOnAdd` option to skip the network scan on `addDevice`; tests wait for state instead of sleeping; declare `RESOURCE_LOCK` for parallel ctest; write a minidump on access violations.
- [#1322](https://github.com/openDAQ/openDAQ/pull/1322) Add the `AlwaysEmptyInput`, `Singleton` and `CommonSettingsTypeId` flags to function block types. A type can now declare that its blocks always keep a free input port, that at most one instance may exist under a single parent, and that its properties act as common settings for nested blocks of another type. The options are set through `IComponentTypeBuilder` and read back from `IFunctionBlockType`; they are serialized with the type, so they survive the config protocol. The options are also fields of the `FunctionBlockType` struct, so they are visible through `IStruct` and take part in equality: two types that differ only in their options no longer compare equal. The `Singleton` flag is enforced by the core: adding a function block of a singleton type to a device or function block that already holds an instance of that type fails with `OPENDAQ_ERR_ALREADYEXISTS`, and a duplicate found while loading a configuration is skipped with a warning.

## Python

## Bug fixes

- [#1322](https://github.com/openDAQ/openDAQ/pull/1322) `IList::equals` no longer dereferences a null element when only one of the two lists holds `nullptr` at a position. The comparison crashed or returned the right answer depending on operand order; it is now symmetric and reports the lists as not equal. Structs compare their field values as lists, so this also fixes crashes when comparing structs with unassigned fields, such as a device type without a connection string prefix.
- [#1326](https://github.com/openDAQ/openDAQ/pull/1326) Fix an exception in a native streaming handler aborting the process; it is logged instead
- [#1326](https://github.com/openDAQ/openDAQ/pull/1326) Fix adding a sub-device failing when a property object class, enumeration or struct type of the same name is already registered on the client
- [#1326](https://github.com/openDAQ/openDAQ/pull/1326) Native client logs a warning when a type received from the server differs from the local definition
- [#1340](https://github.com/openDAQ/openDAQ/pull/1340) Saving and loading a setup no longer stores or restores the device user lock
- [#1327](https://github.com/openDAQ/openDAQ/pull/1327) Fix events not being synchronized: `removeHandler` and `clear` now wait for a handler running on another thread, and handlers are called without the event lock held
- [#1336](https://github.com/openDAQ/openDAQ/pull/1336) Config protocol client logs a child component that fails to apply the server's update
- [#1308](https://github.com/openDAQ/openDAQ/pull/1308) Fixes a deadlock between mDNS query answering and server removal
- [#1305](https://github.com/openDAQ/openDAQ/pull/1305) Fix async races in native streaming shutdown process.
- [#1304](https://github.com/openDAQ/openDAQ/pull/1304) Fix flaky test NativeDeviceModulesTest.GetConnectedClientsInfo by periodically polling instead of racing asynchronous events.
- [#1303](https://github.com/openDAQ/openDAQ/pull/1303) Stop the mDNS discovery server's service thread before an instance releases its root device. A request the thread was answering through the device could keep the device tree alive past the instance's destruction and then destroy the context from the service thread itself, which joined its own thread and terminated the process.
- [#1299](https://github.com/openDAQ/openDAQ/pull/1299) Fix a deadlock when a scheduler worker releases the last reference to the context: the scheduler no longer waits on its own thread while destroying the executor. Such a shutdown could also crash at process exit, with Taskflow's node pool destroyed under live workers, which is what made `test_audio_device_module` fail intermittently on the gcc-7 32-bit CI lane.
- [#1296](https://github.com/openDAQ/openDAQ/pull/1296) Module libraries are loaded with MSVC debug heap tracking off, so debug test runs no longer report their statics as memory leaks.
- [#1295](https://github.com/openDAQ/openDAQ/pull/1295) Fix an intermittent deadlock between `setOperationModeRecursive` and a sub-device's acquisition thread. The device tree lock taken while the operation mode changes no longer locks signals and input ports.
- [#1309](https://github.com/openDAQ/openDAQ/pull/1309) No longer throws `NotFound` on transport status change before connection statuses are published.
- [#1321](https://github.com/openDAQ/openDAQ/pull/1321) Take the property object's lock while serializing its values and local properties, so serialization no longer reads containers that a concurrent property write is modifying.
- [#1324](https://github.com/openDAQ/openDAQ/pull/1324) `setVisible` returns `OPENDAQ_IGNORED` and fires no `AttributeChanged` when the value is unchanged, as `setActive`, `setName` and `setDescription` already do.

## Misc

- [#1323](https://github.com/openDAQ/openDAQ/pull/1323) Fix classifier FB flaky tests
- [#1313](https://github.com/openDAQ/openDAQ/pull/1313) Bump libNativeStreaming to v1.0.21 (precompiled headers and unity builds).
- [#1251](https://github.com/openDAQ/openDAQ/pull/1251), [#1269](https://github.com/openDAQ/openDAQ/pull/1269), [#1278](https://github.com/openDAQ/openDAQ/pull/1278) OpenSSL (>= 1.1.1) is a build dependency of the SDK when `OPENDAQ_ENABLE_WEBSOCKET_STREAMING_WITH_TLS` is on. Unlike most other dependencies it is not fetched automatically and has to be installed on the host system: `libssl-dev` on Debian/Ubuntu (`libssl-dev:i386` for 32-bit builds), `openssl-devel` on RHEL-based distributions. The build documentation and all CI, packaging and docs jobs were updated accordingly.
- [#1307](https://github.com/openDAQ/openDAQ/pull/1307) Precompiled headers and unity builds through opendaq-cmake-utils v1.1.0, on by default.
- [#1310](https://github.com/openDAQ/openDAQ/pull/1310) Provide `OPENDAQ_PACKAGE_VERSION` through a generated header. Use `/MP` for VS CI jobs.
- [#1265](https://github.com/openDAQ/openDAQ/pull/1265) Publish the SDK staging weekly. Add ARM Linux and manylinux CI runners and pin the compilers.

## Required application changes

### [#1327](https://github.com/openDAQ/openDAQ/pull/1327) Event handler removal waits for running handlers

`IEvent::removeHandler` and `IEvent::clear` now return only once the removed handlers are no longer running on another thread. Don't call them while holding a lock that such a handler needs, or the call waits forever.

`daq::RecursiveMutex` was removed from `coretypes/utility_sync.h`. Use `std::recursive_mutex` or `daq::mutex` instead.


## Required module changes

### [#1327](https://github.com/openDAQ/openDAQ/pull/1327) Event handler removal waits for running handlers

`IEvent::removeHandler` and `IEvent::clear` now return only once the removed handlers are no longer running on another thread. Don't call them while holding a lock that such a handler needs, or the call waits forever.

`daq::RecursiveMutex` was removed from `coretypes/utility_sync.h`. Use `std::recursive_mutex` or `daq::mutex` instead.

### [#1322](https://github.com/openDAQ/openDAQ/pull/1322) Function block type options

`IFunctionBlockType` and `IComponentTypeBuilder` each gained new methods. Interface IDs in openDAQ are derived
from the interface name alone, so a module built against an older SDK still passes `queryInterface` while its
vtable is missing the new slots. Modules must be rebuilt against this SDK version.

Modules that construct function block types through the `FunctionBlockType()` factory or `FunctionBlockTypeBuilder()`
need no source changes; the defaults (`False`, `False`, unassigned) preserve the previous behaviour. Any module that
implements `IFunctionBlockType` itself must add `getAlwaysEmptyInput`, `getSingleton` and `getCommonSettingsTypeId`.

The serialized form stays compatible in both directions: a newer SDK reading an older payload falls back to the
defaults, and an older SDK reading a newer payload ignores the three new keys. Note that an older peer therefore
drops the values silently - it will not enforce a `Singleton` constraint declared by a newer server.

`GenericComponentTypeImpl` now has a single constructor, `(type, id, name, description, defaultConfig, extraFields = nullptr)`.
The overload taking a connection string prefix was removed, and `prefix` is no longer a member of the base class; device
and streaming types store it themselves. A component type implementation deriving from `GenericComponentTypeImpl` must
pass any fields beyond `Id`, `Name` and `Description` through `extraFields`, in the order its struct type declares them:
```diff
- : Super(structType, id, name, description, prefix, defaultConfig)
+ : Super(structType, id, name, description, defaultConfig, Dict<IString, IBaseObject>({{"Field1", value1}, {"Field2", value2}}))
```
Passing `Id`, `Name` or `Description` in `extraFields` throws `InvalidParameterException`, and debug builds also throw `InvalidTypeException` when the resulting fields do not match the struct type's field names and order.

## Interface API changes

### New interfaces

### Removed interfaces

### Modified interfaces

#### `Context`
```diff
+ IContext::getRootDevice(IBaseObject** device);
```

#### `IContextInternal`
```diff
+ IContextInternal::setRootDevice(IBaseObject* device);
```

#### `IServerCapability`
```diff
+ IServerCapability::getProtocolGroupId(IString** protocolGroupId);
+ IServerCapability::getProtocolSecurityLevel(IInteger** securityLevel);
```

#### `IServerCapabilityConfig`
```diff
+ IServerCapabilityConfig::setProtocolGroupId(IString* protocolGroupId);
+ IServerCapabilityConfig::setProtocolSecurityLevel(IInteger* securityLevel);
```

#### `IStreaming`
```diff
+ IStreaming::getProtocolGroupId(IString** protocolGroupId);
```

#### `IFunctionBlockType`
```diff
+ IFunctionBlockType::getAlwaysEmptyInput(Bool* alwaysEmpty);
+ IFunctionBlockType::getSingleton(Bool* singleton);
+ IFunctionBlockType::getCommonSettingsTypeId(IString** typeId);
```

#### `IComponentTypeBuilder`
```diff
+ IComponentTypeBuilder::setAlwaysEmptyInput(Bool alwaysEmpty);
+ IComponentTypeBuilder::getAlwaysEmptyInput(Bool* alwaysEmpty);
+ IComponentTypeBuilder::setSingleton(Bool singleton);
+ IComponentTypeBuilder::getSingleton(Bool* singleton);
+ IComponentTypeBuilder::setCommonSettingsTypeId(IString* typeId);
+ IComponentTypeBuilder::getCommonSettingsTypeId(IString** typeId);
```
