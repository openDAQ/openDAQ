# Credential Provider Framework — Usage Scenarios

A backlog of user stories and use-case scenarios for exercising `IComponentType::createAuthenticationConfig`,
`addDevice`/`addStreaming`, and the credential provider framework end to end - meant to drive example/test
implementations that verify the API is actually usable, not just that individual methods return the right
type. Each scenario names the concrete API calls and expected outcome (return value, exception, or observable
side effect) so it can be implemented directly, ideally against `credential_demo_module` the way
`credential_providers.cpp` already does.

**Temporary workaround**
There is only the plain, config-only `addDevice`/
`addStreaming` calls. A caller that wants an authenticated connection stashes the
`IAuthenticationConfig` onto the plain `config` property object under the `"__AuthenticationConfig"` property
name, temporarily, until `IAuthenticationConfig` becomes a real part of the add-device config schema - see
`examples/applications/cpp/credential_providers/credential_providers.cpp`'s `WithAuthenticationConfig` helper
for the current, correct pattern. Every `addDevice(connectionString, authConfig)` call below should be read as
`addDevice(connectionString, WithAuthenticationConfig(authConfig))`.

At most one credential provider can ever be registered for an entire `Instance`
(`IInstanceBuilder::setCredentialProvider`, before build) - there is no per-config provider selection anywhere
in this API, so none of the scenarios below involve choosing among multiple registered providers.

---

## 1. Discovery & feasibility

Before ever prompting a user or attempting a connection, an application needs to know what's even possible.

### 1.1 — Does this type support authentication at all?

As an application developer, I want to check whether a given device/streaming type supports authentication, so
I can decide whether to offer an "authenticated connect" option in my UI at all.

- **Given** a device/streaming type whose module added one or more authentication methods when building it
  (`ComponentTypeBuilder::addAuthenticationMethod`, e.g. `"CredentialDemoDevice"`)
  **When** I call `componentType.createAuthenticationConfig()`
  **Then** it succeeds, returning a config whose `"AuthenticationMethod"` selection has one candidate (a plain
  id string) per method added - the first one added starts out selected.
- **Given** a Device or Streaming type whose module never called `addAuthenticationMethod` at all
  **When** I call `componentType.createAuthenticationConfig()`
  **Then** it throws `NotSupportedException` ("This component type supports no authentication method") -
  there is no implicit "every type supports at least Anonymous" fallback; a type that wants Anonymous among
  its options has to add `StandardAnonymousAuthenticationMethod()` explicitly, the way `credential_demo_module`
  does.
- **Given** a Server or FunctionBlock type (sorts authentication is not a concept for at all, regardless of how
  the type was built)
  **When** I call `componentType.createAuthenticationConfig()` or `IComponentTypePrivate::getSupportedAuthenticationMethods()`
  **Then** both throw `NotImplementedException` - distinguishable from the "Device/Streaming type, zero
  methods added" `NotSupportedException` case above: a developer can tell "this sort never supports auth at
  all" apart from "this specific type just doesn't right now."
- **Given** the plain, anonymous path is what's wanted instead
  **When** I call `instance.addDevice(connectionString)` on any type, whether or not it supports authentication
  **Then** it succeeds regardless - the two paths are fully independent (see the API reference, §3) - *unless*
  the type's own default authentication method requires credentials (see 7.4 below, a sharper edge than it
  first looks).

### 1.2 — Is authentication actually *usable* right now, given the registered provider?

As an application developer, I want to know not just whether a type *supports* authentication, but whether I
can actually *complete* it with whatever's registered on my instance - these are different questions. There is
no client-side way to check the second one in advance - a provider declares no supported field kinds up front
(deliberately: see the API reference, §1) - so the only way to find out is to actually attempt the connection
and see whether `requestCredentials` fails.

- **Given** zero credential providers registered on the instance at all, and a type whose selected method
  requires credentials
  **When** I build the default config and call `instance.addDevice(connectionString, authConfig)`
  **Then** it fails with `CredentialsRequiredException` ("no credential provider is registered") - a good
  negative-path example distinct from 1.1's "type doesn't support auth at all" cases: here the *type* supports
  it, the *instance* just can't currently serve it.
- **Given** a registered provider that can't actually handle one of the method's field kinds (a custom
  provider, not `CmdLineCredentialProvider` - that one handles every kind), or one that fails for any other
  reason (a cancelled prompt, a generic I/O error, etc.)
  **When** I call `instance.addDevice(...)`
  **Then** whatever `ErrCode`/exception that provider's own `requestCredentials` fails with propagates all the
  way up through the add-device call chain to the caller.
