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

py::class_<daq::TimeReader<daq::StreamReaderPtr>> declareTimeStreamReader(pybind11::module_ m)
{
    return py::class_<daq::TimeReader<daq::StreamReaderPtr>>(m, "TimeStreamReader");
}

py::class_<daq::TimeReader<daq::TailReaderPtr>> declareTimeTailReader(pybind11::module_ m)
{
    return py::class_<daq::TimeReader<daq::TailReaderPtr>>(m, "TimeTailReader");
}

py::class_<daq::TimeReader<daq::BlockReaderPtr>> declareTimeBlockReader(pybind11::module_ m)
{
    return py::class_<daq::TimeReader<daq::BlockReaderPtr>>(m, "TimeBlockReader");
}

py::class_<daq::TimeReader<daq::MultiReaderPtr>> declareTimeMultiReader(pybind11::module_ m)
{
    return py::class_<daq::TimeReader<daq::MultiReaderPtr>>(m, "TimeMultiReader");
}

void defineTimeStreamReader(pybind11::module_ m, py::class_<daq::TimeReader<daq::StreamReaderPtr>> cls)
{
    cls.doc() = "A wrapper for stream signal data reader that provides the ability to read samples with timestamps.";

    cls.def(py::init(
        [](daq::IStreamReader* reader)
        {
            const auto objectPtr = daq::StreamReaderPtr::Borrow(reader);
            return std::make_unique<daq::TimeReader<daq::StreamReaderPtr>>(objectPtr);
        }));

    cls.def(
        "read_with_timestamps",
        [](daq::TimeReader<daq::StreamReaderPtr>* object, size_t count, const size_t timeoutMs)
        { return PyTypedReader::readValuesWithDomain(*object, count, timeoutMs); },
        py::arg("count"),
        py::arg("timeout_ms") = 0,
        "Returns the next `count` unread samples and clock-stamps.");
}

void defineTimeTailReader(pybind11::module_ m, py::class_<daq::TimeReader<daq::TailReaderPtr>> cls)
{
    cls.doc() = "A wrapper for tail signal data reader that provides the ability to read samples with timestamps.";

    cls.def(py::init(
        [](daq::ITailReader* reader)
        {
            const auto objectPtr = daq::TailReaderPtr::Borrow(reader);
            return std::make_unique<daq::TimeReader<daq::TailReaderPtr>>(objectPtr);
        }));

    cls.def(
        "read_with_timestamps",
        [](daq::TimeReader<daq::TailReaderPtr>* object, size_t count, const size_t timeoutMs)
        { return PyTypedReader::readValuesWithDomain(*object, count, timeoutMs); },
        py::arg("count"),
        py::arg("timeout_ms") = 0,
        "Returns the next `count` last unread samples and clock-stamps.");
}

void defineTimeBlockReader(pybind11::module_ m, py::class_<daq::TimeReader<daq::BlockReaderPtr>> cls)
{
    cls.doc() = "A wrapper for block signal data reader that provides the ability to read samples with timestamps.";

    cls.def(py::init(
        [](daq::IBlockReader* reader)
        {
            const auto objectPtr = daq::BlockReaderPtr::Borrow(reader);
            return std::make_unique<daq::TimeReader<daq::BlockReaderPtr>>(objectPtr);
        }));

    cls.def(
        "read_with_timestamps",
        [](daq::TimeReader<daq::BlockReaderPtr>* object, size_t count, const size_t timeoutMs)
        { return PyTypedReader::readValuesWithDomain(*object, count, timeoutMs); },
        py::arg("count"),
        py::arg("timeout_ms") = 0,
        "Returns the next `count` unread blocks of samples and clock-stamps.");
}

void defineTimeMultiReader(pybind11::module_ m, py::class_<daq::TimeReader<daq::MultiReaderPtr>> cls)
{
    cls.doc() = "A wrapper for multi signal data reader that provides the ability to read samples with timestamps.";

    cls.def(py::init(
        [](daq::IMultiReader* reader)
        {
            const auto objectPtr = daq::MultiReaderPtr::Borrow(reader);
            return std::make_unique<daq::TimeReader<daq::MultiReaderPtr>>(objectPtr);
        }));

    cls.def(
        "read_with_timestamps",
        [](daq::TimeReader<daq::MultiReaderPtr>* object, size_t count, const size_t timeoutMs)
        { return PyTypedReader::readValuesWithDomain(*object, count, timeoutMs); },
        py::arg("count"),
        py::arg("timeout_ms") = 0,
        "Returns the next `count` unread samples and clock-stamps.");
}