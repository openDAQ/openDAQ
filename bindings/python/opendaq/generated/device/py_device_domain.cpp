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
 * Copyright 2022-2025 openDAQ d.o.o.
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

PyDaqIntf<daq::IDeviceDomain, daq::IBaseObject> declareIDeviceDomain(pybind11::module_ m)
{
    return wrapInterface<daq::IDeviceDomain, daq::IBaseObject>(m, "IDeviceDomain");
}

void defineIDeviceDomain(pybind11::module_ m, PyDaqIntf<daq::IDeviceDomain, daq::IBaseObject> cls)
{
    cls.doc() = "Contains information about the domain of the device.";

    m.def("DeviceDomain", [](std::variant<daq::IRatio*, std::pair<int64_t, int64_t>>& tickResolution, std::variant<daq::IString*, py::str, daq::IEvalValue*>& origin, daq::IUnit* unit){
        return daq::DeviceDomain_Create(getVariantValue<daq::IRatio*>(tickResolution), getVariantValue<daq::IString*>(origin), unit);
    }, py::arg("tick_resolution"), py::arg("origin"), py::arg("unit"));

    m.def("DeviceDomain", [](std::variant<daq::IRatio*, std::pair<int64_t, int64_t>>& tickResolution, std::variant<daq::IString*, py::str, daq::IEvalValue*>& origin, daq::IUnit* unit, daq::IReferenceDomainInfo* referenceDomainInfo){
        return daq::DeviceDomain_Create(getVariantValue<daq::IRatio*>(tickResolution), getVariantValue<daq::IString*>(origin), unit, referenceDomainInfo);
    }, py::arg("tick_resolution"), py::arg("origin"), py::arg("unit"), py::arg("reference_domain_info"));


    cls.def_property_readonly("tick_resolution",
        [](daq::IDeviceDomain *object)
        {
            py::gil_scoped_release release;
            const auto objectPtr = daq::DeviceDomainPtr::Borrow(object);
            return objectPtr.getTickResolution().detach();
        },
        py::return_value_policy::take_ownership,
        "Gets domain (usually time) between two consecutive ticks. Resolution is provided in a domain unit.");
    cls.def_property_readonly("origin",
        [](daq::IDeviceDomain *object)
        {
            py::gil_scoped_release release;
            const auto objectPtr = daq::DeviceDomainPtr::Borrow(object);
            return objectPtr.getOrigin().toStdString();
        },
        "Gets the device's absolute origin. Most often this is a time epoch in the ISO 8601 format.");
    cls.def_property_readonly("unit",
        [](daq::IDeviceDomain *object)
        {
            py::gil_scoped_release release;
            const auto objectPtr = daq::DeviceDomainPtr::Borrow(object);
            return objectPtr.getUnit().detach();
        },
        py::return_value_policy::take_ownership,
        "Gets the domain unit (eg. seconds, hours, degrees...)");
    cls.def_property_readonly("reference_domain_info",
        [](daq::IDeviceDomain *object)
        {
            py::gil_scoped_release release;
            const auto objectPtr = daq::DeviceDomainPtr::Borrow(object);
            return objectPtr.getReferenceDomainInfo().detach();
        },
        py::return_value_policy::take_ownership,
        "Gets the Reference Domain Info.");
}
