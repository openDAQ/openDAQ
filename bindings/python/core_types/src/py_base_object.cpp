#include "py_core_types/py_core_types.h"

void declareAndDefineIBaseObject(pybind11::module_ m)
{
    auto cls = py::class_<daq::IBaseObject, InterfaceWrapper<daq::IBaseObject>>(m, "IBaseObject");

    cls.doc() = "Extends `IUnknown` by providing additional methods for borrowing interfaces, hashing, and equality comparison. "
                "All openDAQ objects implement `IBaseObject` interface or its descendants. "
                "Hashing and equality comparison provides the ability to use the object as an element in dictionaries and lists. "
                "Classes that implement any interface derived from `IBaseObject` should be derived from `ImplementationOf` class, "
                "which provides the default implementation of `IBaseObject` interface methods.";

    m.def("BaseObject", &daq::BaseObject_Create);

    cls.def(py::init(
        [](daq::IBaseObject* obj)
        {
            obj->addRef();
            return obj;
        }));
    cls.def("__str__",
        [](daq::IBaseObject* obj)
        {
            return getString(obj);
        });
    cls.def("__hash__",
        [](daq::IBaseObject* obj)
        {
            return daq::BaseObjectPtr::Borrow(obj).getHashCode();
        });
    cls.def("__int__",
        [](daq::IBaseObject* obj)
        {
            return static_cast<daq::Int>(daq::BaseObjectPtr::Borrow(obj));
        });
    cls.def("__float__",
        [](daq::IBaseObject* obj)
        {
            return static_cast<daq::Float>(daq::BaseObjectPtr::Borrow(obj));
        });
    cls.def("__eq__",
        [](daq::IBaseObject* obj, const py::object& other)
        {
            const auto objPtr = daq::BaseObjectPtr::Borrow(obj);

            try
            {
                if (auto intf = pyQI<daq::IBaseObject>(other))
                {
                    const auto otherPtr = daq::ObjectPtr(std::move(intf));
                    return objPtr == otherPtr;  // this will trigger IBaseObject.equals
                }

                if (py::isinstance<py::int_>(other))
                    return static_cast<daq::Int>(objPtr) == static_cast<daq::Int>(py::int_(other));

                if (py::isinstance<py::float_>(other))
                    return static_cast<daq::Float>(objPtr) == static_cast<daq::Float>(py::float_(other));

                if (py::isinstance<py::str>(other))
                    return static_cast<std::string>(objPtr) == static_cast<std::string>(py::str(other));

                return false;
            }
            catch (std::exception&)
            {
                return false;
            }
        });
    cls.def_property_readonly("core_type",
        [](daq::IBaseObject* obj)
        {
            return daq::BaseObjectPtr::Borrow(obj).getCoreType();
        });
    cls.def_static("cast_from",
        [](daq::IBaseObject* obj)
        {
            return castFrom<daq::IBaseObject>(obj);
        });
    cls.def_static("can_cast_from",
        [](daq::IBaseObject* obj) {
            return canCastFrom<daq::IBaseObject>(obj);
        });
}
