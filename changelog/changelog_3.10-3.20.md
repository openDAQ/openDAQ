# 2025-02-04
## Description
- Enables status messages for connection statuses
- Introduces additional "Message" parameter of string-object-type within "ConnectionStatusChanged" core event arguments

## Required integration changes
- Breaks binary compatibility

## Example
To update existing connection status with status message:
```cpp
const auto typeManager = client.getDevices()[0].getContext().getTypeManager();
const StringPtr statusMessage = "Network connection interrupted or closed by the remote device";
const EnumerationPtr statusValue = Enumeration("ConnectionStatusType", "Reconnecting", typeManager);
const auto connectionStatusContainer = client.getDevices()[0].getConnectionStatusContainer();
connectionStatusContainer.asPtr<IConnectionStatusContainerPrivate>().updateConnectionStatusWithMessage(connectionString, statusValue, nullptr, statusMessage);
```
To get the status message of connection status:
```cpp
StringPtr message = client.getDevices()[0].getConnectionStatusContainer().getStatusMessage("ConfigurationStatus");
```

## API changes
```
+ [function] IConnectionStatusContainerPrivate::updateConnectionStatusWithMessage(IString* connectionString, IEnumeration* value, IStreaming* streamingObject, IString* message)
```

# 2025-02-04
## Description
- Fixed an mDNS issue where multiple devices broadcasting with the same IP address were present, but only one could be detected by the client.
## Required integration changes
- The device manufacturer and serial number are now used as identifiers in mDNS. These properties should create a unique name to prevent conflicts in the mDNS network. Otherwise, name conflicts could occur.

# 2025-01-24
## Description
- Introduces mechanisms to modify the IP configuration parameters of openDAQ-compatible devices.
The implementation follows the DNS-SD standard, utilizing resource records and mDNS header formats with custom header 
flags to facilitate the exchange of IP configuration requests and responses. The availability of the IP configuration 
modification feature for a device is advertised by discovering the _opendaq-ip-modification._udp.local. service on 
port 5353 using the standard mDNS discovery mechanism.
- Disables the Avahi daemon used for advertising the OPC UA service during discovery on the VBox device simulator, 
replacing it with the openDAQ internal mDNS server used for same purpose.

## Required integration changes
- Breaks binary compatibility
- Updates the usage of the openDAQ discovery client, requiring the changes in the initialization of the discovery client
and the implementation of onGetAvailableDevices in client modules.

```
-m [function] IContext::getDiscoveryServers(IDict** services)
+m [function] IContext::getDiscoveryServers(IDict** servers)

+ [interface] INetworkInterface : public IBaseObject
+ [function] INetworkInterface::requestCurrentConfiguration(IPropertyObject** config)
+ [function] INetworkInterface::submitConfiguration(IPropertyObject* config)
+ [function] INetworkInterface::createDefaultConfiguration(IPropertyObject** defaultConfig)
+ [factory] NetworkInterfacePtr NetworkInterface(const StringPtr& name, const StringPtr& ownerDeviceManufacturerName, const StringPtr& ownerDeviceSerialNumber, const BaseObjectPtr& moduleManager)

+ [interface] IDeviceNetworkConfig : public IBaseObject
+ [function] IDeviceNetworkConfig::submitNetworkConfiguration(IString* ifaceName, IPropertyObject* config)
+ [function] IDeviceNetworkConfig::retrieveNetworkConfiguration(IString* ifaceName, IPropertyObject** config)
+ [function] IDeviceNetworkConfig::getNetworkConfigurationEnabled(Bool* enabled)
+ [function] IDeviceNetworkConfig::getNetworkInterfaceNames(IList** ifaceNames)

+ [function] IDeviceInfo::getNetworkInterfaces(IDict** interfaces)
+ [function] IDeviceInfo::getNetworkInterface(IString* interfaceName, INetworkInterface** interface)
+ [function] IDeviceInfoInternal::addNetworkInteface(IString* name, INetworkInterface* networkInterface)

+ [function] IDiscoveryServer::setRootDevice(IDevice* device)

+ [function] IModuleManagerUtils::changeIpConfig(IString* iface, IString* manufacturer, IString* serialNumber, IPropertyObject* config)
+ [function] IModuleManagerUtils::requestIpConfig(IString* iface, IString* manufacturer, IString* serialNumber, IPropertyObject** config)
```

