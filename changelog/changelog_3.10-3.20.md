# Changes from 3.10 to 3.20

## Features

- [#789](https://github.com/openDAQ/openDAQ/pull/789) Optimize Signal Packet Handling by Deferring Last Value Calculation
- [#768](https://github.com/openDAQ/openDAQ/pull/768) Supporting creating empty error info. Add macroses to checking error code
- [#771](https://github.com/openDAQ/openDAQ/pull/771) Improvements to device operation mode, including changes to the Device API for operation mode handling.
- [#764](https://github.com/openDAQ/openDAQ/pull/764) Add support restoring device info fields
- [#773](https://github.com/openDAQ/openDAQ/pull/773) Extend IPropertyObject::hasProperty() to accept nested property lookup via "dot" notation.
- [#733](https://github.com/openDAQ/openDAQ/pull/733) Introduces serializer versioning; openDAQ list objects are now serialized as objects instead of JSON arrays.
- [#730](https://github.com/openDAQ/openDAQ/pull/730) Provide list of connected clients info via DeviceInfo.
- [#718](https://github.com/openDAQ/openDAQ/pull/718) Adds new Native Configuration Protocol RPCs for handling sub-function blocks (function blocks that are children of other FBs.
- [#642](https://github.com/openDAQ/openDAQ/pull/642) Introduces mechanisms to modify the IP configuration parameters of openDAQ-compatible devices.
- [#638](https://github.com/openDAQ/openDAQ/pull/638) Adds a tick tolerance option to the `MultiReader`, allowing for the limitation of inter-sample offsets between read signals.
- [#631](https://github.com/openDAQ/openDAQ/pull/631) "Any read/write" events are added to property object that are triggered whenever any of the object's property values are read or written.
- [#625](https://github.com/openDAQ/openDAQ/pull/625) [#650](https://github.com/openDAQ/openDAQ/pull/650) [#673](https://github.com/openDAQ/openDAQ/pull/673) [#687](https://github.com/openDAQ/openDAQ/pull/687) Introduce Component statuses ( `OK`, `Warning`, and `Error`) and allows for adding status messages.
- [#609](https://github.com/openDAQ/openDAQ/pull/609) Moves module information from `IModule` to a new interface called `IModuleInfo`.
- [#607](https://github.com/openDAQ/openDAQ/pull/607) [#700](https://github.com/openDAQ/openDAQ/pull/700) Introduces configurable Device Info Properties.
- [#606](https://github.com/openDAQ/openDAQ/pull/606) Mechanism for retrieving and monitoring the device's connection statuses, enabling tracking of streaming connections.
- [#605](https://github.com/openDAQ/openDAQ/pull/605) Add support for View Only client to the openDAQ Native configuration protocol.
- [#704](https://github.com/openDAQ/openDAQ/pull/704) Support setting operation mode for the device which notifies all sub components
- [#697](https://github.com/openDAQ/openDAQ/pull/697) [#783](https://github.com/openDAQ/openDAQ/pull/783) Added `IRecorder` interface and the `basic_csv_recorder_module`.
- [#743](https://github.com/openDAQ/openDAQ/pull/743) [#813](https://github.com/openDAQ/openDAQ/pull/813) Include experimental high-performance implementation of WebSocket Streaming Server; also adds CAN streaming capability to the client when used with the new server.

## Python

- [#809](https://github.com/openDAQ/openDAQ/pull/809) Improve Python GUI app: set focus on the opened window. close the window on escape. Fix freezeing the app on calling func property with the args
- [#729](https://github.com/openDAQ/openDAQ/pull/729) Improve Python GUI app: attributes dialog update, device info via generic properties treeview, metadata fields fix
- [#690](https://github.com/openDAQ/openDAQ/pull/690) Enable Python wheels for MacOS
- [#675](https://github.com/openDAQ/openDAQ/pull/675) Adds a command line option to specify an additional module load path when starting the Python GUI application.
- [#662](https://github.com/openDAQ/openDAQ/pull/662) Standardizes property modification mechanisms in Python GUI application.
- [#657](https://github.com/openDAQ/openDAQ/pull/657) Display time domain signal last value as time in Python GUI demo application.
- [#651](https://github.com/openDAQ/openDAQ/pull/651) Support for Python lists in MultiReader and how-to guides update.
- [#637](https://github.com/openDAQ/openDAQ/pull/637) Makes component attributes editable in the Python GUI demo application.
- [#634](https://github.com/openDAQ/openDAQ/pull/634) Support adding and removing nested function blocks in Python GUI.
- [#633](https://github.com/openDAQ/openDAQ/pull/633) Python bindings: `IEvent` interface implementation.
- [#617](https://github.com/openDAQ/openDAQ/pull/617) Enable Python wheels for Python 3.13
- [#596](https://github.com/openDAQ/openDAQ/pull/596) Adds a device info popup the the Python GUI application "Add device" dialog.

## Bug fixes

- [#823](https://github.com/openDAQ/openDAQ/pull/823) Fix property order is preserved from server to client via the native protocol
- [#821](https://github.com/openDAQ/openDAQ/pull/821) Fix CPP 20 and 23 compatibility issues
- [#816](https://github.com/openDAQ/openDAQ/pull/816) Fix registering the new property object class with the type manager on the native client.
- [#806](https://github.com/openDAQ/openDAQ/pull/806) Fix hidden components deserialization
- [#794](https://github.com/openDAQ/openDAQ/pull/794) Small refactor/fixing of child object cloning logic. Ensure thread safety on core event enabling.
- [#790](https://github.com/openDAQ/openDAQ/pull/790) Fix input port to signal notification
- [#778](https://github.com/openDAQ/openDAQ/pull/778) Fixing calling remote function with enumeration inside
- [#776](https://github.com/openDAQ/openDAQ/pull/776) Fixing restoring struct fields while loading configuration
- [#757](https://github.com/openDAQ/openDAQ/pull/757) Device info obtained from device and discovery matching is changed from checking for connection string equality to SN + manufacturer equality.
- [#753](https://github.com/openDAQ/openDAQ/pull/753) Fixes crash when deserializing struct types that have a type name that's present in the list of protected type names.
- [#756](https://github.com/openDAQ/openDAQ/pull/756) With CMake 4.0.0 the Windows builds would no longer find the test executables somehow.
- [#746](https://github.com/openDAQ/openDAQ/pull/746) Initialized IProcedure objects to default value: `nullptr`. Uninitialized objects cause potential use-before-init errors for certain compilers.
- [#754](https://github.com/openDAQ/openDAQ/pull/754) Treat duplicate OPC-UA properties, with names that differ only in case, as warnings instead of fatal errors.
- [#751](https://github.com/openDAQ/openDAQ/pull/751) Fix IPv6 addresses discovering on Windows, improve regex parsing of connection strings, fix LT pseudo-device IPv6 connection info
- [#744](https://github.com/openDAQ/openDAQ/pull/744) Fix leaf device streaming misconfiguration on reconnection
- [#740](https://github.com/openDAQ/openDAQ/pull/740) Fixes restoring connection signals to dynamic input ports of a function block while loading the configuration when the name of the new input does not match the old one.
- [#734](https://github.com/openDAQ/openDAQ/pull/734) Using component config while loading configuration for the devices and function blocks.
- [#733](https://github.com/openDAQ/openDAQ/pull/733) Fixes list/dictionary deserialization not containing key/value/item interface IDs. Requires server-side update.
- [#731](https://github.com/openDAQ/openDAQ/pull/731) Fixes nested object access over OPC UA. Object properties original PropertyObject is now stored in PropertyObjectImpl; PropertyImpl now contains the clone.
- [#719](https://github.com/openDAQ/openDAQ/pull/719) Fixes error when accessing selection property values using "dot" notation (eg. `getPropertySelectionValue("child.val")`).
- [#703](https://github.com/openDAQ/openDAQ/pull/703) Fixes invalid response of openDAQ mDNS wrapper for unicast queries.
- [#696](https://github.com/openDAQ/openDAQ/pull/696) Set of LT bugfixes. Keeping sessions alive, raw json values for linear rules, default start value for constant signals, fix IPv4/6 control connection address handling. 
- [#693](https://github.com/openDAQ/openDAQ/pull/693) Standard example openDAQ modules make use of non-installed openDAQ cmake utilities. Adds said utilities openDAQ packages.
- [#689](https://github.com/openDAQ/openDAQ/pull/689) Fixed an mDNS issue where multiple devices broadcasting with the same IP address were present, but only one could be detected by the client.
- [#685](https://github.com/openDAQ/openDAQ/pull/685) Fixes reconnection when an exclusive connection is established via the openDAQ Native protocol. Enables the native connection activity monitoring by default.
- [#682](https://github.com/openDAQ/openDAQ/pull/682) `IBlockReader::getAvailableCount` returns `count==1` when remaining sample count is smaller than the block size and the remaining samples are followed by an event.
- [#674](https://github.com/openDAQ/openDAQ/pull/674) System clock is no longer considered when calculating maximum resolution of signals read by the `MultiReader`.
- [#660](https://github.com/openDAQ/openDAQ/pull/660) The "Name" attribute was not serialized when equal to the local ID, causing issues when the localID of a deserialized signal was overridden to be different than the original.
- [#654](https://github.com/openDAQ/openDAQ/pull/654) Fix `MultiReader` race condition in connection queue access.
- [#645](https://github.com/openDAQ/openDAQ/pull/645) Fixes component status on reconnection. Adds component status update to the remote-update procedure on configuration client objects.
- [#641](https://github.com/openDAQ/openDAQ/pull/641) Fixes race condition. Status container dictionary returns copy of statuses instead of original.
- [#636](https://github.com/openDAQ/openDAQ/pull/636) Fixes Native reconnection issues.
- [#615](https://github.com/openDAQ/openDAQ/pull/615) Add missing component `active` flag serialization.
- [#590](https://github.com/openDAQ/openDAQ/pull/590) [#593](https://github.com/openDAQ/openDAQ/pull/593) Adds missing bcrypt installation and export rules.
- [#669](https://github.com/openDAQ/openDAQ/pull/659) Fix building openDAQ on android, by removing multiple coping of loaded library to the final vector in ModuleManager constructor.

## Misc

- [#758](https://github.com/openDAQ/openDAQ/pull/758) General update of the quick-start documentation for usage of .NET on Windows and Linux.
- [#742](https://github.com/openDAQ/openDAQ/pull/742) Assign a name to the scheduler, logger, and discovery threads.
- [#747](https://github.com/openDAQ/openDAQ/pull/747) Fixes CMake 4.0.0 compatibility, fixes ctutils library compilation yielding a Warning for lack of nullptr initialization, adds a CMake option to disable access control at compile time
- [#738](https://github.com/openDAQ/openDAQ/pull/738) When clearing a property value, the PropertyValueArgs object obtained in the onWrite callback now contains the default value instead of an empty ptr.
- [#728](https://github.com/openDAQ/openDAQ/pull/728) Add timeout on to re-scan after for available devices after 5s in the module manager call `createDevice`.
- [#725](https://github.com/openDAQ/openDAQ/pull/725) Optimizes packet reading in the openDAQ Native Streaming server. Adds method of dequeuing packets directly into preallocated packet list. 
- [#714](https://github.com/openDAQ/openDAQ/pull/714) Set of permission manager optimizations that reduce the number of Dictionary object creations on Property/PropertyObject construction.
- [#706](https://github.com/openDAQ/openDAQ/pull/706) Limit reference device channel count to min/max values.
- [#702](https://github.com/openDAQ/openDAQ/pull/702) Native streaming packet transmission performance optimizations.
- [#701](https://github.com/openDAQ/openDAQ/pull/701) Hide functions from usage in .NET which return an object reference without incremented reference-count.
- [#699](https://github.com/openDAQ/openDAQ/pull/699) Two new resolution options have been added to the renderer function block: 1920x1080 and 2560x1440.
- [#694](https://github.com/openDAQ/openDAQ/pull/694) Coretype INumber `queryInterface/borrowInterface` optimization.
- [#653](https://github.com/openDAQ/openDAQ/pull/653) Device info objects without server capabilities are no longer grouped by serial and manufacturer in the `getAvailableDevices` call. 
- [#635](https://github.com/openDAQ/openDAQ/pull/635) Before the openDAQ instance object is destroyed, it now calls `rootDevice.remove()` in its desctructor.
- [#630](https://github.com/openDAQ/openDAQ/pull/630) The internal `Device` implementation function `ongetLogFileInfos()` was renamed to `onGetLogFileInfos()`. 
- [#720](https://github.com/openDAQ/openDAQ/pull/720) Introduces additional performance optimizations for native streaming packet transmitting.
- [#723](https://github.com/openDAQ/openDAQ/pull/723) Data path optimizations: no heap allocations on `sendPacket()`, remove dynamic casts on packet reader, use acquisition lock instead of recursive config lock on input port active getter, optimize packet construction, support for manual last value.

## Required application changes

### [#731](https://github.com/openDAQ/openDAQ/pull/731)

`IPropertyObject::clearPropertyValue(IString* name)` when invoked on an Object-type property, no longer removes the property object and restores it to the default, but instead calls `clearPropertyValue` recursively on the object's properties.


```cpp
auto propObj1 = PropertyObject();
auto propObj2 = PropertyObject();

propObj1.addProperty(ObjectProperty("Child", propObj2));

auto propObj3 = PropertyObject();
propObj3.addProperty(IntProperty("IntProp", 0));
propObj3.setPropertyValue("IntProp", 1);
propObj1.asPtr<IPropertyObjectProtected>().setProtectedPropertyValue("Child", propObj3);

// The below code does not restore "Child" to `propObj2`.
// It instead sets "Child.IntProp" to its default value of "1".
propObj1.clearPropertyValue("Child");
```

### [#609](https://github.com/openDAQ/openDAQ/pull/609) Relocated module info functions

The following no longer works:
```cpp
ModulePtr module;

module.getId();
module.getName();
module.getVersionInfo();
```

And should be changed to:

```cpp
ModulePtr module;
ModuleInfoPtr info = module.getModuleInfo()

info.getId();
info.getName();
info.getVersionInfo();
```

### [#607](https://github.com/openDAQ/openDAQ/pull/607) Removed location and userName properties when not changeable

- The "location" and "userName" properties no longer appear on devices on which said properties are not changeable. 
- Applications should first check whether those properties exist before accessing them, or access them through the "DeviceInfo" object as is the case for other properties.

```cpp
// Option 1:
if (device.hasProperty("location"))
    device.setPropertyValue("location", "new_location");

// Option 2:
auto locationProp = device.getInfo().getProperty("location");
if (!location.getReadOnly())
    location.setValue("new_location");
```

### [#606](https://github.com/openDAQ/openDAQ/pull/606) New mechanism for connection status checking

Old method of retrieving configuration status still works, but is deprecated:

```cpp
EnumerationPtr status = instance.getDevices()[0].getStatusContainer().getStatus("ConnectionStatus");
```

New mechanism example:

```cpp
EnumerationPtr status = instance.getDevices()[0].getConnectionStatusContainer().getStatus("ConfigurationStatus");
```

## Required module changes

### [#700](https://github.com/openDAQ/openDAQ/pull/700) `IDeviceInfoConfig` setters no longer have protected write access

- `IDeviceInfoConfig` setters no longer have protected access once `DeviceInfo` has an owner. 
- To change the value of a read-only device info property `IPropertyObject::setProtectedPropertyValue` must be used.
- The old setters can still be used as before when creating the `DeviceInfo` object.

Example required changes:

```cpp
// Manufacturer is a read-only device info property
// The following now fails
device.getInfo().asPtr<IDeviceInfoConfig>().setManufacturer("openDAQ");

// Works, but can only be done by the device module/implementation itself, not by clients.
device.getInfo().asPtr<IPropertyObjectProtected>().setProtectedPropertyValue("manufacturer", "openDAQ"); 
```

The following still works:

```cpp
DeviceInfoPtr DeviceImpl::onGetInfo()
{
    auto info = DeviceInfo(connStr);
    info.setManufacturer("openDAQ"); // The `info` object does not yet have an owner
    return info;
}
```

### [#642](https://github.com/openDAQ/openDAQ/pull/642) Client module `discoveryClient` initialization rework

Updates the usage of the openDAQ discovery client, requiring the changes in the initialization of the discovery client
and the implementation of `onGetAvailableDevices` in client modules:

- The discovery client does not anymore accept callbacks for build the `ServerCapability` as first parameter of its constructor.
- The return type of `DiscoveryClient::discoverMdnsDevices()` changed from `ListPtr<IDeviceInfo>` to `std::vector<MdnsDiscoveredDevice>`

See [#642](https://github.com/openDAQ/openDAQ/pull/642) for a full example on how to update client modules.

### [#630](https://github.com/openDAQ/openDAQ/pull/630) `ongetLogFileInfos` capitalization fix

- The `ongetLogFileInfos` function override must be renamed to `onGetLogFileInfos`.
- Yes, we know that "info" is uncountable.

### [#609](https://github.com/openDAQ/openDAQ/pull/609) `IComponentType` include path change

Include target for component_type.h was changed:

```diff
- #include <coreobjects/component_type_ptr.h>
+ #include <opendaq/component_type_ptr.h>
```

### [#607](https://github.com/openDAQ/openDAQ/pull/607) `location` and `userName` must be explicitly defined as changeable

A device must now explicitly define the "location" and "userName" to be configurable when constructing its DeviceInfo object:

```cpp
auto info = DeviceInfoWithChanegableFields({"userName", "location"});
```

### [#606](https://github.com/openDAQ/openDAQ/pull/606) Configuration connection status publishing

If the device's implementation previously managed configuration connection status as follows:

```cpp
// ... to initialize status:
this->statusContainer.asPtr<IComponentStatusContainerPrivate>().addStatus("ConnectionStatus", statusInitValue);
// ... and to update status:
this->statusContainer.asPtr<IComponentStatusContainerPrivate>().setStatus("ConnectionStatus", value);
```

It now need to be extended to include the new mechanism:

```cpp
// ... to initialize status:
this->connectionStatusContainer.addConfigurationConnectionStatus(connectionString, statusInitValue);
// ... and to update status:
this->connectionStatusContainer.updateConnectionStatus(deviceInfo.getConnectionString(), value, nullptr);
```

Following methods of base streaming implementation moved from protected to private scope:

```cpp
    void startReconnection();
    void completeReconnection();
```

### [#730](https://github.com/openDAQ/openDAQ/pull/730) Provide list of connected clients info via DeviceInfo.

Updates the openDAQ discovery client API and base Module implementation, providing helper method populateDiscoveredDeviceInfo to create discovered DeviceInfo and populate its properties.

Therefore, the following module implementation snippet should be modified as follows:

```diff
DeviceInfoPtr ExampleClientModule::populateDiscoveredDevice(const MdnsDiscoveredDevice& discoveredDevice)
{
-   PropertyObjectPtr deviceInfo = DeviceInfo("");
-   DiscoveryClient::populateDiscoveredInfoProperties(deviceInfo, discoveredDevice);

    auto cap = ServerCapability(...);
    ...

-   deviceInfo.asPtr<IDeviceInfoInternal>().addServerCapability(cap);
-   deviceInfo.asPtr<IPropertyObjectProtected>().setProtectedPropertyValue("connectionString", cap.getConnectionString());
-   deviceInfo.asPtr<IDeviceInfoConfig>().setDeviceType(createDeviceType());

-   return deviceInfo;

+   return populateDiscoveredDeviceInfo(DiscoveryClient::populateDiscoveredInfoProperties, discoveredDevice, cap, createDeviceType());
}
```

### [#734](https://github.com/openDAQ/openDAQ/pull/734) Using component config while loading configuration for the devices and function blocks.

Introduces new methods on IComponentPrivate interface to manage component initialization config PropertyObject and deprecates similar methods from IDevicePrivate.

Therefore, the getting device config is to be modified as follows:

```diff
    auto device = instance.getDevices()[0];
-   PropertyObjectPtr config = device.asPtr<IDevicePrivate>(true).getDeviceConfig();
+   PropertyObjectPtr config = device.asPtr<IComponentPrivate>(true).getComponentConfig();
```