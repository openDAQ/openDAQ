#include <pybind11/pybind11.h>

#include "py_core_objects/py_core_objects.h"
#include "py_core_types/py_core_types.h"
#include "py_opendaq/py_opendaq.h"

PYBIND11_MODULE(opendaq, m)
{
    m.doc() = "openDAQ python bindings";

    blueberry_daq_module = m;
    python_class_fraction = py::module_::import("fractions").attr("Fraction");

    py::class_<daq::IBaseObject, InterfaceWrapper<daq::IBaseObject>>(m, "IBaseObject")
        .def(py::init(
            [](daq::IBaseObject* obj)
            {
                obj->addRef();
                return obj;
            }))
        .def("__str__", [](daq::IBaseObject* obj) { return getString(obj); })
        .def_static("cast_from", [](daq::IBaseObject* obj) { return castFrom<daq::IBaseObject>(obj); })
        .def_static("can_cast_from", [](daq::IBaseObject* obj) { return canCastFrom<daq::IBaseObject>(obj); })
        .def("__hash__", [](daq::IBaseObject* obj) { return daq::BaseObjectPtr::Borrow(obj).getHashCode(); })
        .def("__int__", [](daq::IBaseObject* obj) { return static_cast<daq::Int>(daq::BaseObjectPtr::Borrow(obj)); })
        .def("__float__", [](daq::IBaseObject* obj) { return static_cast<daq::Float>(daq::BaseObjectPtr::Borrow(obj)); })
        .def_property_readonly("core_type", [](daq::IBaseObject* obj) { return daq::BaseObjectPtr::Borrow(obj).getCoreType(); })
        .def("__eq__",
             [](daq::IBaseObject* obj, const py::object& other)
             {
                 const auto objPtr = daq::BaseObjectPtr::Borrow(obj);

                 try
                 {
                     if (auto intf = pyQI<daq::IBaseObject>(other))
                     {
                         const auto otherPtr = daq::ObjectPtr(std::move(intf));
                         return objPtr == otherPtr;  // this will trigger IBaseObject.equals
                     }

                     if (py::isinstance<py::int_>(other))
                         return static_cast<daq::Int>(objPtr) == static_cast<daq::Int>(py::int_(other));

                     if (py::isinstance<py::float_>(other))
                         return static_cast<daq::Float>(objPtr) == static_cast<daq::Float>(py::float_(other));

                     if (py::isinstance<py::str>(other))
                         return static_cast<std::string>(objPtr) == static_cast<std::string>(py::str(other));

                     return false;
                 }
                 catch (std::exception&)
                 {
                     return false;
                 }
             });

    m.def("BaseObject", &daq::BaseObject_Create);

    m.def("get_tracked_object_count", &daqGetTrackedObjectCount);
    m.def("print_tracked_objects", &daqPrintTrackedObjects);
    m.def("clear_error_info", &daqClearErrorInfo);

    // wrap individual components
    wrapDaqComponentCoreTypes(m);
    wrapDaqComponentCoreObjects(m);
    wrapDaqComponentOpenDaq(m);
}
