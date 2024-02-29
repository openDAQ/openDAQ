#include "py_core_types/py_converter.h"
#include "py_core_types/py_base_object.h"
#include "py_core_types/py_opendaq_daq.h"

#include <opendaq/opendaq.h>

daq::ObjectPtr<daq::IBaseObject> PyConverter::ToBaseObject(const py::object& pyObject)
{
    return pyObjectToBaseObject(pyObject);
}

py::object PyConverter::ToPyObject(const daq::ObjectPtr<daq::IBaseObject>& baseObject)
{
    return py::reinterpret_steal<py::object>(baseObjectToPyObject(baseObject));
}

daq::ObjectPtr<daq::IBaseObject> pyObjectToBaseObject(const py::object& handle)
{
    if (auto intf = pyQI<daq::IBaseObject>(handle))
    {
        return intf;
    }
    else if (py::isinstance<py::int_>(handle))
    {
        const daq::Int value = handle.cast<py::int_>();
        return daq::Integer(value);
    }
    else if (py::isinstance<py::float_>(handle))
    {
        const daq::Float value = handle.cast<py::float_>();
        return daq::Floating(value);
    }
    else if (py::isinstance<py::bool_>(handle))
    {
        const daq::Bool value = handle.cast<py::bool_>();
        return daq::Boolean(value);
    }
    else if (py::isinstance<py::str>(handle))
    {
        const std::string value = handle.cast<py::str>();
        return daq::String(value);
    }
    else if (py::isinstance(handle, python_class_fraction))
    {
        const daq::Int numerator = handle.attr("numerator").cast<py::int_>();
        const daq::Int denominator = handle.attr("denominator").cast<py::int_>();
        return daq::Ratio(numerator, denominator);
    }

    auto obj = py::reinterpret_borrow<py::object>(handle);
    return wrapPyObject(std::move(obj));
}

py::object baseObjectToPyObject(const daq::ObjectPtr<daq::IBaseObject>& baseObject, const daq::IntfID requestedInterfaceId)
{
    if (baseObject == nullptr)
        return py::none();

    switch (baseObject.getCoreType())
    {
        case daq::ctBool:
        {
            py::bool_ p(static_cast<daq::Bool>(baseObject));
            return p;
        }
        case daq::ctInt:
        {
            py::int_ p(static_cast<daq::Int>(baseObject));
            return p;
        }
        case daq::ctFloat:
        {
            py::float_ p(static_cast<daq::Float>(baseObject));
            return p;
        }
        case daq::ctString:
        {
            py::str p(static_cast<std::string>(baseObject));
            return p;
        }
        case daq::ctList:
        {
            InterfaceWrapper<daq::IList> wrappedInterface(baseObject.asPtr<daq::IList>().addRefAndReturn());
            return py::cast(wrappedInterface);
        }
        case daq::ctDict:
        {
            InterfaceWrapper<daq::IDict> wrappedInterface(baseObject.asPtr<daq::IDict>().addRefAndReturn());
            return py::cast(wrappedInterface);
        }
        case daq::ctRatio:
        {
            const auto ratio = baseObject.asPtr<daq::IRatio>();
            py::object p = python_class_fraction(ratio.getNumerator(), ratio.getDenominator());
            return p;
        }
        // case daq::ctProc:
        case daq::ctObject:
        {
            // When the function was called with a valid requestedInterfaceId, convert to that object type.
            // If not, ask the object what interface it most likely belongs to via IInspectable and convert to that object type.
            const daq::IntfID interfaceId = (requestedInterfaceId == daq::IBaseObject::Id)
                                               ? baseObject.asPtr<daq::IInspectable>(true).getInterfaceIds()[0]
                                               : requestedInterfaceId;

            if (daqInterfaceIdToClass.find(interfaceId) != daqInterfaceIdToClass.end())
                return daqInterfaceIdToClass[interfaceId](baseObject);

            const auto po = baseObject.asPtrOrNull<IPyObject>();
            if (po != nullptr)
            {
                py::object p;
                const daq::ErrCode err = po->getPyObject(p);
                daq::checkErrorInfo(err);
                return p;
            }

            InterfaceWrapper<daq::IBaseObject> wrappedInterface(baseObject.addRefAndReturn());
            return py::cast(wrappedInterface);
        }
        // case daq::ctBinaryData:
        // case daq::ctFunc:
        case daq::ctComplexNumber:
        {
            const auto number = baseObject.asPtr<daq::IComplexNumber>();
            py::object p = py::cast(static_cast<std::complex<double>>(number.getValue()));
            return p;
        }
        case daq::ctStruct:
        {
            InterfaceWrapper<daq::IStruct> wrappedInterface(baseObject.asPtr<daq::IStruct>().addRefAndReturn());
            return py::cast(wrappedInterface);
        }
        case daq::ctEnumeration:
        {
            InterfaceWrapper<daq::IEnumeration> wrappedInterface(baseObject.asPtr<daq::IEnumeration>().addRefAndReturn());
            return py::cast(wrappedInterface);
        }
        default:
        {
            InterfaceWrapper<daq::IBaseObject> wrappedInterface(baseObject.addRefAndReturn());
            return py::cast(wrappedInterface);
        }
    }
}