- **Given** a registered provider that returns credentials missing one or more fields the selected method
  requires
  **When** I call `instance.addDevice(...)`
  **Then** it fails with `CredentialsRequiredException` ("Credential provider returned credentials missing one
  or more required fields") - a shape check against the method's own fields, done before the credential ever
  reaches the module's own verification step. Distinct from the module rejecting a shape-valid-but-*wrong*
  credential, which is `AuthenticationFailedException` instead (§2.2).

---

## 2. Manual authenticated device — default config, unmodified

The user's original bullets, filled in.

### 2.1 — Default config + correct credentials, compatible provider registered

As an application developer, I want the simplest possible path to work: get the default config, hand it
straight to `addDevice` with no changes, and have it succeed when the user enters correct credentials.

- **Given** a `CmdLineCredentialProvider` (or equivalent, supporting the default method's format) registered on
  the instance
  **And** `authConfig = deviceType.createAuthenticationConfig()` used as-is (default `"AuthenticationMethod"`
  selection)
  **When** I call `instance.addDevice(connectionString, authConfig)` and supply correct credentials at the
  prompt
  **Then** the device is added, `device.getInfo()` is queryable, and `device.getAvailableDeviceTypes()`/other
  calls on it work normally.

### 2.2 — Default config + incorrect credentials

- **Given** the same setup as 2.1
  **When** incorrect credentials are supplied at the prompt
  **Then** `addDevice` throws `AuthenticationFailedException` (the module's own verification step - see the
  API reference, §8 step 4), the device is **not** added.

### 2.3 — Default config, no provider registered at all

- **Given** an instance built with no `setCredentialProvider` call
  **When** I call `addDevice` with a default config attached, selected on a method that requires credentials
  **Then** it throws `CredentialsRequiredException` before ever attempting to prompt for anything (no hang
  waiting on stdin).

---

## 3. Manual authenticated device — supplied credential

### 3.1 — Correct supplied credential, provider registered

- **Given** `authConfig = deviceType.createAuthenticationConfig()` with the desired `"AuthenticationMethod"`
  selected (`setAuthenticationMethod`) and `"SuppliedCredential"` set (`setSuppliedCredential`) to a
  correctly-shaped, correct `credential`, with a format-compatible credential provider registered on the
  instance
  **When** I call `instance.addDevice(connectionString, authConfig)`
  **Then** authentication succeeds with **no interactive prompt at all** - the provider
  is never consulted at all for a supplied credential.

### 3.2 — Correct supplied credential, no provider registered

- **Given** the same as 3.1 but with no credential provider registered on the instance at all
  **When** I call `instance.addDevice(...)`
  **Then** authentication still succeeds with no prompt the supplied credential is used directly, no provider
  ever consulted.

### 3.3 — Incorrect supplied credential

- **Given** a supplied credential shaped correctly but with a wrong value (e.g. wrong password, wrong PIN)
  **When** I call `instance.addDevice(...)`
  **Then** it throws `AuthenticationFailedException` - verifying that a caller can't bypass the module's own
  verification step just by constructing the credential shape correctly; supplying a credential skips the
  *provider*, not the *authenticator*.

### 3.4 — Malformed supplied credential (robustness)

As an application developer, I want a clear failure, not a confusing one, if I get the supplied-credential keys
wrong.

- **Given** a supplied credential dictionary missing a *required* field (e.g. a `UserNamePassword` credential
  missing `"Password"`, or carrying it with an empty value)
  **When** I call `authConfig.setSuppliedCredential(...)` (or `setPropertyValue("SuppliedCredential", ...)`)
  **Then** the write itself throws `InvalidParameterException` immediately (the selected method's
  `IAuthenticationMethodInternal::acceptsCredentials` rejects it), before `addDevice` is ever reached -
  confirming the shape check happens at write time, not deferred to connection time.
- **Given** a supplied credential dictionary carrying an extra, unrecognized key (one that names no field of
  the currently selected method) alongside every required field correctly filled in
  **When** I call `instance.addDevice(...)`
  **Then** authentication still succeeds - a dictionary is loose by design, only required fields are checked;
  an unrecognized key is simply never read by the authenticator, not an error.

---

## 4. Tuning the default config before use

Between "use the unmodified default" (§2) and "supply a credential directly" (§3), there's the actual expected
common path: get the default, change one or two things, use it. With no per-config provider selection, the
only thing left to tune here is `"SuppliedCredential"`, besides the selected method itself.

### 4.1 — An incompatible `"SuppliedCredential"` write is rejected

- **Given** a config with `"AuthenticationMethod"` selected to a two-field method (e.g. `UserNamePassword`)
  **When** I call `setSuppliedCredential(...)` with a non-empty dictionary shaped for a *different* method
  (e.g. a single `"Pin"` entry - so missing both `"UserName"` and `"Password"`, the currently selected method's
  required fields)
  **Then** the write throws `InvalidParameterException` and `getSuppliedCredential()` keeps its previous value
  - confirming validation happens against the *currently selected* method's required fields, not just "any
  non-empty dictionary goes."

### 4.2 — Changing the selected authentication method always resets `"SuppliedCredential"`

- **Given** a config with a valid, non-empty `"SuppliedCredential"` set for the currently-selected
  `"AuthenticationMethod"`
  **When** I switch `"AuthenticationMethod"` to a *different* method (`setAuthenticationMethod`) - even one
  whose required fields the existing credential happens to still satisfy (e.g. two methods that coincidentally
  share a required field name)
  **Then** the selection change succeeds (no exception) and `getSuppliedCredential()` is an empty dictionary
  afterward regardless - the reset is unconditional on a method change, never contingent on whether the old
  value would still "fit" the new method. Calling `addDevice` afterward with no credential re-supplied falls
  through to the normal provider-based path (§2).

---

## 5. Streaming authentication

### 5.1 — Streaming authenticated independently of its device

- **Given** a device connected via one method (or not authenticated at all)
  **When** a streaming connection to it is added via `device.addStreaming(connectionString, streamingAuthConfig)`
  using a *different* method
  **Then** both succeed independently - the streaming leg's authentication outcome (success or failure) is
  unaffected by how the device itself was connected, and vice versa.

### 5.2 — Auto-attached streaming authenticates via the streaming type's *default* method, same as a device would

As an application developer relying on `PrioritizedStreamingProtocols`/`AutomaticallyConnectStreaming`, I want
to know whether this path can end up needing a registered provider behind my back, since there's no way to
attach my own authentication config to it.

`StreamingSourceManager`'s auto-attach has no explicit `IAuthenticationConfig` of its own to supply, but
`ModuleManagerImpl::onCreateStreaming` resolves the streaming type's own *default* authentication config
regardless - exactly the same unconditional resolution a plain, config-less `addDevice` goes through (see
§7's last bullet). Whether auto-attach ends up authenticated or not is entirely a property of the streaming
type's default (first-added) method, not something the auto-attach mechanism itself decides.

- **Given** a device added with auto-attach streaming config enabled, to a streaming type whose *default*
  method is field-less (e.g. `credential_demo_module`'s `"CredentialDemoStreaming"`, which adds Anonymous
  first, the same way its device type does)
  **When** the device connects
  **Then** the auto-attached streaming source is present and working, and was never asked for credentials of
  any kind - confirmable by registering *no* credential provider at all and the auto-attach still succeeding.
- **Given** a device added with auto-attach streaming config enabled, to a streaming type whose *default*
  method instead requires credentials (unlike `credential_demo_module`'s own streaming type), and no compatible
  credential provider registered on the instance
  **When** the device connects
  **Then** auto-attach fails to attach that streaming source (`CredentialsRequiredException` from the same
  `resolveCredentials` path a manual, authenticated `addStreaming` would hit per §2.3) - confirm this doesn't
  abort the whole `addDevice` call, only leaves that streaming source un-attached (device.getStreamingSources()
  not containing it), the same way 6.2's failed reconnect leaves the device itself simply absent rather than
  failing `loadConfiguration` outright.
- There is no way to attach auto-attach's *own* authentication config at all - the only way to control which
  method it resolves to is indirectly, via which method the streaming type's module chose to add first.

---

## 6. Save & load

The custom persistence model: `AuthenticationConfigImpl`'s own `serialize`/`Deserialize` write/read two
pieces - every candidate authentication method (the full `IAuthenticationMethod` dict, not just their ids) and
the selected method's id - as a dedicated, self-contained object, never through generic property serialization.
`"SuppliedCredential"` is never written. There is no credential-provider information saved at all - a provider
is registered once per `Instance`, not per config, so nothing about it needs to survive a save/reload round
trip. Reload rebuilds the config directly from these two saved pieces - it never re-consults the module/type
registry at all.

### 6.1 — Reload with a compatible provider registered succeeds

- **Given** an authenticated device saved via `instance.saveConfiguration()`
  **When** a new instance registers a format-compatible credential provider, then calls
  `loadConfiguration(savedConfiguration)`
  **Then** the device reconnects, re-authenticating from scratch through the normal credential-resolution path
  - prompting fresh every time, since nothing is ever cached.

### 6.2 — Reload with no compatible provider registered

- **Given** the same saved configuration
  **When** the new instance registers no provider at all, or one that doesn't support the persisted method's
  format
  **Then** `loadConfiguration` fails to reconnect the device (confirm via `reloadedInstance.getDevices()` being
  empty).

### 6.3 — Deserializing a corrupted saved config

Since `Deserialize` rebuilds the config entirely from its own two saved pieces - never from the type's
*current* live definition - a method the module has since renamed or dropped can't actually arise here; the
saved authentication-methods dict and the saved selected id either agree with each other or the save was
corrupted some other way (hand-edited, truncated, etc.).

- **Given** a saved configuration whose `selectedAuthenticationMethodId` names a key absent from its own saved
  authentication-methods dict (a corrupted save, not a stale-but-internally-consistent one)
  **When** the new instance calls `loadConfiguration(savedConfiguration)`
  **Then** the reload fails hard (`Deserialize`'s call to `setAuthenticationMethod` throws `NotFoundException`)
  rather than silently substituting some other method - confirming a reload never silently authenticates via a
  method the user didn't actually choose.

### 6.4 — A supplied-credential device does not skip the prompt on reload

- **Given** a device originally authenticated via a directly-supplied `"SuppliedCredential"` (§3.1/3.2)
  **When** it's saved and reloaded into a new instance with a compatible provider registered
  **Then** the reload prompts (or fails, if no provider is registered) exactly as a from-scratch connection
  would - confirming the supplied credential never survives serialization at all, only the candidate
  authentication methods and selected method id do.

### 6.5 — Reload when the live device type's own supported methods have diverged from the saved config

Unlike 6.3 (a corrupted save, caught by `IAuthenticationConfig` deserialization itself), this is about the save
being perfectly self-consistent but the *module* having since changed what it supports - e.g. updated between
save and reload. The device's own reload path (`GenericDevice::updateDevice`) re-attaches the deserialized
config verbatim onto a fresh add-device config, through the exact same `addDevice` call a live connection
would use; `ModuleManagerImpl::resolveDefaultAuthenticationConfig` returns an already-supplied config as-is,
without ever re-deriving it from the live device type - so there is no cross-check against what the type
*currently* supports beyond the coarse "does it support authentication at all right now" check.

- **Given** a device saved with a method the live module no longer recognizes at all (renamed, removed), while
  the live device type still supports authentication via some *other* method(s)
  **When** the new instance reloads it with a compatible provider registered
  **Then** there is no clean, generic "unknown method" failure at the framework level - the stale method and
  its saved fields are carried straight through to a credential request, and whether that's accepted is
  entirely up to the module's own per-method verification (e.g. `credential_demo_module`'s authenticator
  throws `AuthenticationFailedException` for an unrecognized method id - a module-specific outcome, not a
  framework guarantee).
- **Given** a device saved with authentication, where the live device type has since dropped authentication
  support *entirely*
  **When** the new instance reloads it
  **Then** the saved authentication config is silently discarded and the device reconnects via the plain,
  unauthenticated path instead (a warning logged, not a failure) - the same `authenticationConfigSupplied &&
  !authenticationSupported` fallback a live `addDevice` call with an unsupported type hits (§7).
- **Given** either case above actually fails (e.g. `AuthenticationFailedException`, or no compatible provider
  registered for whatever method ends up used)
  **Then** that failure is caught locally by the device's own reload handling and only logged as a warning -
  it does not fail `loadConfiguration` as a whole; this one device is simply absent afterward, same as §6.2.

### 6.6 — Overriding the selected method or supplying a fresh credential through update options *(future scenario - not implemented)*

As an application developer reloading a saved setup, I may want to change *how* a device re-authenticates -
switch it to a different supported method, or hand it a credential obtained some other way, e.g. read from a
file - without hand-editing the saved configuration first.

- **Given** `IDeviceUpdateOptions` has grown an authentication override alongside its other per-device settings
  (new manufacturer/serial number/connection string, update mode) - e.g. an override authentication method id,
  and a credential source (a literal value, or a file path to read one from)
  **When** I set these on the update options passed to `loadConfiguration` and reload a saved device
  **Then** the device re-authenticates using the overridden method/credential instead of the one it was saved
  with, without needing to touch the saved configuration itself.

**Not implemented yet** - sketched here as a placeholder for future work, not a scenario to write a test
against.

---

## 7. Boundary & negative checks worth having somewhere, even briefly

- Calling `componentType.createAuthenticationConfig()`/`getSupportedAuthenticationMethods()` on a Server or
  FunctionBlock type - confirm both throw `NotImplementedException` regardless of anything the type was built
  with, distinct from a Device/Streaming type that simply has zero methods added (`NotSupportedException`, see
  §1.1).
- Calling `instance.addDevice(connectionString, authConfig)` with an authentication config attached, against a
  type that does **not** support authentication at all - confirm this does **not** throw: the module manager
  logs a warning ("does not support authentication; creating it without authentication") and creates the
  device via the plain, unauthenticated path instead, exactly as if no config had been attached (see
  `ModuleManagerImpl::createDeviceInternal`'s own `authenticationConfigSupplied && !authenticationSupported`
  check).
- Two authentication attempts to the *same* connection string in quick succession without an intervening
  `removeDevice` (e.g. accidental double-click in a UI) - confirm the second attempt's failure/success mode is
  well-defined rather than racing the first.
- **A sharper edge than §1.1 suggests:** calling the plain `instance.addDevice(connectionString)` (no
  authentication config attached at all) against a type whose *default* (first-added) authentication method
  requires credentials - confirm that `createDeviceInternal` still resolves that default config internally and
  still attempts the *authenticated* path, needing a registered, compatible credential provider, even though
  the caller never asked for authentication explicitly. `credential_demo_module` avoids ever hitting this by
  adding `StandardAnonymousAuthenticationMethod()` first, so its own default is always field-less - a module
  that orders its methods differently would not get that same, perhaps-unexpected, protection for free.

---

## 8. Anonymous / field-less authentication

A method with no fields at all needs no credentials - selecting it is the entire authentication step, with no
provider and no supplied credential involved anywhere.

### 8.1 — Selecting a field-less method needs no registered provider

- **Given** a type whose module offers a field-less method (e.g. `"Anonymous"`) among its supported
  authentication methods, and *no* credential provider registered on the instance at all
  **When** I select that method (`setAuthenticationMethod("Anonymous")`) and call `instance.addDevice(...)`
  **Then** it succeeds - unlike §2.3, the absence of any registered provider doesn't matter here, since none is
  ever consulted for this method.

### 8.2 — A non-empty `"SuppliedCredential"` write is rejected while a field-less method is selected

- **Given** a config with a field-less method currently selected
  **When** I call `setSuppliedCredential(credential)` with any *non-empty* dictionary at all, even one that
  happens to carry no key any method actually uses
  **Then** the write throws `NotSupportedException` ("...has no credential fields - credential values do not
  apply to it") - `IAuthenticationMethodInternal::acceptsCredentials` itself refuses to even evaluate a
  field-less method against *any* non-empty credential, rather than returning `false` the way it would for an
  incomplete credential against a method that does have fields (compare §4.1, which gets
  `InvalidParameterException` instead). Writing an *empty* dictionary, however, succeeds trivially - it's
  simply the property's own default value.

### 8.3 — Switching *to* a field-less method resets an existing `"SuppliedCredential"`

- **Given** a config with a valid, non-empty `"SuppliedCredential"` set for the currently-selected method (one
  with fields)
  **When** I switch the selection to a field-less method
  **Then** the selection change succeeds and `getSuppliedCredential()` is an empty dictionary afterward - the
  same resetting behavior as §4.2, here covering the case where the new selection accepts no credential at
  all.

### 8.4 — Connecting with no config behaves like explicitly selecting the field-less default

- **Given** a type whose default (first-added) method is field-less (e.g. `credential_demo_module`'s
  `"Anonymous"`)
  **When** I connect once via `addDevice(connectionString)` (no config attached) and once via
  `addDevice(connectionString, anonymousConfig)` (the same method explicitly selected)
  **Then** both succeed identically from the caller's perspective - no prompt, no provider interaction, the
  same resulting device. Both actually go through `createAuthenticatedDevice` internally, even the config-less
  call (see the API reference, §3) - just with an empty credential and nothing to verify. The module's plain
  `createDevice` is never reached here at all; it only runs for a type with zero methods added.
