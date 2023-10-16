#include "py_core_types/py_core_types.h"

PyDaqIntf<daq::IFloat> declareIFloat(pybind11::module_ m)
{
    return wrapInterface<daq::IFloat>(m, "IFloat");
}

void defineIFloat(pybind11::module_ m, PyDaqIntf<daq::IFloat> cls)
{
    cls.doc() = "Represents float number as `IFloat` interface. Use this interface to wrap float variable when you need to add the number "
                "to lists, dictionaries and other containers which accept `IBaseObject` and derived interfaces. Float type is defined as "
                "double-precision IEEE 754 value.";

    m.def("Float", &daq::Float_Create);

    cls.def_property_readonly("value",
        [](daq::IFloat* object) -> daq::Float
        {
            return daq::FloatPtr::Borrow(object);
        },
        "Gets a float value stored in the object.");
}
