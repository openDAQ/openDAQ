#include <pybind11/pybind11.h>

#include "py_core_objects/py_core_objects.h"
#include "py_core_types/py_core_types.h"
#include "py_opendaq/py_opendaq.h"
#include "py_opendaq/py_packet_buffer.h"
#include "py_core_types/py_event_queue.h"


PYBIND11_MODULE(opendaq, m)
{
    m.doc() = "openDAQ python bindings";

    opendaq_daq_module = m;
    python_class_fraction = py::module_::import("fractions").attr("Fraction");

    m.def("get_tracked_object_count", &daqGetTrackedObjectCount);
    m.def("print_tracked_objects", &daqPrintTrackedObjects);
    m.def("clear_tracked_objects", &daqClearTrackedObjects);
    m.def("clear_error_info", &daqClearErrorInfo);

    // wrap individual components
    
    PyBuffer::Buffer::wrap(m);

    wrapDaqComponentCoreTypes(m);
    wrapDaqComponentCoreObjects(m);
    wrapDaqComponentOpenDaq(m);

    auto eventQueueCls = declarePyEventQueue(m);
    definePyEventQueue(m, eventQueueCls);
    // Bind the global instance
    m.attr("event_queue") = PyEventQueue::Create();
}
