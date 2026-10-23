/*
 * Copyright 2022-2026 openDAQ d.o.o.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
 
#pragma once

#include <pybind11/pybind11.h>

#include <coretypes/coretypes.h>

namespace py = pybind11;

extern py::handle opendaq_daq_module;
extern py::handle python_class_fraction;
extern std::unordered_map<daq::IntfID, std::function<py::object(const daq::ObjectPtr<daq::IBaseObject>&)>> daqInterfaceIdToClass;

template <class Interface>
class InterfaceWrapper
{
public:
    InterfaceWrapper()
        : intf(nullptr)
    {
    }

    InterfaceWrapper(Interface* intf):
        intf(intf)
    {
    }

    InterfaceWrapper(const InterfaceWrapper& other)
        : intf(other.intf)
    {
        if (intf != nullptr) 
            intf->addRef();
    }

    InterfaceWrapper(InterfaceWrapper&& other)
        : intf(other.intf)
    {
        other.intf = nullptr;
    }

    ~InterfaceWrapper()
    {
        if (intf != nullptr)
            intf->releaseRef();
    }

    Interface* get() const
    {
        return intf;
    }

private:
    Interface* intf;
};

template <class Interface>
Interface* pyQI(py::handle handle)
{
    if (py::isinstance<Interface>(handle))
    {
        Interface* interface = handle.cast<Interface*>();
        interface->addRef();
        return interface;
    }

    return nullptr;
}

daq::ObjectPtr<daq::IBaseObject> pyObjectToBaseObject(const py::object& handle, bool acquireGil = true);
py::object baseObjectToPyObject(const daq::ObjectPtr<daq::IBaseObject>& baseObject,
                                const daq::IntfID requestedInterfaceId = daq::IBaseObject::Id, bool acquireGil = true);

template <class ObjectType>
py::object baseObjectToPyObjectUsingType(const daq::ObjectPtr<ObjectType>& baseObject)
{
    InterfaceWrapper<ObjectType> wrappedInterface(baseObject.addRefAndReturn());  // must increment reference
    return py::cast(wrappedInterface, py::return_value_policy::take_ownership);
}

struct PyConverter
{
    static daq::ObjectPtr<daq::IBaseObject> ToBaseObject(const py::object& pyObject);
    static py::object ToPyObject(const daq::ObjectPtr<daq::IBaseObject>& baseObject);
};

template <class Interface>
void registerClassConverter()
{
    daqInterfaceIdToClass.emplace(Interface::Id, baseObjectToPyObjectUsingType<Interface>);
}

PYBIND11_DECLARE_HOLDER_TYPE(T, InterfaceWrapper<T>, true);

template <class Interface>
py::object castFrom(daq::IBaseObject* baseObject)
{
    // The first rule of Fight Club is: never return a raw pointer to an interface, 
    // because pybind might find an existing wrapper for it instead of creating a new one, which can lead to a reference count leak.
    Interface* iface = daq::BaseObjectPtr::Borrow(baseObject).as<Interface>();
    InterfaceWrapper<Interface> wrappedInterface(iface);
    return py::cast(wrappedInterface, py::return_value_policy::take_ownership);
}

template <class Interface>
bool canCastFrom(daq::IBaseObject* baseObject)
{
    return daq::BaseObjectPtr::Borrow(baseObject).supportsInterface<Interface>();
}

template <class Interface>
py::object convertFrom(daq::IBaseObject* baseObject)
{
    // The first rule of Fight Club is: never return a raw pointer to an interface, 
    // because pybind might find an existing wrapper for it instead of creating a new one, which can lead to a reference count leak.
    Interface* intf = nullptr;
    const daq::ErrCode err = baseObject->queryInterface(Interface::Id, reinterpret_cast<void**>(&intf));
    if constexpr (std::is_same<Interface, daq::IString>::value)
    {
        if (OPENDAQ_FAILED(err))
        {
            const auto str = daq::getString(baseObject);
            intf = daq::String_Create(str.c_str());
        }
    }
    else if constexpr (daq::Is_ct_conv<typename daq::IntfToCoreType<Interface>::CoreType>::value)
    {
        using Type = typename daq::IntfToCoreType<Interface>::CoreType;

        if (OPENDAQ_FAILED(err))
        {
            auto value = daq::getValueFromConvertible<Type>(baseObject);
            intf = daq::CoreTypeHelper<Type>::Create(value);
        }
    }
    else
    {
        daq::checkErrorInfo(err);
    }

    InterfaceWrapper<Interface> wrappedInterface(intf);
    return py::cast(wrappedInterface, py::return_value_policy::take_ownership);
}

template <class Type, class... Options>
using PyDaqClass = py::class_<Type, Options...>;

template <class Interface, class ParentInterface = daq::IBaseObject>
using PyDaqIntf = PyDaqClass<Interface, InterfaceWrapper<Interface>, ParentInterface>;

// Releasing openDAQ objects can join threads that need the GIL, so instances are destroyed without holding it
template <class Type, class... Options, typename... Extra>
PyDaqClass<Type, Options...> wrapDaqClass(py::handle scope, const char* name, const Extra&... extra)
{
    return PyDaqClass<Type, Options...>(scope, name, py::release_gil_before_calling_cpp_dtor(), extra...);
}

// Scalar values own no other objects, so releasing them cannot join threads.
// Exception: EvalValueImpl also implements IBoolean, IInteger, IFloat and IString, and can own arbitrary
// objects (arguments, function, cached references). It reaches a scalar wrapper only via an explicit
// cast_from/convert_from, which is rare enough to accept.
template <class Interface>
constexpr bool isScalarValueType = std::is_same_v<Interface, daq::IBoolean> || std::is_same_v<Interface, daq::IInteger> ||
                                   std::is_same_v<Interface, daq::IFloat> || std::is_same_v<Interface, daq::IString> ||
                                   std::is_same_v<Interface, daq::IRatio> || std::is_same_v<Interface, daq::IComplexNumber>;

template <class Interface, class ParentInterface = daq::IBaseObject, typename... Extra>
auto wrapInterface(pybind11::module_ m, const char* name, const Extra&... extra)
{
    registerClassConverter<Interface>();

    // Scalars keep the GIL, as releasing it lets other threads take it for a whole switch interval
    auto cls = isScalarValueType<Interface>
                   ? PyDaqIntf<Interface, ParentInterface>(m, name, extra...)
                   : wrapDaqClass<Interface, InterfaceWrapper<Interface>, ParentInterface>(m, name, extra...);
    cls.def_static("cast_from", [](daq::IBaseObject* obj) { return castFrom<Interface>(obj); });
    cls.def_static("convert_from", [](daq::IBaseObject* obj) { return convertFrom<Interface>(obj); });
    cls.def_static("can_cast_from", [](daq::IBaseObject* obj) { return canCastFrom<Interface>(obj); });
    cls.def_static("from_raw_interface",
                   [](const py::capsule& rawInterface)
                   {
                       if (std::strcmp(rawInterface.name(), "opendaq.raw_interface") != 0)
                           throw std::invalid_argument("Invalid capsule");

                       daq::IBaseObject* obj = rawInterface.get_pointer<daq::IBaseObject>();
                       return castFrom<Interface>(obj);
                   });

    return cls;
}
