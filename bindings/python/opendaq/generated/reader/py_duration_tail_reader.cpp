/*
 * Copyright 2022-2026 openDAQ d.o.o.
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

PyDaqIntf<daq::IDurationTailReader, daq::ISampleReader> declareIDurationTailReader(pybind11::module_ m)
{
    return wrapInterface<daq::IDurationTailReader, daq::ISampleReader>(m, "IDurationTailReader", py::dynamic_attr());
}

void defineIDurationTailReader(pybind11::module_ m, PyDaqIntf<daq::IDurationTailReader, daq::ISampleReader> cls)
{
    cls.doc() = "A tail reader that keeps only the samples within a trailing time window, measured in milliseconds from the domain "
                "value of the most recently received sample, instead of a fixed sample count.";

    m.def(
        "DurationTailReader",
        [](daq::ISignal* signal, const uint64_t historyDurationMs, daq::SampleType valueReadType, daq::SampleType domainReadType, daq::ReadMode mode)
        {
            PyTypedReader::checkTypes(valueReadType, domainReadType);
            return daq::DurationTailReader_Create(signal, historyDurationMs, valueReadType, domainReadType, mode);
        },
        py::arg("signal"),
        py::arg("history_duration_ms"),
        py::arg("value_type") = daq::SampleType::Float64,
        py::arg("domain_type") = daq::SampleType::Int64,
        py::arg("read_mode") = daq::ReadMode::Scaled,
        "A tail reader that keeps only the samples within a trailing time window, measured in milliseconds, instead of a fixed "
        "sample count.");

    cls.def(
        "read",
        [](daq::IDurationTailReader* object, size_t count, bool returnStatus)
        {
            py::gil_scoped_release release;
            const auto objectPtr = daq::DurationTailReaderPtr::Borrow(object);
            return PyTypedReader::readValues(objectPtr, count, 0, returnStatus);
        },
        py::arg("count"), py::arg("return_status") = false,
        "Copies at maximum the next `count` unread samples to the values buffer. The amount actually read is returned through the `count` "
        "parameter.");
    cls.def(
        "read_with_domain",
        [](daq::IDurationTailReader* object, size_t count, bool returnStatus)
        {
            py::gil_scoped_release release;
            const auto objectPtr = daq::DurationTailReaderPtr::Borrow(object);
            return PyTypedReader::readValuesWithDomain(objectPtr, count, 0, returnStatus);
        },
        py::arg("count"),
        py::arg("return_status") = false,
        "Copies at maximum the next `count` unread samples and clock-stamps to the `values` and `stamps` buffers. The amount actually "
        "read is returned through the `count` parameter.");
    cls.def_property(
        "history_duration_ms",
        [](daq::IDurationTailReader* object)
        {
            py::gil_scoped_release release;
            const auto objectPtr = daq::DurationTailReaderPtr::Borrow(object);
            return objectPtr.getHistoryDurationMs();
        },
        [](daq::IDurationTailReader* object, uint64_t milliseconds)
        {
            py::gil_scoped_release release;
            const auto objectPtr = daq::DurationTailReaderPtr::Borrow(object);
            objectPtr.setHistoryDurationMs(milliseconds);
        },
        "The trailing history duration, in milliseconds. Buffered packets older than the new duration relative to the latest "
        "received sample are dropped immediately when set.");
}
