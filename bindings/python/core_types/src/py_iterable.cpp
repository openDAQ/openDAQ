#include "py_core_types/py_core_types.h"

PyDaqIntf<daq::IIterable> declareIIterable(pybind11::module_ m)
{
    return wrapInterface<daq::IIterable>(m, "IIterable");
}

void defineIIterable(pybind11::module_ m, PyDaqIntf<daq::IIterable> cls)
{
    cls.doc() = "An iterable object can construct iterators and use them to iterate through items.";

    cls.def("__iter__",
        [](daq::IIterable* object)
        {
            daq::IIterator* it;
            daq::checkErrorInfo(object->createStartIterator(&it));

            return it;
        },
        "Creates and returns the object's start iterator.");
}
