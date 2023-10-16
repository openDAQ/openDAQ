#include "py_core_types/py_core_types.h"

PyDaqIntf<daq::IRatio> declareIRatio(pybind11::module_ m)
{
    return wrapInterface<daq::IRatio>(m, "IRatio");
}

void defineIRatio(pybind11::module_ m, PyDaqIntf<daq::IRatio> cls)
{
    cls.doc() = "Represents rational number as `IRatio` interface. Use this interface to wrap rational number when you need to add the "
                "number to lists, dictionaries and other containers which accept `IBaseObject` and derived interfaces. Rational numbers "
                "are defined as numerator / denominator.";

    m.def("Ratio",
        [](const int numerator, const int denominator)
        {
            return daq::Ratio_Create(numerator, denominator);
        },
        py::arg("numerator"), py::arg("denominator"),
        "Creates a new Ratio object.");

    cls.def_property_readonly("numerator",
        [](daq::IRatio* object)
        {
            return daq::RatioPtr::Borrow(object).getNumerator();
        },
        "Gets numerator part.");
    cls.def_property_readonly("denominator",
        [](daq::IRatio* object)
        {
            return daq::RatioPtr::Borrow(object).getDenominator();
        },
        "Gets denominator part.");
    cls.def("simplify",
        [](daq::IRatio* object)
        {
            auto simplifiedRatio = daq::RatioPtr::Borrow(object).simplify();
            py::object p = python_class_fraction(simplifiedRatio.getNumerator(), simplifiedRatio.getDenominator());
            return p;
        },
        "Simplifies rational number if possible and returns the simplified ratio as a new object.");
}
