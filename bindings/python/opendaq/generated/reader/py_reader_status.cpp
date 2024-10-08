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

PyDaqIntf<daq::IReaderStatus, daq::IBaseObject> declareIReaderStatus(pybind11::module_ m)
{
    py::enum_<daq::ReadStatus>(m, "ReadStatus")
        .value("Ok", daq::ReadStatus::Ok)
        .value("Event", daq::ReadStatus::Event)
        .value("Fail", daq::ReadStatus::Fail)
        .value("Unknown", daq::ReadStatus::Unknown);

    return wrapInterface<daq::IReaderStatus, daq::IBaseObject>(m, "IReaderStatus");
}

void defineIReaderStatus(pybind11::module_ m, PyDaqIntf<daq::IReaderStatus, daq::IBaseObject> cls)
{
    cls.doc() = "Represents the status of the reading process returned by the reader::read function.";

    m.def("ReaderStatus", [](daq::IEventPacket* eventPacket, const bool valid, std::variant<daq::INumber*, double, int64_t, daq::IEvalValue*>& offset){
        return daq::ReaderStatus_Create(eventPacket, valid, getVariantValue<daq::INumber*>(offset));
    }, py::arg("event_packet"), py::arg("valid"), py::arg("offset"));


    cls.def_property_readonly("read_status",
        [](daq::IReaderStatus *object)
        {
            py::gil_scoped_release release;
            const auto objectPtr = daq::ReaderStatusPtr::Borrow(object);
            return objectPtr.getReadStatus();
        },
        "Retrieves the current reading status, indicating whether the reading process is in an \"Ok\" state, has encountered an Event, has failed, or is in an Unknown state.");
    cls.def_property_readonly("event_packet",
        [](daq::IReaderStatus *object)
        {
            py::gil_scoped_release release;
            const auto objectPtr = daq::ReaderStatusPtr::Borrow(object);
            return objectPtr.getEventPacket().detach();
        },
        py::return_value_policy::take_ownership,
        "Retrieves the event packet from the reading process.");
    cls.def_property_readonly("valid",
        [](daq::IReaderStatus *object)
        {
            py::gil_scoped_release release;
            const auto objectPtr = daq::ReaderStatusPtr::Borrow(object);
            return objectPtr.getValid();
        },
        "Checks the validity of the reader.");
    cls.def_property_readonly("offset",
        [](daq::IReaderStatus *object)
        {
            py::gil_scoped_release release;
            const auto objectPtr = daq::ReaderStatusPtr::Borrow(object);
            return objectPtr.getOffset().detach();
        },
        py::return_value_policy::take_ownership,
        "Retrieves the offset of the the read values");
}
