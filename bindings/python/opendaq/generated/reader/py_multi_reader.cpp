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
#include "py_core_objects/py_variant_extractor.h"

PyDaqIntf<daq::IMultiReader, daq::ISampleReader> declareIMultiReader(pybind11::module_ m)
{
    return wrapInterface<daq::IMultiReader, daq::ISampleReader>(m, "IMultiReader", py::dynamic_attr());
}

void defineIMultiReader(pybind11::module_ m, PyDaqIntf<daq::IMultiReader, daq::ISampleReader> cls)
{
    cls.doc() = "Reads multiple Signals at once.";

    m.def("MultiReader", [](std::variant<daq::IList*, py::list>& signals, daq::SampleType valueReadType, daq::SampleType domainReadType, daq::ReadMode mode, daq::ReadTimeoutType timeoutType) {
        PyTypedReader::checkTypes(valueReadType, domainReadType);
        if(domainReadType == daq::SampleType::Undefined)
            throw daq::InvalidParameterException("Domain type cannot be undefined.");
        return daq::MultiReader_Create(getVariantValue<daq::IList*>(signals), valueReadType, domainReadType, mode, timeoutType);
    },
    py::arg("signals"), 
    py::arg("value_type") = daq::SampleType::Float64, 
    py::arg("domain_type") = daq::SampleType::Int64,
    py::arg("read_mode") = daq::ReadMode::Scaled,
    py::arg("timeout_type") = daq::ReadTimeoutType::All,
    "Creates a MultiReader object that reads multiple signals at once.");
    m.def("MultiReaderFromExisting", &daq::MultiReaderFromExisting_Create);

    cls.def("read",
        [](daq::IMultiReader *object, size_t count, const size_t timeoutMs, bool returnStatus)
        {
            py::gil_scoped_release release;
            const auto objectPtr = daq::MultiReaderPtr::Borrow(object);
            return PyTypedReader::readValues(objectPtr, count, timeoutMs, returnStatus);
        },
        py::arg("count"), py::arg("timeout_ms") = 0, py::arg("return_status") = false,
        "Copies at maximum the next `count` unread samples to the values buffer. The amount actually read is returned through the `count` parameter.");
    cls.def("read_with_domain",
        [](daq::IMultiReader *object, size_t count, const size_t timeoutMs, bool returnStatus)
        {
            py::gil_scoped_release release;
            const auto objectPtr = daq::MultiReaderPtr::Borrow(object);
            return PyTypedReader::readValuesWithDomain(objectPtr, count, timeoutMs, returnStatus);
        },
        py::arg("count"), py::arg("timeout_ms") = 0, py::arg("return_status") = false,
        "Copies at maximum the next `count` unread samples and clock-stamps to the `samples` and `domain` buffers. The amount actually read is returned through the `count` parameter.");
    cls.def("skip_samples",
        [](daq::IMultiReader *object, size_t count, bool returnStatus)
        {
            py::gil_scoped_release release;
            const auto objectPtr = daq::MultiReaderPtr::Borrow(object);
            auto status = objectPtr.skipSamples(&count);
            return returnStatus ? SizeReaderStatusVariant<decltype(objectPtr)>{std::make_tuple(count, status.detach())} :
              SizeReaderStatusVariant<decltype(objectPtr)>{count};
        },
        py::arg("count"),
        py::arg("return_status") = false,
        "Skips the specified amount of samples.");
    cls.def_property_readonly("tick_resolution",
        [](daq::IMultiReader *object)
        {
            py::gil_scoped_release release;
            const auto objectPtr = daq::MultiReaderPtr::Borrow(object);
            return objectPtr.getTickResolution().detach();
        },
        py::return_value_policy::take_ownership,
        "Gets the resolution the reader aligned all the signals to. This is the highest resolution (lowest value) of all the signals to not loose the precision.");
    cls.def_property_readonly("origin",
        [](daq::IMultiReader *object)
        {
            py::gil_scoped_release release;
            const auto objectPtr = daq::MultiReaderPtr::Borrow(object);
            return objectPtr.getOrigin().toStdString();
        },
        "Gets the origin the reader aligned all the signals to. This is usually the earliest (lowest value) from all the signals.");
    cls.def_property_readonly("is_synchronized",
        [](daq::IMultiReader *object)
        {
            py::gil_scoped_release release;
            const auto objectPtr = daq::MultiReaderPtr::Borrow(object);
            return objectPtr.getIsSynchronized();
        },
        "Gets the synchronization status of the reader");
    cls.def_property_readonly("common_sample_rate",
        [](daq::IMultiReader *object)
        {
            py::gil_scoped_release release;
            const auto objectPtr = daq::MultiReaderPtr::Borrow(object);
            return objectPtr.getCommonSampleRate();
        },
        "Gets the common sample rate in case input signal have different rates. The value of common sample rate is such that sample rate of any individual signal can be represented as commonSampleRate / Div, where Div is an integer. Unless the required common sample rate is specified in the MultiReader constructor, common sample rate is lowest common multiple of individual signal's sample rates. The number of samples to be read is specified in common sample rate.");
    cls.def_property("active",
        [](daq::IMultiReader *object)
        {
            py::gil_scoped_release release;
            const auto objectPtr = daq::MultiReaderPtr::Borrow(object);
            return objectPtr.getActive();
        },
        [](daq::IMultiReader *object, const bool isActive)
        {
            py::gil_scoped_release release;
            const auto objectPtr = daq::MultiReaderPtr::Borrow(object);
            objectPtr.setActive(isActive);
        },
        "Gets / Sets active or inactive MultiReader state. In inactive state MultiReader will receive only event packets.");
}
