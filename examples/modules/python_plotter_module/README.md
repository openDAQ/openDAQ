# Python Plotter module (example)

`plotter_module.py` is an example openDAQ **Python** plugin: a `Module` that offers one function
block type, `plotter_fb`, which has a single input port and plots the trailing history of
whatever signal gets connected to it (via `DurationTailReader`) to a PNG file, overwritten on
every update.

It's written against the plugin contract in
[`bindings/python/package/opendaq/module.py`](../../../bindings/python/package/opendaq/module.py)
and
[`function_block.py`](../../../bindings/python/package/opendaq/function_block.py) - see those
files for the full hook contract (`on_init`, `on_connected`, `on_packet_received`, ...).

## Dependencies

`numpy` and `matplotlib` - not openDAQ dependencies, install separately:

```sh
pip install numpy matplotlib
```

They need to be importable by whichever Python interpreter ends up loading this file (see below).

## Loading it

This file only defines `create_module(context)` - like every openDAQ Python plugin, it does not
run standalone (`python plotter_module.py` does nothing useful). There is currently no way to
load a `.py` plugin into a real `opendaq.Instance()` from pure Python: `InstanceBuilder`'s module
path only scans for compiled `.so`/`.dylib`/`.dll` modules. A Python plugin has to be loaded by a
C++ host that embeds Python (via `daq::PythonModule`, `bindings/python/py_opendaq_module`) and
side-loaded into the module manager explicitly (see `IModuleManager::addModule`,
[`module_manager.h`](../../../core/opendaq/modulemanager/include/opendaq/module_manager.h)) -
[`bindings/python/py_opendaq_module/tests/test_python_module.cpp`](../../../bindings/python/py_opendaq_module/tests/test_python_module.cpp)
shows the `PythonModule` construction half of this (it doesn't itself call `addModule`, since
tests drive the module directly instead of wiring it into a full `Instance`):

```cpp
#include <py_opendaq_module/python_module.h>

auto module = daq::createWithImplementation<daq::IModule, daq::PythonModule>(
    context, "plotter_module.py");
context.getModuleManager().asPtr<daq::IModuleManager>().addModule(module);
```

## Why a PNG file, not a live window

`on_packet_received()` runs on `PythonRuntime`'s own dedicated dispatch thread (see
[`python_runtime.h`](../../../bindings/python/py_opendaq_module/include/py_opendaq_module/python_runtime.h)),
never the host process's main thread. Most interactive matplotlib backends require GUI calls to
happen on the main thread, so this plugin uses the `Agg` backend (a pure off-screen renderer, no
GUI event loop) and writes `<local_id>.png` instead of showing a window.
