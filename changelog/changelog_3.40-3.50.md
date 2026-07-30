# Changes from 3.40 to 3.50

## Features

- [#1242](https://github.com/openDAQ/openDAQ/pull/1242) Implement IContext::getRootDevice
- [#1244](https://github.com/openDAQ/openDAQ/pull/1244) Static objects and object pool
- [#1258](https://github.com/openDAQ/openDAQ/pull/1258) Make device info the nested device property

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
