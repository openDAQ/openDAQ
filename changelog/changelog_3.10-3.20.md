# Component Statuses [#625](https://github.com/openDAQ/openDAQ/pull/625) [#650](https://github.com/openDAQ/openDAQ/pull/650)

## Description

- Adds Component status types to the Type Manager in `context_impl.cpp` (`"Ok"`, `"Warning"`, and `"Error"`).
- Defines `ComponentStatus` enum, with possible values `OK`, `Warning`, and `Error`.
- Declares and defines `initComponentStatus`, `setComponentStatus`, and `setComponentStatusWithMessage` in `component_impl.h`.
- Adds `getStatusMessage` in `component_status_container.h`, and `addStatusWithMessage`, `setStatusWithMessage` in `component_status_container_private.h`. Implements all three in `component_status_container_impl.h`.
- Uses `initComponentStatus`, `setComponentStatus`, and `setComponentStatusWithMessage` in all reference Function Blocks.

## Usage example

In Function Block implementation, at the beginning of the constructor, call:

```cpp
initComponentStatus();
```

This will initialize the Component status to `Ok` and status message to an empty string.

When the Component status should change to `Warning` with a message, call:

```cpp
setComponentStatusWithMessage(ComponentStatus::Warning, "You warning message here");
```

> [!IMPORTANT]  
> Whenever the Component status changes, the change is also automatically logged.

Similarly, for `Error`, call:
```cpp
setComponentStatusWithMessage(ComponentStatus::Error, "You error message here");
```

To reset the Component status back to `Ok`, call:
```cpp
setComponentStatus(ComponentStatus::Ok);
```

> [!TIP]
> The above method call will also reset the Component status message to an empty string.

## Required integration changes

- None. However, developers of Components (such as Function Blocks) are encouraged to use:
  - `initComponentStatus`
  - `setComponentStatus`
  - `setComponentStatusWithMessage`

## API changes

```diff
+ [function] IComponentStatusContainer::getStatusMessage(IString* name, IString** message)
+ [function] IComponentStatusContainerPrivate::addStatusWithMessage(IString* name, IEnumeration* initialValue, IString* message)
+ [function] IComponentStatusContainerPrivate::setStatusWithMessage(IString* name, IEnumeration* value, IString* message)
```

---

# Fix Method Name to camelCase

## Description

- The module-overridable virtual method `ongetLogFileInfos` has been renamed to `onGetLogFileInfos`.

## API changes

```diff
-m [function] ListPtr<ILogFileInfo> Device::ongetLogFileInfos()
+m [function] ListPtr<ILogFileInfo> Device::onGetLogFileInfos()
```

---

# Add "Any Read/Write" Events to Property Object

## Description

- These events are triggered whenever any property value is read or written.

## API changes

```diff
+ [function] IPropertyObject::getOnAnyPropertyValueWrite(IEvent** event)
+ [function] IPropertyObject::getOnAnyPropertyValueRead(IEvent** event)
```