# 2025-01-21
## Description
- Implementing changeable fields of device info, which are synchronized between server and client.
- Device info is no longer frozen.
- Device properties `userName` and `location` are optional and do not exist by default. See integration changes to enable them.
- mDNS server broadcasts all device info fields.
- mDNS broadcasts synchronized changeable device info properties.
- Device info fields are reflected as property types instead of introspection variables via OPC UA (except for `userName` and `location`).

Changeable device info properties are properties that are synchronized between the server and the client. If a property is modified on either side, it will be updated on the other side as well. A changeable property is set to `ReadOnly: false`.

Non-changeable properties of device info are read-only and can only be modified locally using the `setProtectedPropertyValue` method of the `IPropertyObjectProtected` interface. The modification does not update the other side, and these properties can be considered as a local. However, if a non-changeable property is updated on the server side, the client that connects after the modification will receive the latest value of this property.

If the developer is using the mDNS discovery server, the server will broadcast the latest value of the changeable property but will broadcast the last value of the non-changeable property that was set when the mDNS server was registered.

# Example
To set default device info properties as changeable, use the factory to create Device Info with the list of property names that need to be changeable.
To enable device properties userName or location, please use the Device Info factory:
```cpp
DeviceInfoConfigPtr deviceInfo = DeviceInfoWithChangeableFields({"userName", "location"});
```
If the properties `userName` or `location` are changeable, `deviceInfo` will create these properties on the device.
Custom device info properties are changeable if they are not read-only:
```cpp
DeviceInfoConfigPtr deviceInfo = DeviceInfoWithChangeableFields({"userName", "location", "manufacturer"});
deviceInfo.addProperty(StringPropertyBuilder("CustomChangeableField", "default value").setReadOnly(false).build());
deviceInfo.addProperty(StringPropertyBuilder("CustomNotChangeableField", "default value").setReadOnly(true).build());
```
In the example above, the default properties `userName`, `location`, `manufacturer`, and custom `CustomChangeableField` are changeable. Other default device info properties and the custom property `CustomNotChangeableField` are not changeable.

```
+ [factory] DeviceInfoConfigPtr DeviceInfoWithChanegableFields(const ListPtr<IString>& changeableDefaultPropertyNames)
+ [function] IDeviceInfo::getUserName(IString** userName);
+ [function] IDeviceInfoConfig::setUserName(IString* userName);
```

# Changes since 3.10

## Description

- Serialize component name when equal to localID [#660](https://github.com/openDAQ/openDAQ/pull/660)
- Fix for openDAQ module loading failing on Android [#569](https://github.com/openDAQ/openDAQ/pull/659)
- Show time domain signal last value as time [#657](https://github.com/openDAQ/openDAQ/pull/657)
- Fix local device discovery and serialization on id clashes [#653](https://github.com/openDAQ/openDAQ/pull/653)
- Support for Python lists in MultiReader  [#651](https://github.com/openDAQ/openDAQ/pull/651)
- Make attributes editable in Python GUI Demo Application [#637](https://github.com/openDAQ/openDAQ/pull/637) 
- Remove root device on instance destruction [#635](https://github.com/openDAQ/openDAQ/pull/635)
- Support adding and removing nested function blocks in Python GUI [#634](https://github.com/openDAQ/openDAQ/pull/634)
- Python bindings: IEvent implementation [#633](https://github.com/openDAQ/openDAQ/pull/633)
- Add "onAny" property value read and write events [#631](https://github.com/openDAQ/openDAQ/pull/631)
- Fix onGetLogFileInfos case [#630](https://github.com/openDAQ/openDAQ/pull/630)
- Introduce Component statuses ( `OK`, `Warning`, and `Error`) with messages [#625](https://github.com/openDAQ/openDAQ/pull/625) [#650](https://github.com/openDAQ/openDAQ/pull/650)
- Streaming connection statuses [#606](https://github.com/openDAQ/openDAQ/pull/606)
- Add support for view only client to the config protocol [#605](https://github.com/openDAQ/openDAQ/pull/605)

## Required application changes



## Required module changes


