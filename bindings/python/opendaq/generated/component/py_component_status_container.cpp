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
#include "py_core_objects/py_variant_extractor.h"

PyDaqIntf<daq::IComponentStatusContainer, daq::IBaseObject> declareIComponentStatusContainer(pybind11::module_ m)
{
    return wrapInterface<daq::IComponentStatusContainer, daq::IBaseObject>(m, "IComponentStatusContainer");
}

void defineIComponentStatusContainer(pybind11::module_ m, PyDaqIntf<daq::IComponentStatusContainer, daq::IBaseObject> cls)
{
    cls.doc() = "A container of Component Statuses and their corresponding values.";

    m.def("ComponentStatusContainer", &daq::ComponentStatusContainer_Create);

    cls.def("get_status",
        [](daq::IComponentStatusContainer *object, std::variant<daq::IString*, py::str, daq::IEvalValue*>& name)
        {
            py::gil_scoped_release release;
            const auto objectPtr = daq::ComponentStatusContainerPtr::Borrow(object);
            return objectPtr.getStatus(getVariantValue<daq::IString*>(name)).detach();
        },
        py::arg("name"),
        "Gets the current value of Component status with a given name.");
    cls.def_property_readonly("statuses",
        [](daq::IComponentStatusContainer *object)
        {
            py::gil_scoped_release release;
            const auto objectPtr = daq::ComponentStatusContainerPtr::Borrow(object);
            return objectPtr.getStatuses().detach();
        },
        py::return_value_policy::take_ownership,
        "Gets the current values of all Component statuses.");
    cls.def("get_status_message",
        [](daq::IComponentStatusContainer *object, std::variant<daq::IString*, py::str, daq::IEvalValue*>& name)
        {
            py::gil_scoped_release release;
            const auto objectPtr = daq::ComponentStatusContainerPtr::Borrow(object);
            return objectPtr.getStatusMessage(getVariantValue<daq::IString*>(name)).toStdString();
        },
        py::arg("name"),
        "Gets the status message of Component status with a given name.");
}
