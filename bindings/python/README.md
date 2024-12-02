Python bindings get generated nearly automatically, but the CMake code to build the library out of the freshly generated C++ files hasn't been adapted yet.

The bindings can be updated "semi-manually" by running `run_rtgen.sh` from this directory and commiting the changes to the repository.

When a new interface file has been added (or removed), some additional changes are needed:
* Add (or remove) the corresponding line to `run_rtgen.sh` and run the script.
* Make sure to `git add` (or `git rm`) the affected files.
* Add (or remove) the generated file to (from) the list of source files in one of the three `CMakeLists.txt` (for example in `bindings/python/core_objects/CMakeLists.txt`).
* Open the newly generated files and copy the declaration (for example `PyDaqIntf<daq::IDevice, daq::IComponent> wrapIDevice(pybind11::module_ m)`) to the relevant header file (for example `bindings/python/opendaq/include/py_opendaq/py_opendaq.h`). Or remove that line from the header in case the file has been deleted.
* Add (or remove) the call (for example `wrapIDevice(m)`) to the source file (`bindings/python/opendaq/py_opendaq.cpp` or `bindings/python/core_objects/py_core_objects.cpp`). Please pay attention that the call to `wrapISomething(m)` must come after the parent class has already been wrapped. In the example of `wrapIDevice` above, `IComponent` is a parent interface of `IDevice`, so `wrapIComponent(m)` needs to be called before `wrapIDevice(m)`.

The bindings for `IStreamReader` and `ITailReader` don't work correctly yet, among others because there were still plans to do some code refactoring. Readers may return tuples of values and timestamps, using native numpy arrays and don't need to return the size of the array, so the functions are somewhat special compared to all others, and bindings for those still need to be adjusted manually.
