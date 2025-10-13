# Changes from 3.20 to 3.30

## Features

- [#869](https://github.com/openDAQ/openDAQ/pull/869) Implementing video player function block.
- [#894](https://github.com/openDAQ/openDAQ/pull/894) Allows for objects to specify whether they will use their own mutex, or that of their owner/parent.
- [#884](https://github.com/openDAQ/openDAQ/pull/884) Tampering check interface and example implementation.
- [#878](https://github.com/openDAQ/openDAQ/pull/878) Licensing interface and reference module + example implementation.
- [#867](https://github.com/openDAQ/openDAQ/pull/867) Generalized client-to-device streaming
- [#865](https://github.com/openDAQ/openDAQ/pull/865), [#882](https://github.com/openDAQ/openDAQ/pull/882) onPropertyValueRead events are now supported over native. Suggested and Selection values can be overridden in new onRead events. String properties can now have suggested values.
- [#853](https://github.com/openDAQ/openDAQ/pull/853) Parquet writer
- [#861](https://github.com/openDAQ/openDAQ/pull/861) Add support for list and dictionary item/key type identification in Argument Info.
- [#849](https://github.com/openDAQ/openDAQ/pull/849) Improving error logging and implementing an error guard.
- [#837](https://github.com/openDAQ/openDAQ/pull/837) Add main thread event loop, SFML 3.0 migration, and renderer window control
- [#838](https://github.com/openDAQ/openDAQ/pull/838) Set of multi reader improvements.
- [#828](https://github.com/openDAQ/openDAQ/pull/828) Parallel device connection.
- [#810](https://github.com/openDAQ/openDAQ/pull/810) Load individual modules.
- [#788](https://github.com/openDAQ/openDAQ/pull/788) Component property search.
- [#759](https://github.com/openDAQ/openDAQ/pull/759) Bulk create data packets 

## Python

- [#944](https://github.com/openDAQ/openDAQ/pull/944) Prevent error when removing a device with a subdevice in Python GUI application
- [#841](https://github.com/openDAQ/openDAQ/pull/841) Fix Python GUI app crash for Python versions below 3.11 by mimicking `enum.StrEnum` functionality via `enum.Enum`.
- [#811](https://github.com/openDAQ/openDAQ/pull/811) Add color coding and message display for Component statuses in Python GUI demo application.
- [#807](https://github.com/openDAQ/openDAQ/pull/807) Enable Device operation mode switching in Python GUI Demo Application.

## Bug fixes

- [#941](https://github.com/openDAQ/openDAQ/pull/941) Fix getting on property value read write event for nestead proporties 
- [#936](https://github.com/openDAQ/openDAQ/pull/936) Fix compatibility issue with SDK v3.10 in client-to-device-streaming
- [#930](https://github.com/openDAQ/openDAQ/pull/930) Fixes the operation mode of 3.10 native-client devices with no op-mode support to be "Operation" by default
- [#925](https://github.com/openDAQ/openDAQ/pull/925) Set of discovery fixes for DNS resolution and enabling of multi-network card devices.
- [#880](https://github.com/openDAQ/openDAQ/pull/880) Fix OPC UA warnings related to writing default values to a node.
- [#848](https://github.com/openDAQ/openDAQ/pull/848) Do not throw an exception when getting operation modes on old devices. Return default state instead.
- [#827](https://github.com/openDAQ/openDAQ/pull/827) Fix setting irrelevant streaming source as active.
- [#808](https://github.com/openDAQ/openDAQ/pull/808) Fix examples CMake when downloaded as solo archive via https://docs.opendaq.com/ or https://docs-dev.opendaq.com/ (when not part of whole openDAQ project).
- [#781](https://github.com/openDAQ/openDAQ/pull/781) Fix how Lists are interpreted for Function Property in Python.

## Misc

- [#921](https://github.com/openDAQ/openDAQ/pull/921) Allow getting last value on signals that are invisible
- [#908](https://github.com/openDAQ/openDAQ/pull/908) Add support for Intel-LLVM compiler
- [#903](https://github.com/openDAQ/openDAQ/pull/903) Enable suppressed type conversion warnings on Windows
- [#893](https://github.com/openDAQ/openDAQ/pull/893) Rework disabled permission manager, making module code independent of the OPENDAQ_ENABLE_ACCESS_CONTROL option
- [#835](https://github.com/openDAQ/openDAQ/pull/835) Removes the opendaq_dev target.
- [#829](https://github.com/openDAQ/openDAQ/pull/829) Add search functionality to Antora documentation to significantly improve user experience. 
- [#883](https://github.com/openDAQ/openDAQ/pull/883) Renaming enum TimeSource into TimeProtocol, so that it more accurately reflects its values.

## Required application changes

### [#788](https://github.com/openDAQ/openDAQ/pull/788) Component property search.

Breaking changes to the `ISearchFilter` API require modifications to the input parameter type of the accept and visit functions within custom search filters:

```diff
FunctionPtr customAcceptorFunc = Function(
-   [](const ComponentPtr& component)
+   [](const BaseObjectPtr& obj)
    {
+       auto component = obj.asPtr<IComponent>();
        return isComponentAccepted(component);
    }
);
FunctionPtr customVisitorFunc = Function(
-   [](const ComponentPtr& component)
+   [](const BaseObjectPtr& obj)
    {
+       auto component = obj.asPtr<IComponent>();
        return isComponentVisited(component);
    }
);

auto foundComponents = folder.getItems(search::Recursive(search::Custom(customAcceptorFunc, customVisitorFunc)));
```

### [#883](https://github.com/openDAQ/openDAQ/pull/883) Renaming enum TimeSource into TimeProtocol, so that it more accurately reflects its values.

```cpp
auto info = device.getDomain().getReferenceDomainInfo();
auto timeProtocol = info.getReferenceTimeProtocol();
```

## Required module changes

### [#788](https://github.com/openDAQ/openDAQ/pull/788) Component property search.

Breaks the `ISearchFilter` API by changing the input parameters of its methods and renaming the accept method.
As a result, the following implementation example must be updated accordingly:

```diff
class MySearchFilterImpl final : public ImplementationOf<ISearchFilter>
{
public:
    explicit MySearchFilterImpl();

-   ErrCode INTERFACE_FUNC acceptsComponent(IComponent* component, Bool* accepts) override;
+   ErrCode INTERFACE_FUNC acceptsObject(IBaseObject* obj, Bool* accepts) override;

-   ErrCode INTERFACE_FUNC visitChildren(IComponent* component, Bool* visit) override;
+   ErrCode INTERFACE_FUNC visitChildren(IBaseObject* obj, Bool* visit) override;
};
```

### [#883](https://github.com/openDAQ/opeDAQ/pull/883)

Changed the `TimeSource` into `TimeProtocol`.

```diff
-	ReferenceDomainInfoBuilder()
			.setRefereceTimeSource(TimeSource::Gps)
			.build();
			
-	referenceDomainInfo.getReferenceTimeSource();

+	ReferenceDomainInfoBuilder()
			.setReferenceTimeProtocol(TimeProtocol::Gps)
			.build();
			
+	referenceDomainInfo.getReferenceTimeProtocol();
```

### [#925](https://github.com/openDAQ/openDAQ/pull/925)

Modules that use the openDAQ mDNS discovery should now post all addresses of devices returned by `DiscoveryClient::discoverMdnsDevices`. These are stored in the `MdnsDiscoveredDevice::ipv4Addresses` and `MdnsDiscoveredDevice::ipv6Addresses` fields. Previously, only a single address was returned for IPv4 and IPv6. 

See PR for an example implementation in the Native streaming client module.