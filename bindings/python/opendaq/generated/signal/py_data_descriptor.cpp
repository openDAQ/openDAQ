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

#include "py_opendaq/py_opendaq.h"
#include "py_core_types/py_converter.h"


PyDaqIntf<daq::IDataDescriptor, daq::IBaseObject> declareIDataDescriptor(pybind11::module_ m)
{
    return wrapInterface<daq::IDataDescriptor, daq::IBaseObject>(m, "IDataDescriptor");
}

void defineIDataDescriptor(pybind11::module_ m, PyDaqIntf<daq::IDataDescriptor, daq::IBaseObject> cls)
{
    cls.doc() = "Describes the data sent by a signal, defining how they are to be interpreted by anyone receiving the signal's packets.";

    m.def("DataDescriptorFromBuilder", &daq::DataDescriptorFromBuilder_Create);

    cls.def_property_readonly("name",
        [](daq::IDataDescriptor *object)
        {
            const auto objectPtr = daq::DataDescriptorPtr::Borrow(object);
            return objectPtr.getName().toStdString();
        },
        "Gets a descriptive name of the signal value.");
    cls.def_property_readonly("dimensions",
        [](daq::IDataDescriptor *object)
        {
            const auto objectPtr = daq::DataDescriptorPtr::Borrow(object);
            return objectPtr.getDimensions().detach();
        },
        py::return_value_policy::take_ownership,
        "Gets the list of the descriptor's dimension's.");
    cls.def_property_readonly("sample_type",
        [](daq::IDataDescriptor *object)
        {
            const auto objectPtr = daq::DataDescriptorPtr::Borrow(object);
            return objectPtr.getSampleType();
        },
        "Gets the descriptor's sample type.");
    cls.def_property_readonly("unit",
        [](daq::IDataDescriptor *object)
        {
            const auto objectPtr = daq::DataDescriptorPtr::Borrow(object);
            return objectPtr.getUnit().detach();
        },
        py::return_value_policy::take_ownership,
        "Gets the unit of the data in a signal's packets.");
    cls.def_property_readonly("value_range",
        [](daq::IDataDescriptor *object)
        {
            const auto objectPtr = daq::DataDescriptorPtr::Borrow(object);
            return objectPtr.getValueRange().detach();
        },
        py::return_value_policy::take_ownership,
        "Gets the value range of the data in a signal's packets defining the lowest and highest expected values.");
    cls.def_property_readonly("rule",
        [](daq::IDataDescriptor *object)
        {
            const auto objectPtr = daq::DataDescriptorPtr::Borrow(object);
            return objectPtr.getRule().detach();
        },
        py::return_value_policy::take_ownership,
        "Gets the value Data rule.");
    cls.def_property_readonly("origin",
        [](daq::IDataDescriptor *object)
        {
            const auto objectPtr = daq::DataDescriptorPtr::Borrow(object);
            return objectPtr.getOrigin().toStdString();
        },
        "Gets the absolute origin of a signal value component.");
    cls.def_property_readonly("tick_resolution",
        [](daq::IDataDescriptor *object)
        {
            const auto objectPtr = daq::DataDescriptorPtr::Borrow(object);
            return objectPtr.getTickResolution().detach();
        },
        py::return_value_policy::take_ownership,
        "Gets the Resolution which scales the explicit or implicit value to the physical unit defined in `unit`. It is defined as domain (usually time) between two consecutive ticks.");
    cls.def_property_readonly("post_scaling",
        [](daq::IDataDescriptor *object)
        {
            const auto objectPtr = daq::DataDescriptorPtr::Borrow(object);
            return objectPtr.getPostScaling().detach();
        },
        py::return_value_policy::take_ownership,
        "");
    cls.def_property_readonly("struct_fields",
        [](daq::IDataDescriptor *object)
        {
            const auto objectPtr = daq::DataDescriptorPtr::Borrow(object);
            return objectPtr.getStructFields().detach();
        },
        py::return_value_policy::take_ownership,
        "Gets the fields of the struct, forming a recursive value descriptor definition.");
    cls.def_property_readonly("metadata",
        [](daq::IDataDescriptor *object)
        {
            const auto objectPtr = daq::DataDescriptorPtr::Borrow(object);
            return objectPtr.getMetadata().detach();
        },
        py::return_value_policy::take_ownership,
        "Gets any extra metadata defined by the data descriptor.");
    cls.def_property_readonly("sample_size",
        [](daq::IDataDescriptor *object)
        {
            const auto objectPtr = daq::DataDescriptorPtr::Borrow(object);
            return objectPtr.getSampleSize();
        },
        "Gets the size of one sample in bytes.");
    cls.def_property_readonly("raw_sample_size",
        [](daq::IDataDescriptor *object)
        {
            const auto objectPtr = daq::DataDescriptorPtr::Borrow(object);
            return objectPtr.getRawSampleSize();
        },
        "Gets the actual sample size in buffer of one sample in bytes.");
    cls.def_property_readonly("reference_domain_id",
        [](daq::IDataDescriptor *object)
        {
            const auto objectPtr = daq::DataDescriptorPtr::Borrow(object);
            return objectPtr.getReferenceDomainId().toStdString();
        },
        "Gets the reference domain id.");
    cls.def_property_readonly("reference_domain_offset",
        [](daq::IDataDescriptor *object)
        {
            const auto objectPtr = daq::DataDescriptorPtr::Borrow(object);
            return objectPtr.getReferenceDomainOffset().detach();
        },
        py::return_value_policy::take_ownership,
        "Gets the reference domain offset.");
}
