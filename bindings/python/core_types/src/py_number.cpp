#include "py_core_types/py_core_types.h"

PyDaqIntf<daq::INumber> declareINumber(pybind11::module_ m)
{
    return wrapInterface<daq::INumber>(m, "INumber");
}

void defineINumber(pybind11::module_ m, PyDaqIntf<daq::INumber> cls)
{
    cls.doc() = "Represents either a float or an int number.";

    cls.def("__float__",
        [](daq::INumber* object)
        {
            return daq::NumberPtr::Borrow(object).getFloatValue();
        });
    cls.def("__int__",
        [](daq::INumber* object)
        {
            return daq::NumberPtr::Borrow(object).getIntValue();
        });

    cls.def_property_readonly("float_value",
        [](daq::INumber* object) -> daq::Float
        {
            return daq::NumberPtr::Borrow(object).getFloatValue();
        },
        "Gets a value stored in the object as a floating point value.");
    cls.def_property_readonly("int_value",
       [](daq::INumber* object) -> daq::Int
        {
            return daq::NumberPtr::Borrow(object).getIntValue();
        },
        "Gets a value stored in the object as an integer value.");
}
