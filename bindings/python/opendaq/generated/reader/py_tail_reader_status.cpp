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

PyDaqIntf<daq::ITailReaderStatus, daq::IReaderStatus> declareITailReaderStatus(pybind11::module_ m)
{
    return wrapInterface<daq::ITailReaderStatus, daq::IReaderStatus>(m, "ITailReaderStatus");
}

void defineITailReaderStatus(pybind11::module_ m, PyDaqIntf<daq::ITailReaderStatus, daq::IReaderStatus> cls)
{
    cls.doc() = "ITailReaderStatus inherits from IReaderStatus to expand information returned read function";

    m.def("TailReaderStatus", [](daq::IEventPacket* eventPacket, const bool valid, std::variant<daq::INumber*, double, int64_t, daq::IEvalValue*>& offset, const bool sufficientHistory){
        return daq::TailReaderStatus_Create(eventPacket, valid, getVariantValue<daq::INumber*>(offset), sufficientHistory);
    }, py::arg("event_packet"), py::arg("valid"), py::arg("offset"), py::arg("sufficient_history"));


    cls.def_property_readonly("sufficient_history",
        [](daq::ITailReaderStatus *object)
        {
            py::gil_scoped_release release;
            const auto objectPtr = daq::TailReaderStatusPtr::Borrow(object);
            return objectPtr.getSufficientHistory();
        },
        "");
}
