# Changes since 3.10

## Description

- Enables status messages for connection statuses [#687](https://github.com/openDAQ/openDAQ/pull/687)
- Fixed an mDNS issue where multiple devices broadcasting with the same IP address were present, but only one could be detected by the client [#689](https://github.com/openDAQ/openDAQ/pull/689)
- Introduces mechanisms to modify the IP configuration parameters of openDAQ-compatible devices [#642](https://github.com/openDAQ/openDAQ/pull/642)
- Introduce Changeable Device Info Properties with mDNS Synchronization and Customization [#607](https://github.com/openDAQ/openDAQ/pull/607)
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



## Required module changes


