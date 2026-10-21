# JSON metadata bindings

[`opendaq.json`](./opendaq.json) contains a machine-readable description of the
openDAQ C++ API surface - COM-style interfaces with vtable positions,
`extern "C"` factory symbols, enums and error codes. This is intended as an
easily parsable up-to-date input for generating external language bindings, an
alternative to RTGen.

## Schema notes

- The default `namespace` (`daq`) is declared once at top level; a type
  carries its own `namespace` only when it differs (external `tf`, `spdlog`,
  `std` types).
- Methods are called through the vtable at `absolute_vtable_slot`, which
  counts from the start of the whole vtable; `relative_vtable_slot` counts
  only the interface's own methods, so the two differ by the size of the
  inherited vtable (single inheritance, declaration order).
- Container types carry the element types the headers annotate them with:
  `IList` has a `value_type`, `IDict` a `key_type` and a `value_type` (absent
  when the header leaves the container untyped).
- `interfaces[].factories` are the constructors for objects that have the
  corresponding interface as their main one.
- `functions` are the freestanding `extern "C"` exports that are not tied to
  an interface (the `daqSetErrorInfo`/`daqGetErrorInfo` error-info helpers and
  the serializer-factory registry).

## C binding names

Languages that cannot call through vtables go through the C bindings
(`bindings/c/`), which wrap this same surface 1:1. Their names can be derived
mechanically from the JSON:

- type: `daq` + name without the leading interface `I` - `IStreamReader` →
  `daqStreamReader`; likewise enums (`daqSampleType`) and typedefs.
- method: `daq<Type>_<method>`, with `self` of type `daq<Type>*` prepended to
  the arguments - `daqStreamReader_read(self, samples, count, timeoutMs)`.
- factory: `daq<Type>_<factory name>` —
  `daqStreamReader_createStreamReaderFromPort`.
- per interface there is additionally `daq<Type>_getInterfaceId(daqIntfID*)`,
  which returns the interface's `id` from the JSON. It exists only in the C
  library: in C++ the ID is a static constant, not a method, so it is not
  listed among the JSON methods.

## Known gaps

- Factories declared as plain `PUBLIC_EXPORT` functions without a factory
  macro (`createJsonSerializer`, `createJsonSerializerWithVersion`,
  `createEventHandler`) are not discovered.
- The two interfaces with overloaded methods (`IDataRuleCalcPrivate`,
  `IScalingCalcPrivate`): MSVC lays out adjacent overloads in reverse
  declaration order, so those slots are compiler-dependent.

## Regenerating the JSON

The JSON can be regenerated manually using the `RTGen.Json` project.
Run the `run_rtgen.sh` script from this folder:

```sh
./run_rtgen.sh
```

If the [generator sources](shared/tools/RTGen/src/project/RTGen.Json/) were updated
you first need to rebuild the `RTGen.Json.dll` binary. With mono, from
`shared/tools/RTGen`:

```sh
mcs -target:library -out:bin/RTGen.Json.dll \
    -r:bin/RTGen.Library.dll -r:bin/RTGen.Cpp.dll -r:bin/LightInject.dll -r:System.Core.dll \
    src/project/RTGen.Json/Generators/*.cs \
    src/project/RTGen.Json/JsonCompositionRoot.cs \
    src/project/RTGen.Json/Properties/AssemblyInfo.cs
```

The project is also part of `src/project/RTGen.sln` for building with Visual
Studio.

Please keep the binaries committed to the repository, so that the JSON can be
regenerated without requiring a build environment. When updating the generator,
try and keep the JSON backwards-compatible, so that existing language bindings
do not break.
