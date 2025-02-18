# Changes since 3.10

## Description

- Enables status messages for connection statuses [#687](https://github.com/openDAQ/openDAQ/pull/687)
- Fixed an mDNS issue where multiple devices broadcasting with the same IP address were present, but only one could be detected by the client [#689](https://github.com/openDAQ/openDAQ/pull/689)
- Introduces mechanisms to modify the IP configuration parameters of openDAQ-compatible devices [#642](https://github.com/openDAQ/openDAQ/pull/642)
- Introduce Changeable Device Info Properties with mDNS Synchronization and Customization [#607](https://github.com/openDAQ/openDAQ/pull/607)[#700](https://github.com/openDAQ/openDAQ/pull/700)
- The "Name" attribute was not serialized when equal to the local ID, causing issues when the localID of a deserialized signal was overridden to be different than the original [#660](https://github.com/openDAQ/openDAQ/pull/660)
- Fixing building openDAQ on android, by removing multiple coping of loaded library to the final vector in ModuleManager constructor [#569](https://github.com/openDAQ/openDAQ/pull/659)
- Display time domain signal last value as time in Python GUI demo app [#657](https://github.com/openDAQ/openDAQ/pull/657)
- Device info objects without server capabilities are no longer grouped by serial and manufacturer in the `getAvailableDevices` call [#653](https://github.com/openDAQ/openDAQ/pull/653)
- Support for Python lists in MultiReader and how-to guides update [#651](https://github.com/openDAQ/openDAQ/pull/651)
- Make attributes editable in Python GUI Demo Application [#637](https://github.com/openDAQ/openDAQ/pull/637) 
- Before the openDAQ instance object is destroyed, it now calls `rootDevice.remove()` in its desctructor [#635](https://github.com/openDAQ/openDAQ/pull/635)
- Support adding and removing nested function blocks in Python GUI [#634](https://github.com/openDAQ/openDAQ/pull/634)
- Python bindings: `IEvent` interface implementation [#633](https://github.com/openDAQ/openDAQ/pull/633)
- "Any read/write" events are added to property object that are triggered whenever any of the object's property values are read or written [#631](https://github.com/openDAQ/openDAQ/pull/631)
- The internal `Device` implementation function `ongetLogFileInfos()` was renamed to `onGetLogFileInfos()`. [#630](https://github.com/openDAQ/openDAQ/pull/630)
- Introduce Component statuses ( `OK`, `Warning`, and `Error`) with messages [#625](https://github.com/openDAQ/openDAQ/pull/625) [#650](https://github.com/openDAQ/openDAQ/pull/650) [#673](https://github.com/openDAQ/openDAQ/pull/673)
- Introduces new mechanism for retrieving and monitoring the device's connection statuses, which enables tracking of streaming connections, in comparison with previous one limited to configuration-type of connections [#606](https://github.com/openDAQ/openDAQ/pull/606)
- Add support for View Only client to the config protocol [#605](https://github.com/openDAQ/openDAQ/pull/605)

## Required application changes
### [#607](https://github.com/openDAQ/openDAQ/pull/607)

The "location" and "userName" properties no longer appear on devices on which said properties are not changeable. Applications should first check whether those properties exist before accessing them, or access them through the  "DeviceInfo" object as is the case for other properties.

```cpp
if (device.hasProperty("location"))
	device.setPropertyValue("location", "new_location");
	
// Option 2:
auto locationProp = device.getInfo().getProperty("location");
if (!location.getReadOnly())
	location.setValue("new_location");
```

### [#606](https://github.com/openDAQ/openDAQ/pull/606)

None, since the old mechanism remains available for backward compatibility:

```cpp
EnumerationPtr status = instance.getDevices()[0].getStatusContainer().getStatus("ConnectionStatus");
```

However, user can opt to use the new mechanism instead to retrieve the native device's configuration connection status:

```cpp
EnumerationPtr status = instance.getDevices()[0].getConnectionStatusContainer().getStatus("ConfigurationStatus");
```


## Required module changes

### [#700] https://github.com/openDAQ/openDAQ/pull/700

`IDeviceInfoConfig` setters no longer have protected access once `DeviceInfo` has an owner. To change the value of a read-only 
device info property `IPropertyObject::setProtectedPropertyValue` must be used. The old setters can still be used as Before
during info creation. Said behaviour only works locally - when done on a remote device, the setters will fail as before.

Example required changes:
```cpp
// Manufacturer is a read-only device info property
device.getInfo().asPtr<IDeviceInfoConfig>().setManufacturer("openDAQ"); // Fails
device.getInfo().asPtr<IPropertyObjectProtected>().setProtectedPropertyValue("manufacturer", "openDAQ"); // Works

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

### [#642](https://github.com/openDAQ/openDAQ/pull/642)

Updates the usage of the openDAQ discovery client, requiring the changes in the initialization of the discovery client
and the implementation of `onGetAvailableDevices` in client modules: 
* The discovery client does not anymore accept callbacks for build the `ServerCapability` as first parameter of its constructor. 
* The return type of `DiscoveryClient::discoverMdnsDevices()` changed from `ListPtr<IDeviceInfo>` to `std::vector<MdnsDiscoveredDevice>`

So, the following module implementation snippet:

```cpp
ExampleClientModule::ExampleClientModule(ContextPtr context)
    : Module("ExampleClientModule",
            daq::VersionInfo(MAJOR_VERSION, MINOR_VERSION, PATCH_VERSION),
            std::move(context),
            "ExampleClientModule")
    // create discovery client
    , discoveryClient(
        {
            [context = this->context](MdnsDiscoveredDevice discoveredDevice)
            {
                auto cap = ServerCapability("ExampleProtocolId", "ExampleProtocolName", ProtocolType::Configuration);
                if (!discoveredDevice.ipv4Address.empty())
                {
                   // ... build connection string and address info
                }
                if(!discoveredDevice.ipv6Address.empty())
                {
                    // ... build connection string and address info
                }
                // ... set additional parameter of capability
                return cap;
            }
        },
        {"OPENDAQ"}
    )
{
    // init discovery client
    discoveryClient.initMdnsClient(List<IString>("_example-tcp._tcp.local."));
}

ListPtr<IDeviceInfo> ExampleClientModule::onGetAvailableDevices()
{
    ListPtr<IDeviceInfo> availableDevices = discoveryClient.discoverDevices();
    for (auto device : availableDevices)
    {
        device.asPtr<IDeviceInfoConfig>().setDeviceType(createDeviceType());
    }
    return availableDevices;
}
```

is to be replaced with:

```cpp
ExampleClientModule::ExampleClientModule(ContextPtr context)
    : Module("ExampleClientModule",
            daq::VersionInfo(MAJOR_VERSION, MINOR_VERSION, PATCH_VERSION),
            std::move(context),
            "ExampleClientModule")
    // create discovery client
    , discoveryClient(
        {"OPENDAQ"}
    )
{
    // init discovery client
    discoveryClient.initMdnsClient(List<IString>("_example-tcp._tcp.local."));
}

ListPtr<IDeviceInfo> ExampleClientModule::onGetAvailableDevices()
{
    auto availableDevices = List<IDeviceInfo>();
    std::vector<MdnsDiscoveredDevice> discoveredDevices = discoveryClient.discoverMdnsDevices();
    for (const auto& device : discoveredDevices)
        availableDevices.pushBack(populateDiscoveredDevice(device));
    return availableDevices;
}

DeviceInfoPtr ExampleClientModule::populateDiscoveredDevice(const MdnsDiscoveredDevice& discoveredDevice)
{
    PropertyObjectPtr deviceInfo = DeviceInfo("");
    DiscoveryClient::populateDiscoveredInfoProperties(deviceInfo, discoveredDevice);

    auto cap = ServerCapability("ExampleProtocolId", "ExampleProtocolName", ProtocolType::Configuration);
    if (!discoveredDevice.ipv4Address.empty())
    {
       // ... build connection string and address info
    }
    if(!discoveredDevice.ipv6Address.empty())
    {
       // ... build connection string and address info
    }
    // ... set additional parameter of capability

    deviceInfo.asPtr<IDeviceInfoInternal>().addServerCapability(cap);
    deviceInfo.asPtr<IDeviceInfoConfig>().setConnectionString(cap.getConnectionString());
    deviceInfo.asPtr<IDeviceInfoConfig>().setDeviceType(createDeviceType());

    return deviceInfo;
}
```

### [#630](https://github.com/openDAQ/openDAQ/pull/630)

The `ongetLogFileInfos` function override must be renamed to `onGetLogFileInfos`. This change will cause all modules developed with an older version of openDAQ that override the aforementioned method to no longer compile

### [#607](https://github.com/openDAQ/openDAQ/pull/607)

The "location" and "userName" properties previously always appeared on devices, regardless of whether or not devices used them. A device must explicitly define the "location" and "userName" to be configurable when constructing its DeviceInfo object.

```cpp

DeviceInfoPtr MyDeviceImpl::onGetInfo()
{
    auto info = DeviceInfoWithChanegableFields({"userName", "location"});
	
	...
}

```

### [#606](https://github.com/openDAQ/openDAQ/pull/606)

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
