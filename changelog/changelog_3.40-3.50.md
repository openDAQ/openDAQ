# Changes from 3.40 to 3.50

## Features

- [#1045](https://github.com/openDAQ/openDAQ/pull/1045) Implement ISynchronization and ISyncInterface
- [#1242](https://github.com/openDAQ/openDAQ/pull/1242) Implement IContext::getRootDevice
- [#1244](https://github.com/openDAQ/openDAQ/pull/1244) Static objects and object pool
- [#1258](https://github.com/openDAQ/openDAQ/pull/1258) Make device info the nested device property
- [#1262](https://github.com/openDAQ/openDAQ/pull/1262) Add protocol group ID and security level to server capabilities. Streaming protocols sharing a group ID are treated as variants of one another, so only the most preferred source of each group is attached. A single server can now advertise multiple discovery services.
- [#1251](https://github.com/openDAQ/openDAQ/pull/1251) TLS encrypted channel for the LT streaming module. With `EnableTlsStreamingPort` the server serves the secure channel alongside the plaintext one, registering both the `OpenDAQLTStreaming` and `OpenDAQLTStreamingSecure` capabilities and advertising the `_streaming-lt._tcp` and `_streaming-lts._tcp` mDNS services. Mutual TLS is enabled by default. LT capabilities now carry the `LTStreaming` protocol group ID and a protocol security level (`0` plaintext / `10` secure), so a client ordering streaming protocols by security level prefers the secure channel on its own. The channel is opt-in: without `EnableTlsStreamingPort` the server behaves as before. It is opt-in at build time as well: see [#1278](https://github.com/openDAQ/openDAQ/pull/1278).
- [#1278](https://github.com/openDAQ/openDAQ/pull/1278) Building the TLS channel of the LT streaming modules is controlled by the new CMake option `OPENDAQ_ENABLE_WEBSOCKET_STREAMING_WITH_TLS`, off by default and on in the `full`, `package` and `simulator_package` presets. With it off the LT streaming modules are built without their OpenSSL dependency and the `daq.lts://` channel is absent. The option has no meaning for the legacy LT streaming modules (`DAQMODULES_LT_LEGACY_MODULES`) and is ignored there.

## Python

## Bug fixes

## Misc

- [#1251](https://github.com/openDAQ/openDAQ/pull/1251), [#1269](https://github.com/openDAQ/openDAQ/pull/1269), [#1278](https://github.com/openDAQ/openDAQ/pull/1278) OpenSSL (>= 1.1.1) is a build dependency of the SDK when `OPENDAQ_ENABLE_WEBSOCKET_STREAMING_WITH_TLS` is on. Unlike most other dependencies it is not fetched automatically and has to be installed on the host system: `libssl-dev` on Debian/Ubuntu (`libssl-dev:i386` for 32-bit builds), `openssl-devel` on RHEL-based distributions. The build documentation and all CI, packaging and docs jobs were updated accordingly.

## Required application changes

## Required module changes

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
