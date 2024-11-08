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

PyDaqIntf<daq::IBlockReader, daq::ISampleReader> declareIBlockReader(pybind11::module_ m)
{
    return wrapInterface<daq::IBlockReader, daq::ISampleReader>(m, "IBlockReader", py::dynamic_attr());
}

void defineIBlockReader(pybind11::module_ m, PyDaqIntf<daq::IBlockReader, daq::ISampleReader> cls)
{
    cls.doc() = "A signal data reader that abstracts away reading of signal packets by keeping an internal read-position and automatically "
                "advances it on subsequent reads. The difference to a StreamReader is that instead of reading on per sample basis it "
                "always returns only a full block of samples. This means that even if more samples are available they will not be read "
                "until there is enough of them to fill at least one block.";

    m.def(
        "BlockReader",
        [](daq::ISignal* signal, size_t blockSize, daq::SampleType valueType, daq::SampleType domainType, daq::ReadMode mode, daq::Bool skipEvents)
        {
            PyTypedReader::checkTypes(valueType, domainType);
            if (blockSize < 1u)
                throw daq::InvalidParameterException("Block size must be greater than 0");

            const auto signalPtr = daq::SignalPtr::Borrow(signal);

            return daq::BlockReaderFromBuilder_Create(daq::BlockReaderBuilder()
                    .setSignal(signalPtr)
                    .setBlockSize(blockSize)
                    .setValueReadType(valueType)
                    .setDomainReadType(domainType)
                    .setReadMode(mode)
                    .setSkipEvents(skipEvents));
        },
        py::arg("signal"),
        py::arg("block_size"),
        py::arg("value_type") = daq::SampleType::Float64,
        py::arg("domain_type") = daq::SampleType::Int64,
        py::arg("read_mode") = daq::ReadMode::Scaled,
        py::arg("skip_events") = daq::True,
        "A signal data reader that abstracts away reading of signal packets by keeping an internal read-position and automatically "
        "advances it on subsequent reads. The difference to a StreamReader is that instead of reading on per sample basis it "
        "always returns only a full block of samples. This means that even if more samples are available they will not be read "
        "until there is enough of them to fill at least one block.");
    m.def("BlockReaderFromExisting", &daq::BlockReaderFromExisting_Create);

    cls.def(
        "read",
        [](daq::IBlockReader* object, size_t count, const size_t timeoutMs, bool returnStatus)
        {
            py::gil_scoped_release release;
            return PyTypedReader::readValues(daq::BlockReaderPtr::Borrow(object), count, timeoutMs, returnStatus); 
        },
        py::arg("count"),
        py::arg("timeout_ms") = 0,
        py::arg("return_status") = false,
        "Copies at maximum the next `count` blocks of unread samples to the values buffer."
        "The amount actually read is returned through the `count` parameter");

    cls.def(
        "read_with_domain",
        [](daq::IBlockReader* object, size_t count, const size_t timeoutMs, bool returnStatus)
        {
            py::gil_scoped_release release;
            return PyTypedReader::readValuesWithDomain(daq::BlockReaderPtr::Borrow(object), count, timeoutMs, returnStatus); 
        },
        py::arg("count"),
        py::arg("timeout_ms") = 0,
        py::arg("return_status") = false,
        "Copies at maximum the next `count` blocks of unread samples and clock-stamps to the `dataBlocks` and `domainBlocks` buffers."
        "The amount actually read is returned through the `count` parameter.");

    cls.def_property_readonly(
        "block_size",
        [](daq::IBlockReader* object)
        {
            py::gil_scoped_release release;
            const auto objectPtr = daq::BlockReaderPtr::Borrow(object);
            return objectPtr.getBlockSize();
        },
        "The amount of samples the reader considers as one block.");
}
