#include "py_core_types/py_core_types.h"

PyDaqIntf<daq::IString> declareIString(pybind11::module_ m)
{
    return wrapInterface<daq::IString>(m, "IString");
}

void defineIString(pybind11::module_ m, PyDaqIntf<daq::IString> cls)
{
    cls.doc() = "Represents string variable as `IString` interface. Use this interface to wrap string variable when you need to add the "
                "variable to lists, dictionaries and other containers which accept `IBaseObject` and derived interfaces.";

    m.def("String", &daq::String_Create);

    // function getCharPtr makes no sense in Python, we skip it
    cls.def_property_readonly("length",
        [](daq::IString* object)
        {
            return daq::StringPtr::Borrow(object).getLength();
        },
        "Gets length of string.");
}
