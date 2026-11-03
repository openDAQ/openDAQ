# Credential Provider Framework — API Reference & Authentication Flow

Credentials are modelled as a dictionary of named **fields** per authentication method
(`IAuthenticationMethod::getFields()`), each one an `ICredentialField` naming how its value should be collected
- `Text`, `Secret` (masked), or `FilePath`. Authentication method selection happens through an
`IAuthenticationConfig` object - a single, self-contained property object built with
`componentType.createAuthenticationConfig()`, listing every method the named component type itself declares
supporting (via `IComponentTypePrivate::getSupportedAuthenticationMethods()`, set when the module built the
type) as a candidate the caller selects among directly, tunes, and attaches to a plain `addDevice`/
`addStreaming` call (see [§3](#3-two-ways-to-add-a-device)).

At most one credential provider can ever be registered for an entire `Instance` - on `IInstanceBuilder`, at
build time, flowing into the `ModuleManager` it builds (see [§2](#2-extensions-to-existing-interfaces)) - so
there is no per-config provider selection at all: `IAuthenticationConfig` doesn't carry a provider id, and
`ModuleManagerImpl` always uses whichever single provider (if any) it was constructed with.

---

## 1. Core Interfaces

### `IAuthenticationMethod`

Describes one way to authenticate - its id, a human-readable description, and the named credential fields it needs (if any).

| Member | Description |
|---|---|
| `getId(IString**)` | The id that uniquely identifies this authentication method at least within the module that offers it - the same id a caller matches against when selecting this method out of an `IAuthenticationConfig`'s `"AuthenticationMethod"` candidates (see below). In practice often unique system-wide instead: the `Standard*AuthenticationMethod` factories key off shared, well-known ids, so different modules using the same standard id produce identically-shaped authentication methods, reusing the id deliberately rather than colliding by accident. |
| `getFields(IDict**)` | The named credential fields this method expects, keyed by name, in declaration order - empty for a method that needs no credentials at all, e.g. anonymous access. Each value is an `ICredentialField`. |
| `getDescription(IString**)` | Human-readable description of the authentication method, e.g. *"PIN-code"*, *"username and password"*, *"Path to the SSH private key file"*. |

**`ICredentialField`** - describes a single credential field:

| Member | Description |
|---|---|
| `getId(IString**)` | The key this field is stored under in the credential and in `getFields()` - e.g. `"UserName"`, `"Password"`, `"PrivateKeyFilePath"`. |
| `getKind(CredentialFieldKind*)` | How this field's value should be collected/presented. |
| `getName(IString**)` | A human-readable name for this field, for the user - e.g. *"User name"*, *"Private key file"*. |
| `getMetadata(IDict**)` | Hints for whoever collects this field's value, as a `Dict<IString, IString>`. Well-known key `"Extensions"` for a `FilePath` field: comma-separated, without the dot (e.g. `"pem,key"`); absent or empty means any file. |
| `isRequired(Bool*)` | Whether this field cannot be left unset or empty - e.g. a password might be optional but not the username within the same authentication method. |

```cpp
enum class CredentialFieldKind : EnumType
{
    Text = 0, // shown as typed - e.g. a username
    Secret,   // masked as typed - e.g. a password or PIN
    FilePath  // a path to a file - e.g. a private key file
};
```

**Factory:** `CredentialField(id, kind, name, required = True, metadata = {})`.

**`IAuthenticationMethodInternal::acceptsCredentials(credential)`** - an internal, binding-less interface
(reachable only via `authenticationMethod.asPtr<IAuthenticationMethodInternal>()` from C++, never exposed to
other languages) that checks whether `credential` - a dictionary of field value(s), keyed by their own field id
(see [Credentials](#credentials) below) - is usable as-is: every field the method marks required
(`ICredentialField::isRequired`) must be present among `credential`'s keys with a non-empty value. A field that
isn't required is never checked, whether present or not. For a method with *no* fields at all, calling this
always throws `NotSupportedException` instead of returning a result - credential values simply do not apply to
a method that needs none.

**Factory:** `AuthenticationMethod(id, fields, description)` - the one factory, `fields` a dict of name → `ICredentialField`; pass an empty dict for a method that needs no credentials at all (see `StandardAnonymousAuthenticationMethod()` below for exactly this).

**Standard authentication methods:** `StandardUserNamePasswordAuthenticationMethod()`, `StandardPinAuthenticationMethod()`, `StandardPrivateKeyFileAuthenticationMethod()`, and `StandardAnonymousAuthenticationMethod()` build ready-made authentication methods for four well-known methods, each keyed by its own well-known id (`StandardUserNamePasswordId` = `"UserNamePassword"`, `StandardPinId` = `"Pin"`, `StandardPrivateKeyFileId` = `"PrivateKeyFile"`, `StandardAnonymousId` = `"Anonymous"`) - so every module offering one of these methods produces the exact same authentication method and credential shape, rather than each inventing its own. `StandardUserNamePasswordAuthenticationMethod` has two fields (`"UserName"`: `Text`, `"Password"`: `Secret`); `StandardPinAuthenticationMethod` has one (`"Pin"`: `Secret`); `StandardPrivateKeyFileAuthenticationMethod` has one (`"PrivateKeyFilePath"`: `FilePath`); `StandardAnonymousAuthenticationMethod` has none.

---

### `IAuthenticationConfig`

Carries the authentication settings for a single connection attempt. Lives alongside the base add-component config, never serialized as part of it - though a component created with authentication persists a reduced form of the config it was authenticated with alongside itself instead (see [Serialization](#serialization) below and [§7](#7-application-level-usage)).

`IAuthenticationConfig` extends `IPropertyObject` - the settings below are backed by ordinary properties, so besides the typed getters/setters, they can equally be read and set through the generic `IPropertyObject` interface:

| Property | Description |
|---|---|
| `"AuthenticationMethod"` (Selection) | Candidates are the supported methods' own ids, as plain strings - the selection value is the currently selected id. The real `IAuthenticationMethod` objects (fields and all) are reached separately, via `getAuthenticationMethods()`, keyed by that same id. A config returned by `componentType.createAuthenticationConfig()` (see [§2](#2-extensions-to-existing-interfaces)) is self-contained - every authentication method the component type supports is a selection candidate here, not just one - so the caller switches methods by changing this property's selection, without fetching a different config object. Changing this property unconditionally resets `"SuppliedCredential"` back to empty, whatever it held before. |
| `"SuppliedCredential"` (Dict) | A credential supplied directly by the caller, to be used instead of the registered credential provider obtaining it - a dictionary of field value(s), keyed by their own field id (see [Credentials](#credentials) below). Its default value is an empty dictionary, which means "none supplied" (the registered provider is asked instead). A non-empty value counts as supplied, and is validated against the *currently selected* `"AuthenticationMethod"` on every write: it must carry a non-empty value for every field the method marks required (via `IAuthenticationMethodInternal::acceptsCredentials`), or the write throws `InvalidParameterException`; selecting a field-less method and writing any non-empty value throws `NotSupportedException` instead. Changing `"AuthenticationMethod"` always resets this property back to empty first, whatever it held before - a credential supplied for the previous method is never meaningful for the new one, compatible-looking or not. See [§5](#5-supplying-a-credential-directly). |

| Member | Description |
|---|---|
| `getSelectedAuthenticationMethod(IAuthenticationMethod**)` | Convenience getter for the full authentication method matching the selected `"AuthenticationMethod"` value's id - looked up in `getAuthenticationMethods()` internally, so the caller doesn't have to. |
| `setAuthenticationMethod(IString*)` | Selects the authentication method to use, by its own id - the typed equivalent of setting `"AuthenticationMethod"` directly. Throws `NotFoundException` if the id doesn't match any of `getAuthenticationMethods()`'s own keys. Resets `"SuppliedCredential"` back to empty, same as a direct property write. |
| `getAuthenticationMethods(IDict**)` | Every authentication method this config supports, keyed by their own id - the full set of `"AuthenticationMethod"` selection candidates. |
| `setSuppliedCredential(IDict*)` / `getSuppliedCredential(IDict**)` | Typed equivalents of reading/writing the `"SuppliedCredential"` property directly. |

There is no "additional config" on `IAuthenticationConfig` itself - any other per-connection setting (unrelated to authentication) still travels via the component's own, generic config object instead (the same one `addDevice`/`addStreaming` always take), read by the module from its own `config` parameter alongside the authentication config. Nor is there any credential-provider selection on it at all.

**Factory:** two overloads.
- `componentType.createAuthenticationConfig()` — the recommended way to build a config for a live device/streaming type. Reads the supported methods straight off `componentType`'s own `IComponentTypePrivate::getSupportedAuthenticationMethods()` (see [§2](#2-extensions-to-existing-interfaces)) - the type itself is the single source of truth for what it supports, set via `IComponentTypeBuilder::addAuthenticationMethod` when the module built it. Throws `NotSupportedException` if the type has no methods added, or `NotImplementedException` if the type's sort (Server, FunctionBlock) doesn't carry authentication data at all.
- `AuthenticationConfig(authenticationMethods)` — the lower-level overload, for building a config with no live `IComponentType` object at hand (e.g. deserialization). Each entry of `authenticationMethods` (a dict keyed by each authentication method's own id) becomes one candidate value of the resulting config's `"AuthenticationMethod"` Selection property, so a caller can later switch between methods just by changing that property's selection instead of needing a different config object per method; the dict's first entry, in iteration order, starts out selected.

`"SuppliedCredential"` is never set by either factory - set it afterward via that property directly, the same way regardless of which one built the config.

**Serialization:** entirely custom - replaces the inherited generic `IPropertyObject` mechanism. Writes exactly two things: the full `supportedAuthenticationMethods` dict (every candidate `IAuthenticationMethod`, self-serializing via its own `ISerializable`) and the currently selected id. `"SuppliedCredential"` is never written. Deserializing rebuilds the config directly from those two pieces, via the real constructor plus `setAuthenticationMethod()` to restore the saved selection - fully self-contained, no module/type registry re-consultation needed (throws `NotFoundException` if the saved selected id isn't among the saved methods - only possible if the save itself is corrupted, since both pieces come from the same blob).

---

### `ICredentialRequest`

Carries the details of a credential request, handed to `ICredentialProvider::requestCredentials`. Built via `ICredentialRequestBuilder`.

| Member | Description |
|---|---|
| `getComponentType(IComponentType**)` | The type of component the request is for. |
| `getConnectionString(IString**)` | The *canonical* connection string of this connection attempt - already resolved via the owning module's `onGetCanonicalConnectionString` (routing prefix trimmed, every parameter made explicit), not necessarily the raw string the caller originally supplied. |
| `getDisplayName(IString**)` | A human-readable name for the device, context for the provider to show the user. Populated only for a streaming attach - the already-connected *owning* device's own display name (`DeviceInfo::getName()`), never the streaming source's own identity, which isn't known yet. Always unassigned for a device connection request (there is no owning device to read it from), and unassigned for a streaming one if the owner has no info. |
| `getManufacturer(IString**)` | The device's manufacturer - together with `getSerialNumber`/`getModel`, lets a provider identify which connection a request belongs to (see "Why canonical?" below). Populated the same way as `getDisplayName` - only for a streaming attach, off the owning device's own info - with the same unassigned cases, including always unassigned for a device connection request. |
| `getSerialNumber(IString**)` | The device's serial number - see `getManufacturer` for what it's used for and how/when it's populated; same unassigned cases. |
| `getModel(IString**)` | The device's model - see `getManufacturer` for what it's used for and how/when it's populated; same unassigned cases. |
| `getAuthenticationMethod(IAuthenticationMethod**)` | The authentication method the provider must provide a credential for, read from `IAuthenticationConfig` when the request was built. Its own id names the negotiated authentication method. |

**Why canonical?** Only a streaming attach's request ever carries `(manufacturer, serialNumber, model)` at all - sourced from the already-connected owning device's own info - so a provider can use them there to identify which connection a request belongs to. A device connection request never carries them at all, so the connection string is the *only* identifying key a provider has for one - which only works reliably if it's canonical, so the same connection always identifies itself the same way no matter how the caller originally wrote it.

**Factory:** `CredentialRequestFromBuilder(builder)` — built from a `ICredentialRequestBuilder`.

---

### `ICredentialRequestBuilder`

Builds `ICredentialRequest` objects.

| Member | Description |
|---|---|
| `build(ICredentialRequest**)` | Builds and returns a `CredentialRequest` from the currently configured values. Fails if `componentType`, `connectionString`, or `authentication method` was never set. |
| `setComponentType` / `getComponentType` | The component type the request is being built for. Required. |
| `setConnectionString` / `getConnectionString` | The *canonical* connection string for this attempt - expected to already be resolved via `onGetCanonicalConnectionString` before being set here. Required. |
| `setManufacturer` / `getManufacturer` | The manufacturer of the device the connection is being established to or for - a request can be for a direct connection to that device, or for a streaming connection attached to it. Optional - leave unset if not known. |
| `setSerialNumber` / `getSerialNumber` | The serial number of the device the connection is being established to or for - a request can be for a direct connection to that device, or for a streaming connection attached to it. Optional - leave unset if not known. |
| `setModel` / `getModel` | The model of the device the connection is being established to or for - a request can be for a direct connection to that device, or for a streaming connection attached to it. Optional - leave unset if not known. |
| `setDisplayName` / `getDisplayName` | The device's name to show the user. Optional - leave unset when unknown. |
| `setAuthenticationMethod` / `getAuthenticationMethod` | The authentication method the provider must supply a credential for - typically read from `IAuthenticationConfig` when the request is built. Its own id names the negotiated authentication method. Required. |

**Factory:** `CredentialRequestBuilder()`

---

### Credentials

There is no dedicated credential interface - a credential is simply a `Dict<IString, IString>`, one entry per field, keyed by each field's own id (see `ICredentialField::getId`). An entry for a field that isn't required may be left out entirely, or supplied empty; `IAuthenticationMethodInternal::acceptsCredentials(credential)` is what checks a given credential actually carries a non-empty value for every field the method marks required. Both a credential provider's `requestCredentials` and a caller directly supplying a credential (`IAuthenticationConfig`'s `"SuppliedCredential"` property) produce/consume this exact same shape - see [§5](#5-supplying-a-credential-directly).

A method with no fields needs no credentials at all - an empty dictionary always satisfies it trivially, `"SuppliedCredential"` stays at its empty default, and no credential provider is ever consulted for it (see [§8](#8-module-level-implementation)); selecting `"AuthenticationMethod"` to such a method is the entire authentication step.

---

### `ICredentialProvider`

Supplies the credentials requested via an `ICredentialRequest` — by prompting the user, reading a file, or fetching from a credential store. Declares no supported field kinds up front - if a request needs a field kind an implementation can't handle, `requestCredentials` is expected to fail on its own (an ordinary failed call, propagated like any other failure from it, all the way back to the original `addDevice`/`addStreaming` caller), rather than being pre-checked by the module manager. No caching of any kind - every call is resolved fresh.

| Member | Description |
|---|---|
| `getDescription(IString**)` | A human-readable description of the provider - e.g. for identifying it in a log message or an error. Not an id - not expected to be unique, or stable across implementations. |
| `requestCredentials(ICredentialRequest*, IDict**)` | Requests credentials for the given request, in the shape described by its authentication method — obtaining them interactively (prompting, reading a file, etc.) every time. Returns a dictionary of field value(s), keyed by their own field id. Fails if this implementation can't actually collect one of the request's field kinds. |

**Factory:**
- `CmdLineCredentialProvider()` — the only credential provider implementation the SDK ships. Prompts the user for credentials via the command line, supporting every `CredentialFieldKind`: `Text`/`Secret` fields are read as plain (or masked) input; `FilePath` is read exactly the same way as `Text`, then validated locally (must name an existing, accessible file) right after being read. Every call prompts fresh - nothing is remembered between calls. Cancelling the prompt (EOF - Ctrl+D on POSIX, Ctrl+Z then Enter on Windows) throws `ProviderCancelledException`.

---

## 2. Extensions to Existing Interfaces

A module declares which authentication methods a Device or Streaming type supports by calling
`IComponentTypeBuilder::addAuthenticationMethod(method)` once per method, while building the type - the first
one added becomes the type's default selection on a freshly-built `IAuthenticationConfig`. A type with no
methods added this way simply doesn't support authentication: `componentType.createAuthenticationConfig()`
throws `NotSupportedException`, and `IComponentTypePrivate::getSupportedAuthenticationMethods()` returns an
empty dict - there is no implicit fallback to `"Anonymous"` or any other method. Server and FunctionBlock types
don't carry this data at all, regardless of how they were built: both calls throw `NotImplementedException`
for these sorts instead, distinguishable from the "Device/Streaming type, nothing added" case above (see
`core/opendaq/opendaq/tests/test_factories.cpp` for this distinction tested directly).

There is no dedicated "add an authenticated device" entry point anywhere in the public API - only the plain,
config-only `addDevice`/`addStreaming` exist. A caller that wants an authenticated connection instead attaches
the `IAuthenticationConfig` to the `config` property object it already passes to `addDevice`/`addStreaming`,
under the `"__AuthenticationConfig"` property name - see
`examples/applications/cpp/credential_providers/credential_providers.cpp`'s `WithAuthenticationConfig` helper
for the exact pattern, and [§3](#3-two-ways-to-add-a-device) below. This is a temporary bridge, until
`IAuthenticationConfig` becomes a real part of the add-device/add-streaming config schema with a dedicated
parameter of its own.

A type with at least one method added always goes through `createAuthenticatedDevice`/`createAuthenticatedStreaming`
- even a field-less one like `"Anonymous"`, with an empty credential and nothing to verify. `createDevice`/
`createStreaming` only ever run for a type with *no* methods added at all (see
[§3](#3-two-ways-to-add-a-device)).

### `IInstanceBuilder`

| Member | Description |
|---|---|
| `getCredentialProvider(ICredentialProvider**)` | The credential provider set so far, or `nullptr` if none. |
| `setCredentialProvider(ICredentialProvider*)` | Sets the credential provider to pass to the `ModuleManager` the builder constructs, replacing whatever was set before. At most one can ever be registered on an instance - there is no way to add more than one. Throws `AlreadyExistsException` at build time if a custom `IModuleManager` was also set via `setModuleManager` - set the credential provider directly on that custom module manager instead (`ModuleManager`/`ModuleManagerMultiplePaths`'s own `credentialProvider` parameter). |

### `IModuleManager`

The credential provider lives here - `IInstanceBuilder::setCredentialProvider` (or a direct
`ModuleManager`/`ModuleManagerMultiplePaths` call) passes it in as a constructor parameter, which
`ModuleManagerImpl` keeps privately and consults internally (see [§8](#8-module-level-implementation)). There
is no public getter for it anywhere, on `IModuleManager` or otherwise - once set, it can't be read back, only
used.

### `IModule` (module-author-facing, not called directly by application code)

| Member | Description |
|---|---|
| `createAuthenticatedDevice(IDevice**, IString* connectionString, IComponent* parent, IPropertyObject* config, ILoginParameters* loginParameters)` | Creates a device authenticated with the given, already-resolved login parameters (authentication method + credentials). Called by `ModuleManagerImpl` - never directly by application code - once it has itself resolved the method and credentials (see [§8](#8-module-level-implementation)). The base `Module` implementation dispatches straight to the overridable `onCreateAuthenticatedDevice` hook. |
| `createAuthenticatedStreaming(IStreaming**, IString* connectionString, IPropertyObject* config, ILoginParameters* loginParameters)` | Module-level counterpart for streaming, dispatching to `onCreateAuthenticatedStreaming`. |
| `getCanonicalConnectionString(IString* connectionString, IString** canonicalConnectionString)` | Returns the canonical form of `connectionString` (routing prefix trimmed, every parameter made explicit) - used as the stable identifying key within a `ICredentialRequest` (see above). The base implementation returns `connectionString` unchanged. |

### `ILoginParameters`

Carries the already-resolved authentication method and credentials handed to `createAuthenticatedDevice`/`createAuthenticatedStreaming` above - built by `ModuleManagerImpl`, never by application code, via `ILoginParametersBuilder`. Immutable.

| Member | Description |
|---|---|
| `getAuthenticationMethod(IAuthenticationMethod**)` | The authentication method the credentials are shaped for - the one selected by `IAuthenticationConfig` among the component type's own supported methods. |
| `getCredentials(IDict**)` | The resolved credentials - a value for each of the authentication method's mandatory fields. Always assigned - empty for a method with none (e.g. `"Anonymous"`). |

**`ILoginParametersBuilder`:**

| Member | Description |
|---|---|
| `build(ILoginParameters**)` | Builds and returns a `LoginParameters` from the currently configured values. Throws `InvalidParameterException` unless `authenticationMethod` was set, and `credentials` satisfies its required fields. |
| `setAuthenticationMethod` / `getAuthenticationMethod` | The authentication method the credentials are shaped for. Required - `build()` fails if never set. |
| `setCredentials` / `getCredentials` | The resolved credentials - a value for each of the authentication method's mandatory fields. Defaults to empty if never set. |

**Factories:** `LoginParametersBuilder()` - no values set; `LoginParametersFromBuilder(builder)` - built from a `ILoginParametersBuilder`.

---

## 3. Two Ways to Add a Device

A type with *no* authentication methods added only ever connects through the plain `Module::createDevice` - no
`IAuthenticationConfig` can even be built for it (`createAuthenticationConfig()` throws). A type with *any*
method added always goes through `createAuthenticatedDevice` instead - even for a field-less method like
`"Anonymous"`, with an empty credential and nothing to verify. Which of these two applies depends only on
whether the type has a supported method at all, never on whether a config was attached.

### No config attached

```cpp
auto device = instance.addDevice("daq://openDAQ_1234");
```

Resolves the type's own default (first-added) method, then proceeds exactly like an attached config would
([§6](#6-authentication-flow)/[§8](#8-module-level-implementation)).

### Attaching an `IAuthenticationConfig`

```cpp
// Attaches an `IAuthenticationConfig` to an otherwise plain config object, under the property name
// `ModuleManagerImpl` looks for - see `credential_providers.cpp`'s own copy of this exact helper.
PropertyObjectPtr WithAuthenticationConfig(const AuthenticationConfigPtr& authConfig)
{
    auto config = PropertyObject();
    config.addProperty(ObjectProperty("__AuthenticationConfig", authConfig));
    return config;
}

auto device = instance.addDevice("daq://openDAQ_1234", WithAuthenticationConfig(authenticationConfig));
```

Same connection string, but the caller's own explicitly selected method is resolved instead of the type's
default.

Attaching one for a type with no authentication methods added doesn't throw: `ModuleManagerImpl` logs a warning
and falls back to the plain path, exactly as if none had been attached.

---

## 4. Streaming Authentication

Attaching a streaming connection is authenticated **independently** of however the device itself got connected — a device authenticated via PIN can have a streaming source attached to it that goes through its own, entirely separate credential request. `IDevice::addStreaming`'s `config` resolves exactly like `addDevice`'s does ([§3](#3-two-ways-to-add-a-device)): the streaming type's default method with no config attached, or the caller's explicit selection with one. A streaming type with no methods added goes through `createStreaming`; one with any method added always goes through `createAuthenticatedStreaming` instead, field-less resolved method included.

```cpp
// Resolves the streaming type's default method:
device.addStreaming("daq.credential_demo_streaming://credential_demo_device");

// Authenticated streaming attach, with its own independent authentication config:
auto streamingAuthConfig = streamingType.createAuthenticationConfig();
streamingAuthConfig.setAuthenticationMethod("PrivateKeyFile");
device.addStreaming("daq.credential_demo_streaming://credential_demo_device", WithAuthenticationConfig(streamingAuthConfig));
```

### Auto-attach

`StreamingSourceManager` (behind `PrioritizedStreamingProtocols`/`AutomaticallyConnectStreaming`) has no `IAuthenticationConfig` of its own - it always resolves the streaming type's own default method, same as a config-less `addStreaming` would. A default method with fields still needs a registered, compatible credential provider - failing (`CredentialsRequiredException`) for just that one source if none is available, caught locally and only logged, never failing the surrounding `addDevice` call. A source that needs a *different* method must be attached manually, via `addStreaming`'s own config, as shown above.

---

## 5. Supplying a Credential Directly

Setting `"SuppliedCredential"` hands the module a credential it already has — a dictionary of field value(s), keyed by their own field id — instead of having the registered provider obtain one interactively. A non-empty write is validated against the *currently selected* `"AuthenticationMethod"` (see [§1](#1-core-interfaces)): it must carry a non-empty value for every field the method marks required, or the write throws `InvalidParameterException` - so select the method first:

```cpp
auto authConfig = deviceType.createAuthenticationConfig(); // UserNamePassword is the default method

auto credential = Dict<IString, IString>();
credential.set("UserName", "user");
credential.set("Password", "pass");
authConfig.setSuppliedCredential(credential);

auto device = instance.addDevice("daq://openDAQ_1234", WithAuthenticationConfig(authConfig));
```

Whether or not a credential provider is registered, the module uses the supplied credential directly and authenticates with it, with no prompt of any kind - the provider is never consulted at all when a credential is supplied this way.

---

## 6. Authentication Flow

Both ways of adding a device converge on the same entry point at the application level — a connection string and a config object. A type with no authentication methods always resolves to the module's plain `createDevice`; a type with any method always resolves to `createAuthenticatedDevice` instead. Attaching a config only changes *which* method gets resolved (the caller's explicit pick instead of the type's default) - not which of these two it goes through ([§3](#3-two-ways-to-add-a-device)).

For a type with any method, the module manager hands off to the module together with the authentication config. It works out which credentials are needed - the single registered credential provider, if any, obtaining them interactively, a directly-supplied credential used instead (see [§5](#5-supplying-a-credential-directly)), or nothing at all for a field-less method - and builds a fresh credential request for it, whether this is a first-time connection or a reload. The module then verifies the resolved credentials and constructs the device. The authentication config itself (not the credential request, which is never saved) is then stored on the device afterward, in reduced form - so a future reload can repeat this same resolution process rather than needing the original credentials to be saved anywhere.

Sequence (the same resolution applies verbatim to authenticating a streaming connection, [§4](#4-streaming-authentication) — only the surrounding API call differs):

1. Application calls `instance.addDevice(connectionString, config)` - config optional, as long as the type has at least one supported method.
2. Module manager resolves the method - the attached config's selection, or the type's default if none was attached - and hands off to the module together with the authentication config.
3. Module manager resolves the credential: a directly-supplied one is used as-is, the registered provider is asked interactively, or nothing at all for a field-less method.
4. Module verifies the resolved credential (handed to it as already-resolved login parameters) against the selected authentication method and constructs the device.
5. The authentication config (never the credential itself) is stored on the device, so a future reload repeats this same resolution instead of needing the original credential saved anywhere.

---

## 7. Application-Level Usage

### Registering the credential provider

```cpp
auto credentialProvider = CmdLineCredentialProvider();

auto instanceBuilder = InstanceBuilder();
instanceBuilder.setCredentialProvider(credentialProvider);
auto instance = instanceBuilder.build();
```

Provider registration is **never serialized** — a reloaded instance must register its own provider independently, since provider setup is platform-/host-specific. At most one credential provider can ever be registered, and only at instance-build time (`IInstanceBuilder::setCredentialProvider`) - there is no way to register one on an already-built `Context`, and no way to register more than one. `CmdLineCredentialProvider`, used above, supports every format, so a single instance of it is enough regardless of which authentication methods the connected devices use.

### Selecting an authentication method

`componentType.createAuthenticationConfig()` returns one **self-contained** config listing every method the named device type supports as a candidate of its `"AuthenticationMethod"` selection property (see [§1](#1-core-interfaces)) - not a separate config per method. The application either accepts the type's default selection, or switches it to a different supported method by id, directly through the typed setter:

```cpp
auto deviceType = instance.getAvailableDeviceTypes().get("CredentialDemoDevice");

// Use the type's default authentication method:
auto config = deviceType.createAuthenticationConfig();
auto device = instance.addDevice("daq://openDAQ_1234", WithAuthenticationConfig(config));

// Or explicitly select a specific supported method instead (e.g. "Pin"), on a config of its own:
auto pinConfig = deviceType.createAuthenticationConfig();
pinConfig.setAuthenticationMethod("Pin");
auto device2 = instance.addDevice("daq://openDAQ_1234", WithAuthenticationConfig(pinConfig));
```

(`WithAuthenticationConfig` here is the same one-line helper shown in [§3](#3-two-ways-to-add-a-device) - wraps the config into a plain property object under `"__AuthenticationConfig"`.)

### Example: manipulating the config as a plain property object

`IAuthenticationConfig` extends `IPropertyObject` (see [§1](#1-core-interfaces)), so the application can equally configure and inspect it with plain property object calls instead of the typed getters/setters used elsewhere in this section - `componentType.createAuthenticationConfig()` already returns a fresh, independent, self-contained config, ready to tune directly:

```cpp
auto authConfig = deviceType.createAuthenticationConfig();
authConfig.setPropertySelectionValue("AuthenticationMethod", "Pin");

StringPtr authenticationMethodId = authConfig.getPropertySelectionValue("AuthenticationMethod");
std::cout << "Authentication method id: " << authenticationMethodId << std::endl; // "Pin" - candidates are plain id strings, no cast needed

auto device = instance.addDevice("daq://openDAQ_1234", WithAuthenticationConfig(authConfig));
```

See `demoAuthenticationConfigAsPropertyObject` in the `credential_providers` example app for the full, runnable version of this.

### Example: private-key challenge authentication

```cpp
auto credentialProvider = CmdLineCredentialProvider();

auto instanceBuilder = InstanceBuilder();
instanceBuilder.setCredentialProvider(credentialProvider);
auto instance = instanceBuilder.build();

auto deviceType = instance.getAvailableDeviceTypes().get("CredentialDemoDevice");

// FilePath variant — module reads and parses the PEM file itself.
auto privateKeyFileConfig = deviceType.createAuthenticationConfig();
privateKeyFileConfig.setAuthenticationMethod("PrivateKeyFile");
auto device = instance.addDevice("daq://openDAQ_1234", WithAuthenticationConfig(privateKeyFileConfig));
```

`"PrivateKeyFile"` is only among `deviceType`'s candidates when `credential_demo_module` was built with its
`DAQMODULES_CREDENTIAL_DEMO_MODULE_ENABLE_PRIVATE_KEY_AUTH` option on (off by default) - `setAuthenticationMethod`
throws `NotFoundException` otherwise.

See [§4](#4-streaming-authentication) for authenticating a streaming connection and [§5](#5-supplying-a-credential-directly) for supplying a credential directly.

### Save and reload

Saving an instance persists the connected device's `AuthenticationConfig` as part of the tree - but only in its reduced, serialized form (see [Serialization](#serialization) in [§1](#1-core-interfaces)): every candidate authentication method and the selected method's id. No credential (supplied or obtained interactively) is ever saved. Reloading that saved configuration into a new instance rebuilds the config directly from the saved authentication methods and method id - it never re-consults the module/type registry. The device then re-authenticates from scratch through the same provider-resolution path a fresh connection would (see [§8](#8-module-level-implementation)): the new instance must have its own compatible credential provider registered, or the reload fails with `CredentialsRequiredException`, and that provider is asked for credentials again exactly like it would for a first-time connection - prompting fresh every time, since nothing is ever cached. A failure reloading one particular device is caught and logged, not fatal to the rest of the load.

See `core/opendaq/security/doc/credential_usage_scenarios.md` §6 for the edge cases here in more
detail, including what happens when the live device type's own supported methods have changed since the save.

---

## 8. Module-Level Implementation

This section describes what happens internally whenever a type supports at least one authentication method -
this resolution runs on every `addDevice`/`addStreaming` call for such a type, config attached or not (see
[§3](#3-two-ways-to-add-a-device)/[§6](#6-authentication-flow)) - none of it is visible to, or called directly by,
application code. Steps 1-3 and 5 are `ModuleManagerImpl`'s own responsibility, not the module's - a module
implementation only ever sees step 4, through its own
`onCreateAuthenticatedDevice`/`onCreateAuthenticatedStreaming` hook.

1. **Resolve the authentication method to use.** `ModuleManagerImpl::resolveDefaultAuthenticationConfig` returns an already-attached `IAuthenticationConfig` as-is; otherwise, if the target type supports authentication, it builds a fresh default one via `componentType.createAuthenticationConfig()`. Any type with at least one supported method proceeds from here - a field-less resolved method (e.g. `"Anonymous"`) included.

2. **Build the credential request.** `ModuleManagerImpl` builds a new `ICredentialRequest` via `ICredentialRequestBuilder`, populating connection string (canonicalized via the module's own `onGetCanonicalConnectionString` override), the authentication method, and component type - the same way whether this is a fresh connection or a reload, since the request is always built fresh from whichever `IAuthenticationConfig` is in hand (a freshly-built one, or one reconstructed from its reduced saved form - see step 5 and [Serialization](#serialization) in [§1](#1-core-interfaces)). Manufacturer/serial number/model/display name are only ever populated for a streaming attach ([§4](#4-streaming-authentication)) - read off the streaming's already-connected *owning* device's own info, never the streaming source's own identity (which isn't known yet) - and left unassigned entirely for a device connection request.

3. **Resolve the credentials.** `ModuleManagerImpl::resolveCredentials` returns an empty credential immediately for a field-less method (e.g. `"Anonymous"`) - no provider ever consulted. Otherwise it reads the authentication config's `getSuppliedCredential()`, and branches on whether it's non-empty (supplied) or empty (not supplied):
   - **A credential is supplied (non-empty):** no provider is consulted at all — the supplied dictionary (already validated against the method's required fields when it was set, see [§1](#1-core-interfaces)) is used directly as the credential for this connection.
   - **No credential supplied (empty):** throws `CredentialsRequiredException` immediately if no provider is registered at all; otherwise the registered provider is asked via `provider.requestCredentials(request)`, obtaining credentials - a dictionary of field value(s), keyed by their own field id - interactively, fresh every time. What it returns is then checked against the method's required fields the same way a directly-supplied credential is - `CredentialsRequiredException` again if incomplete. Any other failure the provider's own `requestCredentials` raises (a cancelled prompt, an unsupported field kind, etc.) propagates up unmodified.

4. **Construct the component and authenticate.** The only step a module's own `onCreateAuthenticatedDevice`/`onCreateAuthenticatedStreaming` override implements: construct the device or streaming object and run its authentication step — reading the credentials' property values (already resolved and handed in as the `authenticationMethodId`/`credentials` parameters) and verifying them against whatever the specific method requires (a fixed value, a signed challenge, nothing at all for a field-less method, etc.). A mismatch throws `AuthenticationFailedException`, and construction fails.

5. **Persist the authentication config for later reload (devices only).** Also handled by `ModuleManagerImpl` itself, after the device is constructed: it stores the *original* authentication config it resolved on the newly created device (`IComponentPrivate::setAuthenticationConfig`) — its custom serialization then reduces this, on save, to every candidate authentication method and the selected method id (see [Serialization](#serialization) in [§1](#1-core-interfaces)) — so a future reload can repeat this same resolution process (see [Save and reload](#save-and-reload)) rather than needing the credential itself, or how it was obtained, to be saved anywhere. The module implementation never sees this happen.
