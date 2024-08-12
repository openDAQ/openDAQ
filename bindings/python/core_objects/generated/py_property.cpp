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

#include "py_core_objects/py_core_objects.h"
#include "py_core_types/py_converter.h"
#include "py_core_objects/py_variant_extractor.h"

PyDaqIntf<daq::IProperty, daq::IBaseObject> declareIProperty(pybind11::module_ m)
{
    return wrapInterface<daq::IProperty, daq::IBaseObject>(m, "IProperty");
}

void defineIProperty(pybind11::module_ m, PyDaqIntf<daq::IProperty, daq::IBaseObject> cls)
{
    cls.doc() = "Defines a set of metadata that describes the values held by a Property object stored under the key equal to the property's name.";

    m.def("BoolProperty", [](std::variant<daq::IString*, py::str, daq::IEvalValue*>& name, std::variant<daq::IBoolean*, bool, daq::IEvalValue*>& defaultValue, std::variant<daq::IBoolean*, bool, daq::IEvalValue*>& visible){
        return daq::BoolProperty_Create(getVariantValue<daq::IString*>(name), getVariantValue<daq::IBoolean*>(defaultValue), getVariantValue<daq::IBoolean*>(visible));
    }, py::arg("name"), py::arg("default_value"), py::arg("visible"));

    m.def("IntProperty", [](std::variant<daq::IString*, py::str, daq::IEvalValue*>& name, std::variant<daq::IInteger*, int64_t, daq::IEvalValue*>& defaultValue, std::variant<daq::IBoolean*, bool, daq::IEvalValue*>& visible){
        return daq::IntProperty_Create(getVariantValue<daq::IString*>(name), getVariantValue<daq::IInteger*>(defaultValue), getVariantValue<daq::IBoolean*>(visible));
    }, py::arg("name"), py::arg("default_value"), py::arg("visible"));

    m.def("FloatProperty", [](std::variant<daq::IString*, py::str, daq::IEvalValue*>& name, std::variant<daq::IFloat*, double, daq::IEvalValue*>& defaultValue, std::variant<daq::IBoolean*, bool, daq::IEvalValue*>& visible){
        return daq::FloatProperty_Create(getVariantValue<daq::IString*>(name), getVariantValue<daq::IFloat*>(defaultValue), getVariantValue<daq::IBoolean*>(visible));
    }, py::arg("name"), py::arg("default_value"), py::arg("visible"));

    m.def("StringProperty", [](std::variant<daq::IString*, py::str, daq::IEvalValue*>& name, std::variant<daq::IString*, py::str, daq::IEvalValue*>& defaultValue, std::variant<daq::IBoolean*, bool, daq::IEvalValue*>& visible){
        return daq::StringProperty_Create(getVariantValue<daq::IString*>(name), getVariantValue<daq::IString*>(defaultValue), getVariantValue<daq::IBoolean*>(visible));
    }, py::arg("name"), py::arg("default_value"), py::arg("visible"));

    m.def("ListProperty", [](std::variant<daq::IString*, py::str, daq::IEvalValue*>& name, std::variant<daq::IList*, py::list, daq::IEvalValue*>& defaultValue, std::variant<daq::IBoolean*, bool, daq::IEvalValue*>& visible){
        return daq::ListProperty_Create(getVariantValue<daq::IString*>(name), getVariantValue<daq::IList*>(defaultValue), getVariantValue<daq::IBoolean*>(visible));
    }, py::arg("name"), py::arg("default_value"), py::arg("visible"));

    m.def("DictProperty", [](std::variant<daq::IString*, py::str, daq::IEvalValue*>& name, std::variant<daq::IDict*, py::dict>& defaultValue, std::variant<daq::IBoolean*, bool, daq::IEvalValue*>& visible){
        return daq::DictProperty_Create(getVariantValue<daq::IString*>(name), getVariantValue<daq::IDict*>(defaultValue), getVariantValue<daq::IBoolean*>(visible));
    }, py::arg("name"), py::arg("default_value"), py::arg("visible"));

    m.def("RatioProperty", [](std::variant<daq::IString*, py::str, daq::IEvalValue*>& name, std::variant<daq::IRatio*, std::pair<int64_t, int64_t>>& defaultValue, std::variant<daq::IBoolean*, bool, daq::IEvalValue*>& visible){
        return daq::RatioProperty_Create(getVariantValue<daq::IString*>(name), getVariantValue<daq::IRatio*>(defaultValue), getVariantValue<daq::IBoolean*>(visible));
    }, py::arg("name"), py::arg("default_value"), py::arg("visible"));

    m.def("ObjectProperty", [](std::variant<daq::IString*, py::str, daq::IEvalValue*>& name, daq::IPropertyObject* defaultValue){
        return daq::ObjectProperty_Create(getVariantValue<daq::IString*>(name), defaultValue);
    }, py::arg("name"), py::arg("default_value"));

    m.def("ReferenceProperty", [](std::variant<daq::IString*, py::str, daq::IEvalValue*>& name, daq::IEvalValue* referencedPropertyEval){
        return daq::ReferenceProperty_Create(getVariantValue<daq::IString*>(name), referencedPropertyEval);
    }, py::arg("name"), py::arg("referenced_property_eval"));

    m.def("FunctionProperty", [](std::variant<daq::IString*, py::str, daq::IEvalValue*>& name, daq::ICallableInfo* callableInfo, std::variant<daq::IBoolean*, bool, daq::IEvalValue*>& visible){
        return daq::FunctionProperty_Create(getVariantValue<daq::IString*>(name), callableInfo, getVariantValue<daq::IBoolean*>(visible));
    }, py::arg("name"), py::arg("callable_info"), py::arg("visible"));

    m.def("SelectionProperty", [](std::variant<daq::IString*, py::str, daq::IEvalValue*>& name, std::variant<daq::IList*, py::list, daq::IEvalValue*>& selectionValues, std::variant<daq::IInteger*, int64_t, daq::IEvalValue*>& defaultValue, std::variant<daq::IBoolean*, bool, daq::IEvalValue*>& visible){
        return daq::SelectionProperty_Create(getVariantValue<daq::IString*>(name), getVariantValue<daq::IList*>(selectionValues), getVariantValue<daq::IInteger*>(defaultValue), getVariantValue<daq::IBoolean*>(visible));
    }, py::arg("name"), py::arg("selection_values"), py::arg("default_value"), py::arg("visible"));

    m.def("SparseSelectionProperty", [](std::variant<daq::IString*, py::str, daq::IEvalValue*>& name, std::variant<daq::IDict*, py::dict>& selectionValues, std::variant<daq::IInteger*, int64_t, daq::IEvalValue*>& defaultValue, std::variant<daq::IBoolean*, bool, daq::IEvalValue*>& visible){
        return daq::SparseSelectionProperty_Create(getVariantValue<daq::IString*>(name), getVariantValue<daq::IDict*>(selectionValues), getVariantValue<daq::IInteger*>(defaultValue), getVariantValue<daq::IBoolean*>(visible));
    }, py::arg("name"), py::arg("selection_values"), py::arg("default_value"), py::arg("visible"));

    m.def("StructProperty", [](std::variant<daq::IString*, py::str, daq::IEvalValue*>& name, daq::IStruct* defaultValue, std::variant<daq::IBoolean*, bool, daq::IEvalValue*>& visible){
        return daq::StructProperty_Create(getVariantValue<daq::IString*>(name), defaultValue, getVariantValue<daq::IBoolean*>(visible));
    }, py::arg("name"), py::arg("default_value"), py::arg("visible"));

    m.def("EnumerationProperty", [](std::variant<daq::IString*, py::str, daq::IEvalValue*>& name, daq::IEnumeration* defaultValue, std::variant<daq::IBoolean*, bool, daq::IEvalValue*>& visible){
        return daq::EnumerationProperty_Create(getVariantValue<daq::IString*>(name), defaultValue, getVariantValue<daq::IBoolean*>(visible));
    }, py::arg("name"), py::arg("default_value"), py::arg("visible"));


    cls.def_property_readonly("value_type",
        [](daq::IProperty *object)
        {
            const auto objectPtr = daq::PropertyPtr::Borrow(object);
            return objectPtr.getValueType();
        },
        "Gets the Value type of the Property. Values written to the corresponding Property value must be of the same type.");
    cls.def_property_readonly("key_type",
        [](daq::IProperty *object)
        {
            const auto objectPtr = daq::PropertyPtr::Borrow(object);
            return objectPtr.getKeyType();
        },
        "Gets the Key type of the Property. Configured only if the Value type is `ctDict`. If so, the key type of the dictionary Property values must match the Property's Key type.");
    cls.def_property_readonly("item_type",
        [](daq::IProperty *object)
        {
            const auto objectPtr = daq::PropertyPtr::Borrow(object);
            return objectPtr.getItemType();
        },
        "Gets the Item type of the Property. Configured only if the Value type is `ctDict` or `ctList`. If so, the item types of the list/dictionary must match the Property's Item type.");
    cls.def_property_readonly("name",
        [](daq::IProperty *object)
        {
            const auto objectPtr = daq::PropertyPtr::Borrow(object);
            return objectPtr.getName().toStdString();
        },
        "Gets the Name of the Property. The names of Properties in a Property object must be unique. The name is used as the key to the corresponding Property value when getting/setting the value.");
    cls.def_property_readonly("description",
        [](daq::IProperty *object)
        {
            const auto objectPtr = daq::PropertyPtr::Borrow(object);
            return objectPtr.getDescription().toStdString();
        },
        "Gets the short string Description of the Property.");
    cls.def_property_readonly("unit",
        [](daq::IProperty *object)
        {
            const auto objectPtr = daq::PropertyPtr::Borrow(object);
            return objectPtr.getUnit().detach();
        },
        py::return_value_policy::take_ownership,
        "Gets the Unit of the Property.");
    cls.def_property_readonly("min_value",
        [](daq::IProperty *object)
        {
            const auto objectPtr = daq::PropertyPtr::Borrow(object);
            return objectPtr.getMinValue().detach();
        },
        py::return_value_policy::take_ownership,
        "Gets the Minimum value of the Property. Available only if the Value type is `ctInt` or `ctFloat`.");
    cls.def_property_readonly("max_value",
        [](daq::IProperty *object)
        {
            const auto objectPtr = daq::PropertyPtr::Borrow(object);
            return objectPtr.getMaxValue().detach();
        },
        py::return_value_policy::take_ownership,
        "Gets the Maximum value of the Property. Available only if the Value type is `ctInt` or `ctFloat`.");
    cls.def_property_readonly("default_value",
        [](daq::IProperty *object)
        {
            const auto objectPtr = daq::PropertyPtr::Borrow(object);
            return baseObjectToPyObject(objectPtr.getDefaultValue());
        },
        py::return_value_policy::take_ownership,
        "Gets the Default value of the Property. The Default value must always be configured for a Property to be in a valid state. Exceptions are Function/Procedure and Reference properties.");
    cls.def_property_readonly("suggested_values",
        [](daq::IProperty *object)
        {
            const auto objectPtr = daq::PropertyPtr::Borrow(object);
            return objectPtr.getSuggestedValues().detach();
        },
        py::return_value_policy::take_ownership,
        "Gets the list of Suggested values. Contains values that are the optimal settings for the corresponding Property value. These values, however, are not enforced when setting a new Property value.");
    cls.def_property_readonly("visible",
        [](daq::IProperty *object)
        {
            const auto objectPtr = daq::PropertyPtr::Borrow(object);
            return objectPtr.getVisible();
        },
        "Used to determine whether the property is visible or not.");
    cls.def_property_readonly("read_only",
        [](daq::IProperty *object)
        {
            const auto objectPtr = daq::PropertyPtr::Borrow(object);
            return objectPtr.getReadOnly();
        },
        "Used to determine whether the Property is a read-only property or not.");
    cls.def_property_readonly("selection_values",
        [](daq::IProperty *object)
        {
            const auto objectPtr = daq::PropertyPtr::Borrow(object);
            return baseObjectToPyObject(objectPtr.getSelectionValues());
        },
        py::return_value_policy::take_ownership,
        "Gets the list or dictionary of selection values. If the list/dictionary is not empty, the property is a Selection property, and must have the Value type `ctInt`.");
    cls.def_property_readonly("referenced_property",
        [](daq::IProperty *object)
        {
            const auto objectPtr = daq::PropertyPtr::Borrow(object);
            return objectPtr.getReferencedProperty().detach();
        },
        py::return_value_policy::take_ownership,
        "Gets the referenced property. If set, all getters except for the `Name`, `Referenced property`, and `Is referenced` getters will return the value of the `Referenced property`.");
    cls.def_property_readonly("is_referenced",
        [](daq::IProperty *object)
        {
            const auto objectPtr = daq::PropertyPtr::Borrow(object);
            return objectPtr.getIsReferenced();
        },
        "Used to determine whether the Property is referenced by another property.");
    cls.def_property_readonly("validator",
        [](daq::IProperty *object)
        {
            const auto objectPtr = daq::PropertyPtr::Borrow(object);
            return objectPtr.getValidator().detach();
        },
        py::return_value_policy::take_ownership,
        "Gets the validator of the Property.");
    cls.def_property_readonly("coercer",
        [](daq::IProperty *object)
        {
            const auto objectPtr = daq::PropertyPtr::Borrow(object);
            return objectPtr.getCoercer().detach();
        },
        py::return_value_policy::take_ownership,
        "Gets the coercer of the Property.");
    cls.def_property_readonly("callable_info",
        [](daq::IProperty *object)
        {
            const auto objectPtr = daq::PropertyPtr::Borrow(object);
            return objectPtr.getCallableInfo().detach();
        },
        py::return_value_policy::take_ownership,
        "Gets the Callable information objects of the Property that specifies the argument and return types of the callable object stored as the Property value.");
    cls.def_property_readonly("struct_type",
        [](daq::IProperty *object)
        {
            const auto objectPtr = daq::PropertyPtr::Borrow(object);
            return objectPtr.getStructType().detach();
        },
        py::return_value_policy::take_ownership,
        "Gets the Struct type object of the Property, if the Property is a Struct property.");
    /*
    cls.def_property_readonly("on_property_value_write",
        [](daq::IProperty *object)
        {
            const auto objectPtr = daq::PropertyPtr::Borrow(object);
            return objectPtr.getOnPropertyValueWrite().detach();
        },
        py::return_value_policy::take_ownership,
        "Gets the event object that is triggered when a value is written to the corresponding Property value.");
    */
    /*
    cls.def_property_readonly("on_property_value_read",
        [](daq::IProperty *object)
        {
            const auto objectPtr = daq::PropertyPtr::Borrow(object);
            return objectPtr.getOnPropertyValueRead().detach();
        },
        py::return_value_policy::take_ownership,
        "Gets the event object that is triggered when the corresponding Property value is read.");
    */
    cls.def_property("value",
        [](daq::IProperty *object)
        {
            const auto objectPtr = daq::PropertyPtr::Borrow(object);
            return baseObjectToPyObject(objectPtr.getValue());
        },
        [](daq::IProperty *object, const py::object& value)
        {
            const auto objectPtr = daq::PropertyPtr::Borrow(object);
            objectPtr.setValue(pyObjectToBaseObject(value));
        },
        py::return_value_policy::take_ownership,
        "Gets the value of the Property. Available only if the Property is bound to a Property object. / Sets the value of the Property. Available only if the Property is bound to a Property object.");
}
