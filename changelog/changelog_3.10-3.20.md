# Changes since 3.10

## Description

- Enables status messages for connection statuses [#687](https://github.com/openDAQ/openDAQ/pull/687)
- Fixed an mDNS issue where multiple devices broadcasting with the same IP address were present, but only one could be detected by the client [#689](https://github.com/openDAQ/openDAQ/pull/689)
- Introduces mechanisms to modify the IP configuration parameters of openDAQ-compatible devices [#642](https://github.com/openDAQ/openDAQ/pull/642)
- Introducing Changeable Device Info Properties with mDNS Synchronization and Customization [#607](https://github.com/openDAQ/openDAQ/pull/607)
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
- Introduce Component statuses ( `OK`, `Warning`, and `Error`) with messages [#625](https://github.com/openDAQ/openDAQ/pull/625) [#650](https://github.com/openDAQ/openDAQ/pull/650) [#673](https://github.com/openDAQ/openDAQ/pull/673)
- Streaming connection statuses [#606](https://github.com/openDAQ/openDAQ/pull/606)
- Add support for view only client to the config protocol [#605](https://github.com/openDAQ/openDAQ/pull/605)

## Required application changes



## Required module changes


