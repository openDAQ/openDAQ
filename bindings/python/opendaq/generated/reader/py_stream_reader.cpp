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

#include "py_opendaq/py_typed_reader.h"

PyDaqIntf<daq::IStreamReader, daq::ISampleReader> declareIStreamReader(pybind11::module_ m)
{
    return wrapInterface<daq::IStreamReader, daq::ISampleReader>(m, "IStreamReader", py::dynamic_attr());
}

void defineIStreamReader(pybind11::module_ m, PyDaqIntf<daq::IStreamReader, daq::ISampleReader> cls)
{
    cls.doc() = "A signal data reader that abstracts away reading of signal packets by keeping an internal read-position and automatically "
                "advances it on subsequent reads.";

    m.def(
        "StreamReader",
        [](daq::ISignal* signal, daq::SampleType valueType, daq::SampleType domainType, daq::ReadMode mode, daq::ReadTimeoutType timeoutType)
        {
            PyTypedReader::checkTypes(valueType, domainType);
            const auto signalPtr = daq::SignalPtr::Borrow(signal);
            return daq::StreamReader_Create(signal, valueType, domainType, mode, timeoutType);
        },
        py::arg("signal"),
        py::arg("value_type") = daq::SampleType::Float64,
        py::arg("domain_type") = daq::SampleType::Int64,
        py::arg("read_mode") = daq::ReadMode::Scaled,
        py::arg("timeout_type") = daq::ReadTimeoutType::All,
        "");
    m.def("StreamReaderFromExisting", &daq::StreamReaderFromExisting_Create);

    cls.def(
        "read",
        [](daq::IStreamReader* object, size_t count, const size_t timeoutMs, bool returnStatus)
        {
            py::gil_scoped_release release;
            return PyTypedReader::readValues(daq::StreamReaderPtr::Borrow(object), count, timeoutMs, returnStatus); 
        },
        py::arg("count"),
        py::arg("timeout_ms") = 0,
        py::arg("return_status") = false,
        "Copies at maximum the next `count` unread samples to the values buffer. The amount actually read is returned through the `count` "
        "parameter.");
    cls.def(
        "read_with_domain",
        [](daq::IStreamReader* object, size_t count, const size_t timeoutMs, bool returnStatus)
        {
            py::gil_scoped_release release;
            return PyTypedReader::readValuesWithDomain(daq::StreamReaderPtr::Borrow(object), count, timeoutMs, returnStatus); 
        },
        py::arg("count"),
        py::arg("timeout_ms") = 0,
        py::arg("return_status") = false,
        "Copies at maximum the next `count` unread samples and clock-stamps to the `values` and `stamps` buffers. The amount actually read "
        "is returned through the `count` parameter.");
    cls.def(
        "skip_samples",
        [](daq::IStreamReader* object, size_t count, bool returnStatus)
        {
            py::gil_scoped_release release;
            const auto objectPtr = daq::StreamReaderPtr::Borrow(object);
            auto status = objectPtr.skipSamples(&count);
            return returnStatus ? SizeReaderStatusVariant<decltype(objectPtr)>{std::make_tuple(count, status.detach())} :
              SizeReaderStatusVariant<decltype(objectPtr)>{count};
        },
        py::arg("count"),
        py::arg("return_status") = false,
        "Skips the specified amount of samples.");
}
