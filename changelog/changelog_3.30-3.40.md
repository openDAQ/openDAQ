# Changes from 3.20 to 3.30

## Features

- [#914](https://github.com/openDAQ/openDAQ/pull/914) Implementing time delay function block. Fix overflow while calculating timestamp in renderer
- [#886](https://github.com/openDAQ/openDAQ/pull/886) Adds simulator device module.
- [#975](https://github.com/openDAQ/openDAQ/pull/975) Add `DevelopmentVersionInfo` for more detailed version information (tweak, branch and hash).
- [#1000](https://github.com/openDAQ/openDAQ/pull/1000) Added a mode to the CSV recorder that allows data from multiple same-rate signals to be written to a single file.

## Python

- [#980](https://github.com/openDAQ/openDAQ/pull/980) Update add device with configuration dialog in Python GUI demo app.
- [#986](https://github.com/openDAQ/openDAQ/pull/986) Make Struct Properties editable in Python GUI demo application
- [#987](https://github.com/openDAQ/openDAQ/pull/987) Make Enumeration Properties editable in Python GUI demo application
- [#991](https://github.com/openDAQ/openDAQ/pull/991) Add Function Block configuration to Python GUI demo app
- [#1007](https://github.com/openDAQ/openDAQ/pull/1007) Add a missing error popup when adding a Function Block fails in Python GUI demo app.

## Bug fixes

## Misc

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

## Required module changes
