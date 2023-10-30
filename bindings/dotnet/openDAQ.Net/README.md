# openDAQ.Net

This project contains the .NET-Bindings to the openDAQ SDK.  

## Table of Contents
- [Prerequisites](#prerequisites)  
- [C# to C++ access considerations](#c-to-c-access-considerations)  
   - [Reference counting](#reference-counting)  
   - [Memory consumption](#memory-consumption)  
- [Bindings](#bindings)  
   - [Auto generated code](#auto-generated-code)  
   - [openDAQ core-type conversion](#opendaq-core-type-conversion)  
   - [Handling new interfaces](#handling-new-interfaces)  
   - [Inline documentation](#inline-documentation)  
   - [Special implementations](#special-implementations)  


## Development environment
- Visual Studio 2022 (V17.7)  
- TargetFramework: net6.0  
- Platform: x64, x86  
- Language: C#  
- Test-framework: NUnit  

## Prerequisites
Place the openDAQ binaries (on Windows `*.dll` and `modules/*.module.dll`) into the directories
`./openDAQ.Net/runtimes-*/`.  
`Debug` libraries will be copied to the output directory directly (e.g. for use in the test
project).  
`Release` libraries will be copied keeping the structure `runtime/win-x64/native/`
(e.g. for use in NuGet package).  

```
./openDAQ.Net
  /runtime-debug/win-x64/native
    /daqcoreobjects-64-2-debug.dll
    /daqcoreobjects-64-2-debug.pdb
    /daqcoretypes-64-2-debug.dll
    /daqcoretypes-64-2-debug.pdb
    /opendaq-64-2-debug.dll
    /opendaq-64-2-debug.pdb
    /modules
      /audio_device_module-64-2-debug.module.dll
      /empty_module-64-2-debug.module.dll
      /opcua_client_module-64-2-debug.module.dll
      /opcua_server_module-64-2-debug.module.dll
      /ref_device_module-64-2-debug.module.dll
      /ref_fb_module-64-2-debug.module.dll
      /websocket_client_module-64-2-debug.module.dll
      /websocket_server_module-64-2-debug.module.dll
  /runtime-release/win-x64/native
    /daqcoreobjects-64-2.dll
    /daqcoreobjects-64-2.pdb
    /daqcoretypes-64-2.dll
    /daqcoretypes-64-2.pdb
    /opendaq-64-2.dll
    /opendaq-64-2.pdb
    /modules
      /audio_device_module-64-2.module.dll
      /empty_module-64-2.module.dll
      /opcua_client_module-64-2.module.dll
      /opcua_server_module-64-2.module.dll
      /ref_device_module-64-2.module.dll
      /ref_fb_module-64-2.module.dll
      /websocket_client_module-64-2.module.dll
      /websocket_server_module-64-2.module.dll
```

## C# to C++ access considerations

### Reference counting
The openDAQ SDK is written in C++ and all objects implement the `IUnknown` interface
(interface querying and reference counting). In C++ (and Python) _smart pointers_ are used
which automatically handle _object destruction_ and thus _reference counters_.  

When used in C#, only object pointers (`IntPtr`) will be exchanged. Dereferencing a C++
object has to be handled explicitly (the `BaseObject` implements the _Disposable_ pattern,
which dereferences the underlying C++ object).  
Though usually a .NET developer rarely would use `obj.Dispose()` directly, it will be
automatically called on (.NET) object finalization / destruction by the
_.NET Garbage Collector_.  

### Memory consumption
Internally we agreed that it will not be necessary to always dispose of those C# objects,
because eventually the _.NET Garbage Collector_ will collect (and finalize/destroy)
unreferenced (.NET) objects, which in turn dereferences the C++ object in the
finalizer of `BaseObject` by calling `this.Dispose()`.  
Regarding memory consumption, this is similar to collecting pure .NET objects that are no longer
referenced. The user has still the choice to manually free up no longer needed objects by
calling the objects `Dispose()` method (or by using the `using` statement to automatically dispose
the object when leaving the scope).  

## Bindings

### Auto generated code
Generally, code is automatically generated using _RTGen_ and output to the
`build/bindings/CSharp/core` directory (configured in `cmake/RTGen.cmake`).  
Most of the core types can be generated using _RTGen_ (at least in C#), but they are still held
manually in the repository within the `openDAQ.NET` project since they have no _RTGen_ tasks
connected.  
There are some [Special implementations](#special-implementations) though.  

The class names come from the C++ interface names, where some names are adjusted due to possible
duplicity of .NET class names
(e.g. `IBoolean`<sub>C++</sub> -> ~~`Boolean`~~<sub>C#</sub> -> `BoolObject`<sub>C#</sub>
or `IList`<sub>C++</sub> -> ~~`List`~~<sub>C#</sub> -> `ListObject`<sub>C#</sub>).  

The project file expects binding code in the `openDAQ.NET` project's local `core` directory as
well as in the `build/bindings/CSharp/core` directory.  

### openDAQ core-type conversion
Some openDAQ objects (immutable) are wrappers around types like `long` or `string`
which would be uncomfortable to use in managed C# notation.  
For this the .NET Bindings provide implicit cast operators to translate between
C# and openDAQ objects wherever possible.  

### Handling new interfaces
When a new interface file has been added (or removed), the bindings will be created automatically
using _RTGen_.  
Normally no adjustments to _RTGen_ or manual re-work of the output would be necessary, but in
special cases it cannot be ruled out (see also
[Future manual code changes](#future-manual-code-changes)).

### Inline documentation
Documentation is also extracted by _RTGen_, though some "translations" are quite hard to apply
(e.g. mentioning of any C++ interface is currently not recognized and kept as is).  
Syntax filters might also be necessary to be adapted for future comments.

### Special implementations
Some classes, interfaces or enumerations have been written manually as _RTGen_ would not be able
to parse the source or there are just too many .NET specific adjustments inside.  

#### _RTGen_
In the _CSharp_ part of _RTGen_ many special cases are handled, be it in code or with templates.  
For C# specific template-variables the prefix `CS` is used for better differentiation
(e.g. `$CSLicenseComment$` or `$CSInterfaceDoc$`).

#### Core types
- `BaseObject`  
  The basis for all objects, implementing the _Disposable_ pattern, providing some implicit cast
  operators (e.g. to .NET type `string`), overloading (.NET-) functions like `Equals()` and
  `ToString()`.  
- `FuncCall` (delegate, function pointer `typedef` in `function.h`, not parsable by _RTGen_)  
- `ProcCall` (delegate, function pointer `typedef` in `procedure.h`, not parsable by _RTGen_)  
- `enum ErrorCode` (just `#define`s in C++, not parsable by _RTGen_)  
- `Version` (not an interface parsable by _RTGen_)  

All other core types can be generated using _RTGen_ manually and have to be placed in the
`openDAQ.Net` project's local `core` directory:

```
<RTGen_exe> -ns daq.core.coretypes -ln CoreTypes -d "<outDir>/coretypes" -f "" --lang csharp --source="<srcDir>/coretypes/include/coretypes/coretype.h"
```  

Where 
- `<RTGen_exe>` is the path to `RTGen.exe` including the file name.  
- `<outDir>` is the base-path where the generated output will be stored
  (e.g. `build/bindings-run/csharp/core`).  
- `<srcDir>` is the path to the C++ source directory (usually named `core`).  

#### Core containers
Those are placed into the `core/coretypes` directory.
- `IListObject<TValue>`  
  Special .NET interface to `ListObject<TValue>`, combining .NET interfaces `IList<TValue>` and
  `IDisposable`.  
  All functions returning a list will return this interface, which hides the bound functions
  behind the .NET interface implementation (type casting always possible).  
- `IDictObject<TKey, TValue>`  
  Special .NET interface to `DictObject<TKey, TValue>`, combining .NET interfaces
  `IDictionary<TKey, TValue>` and `IDisposable`.  
  All functions returning a dictionary will return this interface, which hides the bound functions
  behind the .NET interface implementation (type casting always possible).  

#### Core objects
Nothing special here.  

#### openDAQ types
- `SampleReader` derivatives  
  Those are generated automatically, replacing `void*` with managed-type arrays (specified as
  generic type parameter) to be filled in the `Read()` functions, which exist in two variants:
  With and without a start index (e.g. for circular buffers).  
  Though, for the `OpenDAQFactory` there exist a couple of manually created functions with
  different defaults in the generic parameters, simplifying reader creation in some cases.  
- `struct SourceLocation` (not parsable by _RTGen_)  
- `enum SampleType` (not parsable by _RTGen_)  
- `enum ScaledSampleType` (not parsable by _RTGen_)  

#### Future manual code changes
In the future there might be changes necessary to be made to auto-generated code which have then
to be placed in the `openDAQ.NET` project's `core` directory structure.  
Any manually changed file has then to be excluded from then auto-generated input. This has to be
done manually in the project file so that the class will not be compiled twice (which would
produce an error).  

Therefore find the following line in the project file `openDAQ.Net.csproj`:  

```xml
<!-- include the RTGen build output; exclude files which are managed manually -->
```  

and adapt the adjacent line to have an active `Exclude=` attribute containing the file(s) to
exclude.  

```xml
<Compile Include="$(_RTGenOutputPath)\**\*.cs" /> <!-- Exclude="$(_RTGenOutputPath)\**\Logger.cs;$(_RTGenOutputPath)\**\StreamReader.cs"-->
```  

Here one can list multiple files with full path to the file(s) to be excluded (variables and
patterns allowed as displayed).  

- Separate files by semicolon (`;`).  
- The variable `$(_RTGenOutputPath)` points to the base directory holding the
  auto-generated code.  
- The pattern `x\**\y` means that all subdirectories behind `x` will be searched for the file `y`
  and thus excluded.  

_<sub>Last changed on 2023-12-05 &copy; 2023 by Blueberry d.o.o.</sub>_