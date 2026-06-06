# C Bindings

The C bindings are auto-generated from C++ interface headers in `core/` using the RTGen tool.

## Regenerating

Run the regeneration script from this directory:

```bash
./run_rtgen.sh
```

The script passes each C++ header to `rtgen.exe` (via Mono on Linux), producing a `.h` and `.cpp` file under `include/` and `src/`. To add a new interface, add a `run_rtgen` call to the script following the existing pattern.

If you've modified the RTGen C generator (`shared/tools/RTGen/src/project/RTGen.C/Generators/CGenerator.cs`) or its templates, rebuild `RTGen.C.dll` first. On Linux with Mono:

```bash
cd shared/tools/RTGen
mcs -target:library -out:bin/RTGen.C.dll \
    -r:bin/LightInject.dll -r:bin/RTGen.Library.dll -r:System.dll \
    src/project/RTGen.C/CCompositionRoot.cs \
    src/project/RTGen.C/Generators/CGenerator.cs \
    src/project/RTGen.C/Properties/AssemblyInfo.cs
```

On Windows, build `shared/tools/RTGen/src/project/RTGen.sln` in Visual Studio instead.

Note that template files exist both under `src/project/RTGen.C/Templates/` (source) and `bin/Templates/` (runtime copies) - both must be kept in sync.

## CMake

Set `OPENDAQ_GENERATE_C_BINDINGS=ON` to include the bindings in the build. The CMake build only **compiles** the already-generated files - it does not run RTGen.
