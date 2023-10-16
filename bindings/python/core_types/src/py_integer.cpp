#include "py_core_types/py_core_types.h"

PyDaqIntf<daq::IInteger> declareIInteger(pybind11::module_ m)
{
    return wrapInterface<daq::IInteger>(m, "IInteger");
}

void defineIInteger(pybind11::module_ m, PyDaqIntf<daq::IInteger> cls)
{
    cls.doc() = "Represents int number as `IInteger` interface. Use this interface to wrap integer variable when you need to add the "
                "number to lists, dictionaries and other containers which accept `IBaseObject` and derived interfaces.";

    m.def("Integer", &daq::Integer_Create);

    cls.def_property_readonly("value",
        [](daq::IInteger* object) -> daq::Int
        {
            return daq::IntegerPtr::Borrow(object);
        },
        "Gets an int value stored in the object.");
}
