# Changes from 3.30 to 3.40

## Features

- [#1056](https://github.com/openDAQ/openDAQ/pull/1056) implement attempting to reconnect with all available addresses#1056
- [#914](https://github.com/openDAQ/openDAQ/pull/914) Implementing time delay function block. Fix overflow while calculating timestamp in renderer
- [#886](https://github.com/openDAQ/openDAQ/pull/886) Adds simulator device module.
- [#975](https://github.com/openDAQ/openDAQ/pull/975) Add `DevelopmentVersionInfo` for more detailed version information (tweak, branch and hash).
- [#1000](https://github.com/openDAQ/openDAQ/pull/1000) Added a mode to the CSV recorder that allows data from multiple same-rate signals to be written to a single file.
- [#1018](https://github.com/openDAQ/openDAQ/pull/1018) Optimized multireader synchronization for linear data rule domain signals by avoiding iteration through timestamps.
- [#1034](https://github.com/openDAQ/openDAQ/pull/1034) Add string support for data packets and signals
- [#1036](https://github.com/openDAQ/openDAQ/pull/1036) Allow creation of empty multi reader, add/remove inputs dyinamically. Mark inputs as invalid to ignore sync failures from specific ports.
- [#1037](https://github.com/openDAQ/openDAQ/pull/1037) Function blocks and devices of which type is not in their parent's `getAvailableFunctionBlock/DeviceTypes` output can no longer be removed.

## Python

- [#980](https://github.com/openDAQ/openDAQ/pull/980) Update add device with configuration dialog in Python GUI demo app.
- [#986](https://github.com/openDAQ/openDAQ/pull/986) Make Struct Properties editable in Python GUI demo application
- [#987](https://github.com/openDAQ/openDAQ/pull/987) Make Enumeration Properties editable in Python GUI demo application
- [#991](https://github.com/openDAQ/openDAQ/pull/991) Add Function Block configuration to Python GUI demo app
- [#1007](https://github.com/openDAQ/openDAQ/pull/1007) Add a missing error popup when adding a Function Block fails in Python GUI demo app.
- [#1029](https://github.com/openDAQ/openDAQ/pull/1029) Fix python binding for iterators to enable list comprehensions.
- [#1035](https://github.com/openDAQ/openDAQ/pull/1035) Fix showing description metadata of properties in Python GUI app

## Bug fixes

- [#1062](https://github.com/openDAQ/openDAQ/pull/1062) Fix nested PropertyObject update order in beginUpdate/endUpdate
- [#1054](https://github.com/openDAQ/openDAQ/pull/1054) Add validation default value items type for List And Dict Property
- [#1077](https://github.com/openDAQ/openDAQ/pull/1077) Transfer default add-device config from server to client via Native protocol
- [#1015](https://github.com/openDAQ/openDAQ/pull/1015) IPropertyObject::hasProperty returns false instead of throwing not found error if the parent property does not exists
- [#1046](https://github.com/openDAQ/openDAQ/pull/1046) Prevent updating locked attributes during the update process
- [#1052](https://github.com/openDAQ/openDAQ/pull/1052) Renderer FB labels take into account reference domain offset from ReferenceDomainInfo.
- [#1054](https://github.com/openDAQ/openDAQ/pull/1054) Add validation default value items type for List And Dict Property
- [#1059](https://github.com/openDAQ/openDAQ/pull/1059) Prevent post scaling in descriptors with vector/matrix dimensions.

## Misc

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

## Required module changes

### [#1037](https://github.com/openDAQ/openDAQ/pull/1037) Mandatory device types
To enable static components, devices must include the Device Type in their Device Info objects. Modules set the value within the onGetInfo overriding method by calling IDeviceInfoConfig::setDeviceType().