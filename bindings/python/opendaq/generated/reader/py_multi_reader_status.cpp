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

#include "py_opendaq/py_opendaq.h"
#include "py_core_types/py_converter.h"
#include "py_core_objects/py_variant_extractor.h"

PyDaqIntf<daq::IMultiReaderStatus, daq::IReaderStatus> declareIMultiReaderStatus(pybind11::module_ m)
{
    return wrapInterface<daq::IMultiReaderStatus, daq::IReaderStatus>(m, "IMultiReaderStatus");
}

void defineIMultiReaderStatus(pybind11::module_ m, PyDaqIntf<daq::IMultiReaderStatus, daq::IReaderStatus> cls)
{
    cls.doc() = "IMultiReaderStatus inherits from IReaderStatus to expand information returned read function";

    m.def("MultiReaderStatus", [](std::variant<daq::IDict*, py::dict>& eventPackets, const bool valid, std::variant<daq::INumber*, double, daq::IEvalValue*>& offset){
        return daq::MultiReaderStatus_Create(getVariantValue<daq::IDict*>(eventPackets), valid, getVariantValue<daq::INumber*>(offset));
    }, py::arg("event_packets"), py::arg("valid"), py::arg("offset"));


    cls.def_property_readonly("event_packets",
        [](daq::IMultiReaderStatus *object)
        {
            const auto objectPtr = daq::MultiReaderStatusPtr::Borrow(object);
            return objectPtr.getEventPackets().detach();
        },
        py::return_value_policy::take_ownership,
        "Retrieves the dictionary of event packets from the reading process, ordered by signals.");
}
