#include <py_opendaq/py_opendaq.h>
#include <py_core_objects/py_variant_extractor.h>
#include <py_opendaq_module/python_module.h>

void defineIModuleManagerCustomMethods(pybind11::module_ m, PyDaqIntf<daq::IModuleManager, daq::IBaseObject> cls)
{
    cls.def(
        "load_python_module",
        [](daq::IModuleManager* object, daq::IContext* context, std::variant<daq::IString*, py::str, daq::IEvalValue*>& path) -> daq::IModule*
        {
            // Must release the GIL before calling in: createPythonModule() (like every
            // PythonRuntime-backed call - see its class comment) blocks the calling thread
            // until PythonRuntime's dispatch thread acquires the GIL to do its own work, which
            // it can never do while this thread both holds it and is blocked waiting.
            py::gil_scoped_release release;
            const std::string pathStr = daq::StringPtr(getVariantValue<daq::IString*>(path)).toStdString();
            daq::ModulePtr module = daq::createPythonModule(daq::ContextPtr::Borrow(context), pathStr);
            daq::ModuleManagerPtr::Borrow(object).addModule(module);
            return module.detach();
        },
        py::arg("context"),
        py::arg("path"),
        "Loads a Python (.py) plugin file and adds it to the module manager - the Python-file "
        "equivalent of load_module() for compiled shared-library modules.");
    cls.def(
        "load_python_module",
        [](daq::IModuleManager* object, py::object moduleInstance) -> daq::IModule*
        {
            // Read this before releasing the GIL below - accessing a Python attribute needs
            // it. Every Module subclass carries its own `.context` (set by Module.__init__ -
            // see bindings/python/package/opendaq/module.py), the same one it was built with
            // (e.g. via create_module(context)), so this overload doesn't need a separate
            // context argument the way the path-based one above does.
            py::object contextAttr = moduleInstance.attr("context");
            const daq::ContextPtr context = pyObjectToBaseObject(contextAttr, false).asPtr<daq::IContext>();

            // Must release the GIL before calling in - see the other load_python_module
            // overload for why.
            py::gil_scoped_release release;
            daq::ModulePtr module = daq::createPythonModule(context, std::move(moduleInstance));
            daq::ModuleManagerPtr::Borrow(object).addModule(module);
            return module.detach();
        },
        py::arg("module_instance"),
        "Wraps an already-constructed Python module object (e.g. the return value of a "
        "plugin's create_module(context)) and adds it to the module manager, reading its "
        "context from its `.context` attribute.");

}