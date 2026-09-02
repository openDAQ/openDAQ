# py_opendaq_module — writing openDAQ modules and function blocks in Python

`py_opendaq_module` is the C++ infrastructure that lets an openDAQ **module** (and the function
blocks it offers) be implemented as a plain Python file instead of a compiled `.so`/`.dylib`/
`.dll`. A plugin file never links against openDAQ's C++ headers or subclasses `IModule`/
`IFunctionBlock` directly — it just defines a module-level `create_module(context)` function that
returns a duck-typed object, and the C++ side (`PythonModule`, `PythonFunctionBlockImpl`) drives
it for its whole lifetime.

This is not the same thing as `bindings/python/opendaq` (the `opendaq` pip package — the
**client** SDK used to control openDAQ from Python scripts). `py_opendaq_module` is the other
direction: it's how a device/host written in C++ (or in Python, via the pip package itself) loads
Python *plugins*.

## Two ways a plugin gets loaded

**1. From a pure-Python host, using the `opendaq` pip package.** No C++ code required — the
`opendaq` module itself provides `IModuleManager.load_python_module`, which wraps
`daq::createPythonModule` under the hood (see `py_module_manager.cpp` below):

```python
import opendaq

instance.module_manager.load_python_module(instance.context, "plotter_module.py")
```

See `bindings/python/opendaq/src/py_module_manager.cpp` for both overloads.

**2. From a C++ host that embeds Python**, via `daq::PythonModule`
(`include/py_opendaq_module/python_module.h`):

```cpp
#include <py_opendaq_module/python_module.h>

auto module = daq::createPythonModule(context, "plotter_module.py");
instance.getModuleManager().addModule(module);
```

There is no folder scanning or manifest — each call loads exactly one plugin file. This is the
path used by a native application that wants to offer Python-pluggability without requiring a
Python-based host process.

