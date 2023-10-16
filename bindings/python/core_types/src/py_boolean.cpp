#include "py_core_types/py_core_types.h"

PyDaqIntf<daq::IBoolean> declareIBoolean(pybind11::module_ m)
{
    return wrapInterface<daq::IBoolean>(m, "IBoolean");
}

void defineIBoolean(pybind11::module_ m, PyDaqIntf<daq::IBoolean> cls)
{
    cls.doc() = "Represents boolean variable as `IBoolean` interface. Use this interface to wrap boolean variable when you need to add the "
                "variable to lists, dictionaries and other containers which accept `IBaseObject` interface.";

    m.def("Boolean",
        [](const bool value)
        {
            return daq::Boolean_Create(value);
        },
        py::arg("value"),
        "Creates a new Boolean.");

    cls.def("__bool__",
        [](daq::IBoolean* object) -> bool
        {
            return daq::BooleanPtr::Borrow(object);
        });

    cls.def_property_readonly("value",
        [](daq::IBoolean* object) -> bool
        {
            return daq::BooleanPtr::Borrow(object);
        },
        "Gets a boolean value stored in the object.");
}
