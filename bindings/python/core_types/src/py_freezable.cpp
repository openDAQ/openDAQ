#include "py_core_types/py_core_types.h"

PyDaqIntf<daq::IFreezable, daq::IBaseObject> declareIFreezable(pybind11::module_ m)
{
    return wrapInterface<daq::IFreezable, daq::IBaseObject>(m, "IFreezable");
}

void defineIFreezable(pybind11::module_ m, PyDaqIntf<daq::IFreezable, daq::IBaseObject> cls)
{
    cls.doc() = "Transforms a mutable object to an immutable object. "
                "Once frozen, the object should not allow changes to its properties or internal state.";

    cls.def("freeze",
        [](daq::IFreezable* object)
        {
            const auto errCode = object->freeze();
            if (errCode != OPENDAQ_IGNORED)
                daq::checkErrorInfo(errCode);
        },
        "Makes the object frozen/immutable. Returns without error if already frozen.");

    cls.def_property_readonly("is_frozen",
        [](daq::IFreezable* object) -> bool
        {
            daq::Bool frozen;
            daq::checkErrorInfo(object->isFrozen(&frozen));
            return frozen;
        },
        "Checks if the object is frozen/immutable.");
}