Either way, every call into the Python side is marshaled onto a single dedicated dispatch thread
owned by `PythonRuntime` (`include/py_opendaq_module/python_runtime.h`) — see
[Threading and the GIL](#threading-and-the-gil) below.

## The plugin contract

A plugin file is a normal `.py` file with one required entry point:

```python
def create_module(context) -> Module: ...
```

`Module` and `FunctionBlock` are plain-Python base classes shipped both in the `opendaq` pip
package and embedded as string constants into the C++-hosted interpreter (see
`include/py_opendaq_module/plugin_base_py.h.in`, generated from the same
`bindings/python/package/opendaq/{module,function_block}.py` files) — so the contract is
identical regardless of which of the two loading paths above is used. Subclassing them is
optional (every hook is read via `getattr`/`hasattr`), but they document the contract and give
safe no-op defaults.

```python
import opendaq

class MyFunctionBlock(opendaq.FunctionBlock):
    @staticmethod
    def create_function_block_type():
        return opendaq.FunctionBlockType("MyFb", "My FB", "Does a thing", None)

    def on_init(self):
        # add_signal()/add_input_port() require the new object's parent to be exactly
        # self.signals_folder / self.input_ports_folder.
        self.add_signal(opendaq.Signal(self.context, self.signals_folder, "output", None))
        self.add_input_port(opendaq.InputPort(self.context, self.input_ports_folder, "input", False))

    def on_connected(self, port):
        pass  # a signal was connected to `port`

    def on_packet_received(self, port):
        pass  # a new packet is available to read on `port`

class MyModule(opendaq.Module):
    def __init__(self, context):
        super().__init__(context, name="MyModule", version=(1, 0, 0), id="MyModuleId")

    def on_get_available_function_block_types(self):
        return {"my_fb": MyFunctionBlock.create_function_block_type()}

    def on_create_function_block(self, id, parent, local_id, config):
        if id == "my_fb":
            return MyFunctionBlock(self.context, parent, local_id)
        return None

def create_module(context):
    return MyModule(context)
```

Full hook list and docs: `bindings/python/package/opendaq/module.py` and
`bindings/python/package/opendaq/function_block.py`.

Key points:
- `version` is required — a module with `version=None` fails to load.
- `on_init()` runs synchronously right after construction (`self._cpp_fb` is already set by
  then); `__init__` itself runs too early to add signals/ports.
- `on_connected`/`on_disconnected`/`on_packet_received` run **asynchronously**, on
  `PythonRuntime`'s dispatch thread, after the C++ call that triggered them has already
  returned. Exceptions raised inside them are only logged, never propagated.
- `on_accepts_signal()` runs synchronously on whichever thread attempts the connection — keep it
  fast and side-effect free.

## Threading and the GIL

`PythonRuntime` (`include/py_opendaq_module/python_runtime.h`) owns one dispatch thread that is
the only thread allowed to touch the interpreter; every openDAQ→Python call is marshaled through
it via `IScheduler::scheduleWorkOnMainLoop()`, rather than acquiring the GIL ad hoc on whatever
native thread (scheduler worker, streaming thread) happens to call in.

Two runtime modes, picked automatically:
- **Embedded** — a C++ host with no Python of its own. The dispatch thread itself calls
  `Py_Initialize()` and imports the in-process `opendaq` module that
  `src/python_opendaq_embed.cpp` registers.
- **Attached** — this code is linked into a Python extension module (e.g. `opendaq.so` itself,
  `py_opendaq_daq`). The interpreter is already running; the dispatch thread only
  acquires/releases the GIL for its own dispatched work.

Practical consequence for plugin authors: anything requiring the *real* process main thread (GUI
toolkits, some signal handling) cannot run inside a normal hook — those hooks all run on
`PythonRuntime`'s dispatch thread. The plotter/flappy-duck examples below work around this by
explicitly scheduling their per-frame draw calls onto `context.scheduler`'s main loop instead
(see `PlotterFb.show()` in `plotter_module.py`).

## Building

The whole embedded-interpreter half of this directory (the library, its examples, its tests) is
gated behind the CMake option `OPENDAQ_GENERATE_PYTHON_MODULE_BINDINGS`, itself dependent on
`OPENDAQ_GENERATE_PYTHON_BINDINGS`:

```sh
cmake -B build \
  -DOPENDAQ_GENERATE_PYTHON_BINDINGS=ON \
  -DOPENDAQ_GENERATE_PYTHON_MODULE_BINDINGS=ON \
  -DOPENDAQ_ENABLE_TESTS=ON

cmake --build build
```

Turning on `OPENDAQ_GENERATE_PYTHON_MODULE_BINDINGS` additionally requires CMake's
`Development.Embed` Python component (the real embeddable libpython, not just the
`Development.Module` headers `OPENDAQ_GENERATE_PYTHON_BINDINGS` alone needs) — see
`CMakeLists.txt:255-273` at the repo root. Make sure a matching Python dev install
(`python3-dev`/`python3-devel`, or the Windows installer's "Download debug binaries"/embeddable
package) is available, or `find_package(Python ... COMPONENTS Development.Embed)` fails.

Two targets come out of `bindings/python/py_opendaq_module/CMakeLists.txt`:
- `daq::py_opendaq_module_core` — `PythonRuntime`/`PythonModule`/`PythonFunctionBlockImpl` on
  their own, always built (no embedding). Linked directly by `py_opendaq_daq`
  (the pip package's native extension), which is itself already a Python host and doesn't need a
  from-scratch embedded interpreter.
- `daq::py_opendaq_module` — the above, plus the embedded-interpreter half
  (`Py_Initialize()`-owning support and the in-process `opendaq` module). This is what a C++ host
  with no Python of its own links against. Only built when
  `OPENDAQ_GENERATE_PYTHON_MODULE_BINDINGS=ON`.

### Running the tests

```sh
ctest --test-dir build -R py_opendaq_module
```

`test_py_opendaq_module` covers `PythonModule`/`PythonFunctionBlockImpl` against the mock plugins
in `tests/mock/`. `test_py_opendaq_module_attach` is a separate binary — it has to call
`Py_Initialize()` itself before anything touches `PythonRuntime::instance()`, to exercise the
"attached to an already-running interpreter" branch, so it can't share a process with the other
tests.

### Building the pip package

`bindings/python/package/build_pip.py` assembles the `opendaq` wheel (client SDK, `Module`/
`FunctionBlock` base classes, and — for the GUI demo entry point — a copy of
`examples/applications/python/GUI Application/gui_demo.py`). It's driven from a build's `bin`
output directory; see the script for its `--build-dir`/`--lib-dir`/`--stage-dir` flags.

## Examples

- **[`examples/modules/python_plotter_module/`](../../../examples/modules/python_plotter_module/)**
  — `plotter_fb`: one input port, plots the trailing history of whatever signal is connected to
  it via `DurationTailReader`. Runnable directly (`python plotter_module.py`, builds its own
  `opendaq.Instance()`) or embedded from a C++ host — see that directory's own README for the
  live-window/`Agg`-backend split and both loading paths in full.
- **[`examples/modules/python_flappy_bird_module/flappy_duck_module.py`](../../../examples/modules/python_flappy_bird_module/flappy_duck_module.py)**
  — two playable games (`FlappyDuckFb`, `StarWarsDuckFb`) built the same way, purely to exercise
  keyboard-driven signal flow and the same live-window/main-thread constraints under something
  more demanding than a static plot. Run with `python flappy_duck_module.py [flappy|starwars]`.
- **[`tests/mock/`](tests/mock/)** — minimal plugins (`mock_module.py`, `mock_module_fb.py`, and
  two deliberately-broken ones) used by the C++ test suite; the smallest complete examples of the
  contract if you just want to see the bare minimum that loads.

## Regenerating the underlying bindings

The `opendaq`/`core_types`/`core_objects` Python bindings that all of the above sits on top of
are semi-generated. See [`../README.md`](../README.md) (`bindings/python/README.md`) for the
`run_rtgen.sh` workflow when adding or removing a wrapped interface.
