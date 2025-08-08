# Changes from 3.20 to 3.30

## Features

- [#853](https://github.com/openDAQ/openDAQ/pull/853) Parquet writer
- [#861](https://github.com/openDAQ/openDAQ/pull/861) Add support for list and dictionary item/key type identification in Argument Info.
- [#849] (https://github.com/openDAQ/openDAQ/pull/849) Improving error logging and implementing an error guard.
- [#837](https://github.com/openDAQ/openDAQ/pull/837) Add main thread event loop, SFML 3.0 migration, and renderer window control
- [#838](https://github.com/openDAQ/openDAQ/pull/838) Set of multi reader improvements.
- [#828](https://github.com/openDAQ/openDAQ/pull/828) Parallel device connection.
- [#810](https://github.com/openDAQ/openDAQ/pull/810) Load individual modules.
- [#788](https://github.com/openDAQ/openDAQ/pull/788) Component property search.
- [#759](https://github.com/openDAQ/openDAQ/pull/759) Bulk create data packets 


## Python

- [#841](https://github.com/openDAQ/openDAQ/pull/841) Fix Python GUI app crash for Python versions below 3.11 by mimicking `enum.StrEnum` functionality via `enum.Enum`.
- [#811](https://github.com/openDAQ/openDAQ/pull/811) Add color coding and message display for Component statuses in Python GUI demo application.
- [#807](https://github.com/openDAQ/openDAQ/pull/807) Enable Device operation mode switching in Python GUI Demo Application.


## Bug fixes

- [#848](https://github.com/openDAQ/openDAQ/pull/848) Do not throw an exception when getting operation modes on old devices. Return default state instead.
- [#827](https://github.com/openDAQ/openDAQ/pull/827) Fix setting irrelevant streaming source as active.
- [#808](https://github.com/openDAQ/openDAQ/pull/808) Fix examples CMake when downloaded as solo archive via https://docs.opendaq.com/ or https://docs-dev.opendaq.com/ (when not part of whole openDAQ project).
- [#781](https://github.com/openDAQ/openDAQ/pull/781) Fix how Lists are interpreted for Function Property in Python.

## Misc

- [#835](https://github.com/openDAQ/openDAQ/pull/835) Removes the opendaq_dev target.
- [#829](https://github.com/openDAQ/openDAQ/pull/829) Add search functionality to Antora documentation to significantly improve user experience. 


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