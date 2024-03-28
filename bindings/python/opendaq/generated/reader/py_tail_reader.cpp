/*
 * Copyright 2022-2024 Blueberry d.o.o.
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

PyDaqIntf<daq::ITailReader, daq::ISampleReader> declareITailReader(pybind11::module_ m)
{
    return wrapInterface<daq::ITailReader, daq::ISampleReader>(m, "ITailReader");
}

void defineITailReader(pybind11::module_ m, PyDaqIntf<daq::ITailReader, daq::ISampleReader> cls)
{
    cls.doc() = "A reader that only ever reads the last N samples, subsequent calls may result in overlapping data.";

    m.def(
        "TailReader",
        [](daq::ISignal* signal, const size_t historySize, daq::SampleType valueReadType, daq::SampleType domainReadType)
        {
            const auto signalPtr = daq::SignalPtr::Borrow(signal);
            if (valueReadType != daq::SampleType::Invalid || domainReadType != daq::SampleType::Invalid)
            {
                PyTypedReader::checkTypes(valueReadType, domainReadType);
                return daq::TailReader(signalPtr, historySize, valueReadType, domainReadType).detach();
            }
            else
            {
                return daq::TailReader(signalPtr, historySize).detach();
            }
        },
        py::arg("signal"),
        py::arg("history_size"),
        py::arg("value_type") = daq::SampleType::Invalid,
        py::arg("domain_type") = daq::SampleType::Invalid,
        "A reader that only ever reads the last N samples, subsequent calls may result in overlapping data.");

    m.def("TailReaderFromExisting", &daq::TailReaderFromExisting_Create);

    cls.def(
        "read",
        [](daq::ITailReader* object, size_t count)
        {
            const auto objectPtr = daq::TailReaderPtr::Borrow(object);
            return PyTypedReader::readValues(objectPtr, count, 0);
        },
        py::arg("count"),
        "Copies at maximum the next `count` unread samples to the values buffer. The amount actually read is returned through the `count` "
        "parameter.");
    cls.def(
        "read_with_domain",
        [](daq::ITailReader* object, size_t count)
        {
            const auto objectPtr = daq::TailReaderPtr::Borrow(object);
            return PyTypedReader::readValuesWithDomain(objectPtr, count, 0);
        },
        py::arg("count"),
        "Copies at maximum the next `count` unread samples and clock-stamps to the `values` and `stamps` buffers. The amount actually read "
        "is returned through the `count` parameter.");
    cls.def_property_readonly(
        "history_size",
        [](daq::ITailReader* object)
        {
            const auto objectPtr = daq::TailReaderPtr::Borrow(object);
            return objectPtr.getHistorySize();
        },
        "The maximum amount of samples in history to keep.");
}
