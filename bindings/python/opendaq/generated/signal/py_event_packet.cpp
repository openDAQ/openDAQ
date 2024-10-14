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

PyDaqIntf<daq::IEventPacket, daq::IPacket> declareIEventPacket(pybind11::module_ m)
{
    return wrapInterface<daq::IEventPacket, daq::IPacket>(m, "IEventPacket");
}

void defineIEventPacket(pybind11::module_ m, PyDaqIntf<daq::IEventPacket, daq::IPacket> cls)
{
    cls.doc() = "As with Data packets, Event packets travel along the signal paths. They are used to notify recipients of any relevant changes to the signal sending the packet.";

    m.def("EventPacket", [](std::variant<daq::IString*, py::str, daq::IEvalValue*>& id, std::variant<daq::IDict*, py::dict>& params){
        return daq::EventPacket_Create(getVariantValue<daq::IString*>(id), getVariantValue<daq::IDict*>(params));
    }, py::arg("id"), py::arg("params"));

    m.def("DataDescriptorChangedEventPacket", &daq::DataDescriptorChangedEventPacket_Create);
    m.def("ImplicitDomainGapDetectedEventPacket", [](std::variant<daq::INumber*, double, int64_t, daq::IEvalValue*>& diff){
        return daq::ImplicitDomainGapDetectedEventPacket_Create(getVariantValue<daq::INumber*>(diff));
    }, py::arg("diff"));


    cls.def_property_readonly("event_id",
        [](daq::IEventPacket *object)
        {
            py::gil_scoped_release release;
            const auto objectPtr = daq::EventPacketPtr::Borrow(object);
            return objectPtr.getEventId().toStdString();
        },
        "Gets the ID of the event as a string. In example \"DATA_DESCRIPTOR_CHANGED\".");
    cls.def_property_readonly("parameters",
        [](daq::IEventPacket *object)
        {
            py::gil_scoped_release release;
            const auto objectPtr = daq::EventPacketPtr::Borrow(object);
            return objectPtr.getParameters().detach();
        },
        py::return_value_policy::take_ownership,
        "Dictionary containing parameters as <String, BaseObject> pairs relevant to the event signalized by the Event packet.");
}
