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
 * Copyright 2022-2023 Blueberry d.o.o.
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

PyDaqIntf<daq::IMultiReaderBuilder, daq::IBaseObject> declareIMultiReaderBuilder(pybind11::module_ m)
{
    return wrapInterface<daq::IMultiReaderBuilder, daq::IBaseObject>(m, "IMultiReaderBuilder");
}

void defineIMultiReaderBuilder(pybind11::module_ m, PyDaqIntf<daq::IMultiReaderBuilder, daq::IBaseObject> cls)
{
    cls.doc() = "Builder component of Multi reader objects. Contains setter methods to configure the Multi reader parameters and a `build` method that builds the Unit object.";

    m.def("MultiReaderBuilder", &daq::MultiReaderBuilder_Create);

    cls.def("build",
        [](daq::IMultiReaderBuilder *object)
        {
            const auto objectPtr = daq::MultiReaderBuilderPtr::Borrow(object);
            return objectPtr.build().detach();
        },
        "Builds and returns a Multi reader object using the currently set values of the Builder.");
    cls.def("add_signal",
        [](daq::IMultiReaderBuilder *object, daq::ISignal* signal)
        {
            const auto objectPtr = daq::MultiReaderBuilderPtr::Borrow(object);
            objectPtr.addSignal(signal);
        },
        py::arg("signal"),
        "Adds the signal to list in multi reader");
    cls.def("add_input_port",
        [](daq::IMultiReaderBuilder *object, daq::IInputPort* port)
        {
            const auto objectPtr = daq::MultiReaderBuilderPtr::Borrow(object);
            objectPtr.addInputPort(port);
        },
        py::arg("port"),
        "Adds the input port to list in multi reader");
    cls.def_property_readonly("input_port_list",
        [](daq::IMultiReaderBuilder *object)
        {
            const auto objectPtr = daq::MultiReaderBuilderPtr::Borrow(object);
            return objectPtr.getInputPortList().detach();
        },
        py::return_value_policy::take_ownership,
        "Gets the list of input ports");
    cls.def_property("value_read_type",
        [](daq::IMultiReaderBuilder *object)
        {
            const auto objectPtr = daq::MultiReaderBuilderPtr::Borrow(object);
            return objectPtr.getValueReadType();
        },
        [](daq::IMultiReaderBuilder *object, daq::SampleType type)
        {
            const auto objectPtr = daq::MultiReaderBuilderPtr::Borrow(object);
            objectPtr.setValueReadType(type);
        },
        "Gets the value signal read type / Sets the value signal read type");
    cls.def_property("domain_read_type",
        [](daq::IMultiReaderBuilder *object)
        {
            const auto objectPtr = daq::MultiReaderBuilderPtr::Borrow(object);
            return objectPtr.getDomainReadType();
        },
        [](daq::IMultiReaderBuilder *object, daq::SampleType type)
        {
            const auto objectPtr = daq::MultiReaderBuilderPtr::Borrow(object);
            objectPtr.setDomainReadType(type);
        },
        "Gets the domain signal read type / Sets the domain signal read type");
    cls.def_property("read_mode",
        [](daq::IMultiReaderBuilder *object)
        {
            const auto objectPtr = daq::MultiReaderBuilderPtr::Borrow(object);
            return objectPtr.getReadMode();
        },
        [](daq::IMultiReaderBuilder *object, daq::ReadMode mode)
        {
            const auto objectPtr = daq::MultiReaderBuilderPtr::Borrow(object);
            objectPtr.setReadMode(mode);
        },
        "Gets the read mode (Unscaled, Scaled, RawValue) / Sets the read mode (Unscaled, Scaled, RawValue)");
    cls.def_property("read_timeout_type",
        [](daq::IMultiReaderBuilder *object)
        {
            const auto objectPtr = daq::MultiReaderBuilderPtr::Borrow(object);
            return objectPtr.getReadTimeoutType();
        },
        [](daq::IMultiReaderBuilder *object, daq::ReadTimeoutType type)
        {
            const auto objectPtr = daq::MultiReaderBuilderPtr::Borrow(object);
            objectPtr.setReadTimeoutType(type);
        },
        "Gets the read timeout mode / Sets the read timeout mode");
    cls.def_property("required_common_sample_rate",
        [](daq::IMultiReaderBuilder *object)
        {
            const auto objectPtr = daq::MultiReaderBuilderPtr::Borrow(object);
            return objectPtr.getRequiredCommonSampleRate();
        },
        [](daq::IMultiReaderBuilder *object, daq::Int sampleRate)
        {
            const auto objectPtr = daq::MultiReaderBuilderPtr::Borrow(object);
            objectPtr.setRequiredCommonSampleRate(sampleRate);
        },
        "Gets the required common sample rate / Sets the required common sample rate");
    cls.def_property("start_on_full_unit_of_domain",
        [](daq::IMultiReaderBuilder *object)
        {
            const auto objectPtr = daq::MultiReaderBuilderPtr::Borrow(object);
            return objectPtr.getStartOnFullUnitOfDomain();
        },
        [](daq::IMultiReaderBuilder *object, const bool enabled)
        {
            const auto objectPtr = daq::MultiReaderBuilderPtr::Borrow(object);
            objectPtr.setStartOnFullUnitOfDomain(enabled);
        },
        "Gets the start on full unit of domain / Sets the start on full unit of domain");
}
