# Changes from 3.40 to 3.50

## Features

- [#1241](https://github.com/openDAQ/openDAQ/pull/1241) Implement IDevice::getDomainSignal()
- [#1242](https://github.com/openDAQ/openDAQ/pull/1242) implement IContext::getRootDevice

## Python

## Bug fixes

## Misc

## Required application changes

## Required module changes

## Interface API changes

### New interfaces

### Removed interfaces

### Modified interfaces

#### `IDevice`
```diff
+ IDevice::getDomainSignal(ISignal** signal);
```
#### `Context`
```diff
+ IContext::getRootDevice(IBaseObject** device);
```

#### `IContextInternal`
```diff
+ IContextInternal::setRootDevice(IBaseObject* device);
```
