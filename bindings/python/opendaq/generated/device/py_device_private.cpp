//------------------------------------------------------------------------------
// <auto-generated>
//     This code was generated by a tool.
//
//     Changes to this file may cause incorrect behavior and will be lost if
//     the code is regenerated.
//
//     RTGen (PythonGenerator).
// </auto-generated>
//------------------------------------------------------------------------------

/*
 * Copyright 2022-2024 openDAQ d.o.o.
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

#include <pybind11/gil.h>

#include "py_opendaq/py_opendaq.h"
#include "py_core_types/py_converter.h"

PyDaqIntf<daq::IDevicePrivate, daq::IBaseObject> declareIDevicePrivate(pybind11::module_ m)
{
    return wrapInterface<daq::IDevicePrivate, daq::IBaseObject>(m, "IDevicePrivate");
}

void defineIDevicePrivate(pybind11::module_ m, PyDaqIntf<daq::IDevicePrivate, daq::IBaseObject> cls)
{
    cls.doc() = "";

    cls.def_property("as_root",
        nullptr,
        [](daq::IDevicePrivate *object)
        {
            py::gil_scoped_release release;
            const auto objectPtr = daq::DevicePrivatePtr::Borrow(object);
            objectPtr.setAsRoot();
        },
        "");
    cls.def_property("device_config",
        [](daq::IDevicePrivate *object)
        {
            py::gil_scoped_release release;
            const auto objectPtr = daq::DevicePrivatePtr::Borrow(object);
            return objectPtr.getDeviceConfig().detach();
        },
        [](daq::IDevicePrivate *object, daq::IPropertyObject* config)
        {
            py::gil_scoped_release release;
            const auto objectPtr = daq::DevicePrivatePtr::Borrow(object);
            objectPtr.setDeviceConfig(config);
        },
        py::return_value_policy::take_ownership,
        "");
    cls.def("lock",
        [](daq::IDevicePrivate *object, daq::IUser* user)
        {
            py::gil_scoped_release release;
            const auto objectPtr = daq::DevicePrivatePtr::Borrow(object);
            objectPtr.lock(user);
        },
        py::arg("user"),
        "");
    cls.def("unlock",
        [](daq::IDevicePrivate *object, daq::IUser* user)
        {
            py::gil_scoped_release release;
            const auto objectPtr = daq::DevicePrivatePtr::Borrow(object);
            objectPtr.unlock(user);
        },
        py::arg("user"),
        "");
    cls.def_property_readonly("locked_internal",
        [](daq::IDevicePrivate *object)
        {
            py::gil_scoped_release release;
            const auto objectPtr = daq::DevicePrivatePtr::Borrow(object);
            return objectPtr.isLockedInternal();
        },
        "");
    cls.def("force_unlock",
        [](daq::IDevicePrivate *object)
        {
            py::gil_scoped_release release;
            const auto objectPtr = daq::DevicePrivatePtr::Borrow(object);
            objectPtr.forceUnlock();
        },
        "");
}
