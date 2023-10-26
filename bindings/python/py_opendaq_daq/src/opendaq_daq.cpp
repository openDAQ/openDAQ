#include <pybind11/pybind11.h>

#include "py_core_objects/py_core_objects.h"
#include "py_core_types/py_core_types.h"
#include "py_opendaq/py_opendaq.h"

PYBIND11_MODULE(opendaq, m)
{
    m.doc() = "openDAQ python bindings";

    blueberry_daq_module = m;
    python_class_fraction = py::module_::import("fractions").attr("Fraction");

    m.def("get_tracked_object_count", &daqGetTrackedObjectCount);
    m.def("print_tracked_objects", &daqPrintTrackedObjects);
    m.def("clear_error_info", &daqClearErrorInfo);

    // wrap individual components
    wrapDaqComponentCoreTypes(m);
    wrapDaqComponentCoreObjects(m);
    wrapDaqComponentOpenDaq(m);
}
