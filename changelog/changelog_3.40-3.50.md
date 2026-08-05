# Changes from 3.40 to 3.50

## Features

- [#1242](https://github.com/openDAQ/openDAQ/pull/1242) Implement IContext::getRootDevice
- [#1244](https://github.com/openDAQ/openDAQ/pull/1244) Static objects and object pool
- [#1262](https://github.com/openDAQ/openDAQ/pull/1262) Add protocol group ID and security level to server capabilities. Streaming protocols sharing a group ID are treated as variants of one another, so only the most preferred source of each group is attached. A single server can now advertise multiple discovery services.

## Python

## Bug fixes

## Misc

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
