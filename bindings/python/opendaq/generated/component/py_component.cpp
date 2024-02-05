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
 * Copyright 2022-2023 Blueberry d.o.o.
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

#include "py_opendaq/py_opendaq.h"
#include "py_core_types/py_converter.h"

PyDaqIntf<daq::IComponent, daq::IPropertyObject> declareIComponent(pybind11::module_ m)
{
    return wrapInterface<daq::IComponent, daq::IPropertyObject>(m, "IComponent");
}

void defineIComponent(pybind11::module_ m, PyDaqIntf<daq::IComponent, daq::IPropertyObject> cls)
{
    cls.doc() = "Acts as a base interface for components, such as device, function block, channel and signal.";

    m.def("Component", &daq::Component_Create);

    cls.def_property_readonly("local_id",
        [](daq::IComponent *object)
        {
            const auto objectPtr = daq::ComponentPtr::Borrow(object);
            return objectPtr.getLocalId().toStdString();
        },
        "Gets the local ID of the component.");
    cls.def_property_readonly("global_id",
        [](daq::IComponent *object)
        {
            const auto objectPtr = daq::ComponentPtr::Borrow(object);
            return objectPtr.getGlobalId().toStdString();
        },
        "Gets the global ID of the component.");
    cls.def_property("active",
        [](daq::IComponent *object)
        {
            const auto objectPtr = daq::ComponentPtr::Borrow(object);
            return objectPtr.getActive();
        },
        [](daq::IComponent *object, const bool active)
        {
            const auto objectPtr = daq::ComponentPtr::Borrow(object);
            objectPtr.setActive(active);
        },
        "Returns true if the component is active; false otherwise. / Sets the component to be either active or inactive.");
    cls.def_property_readonly("context",
        [](daq::IComponent *object)
        {
            const auto objectPtr = daq::ComponentPtr::Borrow(object);
            return objectPtr.getContext().detach();
        },
        py::return_value_policy::take_ownership,
        "Gets the context object.");
    cls.def_property_readonly("parent",
        [](daq::IComponent *object)
        {
            const auto objectPtr = daq::ComponentPtr::Borrow(object);
            return objectPtr.getParent().detach();
        },
        py::return_value_policy::take_ownership,
        "Gets the parent of the component.");
    cls.def_property("name",
        [](daq::IComponent *object)
        {
            const auto objectPtr = daq::ComponentPtr::Borrow(object);
            return objectPtr.getName().toStdString();
        },
        [](daq::IComponent *object, const std::string& name)
        {
            const auto objectPtr = daq::ComponentPtr::Borrow(object);
            objectPtr.setName(name);
        },
        "Gets the name of the component. / Sets the name of the component.");
    cls.def_property("description",
        [](daq::IComponent *object)
        {
            const auto objectPtr = daq::ComponentPtr::Borrow(object);
            return objectPtr.getDescription().toStdString();
        },
        [](daq::IComponent *object, const std::string& description)
        {
            const auto objectPtr = daq::ComponentPtr::Borrow(object);
            objectPtr.setDescription(description);
        },
        "Gets the description of the component. / Sets the description of the component.");
    cls.def_property_readonly("tags",
        [](daq::IComponent *object)
        {
            const auto objectPtr = daq::ComponentPtr::Borrow(object);
            return objectPtr.getTags().detach();
        },
        py::return_value_policy::take_ownership,
        "Gets the tags of the component.");
    cls.def_property("visible",
        [](daq::IComponent *object)
        {
            const auto objectPtr = daq::ComponentPtr::Borrow(object);
            return objectPtr.getVisible();
        },
        [](daq::IComponent *object, const bool visible)
        {
            const auto objectPtr = daq::ComponentPtr::Borrow(object);
            objectPtr.setVisible(visible);
        },
        "Gets `visible` metadata state of the component / Sets `visible` attribute state of the component");
    cls.def_property_readonly("locked_attributes",
        [](daq::IComponent *object)
        {
            const auto objectPtr = daq::ComponentPtr::Borrow(object);
            return objectPtr.getLockedAttributes().detach();
        },
        py::return_value_policy::take_ownership,
        "Gets a list of the component's locked attributes. The locked attributes cannot be modified via their respective setters.");
    /*
    cls.def_property_readonly("on_component_core_event",
        [](daq::IComponent *object)
        {
            const auto objectPtr = daq::ComponentPtr::Borrow(object);
            return objectPtr.getOnComponentCoreEvent().detach();
        },
        py::return_value_policy::take_ownership,
        "Gets the Core Event object that triggers whenever a change to this component happens within the openDAQ core structure.");
    */
    cls.def_property_readonly("status_container",
        [](daq::IComponent *object)
        {
            const auto objectPtr = daq::ComponentPtr::Borrow(object);
            return objectPtr.getStatusContainer().detach();
        },
        py::return_value_policy::take_ownership,
        "Gets the container of Component statuses.");
}
