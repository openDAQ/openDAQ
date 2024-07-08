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
        [](daq::ISignal* signal, daq::SampleType valueType, daq::SampleType domainType, daq::ReadTimeoutType timeoutType)
        {
            const auto signalPtr = daq::SignalPtr::Borrow(signal);
            if (valueType != daq::SampleType::Invalid || domainType != daq::SampleType::Invalid)
            {
                PyTypedReader::checkTypes(valueType, domainType);
                return daq::StreamReader(signalPtr, valueType, domainType, daq::ReadMode::Scaled, timeoutType).detach();
            }
            else
                return daq::StreamReader(signalPtr, daq::ReadMode::Scaled, timeoutType).detach();
        },
        py::arg("signal"),
        py::arg("value_type") = daq::SampleType::Invalid,
        py::arg("domain_type") = daq::SampleType::Invalid,
        py::arg("timeout_type") = daq::ReadTimeoutType::All,
        "");
    m.def("StreamReaderFromExisting", &daq::StreamReaderFromExisting_Create);

    cls.def(
        "read",
        [](daq::IStreamReader* object, size_t count, const size_t timeoutMs)
        { return PyTypedReader::readValues(daq::StreamReaderPtr::Borrow(object), count, timeoutMs); },
        py::arg("count"),
        py::arg("timeout_ms") = 0,
        "Copies at maximum the next `count` unread samples to the values buffer. The amount actually read is returned through the `count` "
        "parameter.");
    cls.def(
        "read_with_domain",
        [](daq::IStreamReader* object, size_t count, const size_t timeoutMs)
        { return PyTypedReader::readValuesWithDomain(daq::StreamReaderPtr::Borrow(object), count, timeoutMs); },
        py::arg("count"),
        py::arg("timeout_ms") = 0,
        "Copies at maximum the next `count` unread samples and clock-stamps to the `values` and `stamps` buffers. The amount actually read "
        "is returned through the `count` parameter.");
    cls.def(
        "skip_samples",
        [](daq::IStreamReader* object, const size_t count)
        {
            const auto objectPtr = daq::StreamReaderPtr::Borrow(object);
            return objectPtr.skipSamples(count).detach();
        },
        py::arg("count"),
        "Skips the specified amount of samples.");
}
