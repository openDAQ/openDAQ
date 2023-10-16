#include "py_core_types/py_core_types.h"

PyDaqIntf<daq::IComplexNumber> declareIComplexNumber(pybind11::module_ m)
{
    return wrapInterface<daq::IComplexNumber>(m, "IComplexNumber");
}

void defineIComplexNumber(pybind11::module_ m, PyDaqIntf<daq::IComplexNumber> cls)
{
    cls.doc() = "Represents a complex number as `IComplexNumber` interface. Use this interface to wrap complex number when you need to add "
                "the number to lists, dictionaries and other containers which accept `IBaseObject` and derived interfaces. "
                "Complex numbers have two components: real and imaginary. Both of them are of Float type.";

    m.def("ComplexNumber",
        [](const double real, const double imaginary)
        {
            return daq::ComplexNumber_Create(real, imaginary);
        },
        py::arg("real"), py::arg("imaginary"),
        "Creates a new ComplexNumber object.");
    m.def("ComplexNumber",
        [](std::complex<double> value)
        {
            return daq::ComplexNumber_Create(value.real(), value.imag());
        },
        py::arg("value"),
        "Creates a new ComplexNumber object.");

    cls.def("__complex__",
        [](daq::IComplexNumber* object)
        {
            const auto objectPtr = daq::ComplexNumberPtr::Borrow(object);
            return std::complex<double>(objectPtr.getReal(), objectPtr.getImaginary());
        });

    cls.def_property_readonly("value",
        [](daq::IComplexNumber* object)
        {
            const auto objectPtr = daq::ComplexNumberPtr::Borrow(object);
            return std::complex<double>(objectPtr.getReal(), objectPtr.getImaginary());
        },
        "Gets a complex value stored in the object.");
    cls.def("equals_value",
        [](daq::IComplexNumber* object, const daq::ComplexFloat64 value)
        {
            return daq::ComplexNumberPtr::Borrow(object).equalsValue(value);
        },
        py::arg("value"),
        "Compares stored complex value to the complex number parameter.");
    cls.def_property_readonly("real",
        [](daq::IComplexNumber* object)
        {
            return daq::ComplexNumberPtr::Borrow(object).getReal();
        },
        "Gets the real part of the complex number value.");
    cls.def_property_readonly("imaginary",
        [](daq::IComplexNumber* object)
        {
            return daq::ComplexNumberPtr::Borrow(object).getImaginary();
        },
        "Gets the imaginary part of the complex number value.");
}
