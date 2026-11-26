# Changes from 3.40 to 3.50

## Features

- [#1045](https://github.com/openDAQ/openDAQ/pull/1045) Implement ISynchronization and ISyncInterface
- [#1242](https://github.com/openDAQ/openDAQ/pull/1242) Implement IContext::getRootDevice
- [#1244](https://github.com/openDAQ/openDAQ/pull/1244) Static objects and object pool
- [#1258](https://github.com/openDAQ/openDAQ/pull/1258) Make device info the nested device property
- [#1262](https://github.com/openDAQ/openDAQ/pull/1262) Add protocol group ID and security level to server capabilities. Streaming protocols sharing a group ID are treated as variants of one another, so only the most preferred source of each group is attached. A single server can now advertise multiple discovery services.
- [#1251](https://github.com/openDAQ/openDAQ/pull/1251) TLS encrypted channel for the LT streaming module. With `EnableTlsStreamingPort` the server serves the secure channel alongside the plaintext one, registering both the `OpenDAQLTStreaming` and `OpenDAQLTStreamingSecure` capabilities and advertising the `_streaming-lt._tcp` and `_streaming-lts._tcp` mDNS services. Mutual TLS is enabled by default. LT capabilities now carry the `LTStreaming` protocol group ID and a protocol security level (`0` plaintext / `10` secure), so a client ordering streaming protocols by security level prefers the secure channel on its own. The channel is opt-in: without `EnableTlsStreamingPort` the server behaves as before. It is opt-in at build time as well: see [#1278](https://github.com/openDAQ/openDAQ/pull/1278).
- [#1278](https://github.com/openDAQ/openDAQ/pull/1278) Building the TLS channel of the LT streaming modules is controlled by the new CMake option `OPENDAQ_ENABLE_WEBSOCKET_STREAMING_WITH_TLS`, off by default and on in the `full`, `package` and `simulator_package` presets. With it off the LT streaming modules are built without their OpenSSL dependency and the `daq.lts://` channel is absent. The option has no meaning for the legacy LT streaming modules (`DAQMODULES_LT_LEGACY_MODULES`) and is ignored there.
- [#1311](https://github.com/openDAQ/openDAQ/pull/1311) Add the `ScanOnAdd` option to skip the network scan on `addDevice`; tests wait for state instead of sleeping; declare `RESOURCE_LOCK` for parallel ctest; write a minidump on access violations.
- [#1361](https://github.com/openDAQ/openDAQ/pull/1361) Device types declare a kind with `setDeviceKind` on the device type builder. Root types can only be the root device, and Adapter and Mirrored types only sub-devices. Module load logs a message for each device type without a kind, and later releases raise its level.
- [#1360](https://github.com/openDAQ/openDAQ/pull/1360) Add `FunctionModuleAuthenticator`, a module authenticator that forwards `authenticateModuleBinary` to a function, so a custom authenticator can be written in any binding. The function returns a dictionary with the mandatory `BinaryValid` and `VendorKey` keys. Available in C, Python and .NET.
- [#1372](https://github.com/openDAQ/openDAQ/pull/1372) Ping discovered IPv6 addresses as well as IPv4 ones, without root or administrator rights where the operating system allows it.
- [#1389](https://github.com/openDAQ/openDAQ/pull/1389) Device, Function Block and Servers send their `ComponentType` objects over native. Server types have a new field `ServerKind`
- [#1389](https://github.com/openDAQ/openDAQ/pull/1389) Servers can own function blocks - they can be added/removed by applications.
- [#1398](https://github.com/openDAQ/openDAQ/pull/1398) `createAddDeviceConfig` picks the default device type from the device's capabilities: native configuration first, else the first Mirrored type. The default streaming type is the one linked to the device type, else native streaming, else the first one the device advertises. `addDevice` without a config uses the same defaults.
- [#1401](https://github.com/openDAQ/openDAQ/pull/1401) Devices start in the Operation mode by default.
- [#1322](https://github.com/openDAQ/openDAQ/pull/1322) Add the `InputPolicy`, `Singleton` and `CommonSettingsTypeId` options to function block types: how their blocks create input ports, at most one instance per parent (enforced by the core), and common settings for nested blocks of another type.
- [#1407](https://github.com/openDAQ/openDAQ/pull/1407) `createAddDeviceConfig` works on a remote device. A server at protocol 26 or later builds the config. For an older server, the client builds it from the server's device types.

## Python

- [#1322](https://github.com/openDAQ/openDAQ/pull/1322) Expose the function block type options: the `FunctionBlockInputPolicy` enum and the read-only `IFunctionBlockType` properties `input_policy`, `singleton` and `common_settings_type_id`. `common_settings_type_id` is `None` when the type does not declare a common settings relationship. The options can only be read from Python, as the component type builder has no Python binding.

## Bug fixes

- [#1322](https://github.com/openDAQ/openDAQ/pull/1322) `IList::equals` no longer dereferences a null element when only one of the two lists holds `nullptr` at a position. The comparison crashed or returned the right answer depending on operand order; it is now symmetric and reports the lists as not equal. Structs compare their field values as lists, so this also fixes crashes when comparing structs with unassigned fields, such as a device type without a connection string prefix.
- [#1399](https://github.com/openDAQ/openDAQ/pull/1399) Harden the native server's connection accept path against slow, silent or malformed clients
- [#1349](https://github.com/openDAQ/openDAQ/pull/1349) Property getters take the object's lock, and nested property objects share their owner's lock, fixing crashes when properties are read while another thread adds, removes or reorders them
- [#1326](https://github.com/openDAQ/openDAQ/pull/1326) Fix an exception in a native streaming handler aborting the process; it is logged instead
- [#1326](https://github.com/openDAQ/openDAQ/pull/1326) Fix adding a sub-device failing when a property object class, enumeration or struct type of the same name is already registered on the client
- [#1326](https://github.com/openDAQ/openDAQ/pull/1326) Native client logs a warning when a type received from the server differs from the local definition
- [#1340](https://github.com/openDAQ/openDAQ/pull/1340) Saving and loading a setup no longer stores or restores the device user lock
- [#1327](https://github.com/openDAQ/openDAQ/pull/1327) Fix events not being synchronized: `removeHandler` and `clear` now wait for a handler running on another thread, and handlers are called without the event lock held
- [#1336](https://github.com/openDAQ/openDAQ/pull/1336) Config protocol client logs a child component that fails to apply the server's update
- [#1308](https://github.com/openDAQ/openDAQ/pull/1308) Fixes a deadlock between mDNS query answering and server removal
- [#1352](https://github.com/openDAQ/openDAQ/pull/1352) Fix process termination on mirrored signal teardown after a failed subscribe: a mirrored signal is marked as listened only once its subscribe succeeded, a failed teardown unsubscribe is logged instead of thrown, `ComponentImpl::remove` returns an error code instead of letting exceptions escape, and reader destructors no longer throw
- [#1352](https://github.com/openDAQ/openDAQ/pull/1352) A streaming unsubscribes the value signal even when its domain signal is no longer in the streaming, so the value signal does not stay subscribed after its reader is gone
- [#1305](https://github.com/openDAQ/openDAQ/pull/1305) Fix async races in native streaming shutdown process.
- [#1381](https://github.com/openDAQ/openDAQ/pull/1381) The native streaming client replaces its streaming and config callbacks as one set and runs every call on a copy, so replacing or resetting them no longer destroys a callback while it runs.
- [#1380](https://github.com/openDAQ/openDAQ/pull/1380) Raise the default native inactivity timeout from 1.5 s to 3 s, so a connection with the default 1 s heartbeat survives 2-3 missed heartbeats instead of 1.
- [#1304](https://github.com/openDAQ/openDAQ/pull/1304) Fix flaky test NativeDeviceModulesTest.GetConnectedClientsInfo by periodically polling instead of racing asynchronous events.
- [#1303](https://github.com/openDAQ/openDAQ/pull/1303) Stop the mDNS discovery server's service thread before an instance releases its root device. A request the thread was answering through the device could keep the device tree alive past the instance's destruction and then destroy the context from the service thread itself, which joined its own thread and terminated the process.
- [#1299](https://github.com/openDAQ/openDAQ/pull/1299) Fix a deadlock when a scheduler worker releases the last reference to the context: the scheduler no longer waits on its own thread while destroying the executor. Such a shutdown could also crash at process exit, with Taskflow's node pool destroyed under live workers, which is what made `test_audio_device_module` fail intermittently on the gcc-7 32-bit CI lane.
- [#1296](https://github.com/openDAQ/openDAQ/pull/1296) Module libraries are loaded with MSVC debug heap tracking off, so debug test runs no longer report their statics as memory leaks.
- [#1295](https://github.com/openDAQ/openDAQ/pull/1295) Fix an intermittent deadlock between `setOperationModeRecursive` and a sub-device's acquisition thread. The device tree lock taken while the operation mode changes no longer locks signals and input ports.
- [#1309](https://github.com/openDAQ/openDAQ/pull/1309) No longer throws `NotFound` on transport status change before connection statuses are published.
- [#1321](https://github.com/openDAQ/openDAQ/pull/1321) Take the property object's lock while serializing its values and local properties, so serialization no longer reads containers that a concurrent property write is modifying.
- [#1324](https://github.com/openDAQ/openDAQ/pull/1324) `setVisible` returns `OPENDAQ_IGNORED` and fires no `AttributeChanged` when the value is unchanged, as `setActive`, `setName` and `setDescription` already do.
- [#1350](https://github.com/openDAQ/openDAQ/pull/1350) The client no longer sends requests while applying a server update.
- [#1366](https://github.com/openDAQ/openDAQ/pull/1366) Components send their locked attributes to native configuration clients, so a client shows the same locked attributes as the device.
- [#1374](https://github.com/openDAQ/openDAQ/pull/1374) Error messages list the error-info chain from the newest error down to the root cause in every build, capped at 4 KiB, so release builds no longer hide the root cause behind added context.
- [#1374](https://github.com/openDAQ/openDAQ/pull/1374) An exception message containing `{` or `}` no longer turns into an `fmt::format_error`.
- [#1367](https://github.com/openDAQ/openDAQ/pull/1367) Fix a crash when a device's info or domain, or a component's name, description or visibility, is read while the config client applies a server update or a domain change.
- [#1387](https://github.com/openDAQ/openDAQ/pull/1387) Fix a process abort when a formatted error message is longer than 1024 bytes or its format string is malformed.
- [#1385](https://github.com/openDAQ/openDAQ/pull/1385) `JsonSerializedList::readInt` reads 64-bit integers instead of failing on values above the 32-bit range, and `readSerializedObject`, `readList` and `readString` move past a `null` item instead of returning it again on every call. Code that stepped over such a `null` with an extra `readObject` call has to drop that call.
- [#1402](https://github.com/openDAQ/openDAQ/pull/1402) Fix a deadlock in the native configuration server between sending core events and threads that change the device, and client calls that stalled until the request timeout when a server event arrived.
- [#1402](https://github.com/openDAQ/openDAQ/pull/1402) Fix a crash of the native streaming pseudo device when it is removed while its signals change.
- [#1402](https://github.com/openDAQ/openDAQ/pull/1402) Fix the property type that a client reports for properties with `onSelectionValuesRead` listeners.
- [#1386](https://github.com/openDAQ/openDAQ/pull/1386) Fix a hang when a native streaming client is destroyed: its transport thread is drained instead of stopped, and a connection attempt still in flight is closed (libNativeStreaming v1.0.23), so removing a device that is connecting or reconnecting to a host that does not answer no longer blocks.
- [#1386](https://github.com/openDAQ/openDAQ/pull/1386) The native inactivity timeout runs from the moment connection activity monitoring starts, so a peer that never sends anything after connecting is timed out like any other silent peer. Before, the timeout started with the first received frame.

## Misc

- [#1376](https://github.com/openDAQ/openDAQ/pull/1376) Fix the package build on macOS runners with OpenSSL 4, and when Apache's mirrors fail to serve Thrift
- [#1339](https://github.com/openDAQ/openDAQ/pull/1339) Publish the core stagings tagged by the run that published them, with the revision on every package.
- [#1343](https://github.com/openDAQ/openDAQ/pull/1343) Test the oldest and newest supported compilers on the newest GitHub runners (Visual Studio 2026, Ubuntu 26.04, macOS 27).
- [#1323](https://github.com/openDAQ/openDAQ/pull/1323) Fix classifier FB flaky tests
- [#1313](https://github.com/openDAQ/openDAQ/pull/1313) Bump libNativeStreaming to v1.0.21 (precompiled headers and unity builds).
- [#1251](https://github.com/openDAQ/openDAQ/pull/1251), [#1269](https://github.com/openDAQ/openDAQ/pull/1269), [#1278](https://github.com/openDAQ/openDAQ/pull/1278) OpenSSL (>= 1.1.1) is a build dependency of the SDK when `OPENDAQ_ENABLE_WEBSOCKET_STREAMING_WITH_TLS` is on. Unlike most other dependencies it is not fetched automatically and has to be installed on the host system: `libssl-dev` on Debian/Ubuntu (`libssl-dev:i386` for 32-bit builds), `openssl-devel` on RHEL-based distributions. The build documentation and all CI, packaging and docs jobs were updated accordingly.
- [#1307](https://github.com/openDAQ/openDAQ/pull/1307) Precompiled headers and unity builds through opendaq-cmake-utils v1.1.0, on by default.
- [#1310](https://github.com/openDAQ/openDAQ/pull/1310) Provide `OPENDAQ_PACKAGE_VERSION` through a generated header. Use `/MP` for VS CI jobs.
- [#1265](https://github.com/openDAQ/openDAQ/pull/1265) Publish the SDK staging weekly. Add ARM Linux and manylinux CI runners and pin the compilers.
- [#1318](https://github.com/openDAQ/openDAQ/pull/1318) Runs the discovery-free device module tests as four gtest shards on separate ports so ctest can run them in parallel.
- [#1342](https://github.com/openDAQ/openDAQ/pull/1342) Skip the macOS CI tests only on macOS instead of on every clang build.
- [#1368](https://github.com/openDAQ/openDAQ/pull/1368) The native streaming server's read loop no longer looks up every subscribed signal by global ID on each polling cycle, lowering its CPU load with many subscribed signals.
- [#1386](https://github.com/openDAQ/openDAQ/pull/1386) Reduce the native streaming client's cost per received packet: a packet buffer is received into one allocation, received packets reach the processing thread in bursts, and session inactivity is checked from a timestamp instead of re-arming a timer on every read.
- [#1386](https://github.com/openDAQ/openDAQ/pull/1386) Bump libNativeStreaming to v1.0.23.

## Required application changes

### [#1361](https://github.com/openDAQ/openDAQ/pull/1361) Device types limit where a device can be added

`setRootDevice` and `createDevice` without a parent reject Adapter and Mirrored device types, and `addDevice` rejects Root types, with `OPENDAQ_ERR_INVALIDPARAMETER`.
Setting the reference device as the root with `daqref://` now fails, so existing code and config files that do this break.
Use `daqref.root://` instead, also in `RootDevice.ConnectionString` of config files.

### [#1398](https://github.com/openDAQ/openDAQ/pull/1398) `General.PrimaryAddressType` is a preference

A set `General.PrimaryAddressType` orders the addresses, the preferred type first, instead of dropping those of the other type.
A reachable address comes before an unreachable one of the preferred type.

### [#1398](https://github.com/openDAQ/openDAQ/pull/1398) `daq://` prefers IPv4 addresses

A blank `General.PrimaryAddressType`, the default, now prefers IPv4 on `daq://`, also for an add without a config.
Before, `daq://` connected to the first address the device lists, which could be IPv6.

### [#1398](https://github.com/openDAQ/openDAQ/pull/1398) A legacy config connects another allowed streaming protocol

`General.PrioritizedStreamingProtocols` defaults to `OpenDAQNativeStreaming` alone.
When no prioritized protocol is loaded and allowed, `addDevice` picks a protocol with the new streaming protocol selection logic. This can lead to an existing `addDevice` call producing a device with attached streaming, where it had none before.
Set `General.AutomaticallyConnectStreaming` to false to add a device without streaming.

### [#1398](https://github.com/openDAQ/openDAQ/pull/1398) An add without a config uses the `MinHops` streaming mode

`addDevice` without a config now streams in `MinHops` mode instead of `MinConnections`, so nested devices connect their own streaming where they can.
Pass a config with `MinConnections` to keep the old behaviour.

### [#1327](https://github.com/openDAQ/openDAQ/pull/1327) Event handler removal waits for running handlers

`IEvent::removeHandler` and `IEvent::clear` now return only once the removed handlers are no longer running on another thread. Don't call them while holding a lock that such a handler needs, or the call waits forever.

`daq::RecursiveMutex` was removed from `coretypes/utility_sync.h`. Use `std::recursive_mutex` or `daq::mutex` instead.

### [#1349](https://github.com/openDAQ/openDAQ/pull/1349) Property getters take the object's lock

`getProperty`, `hasProperty`, `getAllProperties`, `getVisibleProperties` and `clone` now take the object's recursive lock, so they wait while another thread holds it (an `addDevice` on the parent, a slow write handler).

Nested property objects share their owner's lock, so a handler on a nested object runs under the owning component's lock. To keep a separate lock, set `LockingStrategy::OwnLock` before the object is assigned; clones keep the strategy.

### [#1360](https://github.com/openDAQ/openDAQ/pull/1360) Module authenticator errors fail module loading

The module manager no longer ignores the error code returned by `IModuleAuthenticator::authenticateModuleBinary`. A failure code aborts loading of the module and is returned to the caller with the authenticator's error message.

### [#1045](https://github.com/openDAQ/openDAQ/pull/1045) ISynchronization replaces ISyncComponent

- `UsesOffset`, `IReferenceDomainInfo::getUsesOffset` and `IReferenceDomainInfoBuilder::setUsesOffset`/`getUsesOffset` are removed in C++, C, Python and .NET; `getReferenceDomainIds` lists the reference domains instead.
- The reference device reports the reference domain ID `local:openDAQ_<serial>` instead of `openDAQ_<serial>`, the simulator device `local:<localId>` instead of `<localId>`, and both report the time protocol `Utc`.
- The reference device's deprecated `SyncComponent` holds no interfaces; `IDevice::getSynchronization` replaces it.
- Calls to `getSyncComponent` and the `SyncComponentPtr` methods produce deprecation warnings, which fail builds that treat warnings as errors.
- Python: the `opendaq.SyncComponent` argument `parse_failed_exception` is renamed to `parent`.
- Clients built before this change cannot add a device that exposes `ISynchronization`, which includes the reference and simulator devices.

### [#1389](https://github.com/openDAQ/openDAQ/pull/1389) Loading a setup restores existing servers

`loadConfiguration` now restores the saved attributes, properties and function blocks of servers that already exist; it does not add or remove servers. Setups saved by earlier versions already contain the server settings, so loading them now applies those settings too.

### [#1407](https://github.com/openDAQ/openDAQ/pull/1407) A remote device's add config is an `IAddDeviceConfig`

On a client connected with protocol 26 or later, `IComponentPrivate::getComponentConfig` of a remote device that its server added through the module manager returns an `IAddDeviceConfig`, not the old three-section config.
Read its values through the `IAddDeviceConfig` getters instead of property names.

## Required module changes

### [#1352](https://github.com/openDAQ/openDAQ/pull/1352) A public value signal cannot have a private domain signal

`setDomainSignal` returns `OPENDAQ_ERR_INVALIDPARAMETER` when the signal is public and the domain signal is private, and `setPublic` returns it for either change that would create that pair. Make the domain signal public, or the value signal private. Such value signals could not subscribe over native streaming before this change; a client connected to an older server that still serves the pair subscribes the value signal alone and logs a warning.

### [#1327](https://github.com/openDAQ/openDAQ/pull/1327) Event handler removal waits for running handlers

`IEvent::removeHandler` and `IEvent::clear` now return only once the removed handlers are no longer running on another thread. Don't call them while holding a lock that such a handler needs, or the call waits forever.

`daq::RecursiveMutex` was removed from `coretypes/utility_sync.h`. Use `std::recursive_mutex` or `daq::mutex` instead.

### [#1349](https://github.com/openDAQ/openDAQ/pull/1349) Property getters take the object's lock

`getProperty`, `hasProperty`, `getAllProperties`, `getVisibleProperties` and `clone` now take the object's recursive lock. Under the non-recursive `getLockGuard` use the `NoLock` variants. Under another mutex, such as the acquisition lock, they take the property lock second, which inverts the order of a write handler that takes that mutex under the property lock; read property metadata before taking the acquisition lock.

Nested property objects share their owner's lock, so a handler on a nested object runs under the owning component's lock. To keep a separate lock, set `LockingStrategy::OwnLock` before the object is assigned; clones keep the strategy.

### [#1366](https://github.com/openDAQ/openDAQ/pull/1366) Lock component attributes during construction

Lock attributes when the component is constructed. After that, unlock an attribute only briefly to apply an internal change, and lock it again right after. Locking or unlocking does not trigger a core event, so locks changed later do not reach connected clients.

### [#1367](https://github.com/openDAQ/openDAQ/pull/1367) Component and device getters take the component's lock

`getName`, `getDescription`, `getVisible`, `IDevice::getInfo` and `IDevice::getDomain` now take the component's recursive lock. Don't call them on the same component while holding `getAcquisitionLock` or `getUniqueLock`: those lock the same mutex without the re-entry check, and the call blocks forever.

### [#1045](https://github.com/openDAQ/openDAQ/pull/1045) ISynchronization replaces ISyncComponent

- `IDevice` implementations not derived from `GenericDevice` implement `getSynchronization`.
- Modules that set `UsesOffset` remove it and set `ReferenceDomainIds`.
- `IReferenceDomainInfo::getReferenceDomainIds` takes the interface slot of the removed `getUsesOffset`; modules built against older headers must be rebuilt.

### [#1398](https://github.com/openDAQ/openDAQ/pull/1398) A device stores its add config as an `IAddDeviceConfig`

`IComponentPrivate::getComponentConfig` of a device added through the module manager now returns an `IAddDeviceConfig`, not the old three-section config.
Read its values through the `IAddDeviceConfig` getters instead of property names.

### [#1401](https://github.com/openDAQ/openDAQ/pull/1401) Devices start in the Operation mode by default

A device that does not override `onGetAvailableOperationModes` lists `Operation` as its only mode instead of `SafeOperation`. A device that lists `Operation` and does not override `onGetDefaultOperationMode` starts in `Operation` instead of `SafeOperation`; override `onGetDefaultOperationMode` to keep starting in `SafeOperation`. A device that does not list `Operation` still starts in `SafeOperation`.

### [#1322](https://github.com/openDAQ/openDAQ/pull/1322) Function block type options

`IFunctionBlockType` and `IComponentTypeBuilder` each gained new methods. Interface IDs in openDAQ are derived
from the interface name alone, so a module built against an older SDK still passes `queryInterface` while its
vtable is missing the new slots. Modules must be rebuilt against this SDK version.

Modules that construct function block types through the `FunctionBlockType()` factory or `FunctionBlockTypeBuilder()`
need no source changes; the defaults (`Unspecified`, `False`, unassigned) preserve the previous behaviour. Any module that
implements `IFunctionBlockType` itself must add `getInputPolicy`, `getSingleton` and `getCommonSettingsTypeId`.

The serialized form stays compatible in both directions: a newer SDK reading an older payload falls back to the
defaults, and an older SDK reading a newer payload ignores the three new keys. Note that an older peer therefore
drops the values silently - it will not enforce a `Singleton` constraint declared by a newer server. The input policy is
serialized as an integer; a value this SDK version does not know is read as `Unspecified`.

Building a type with an input policy outside of `FunctionBlockInputPolicy` fails with `OPENDAQ_ERR_INVALIDPARAMETER`, and an
empty `CommonSettingsTypeId` is stored as unassigned. The options are also fields of the `FunctionBlockType` struct (the input
policy as an `Int`), so they are visible through `IStruct` and take part in equality: two types that differ only in their
options do not compare equal.

The `Singleton` flag is enforced by the function blocks folder of devices, servers and function blocks: adding a function block
of a singleton type to a folder that already holds an instance of that type fails with `OPENDAQ_ERR_ALREADYEXISTS`. The
check runs under the folder's lock together with the addition, so it covers concurrent additions, and function blocks
that modules add themselves with `addNestedFunctionBlock` or `createAndAddNestedFunctionBlock`. `addFunctionBlock` also
checks the flag before calling `onAddFunctionBlock`, so module code does not create an instance that would be refused.
Modules must create function block instances with the same type, including its flags, that they advertise through
`onGetAvailableFunctionBlockTypes`: the check reads the types of the instances, so an instance created with a separately
built type that lacks the flag is not counted. Loading a configuration that holds a second instance of a singleton type
skips that instance with a warning, also on devices reached through the config protocol.

## Interface API changes

### New interfaces

```diff
+ [interface] ISynchronization
+ [interface] ISynchronizationPrivate
+ [interface] ISyncInterface
+ [interface] ISyncInterfaceInternal
+ [enum] SyncMode, SyncSourceStatus, SyncRoleStatus
+ [enum] ServerKind
```

### Removed interfaces

### Modified interfaces

#### `IDevice`
```diff
+ IDevice::getSynchronization(ISynchronization** sync);
```

#### `IReferenceDomainInfo`
```diff
- IReferenceDomainInfo::getUsesOffset(UsesOffset* usesOffset);
+ IReferenceDomainInfo::getReferenceDomainIds(IList** referenceDomainIds);
```

#### `IReferenceDomainInfoBuilder`
```diff
- IReferenceDomainInfoBuilder::setUsesOffset(UsesOffset usesOffset);
- IReferenceDomainInfoBuilder::getUsesOffset(UsesOffset* usesOffset);
+ IReferenceDomainInfoBuilder::setReferenceDomainIds(IList* referenceDomainIds);
+ IReferenceDomainInfoBuilder::getReferenceDomainIds(IList** referenceDomainIds);
```

#### `Context`
```diff
+ IContext::getRootDevice(IBaseObject** device);
```

#### `IContextInternal`
```diff
+ IContextInternal::setRootDevice(IBaseObject* device);
```

#### `IPropertyObjectInternal`
```diff
+ IPropertyObjectInternal::getPropertyNoLock(IString* propertyName, IProperty** property);
+ IPropertyObjectInternal::getAllPropertiesNoLock(IList** properties);
+ IPropertyObjectInternal::hasPropertyNoLock(IString* propertyName, Bool* hasProperty);
+ IPropertyObjectInternal::getVisiblePropertiesNoLock(IList** properties);
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

#### `IServer`
```diff
+ IServer::getServerType(IServerType** type);
+ IServer::getFunctionBlocks(IList** functionBlocks, ISearchFilter* searchFilter = nullptr);
+ IServer::getAvailableFunctionBlockTypes(IDict** functionBlockTypes);
+ IServer::addFunctionBlock(IFunctionBlock** functionBlock, IString* typeId, IPropertyObject* config = nullptr);
+ IServer::removeFunctionBlock(IFunctionBlock* functionBlock);
```

#### `IServerType`
```diff
+ IServerType::getServerKind(ServerKind* kind);
```

#### `IComponentTypeBuilder`
```diff
+ IComponentTypeBuilder::setServerKind(ServerKind kind);
+ IComponentTypeBuilder::getServerKind(ServerKind* kind);
+ IComponentTypeBuilder::setInputPolicy(FunctionBlockInputPolicy policy);
+ IComponentTypeBuilder::getInputPolicy(FunctionBlockInputPolicy* policy);
+ IComponentTypeBuilder::setSingleton(Bool singleton);
+ IComponentTypeBuilder::getSingleton(Bool* singleton);
+ IComponentTypeBuilder::setCommonSettingsTypeId(IString* typeId);
+ IComponentTypeBuilder::getCommonSettingsTypeId(IString** typeId);
```

#### `IComponentTypePrivate`
```diff
+ IComponentTypePrivate::serializeWithoutDefaultConfig(ISerializer* serializer);
```

#### `IFunctionBlockType`
```diff
+ IFunctionBlockType::getInputPolicy(FunctionBlockInputPolicy* policy);
+ IFunctionBlockType::getSingleton(Bool* singleton);
+ IFunctionBlockType::getCommonSettingsTypeId(IString** typeId);
```
