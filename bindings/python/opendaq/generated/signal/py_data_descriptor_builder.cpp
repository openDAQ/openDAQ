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

PyDaqIntf<daq::IDataDescriptorBuilder, daq::IBaseObject> declareIDataDescriptorBuilder(pybind11::module_ m)
{
    return wrapInterface<daq::IDataDescriptorBuilder, daq::IBaseObject>(m, "IDataDescriptorBuilder");
}

void defineIDataDescriptorBuilder(pybind11::module_ m, PyDaqIntf<daq::IDataDescriptorBuilder, daq::IBaseObject> cls)
{
    cls.doc() = "Builder component of Data descriptor objects. Contains setter methods that allow for Data descriptor parameter configuration, and a `build` method that builds the Data descriptor.";

    m.def("DataDescriptorBuilder", &daq::DataDescriptorBuilder_Create);
    m.def("DataDescriptorBuilderFromExisting", &daq::DataDescriptorBuilderFromExisting_Create);

    cls.def("build",
        [](daq::IDataDescriptorBuilder *object)
        {
            py::gil_scoped_release release;
            const auto objectPtr = daq::DataDescriptorBuilderPtr::Borrow(object);
            return objectPtr.build().detach();
        },
        "Builds and returns a Data descriptor object using the currently set values of the Builder.");
    cls.def_property("name",
        [](daq::IDataDescriptorBuilder *object)
        {
            py::gil_scoped_release release;
            const auto objectPtr = daq::DataDescriptorBuilderPtr::Borrow(object);
            return objectPtr.getName().toStdString();
        },
        [](daq::IDataDescriptorBuilder *object, std::variant<daq::IString*, py::str, daq::IEvalValue*>& name)
        {
            py::gil_scoped_release release;
            const auto objectPtr = daq::DataDescriptorBuilderPtr::Borrow(object);
            objectPtr.setName(getVariantValue<daq::IString*>(name));
        },
        "Gets a descriptive name for the signal's value. / Sets a descriptive name for the signal's value.");
    cls.def_property("dimensions",
        [](daq::IDataDescriptorBuilder *object)
        {
            py::gil_scoped_release release;
            const auto objectPtr = daq::DataDescriptorBuilderPtr::Borrow(object);
            return objectPtr.getDimensions().detach();
        },
        [](daq::IDataDescriptorBuilder *object, std::variant<daq::IList*, py::list, daq::IEvalValue*>& dimensions)
        {
            py::gil_scoped_release release;
            const auto objectPtr = daq::DataDescriptorBuilderPtr::Borrow(object);
            objectPtr.setDimensions(getVariantValue<daq::IList*>(dimensions));
        },
        py::return_value_policy::take_ownership,
        "Gets the list of the descriptor's dimension's. / Sets the list of the descriptor's dimension's.");
    cls.def_property("sample_type",
        [](daq::IDataDescriptorBuilder *object)
        {
            py::gil_scoped_release release;
            const auto objectPtr = daq::DataDescriptorBuilderPtr::Borrow(object);
            return objectPtr.getSampleType();
        },
        [](daq::IDataDescriptorBuilder *object, daq::SampleType sampleType)
        {
            py::gil_scoped_release release;
            const auto objectPtr = daq::DataDescriptorBuilderPtr::Borrow(object);
            objectPtr.setSampleType(sampleType);
        },
        "Gets the descriptor's sample type. / Sets the descriptor's sample type.");
    cls.def_property("unit",
        [](daq::IDataDescriptorBuilder *object)
        {
            py::gil_scoped_release release;
            const auto objectPtr = daq::DataDescriptorBuilderPtr::Borrow(object);
            return objectPtr.getUnit().detach();
        },
        [](daq::IDataDescriptorBuilder *object, daq::IUnit* unit)
        {
            py::gil_scoped_release release;
            const auto objectPtr = daq::DataDescriptorBuilderPtr::Borrow(object);
            objectPtr.setUnit(unit);
        },
        py::return_value_policy::take_ownership,
        "Gets the unit of the data in a signal's packets. / Sets the unit of the data in a signal's packets.");
    cls.def_property("value_range",
        [](daq::IDataDescriptorBuilder *object)
        {
            py::gil_scoped_release release;
            const auto objectPtr = daq::DataDescriptorBuilderPtr::Borrow(object);
            return objectPtr.getValueRange().detach();
        },
        [](daq::IDataDescriptorBuilder *object, daq::IRange* range)
        {
            py::gil_scoped_release release;
            const auto objectPtr = daq::DataDescriptorBuilderPtr::Borrow(object);
            objectPtr.setValueRange(range);
        },
        py::return_value_policy::take_ownership,
        "Gets the value range of the data in a signal's packets defining the lowest and highest expected values. / Sets the value range of the data in a signal's packets defining the lowest and highest expected values.");
    cls.def_property("rule",
        [](daq::IDataDescriptorBuilder *object)
        {
            py::gil_scoped_release release;
            const auto objectPtr = daq::DataDescriptorBuilderPtr::Borrow(object);
            return objectPtr.getRule().detach();
        },
        [](daq::IDataDescriptorBuilder *object, daq::IDataRule* rule)
        {
            py::gil_scoped_release release;
            const auto objectPtr = daq::DataDescriptorBuilderPtr::Borrow(object);
            objectPtr.setRule(rule);
        },
        py::return_value_policy::take_ownership,
        "Gets the value Data rule. / Sets the value Data rule.");
    cls.def_property("origin",
        [](daq::IDataDescriptorBuilder *object)
        {
            py::gil_scoped_release release;
            const auto objectPtr = daq::DataDescriptorBuilderPtr::Borrow(object);
            return objectPtr.getOrigin().toStdString();
        },
        [](daq::IDataDescriptorBuilder *object, std::variant<daq::IString*, py::str, daq::IEvalValue*>& origin)
        {
            py::gil_scoped_release release;
            const auto objectPtr = daq::DataDescriptorBuilderPtr::Borrow(object);
            objectPtr.setOrigin(getVariantValue<daq::IString*>(origin));
        },
        "Gets the absolute origin of a signal value component. / Sets the absolute origin of a signal value component.");
    cls.def_property("tick_resolution",
        [](daq::IDataDescriptorBuilder *object)
        {
            py::gil_scoped_release release;
            const auto objectPtr = daq::DataDescriptorBuilderPtr::Borrow(object);
            return objectPtr.getTickResolution().detach();
        },
        [](daq::IDataDescriptorBuilder *object, std::variant<daq::IRatio*, std::pair<int64_t, int64_t>>& tickResolution)
        {
            py::gil_scoped_release release;
            const auto objectPtr = daq::DataDescriptorBuilderPtr::Borrow(object);
            objectPtr.setTickResolution(getVariantValue<daq::IRatio*>(tickResolution));
        },
        py::return_value_policy::take_ownership,
        "Gets the Resolution which scales the an explicit or implicit value to the physical unit defined in `unit`. / Sets the Resolution which scales the an explicit or implicit value to the physical unit defined in `unit`.");
    cls.def_property("post_scaling",
        [](daq::IDataDescriptorBuilder *object)
        {
            py::gil_scoped_release release;
            const auto objectPtr = daq::DataDescriptorBuilderPtr::Borrow(object);
            return objectPtr.getPostScaling().detach();
        },
        [](daq::IDataDescriptorBuilder *object, daq::IScaling* scaling)
        {
            py::gil_scoped_release release;
            const auto objectPtr = daq::DataDescriptorBuilderPtr::Borrow(object);
            objectPtr.setPostScaling(scaling);
        },
        py::return_value_policy::take_ownership,
        "Gets the scaling rule that needs to be applied to explicit/implicit data by readers. / Sets the scaling rule that needs to be applied to explicit/implicit data by readers.");
    cls.def_property("struct_fields",
        [](daq::IDataDescriptorBuilder *object)
        {
            py::gil_scoped_release release;
            const auto objectPtr = daq::DataDescriptorBuilderPtr::Borrow(object);
            return objectPtr.getStructFields().detach();
        },
        [](daq::IDataDescriptorBuilder *object, std::variant<daq::IList*, py::list, daq::IEvalValue*>& structFields)
        {
            py::gil_scoped_release release;
            const auto objectPtr = daq::DataDescriptorBuilderPtr::Borrow(object);
            objectPtr.setStructFields(getVariantValue<daq::IList*>(structFields));
        },
        py::return_value_policy::take_ownership,
        "Gets the fields of the struct, forming a recursive value descriptor definition. / Sets the fields of the struct, forming a recursive value descriptor definition.");
    cls.def_property("metadata",
        [](daq::IDataDescriptorBuilder *object)
        {
            py::gil_scoped_release release;
            const auto objectPtr = daq::DataDescriptorBuilderPtr::Borrow(object);
            return objectPtr.getMetadata().detach();
        },
        [](daq::IDataDescriptorBuilder *object, std::variant<daq::IDict*, py::dict>& metadata)
        {
            py::gil_scoped_release release;
            const auto objectPtr = daq::DataDescriptorBuilderPtr::Borrow(object);
            objectPtr.setMetadata(getVariantValue<daq::IDict*>(metadata));
        },
        py::return_value_policy::take_ownership,
        "Gets any extra metadata defined by the data descriptor. / Sets any extra metadata defined by the data descriptor.");
    cls.def_property("reference_domain_info",
        [](daq::IDataDescriptorBuilder *object)
        {
            py::gil_scoped_release release;
            const auto objectPtr = daq::DataDescriptorBuilderPtr::Borrow(object);
            return objectPtr.getReferenceDomainInfo().detach();
        },
        [](daq::IDataDescriptorBuilder *object, daq::IReferenceDomainInfo* referenceDomainInfo)
        {
            py::gil_scoped_release release;
            const auto objectPtr = daq::DataDescriptorBuilderPtr::Borrow(object);
            objectPtr.setReferenceDomainInfo(referenceDomainInfo);
        },
        py::return_value_policy::take_ownership,
        "Gets the Reference Domain Info. / Sets the Reference Domain Info.");
}
