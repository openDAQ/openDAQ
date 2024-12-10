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

#pragma once
#include <coreobjects/property_ptr.h>
#include <coreobjects/property_builder_ptr.h>
#include <coreobjects/property_object_ptr.h>

BEGIN_NAMESPACE_OPENDAQ

/*!
 * @ingroup objects_property_obj
 * @addtogroup objects_property_obj_factories Factories
 * @{
 */

/*!
 * @brief Creates an Property builder object with only the name field configured.
 *
 * The default Value type is `ctUndefined`.
 */
inline PropertyBuilderPtr PropertyBuilder(const StringPtr& name)
{
    PropertyBuilderPtr obj(PropertyBuilder_Create(name));
    return obj;
}

/*!
 * @brief Creates a boolean Property object with a default value and optional Visible state.
 * @param name The name of the Property.
 * @param defaultValue The boolean default value. Can be an EvalValue.
 * @param visible If true, the Property is visible. Can be an EvalValue.
 *
 * The Property Value type is `ctBool`. Note that the defaultValue and visible parameters can be EvalValues.
 */
inline PropertyPtr BoolProperty(const StringPtr& name, const BooleanPtr& defaultValue, const BooleanPtr& visible = true)
{
    PropertyPtr obj(BoolProperty_Create(name, defaultValue, visible));
    return obj;
}

/*!
 * @brief Creates a boolean Property builder object with a specified name and default value.
 * @param name The name of the Property.
 * @param defaultValue The boolean default value. Can be an EvalValue.
 *
 * The Property Value type is `ctBool`. Note that the defaultValue parameter can be EvalValue.
 */
inline PropertyBuilderPtr BoolPropertyBuilder(const StringPtr& name, const BooleanPtr& defaultValue)
{
    PropertyBuilderPtr obj(BoolPropertyBuilder_Create(name, defaultValue));
    return obj;
}

/*!
 * @brief Creates an integer Property object with a default value and optional Visible state.
 * @param name The name of the Property.
 * @param defaultValue The integer default value. Can be an EvalValue.
 * @param visible If true, the Property is visible. Can be an EvalValue.
 *
 * The Property Value type is `ctInt`. Note that the defaultValue and visible parameters can be EvalValues.
 */
inline PropertyPtr IntProperty(const StringPtr& name, const IntegerPtr& defaultValue, const BooleanPtr& visible = true)
{
    PropertyPtr obj(IntProperty_Create(name, defaultValue, visible));
    return obj;
}

/*!
 * @brief Creates an integer Property builder object with a specified name and default value.
 * @param name The name of the Property.
 * @param defaultValue The integer default value. Can be an EvalValue.
 *
 * The Property Value type is `ctInt`. Note that the defaultValue parameter can be EvalValue.
 */
inline PropertyBuilderPtr IntPropertyBuilder(const StringPtr& name, const IntegerPtr& defaultValue)
{
    PropertyBuilderPtr obj(IntPropertyBuilder_Create(name, defaultValue));
    return obj;
}

/*!
 * @brief Creates a floating point value Property object with a default value and optional Visible state.
 * @param name The name of the Property.
 * @param defaultValue The float default value. Can be an EvalValue.
 * @param visible If true, the Property is visible. Can be an EvalValue.
 *
 * The Property Value type is `ctFloat`. Note that the defaultValue and visible parameters can be EvalValues.
 */
inline PropertyPtr FloatProperty(const StringPtr& name, const FloatPtr& defaultValue, const BooleanPtr& visible = true)
{
    PropertyPtr obj(FloatProperty_Create(name, defaultValue, visible));
    return obj;
}

/*!
 * @brief Creates a floating point value Property builder object with a specified name and default value.
 * @param name The name of the Property.
 * @param defaultValue The float default value. Can be an EvalValue
 *
 * The Property Value type is `ctFloat`. Note that the defaultValue parameter can be EvalValue.
 */
inline PropertyBuilderPtr FloatPropertyBuilder(const StringPtr& name, const FloatPtr& defaultValue)
{
    PropertyBuilderPtr obj(FloatPropertyBuilder_Create(name, defaultValue));
    return obj;
}

/*!
 * @brief Creates a string Property object with a default value and optional Visible state.
 * @param name The name of the Property.
 * @param defaultValue The integer default value. Can be an EvalValue.
 * @param visible If true, the Property is visible. Can be an EvalValue.
 *
 * The Property Value type is `ctString`. Note that the defaultValue and visible parameters can be EvalValues.
 */
inline PropertyPtr StringProperty(const StringPtr& name, const StringPtr& defaultValue, const BooleanPtr& visible = true)
{
    PropertyPtr obj(StringProperty_Create(name, defaultValue, visible));
    return obj;
}

/*!
 * @brief Creates a string Property builder object with a specified name and default value.
 * @param name The name of the Property.
 * @param defaultValue The integer default value. Can be an EvalValue.
 *
 * The Property Value type is `ctString`. Note that the defaultValue parameter can be EvalValue.
 */
inline PropertyBuilderPtr StringPropertyBuilder(const StringPtr& name, const StringPtr& defaultValue)
{
    PropertyBuilderPtr obj(StringPropertyBuilder_Create(name, defaultValue));
    return obj;
}

/*!
 * @brief Creates a list Property object with a default value and optional Visible state.
 * @param name The name of the Property.
 * @param defaultValue The list default value. Can be an EvalValue.
 * @param visible If true, the Property is visible. Can be an EvalValue.
 *
 * The Property Value type is `ctList`. Note that the defaultValue and visible parameters can be EvalValues.
 * The list passed as `defaultValue` must be homogeneous.
 *
 * The Property's Item type field will be set according to defaultValue list type.
 */
inline PropertyPtr ListProperty(const StringPtr& name,
                                      const ListPtr<IBaseObject>& defaultValue,
                                      const BooleanPtr& visible = true)
{
    PropertyPtr obj(ListProperty_Create(name, defaultValue, visible));
    return obj;
}

/*!
 * @brief Creates a list Property builder object with a specified name and default value.
 * @param name The name of the Property.
 * @param defaultValue The list default value. Can be an EvalValue.
 *
 * The Property Value type is `ctList`. Note that the defaultValue parameter can be EvalValue.
 * The list passed as `defaultValue` must be homogeneous.
 *
 * The Property's Item type field will be set according to defaultValue list type.
 */
inline PropertyBuilderPtr ListPropertyBuilder(const StringPtr& name,
                                      const ListPtr<IBaseObject>& defaultValue)
{
    PropertyBuilderPtr obj(ListPropertyBuilder_Create(name, defaultValue));
    return obj;
}

/*!
 * @brief Creates a dictionary Property object with a default value and optional Visible state.
 * @param name The name of the Property.
 * @param defaultValue The dictionary default value.
 * @param visible If true, the Property is visible.  Can be an EvalValue.
 *
 * The Property Value type is `ctDict`. The visible parameter can be an EvalValue. The dictionary passed as
 * default value must have homogeneous key and value lists.
 * 
 * The Property's Item type field will be set according to defaultValue dictionary Item type. The same goes for
 * the Key type.
 *
 * TODO: defaultValue can be an EvalValue once dictionaries are supported.
 */
inline PropertyPtr DictProperty(const StringPtr& name,
                                      const DictPtr<IBaseObject, IBaseObject>& defaultValue,
                                      const BooleanPtr& visible = true)
{
    PropertyPtr obj(DictProperty_Create(name, defaultValue, visible));
    return obj;
}

/*!
 * @brief Creates a dictionary Property builder object with a specified name and default value.
 * @param name The name of the Property.
 * @param defaultValue The dictionary default value.
 *
 * The Property Value type is `ctDict`. The dictionary passed as default value must have homogeneous key
 * and value lists (all dictionary keys/values must be of the same type).
 *
 * The Property's Item type field will be set according to defaultValue dictionary Item type. The same goes for
 * the Key type.
 *
 * TODO: defaultValue can be an EvalValue once dictionaries are supported.
 */
inline PropertyBuilderPtr DictPropertyBuilder(const StringPtr& name,
                                      const DictPtr<IBaseObject, IBaseObject>& defaultValue)
{
    PropertyBuilderPtr obj(DictPropertyBuilder_Create(name, defaultValue));
    return obj;
}

/*!
 * @brief Creates a ratio Property object with a default value and optional Visible state.
 * @param name The name of the Property.
 * @param defaultValue The ratio default value.
 * @param visible If true, the Property is visible. Can be an EvalValue.
 *
 * The Property Value type is `ctRatio`. Note that the visible parameter can be an EvalValue.
 *
 * TODO: defaultValue can be an EvalValue once ratios are supported.
 */
inline PropertyPtr RatioProperty(const StringPtr& name, const RatioPtr& defaultValue, const BooleanPtr& visible = true)
{
    PropertyPtr obj(RatioProperty_Create(name, defaultValue, visible));
    return obj;
}

/*!
 * @brief Creates a ratio Property builder object with a specified name and default value.
 * @param name The name of the Property.
 * @param defaultValue The ratio default value.
 *
 * The Property Value type is `ctRatio`.
 *
 * TODO: defaultValue can be an EvalValue once ratios are supported.
 */
inline PropertyBuilderPtr RatioPropertyBuilder(const StringPtr& name, const RatioPtr& defaultValue)
{
    PropertyBuilderPtr obj(RatioPropertyBuilder_Create(name, defaultValue));
    return obj;
}

/*!
 * @brief Creates an object-type config object with a default value.
 * @param name The name of the Property.
 * @param defaultValue The Property object default value.
 *
 * The Property Value type is `ctObject`. Object properties cannot be have any metadata other than
 * their name, description, and default value configured. The PropertyObject default value can only
 * be a base PropertyObject type (not a descendant type).
 *
 * If the defaultValue is not specified, it will automatically be configured to an empty Property Object.
 */
inline PropertyPtr ObjectProperty(const StringPtr& name, const PropertyObjectPtr& defaultValue = nullptr)
{
    PropertyPtr obj(ObjectProperty_Create(name, defaultValue));
    return obj;
}

/*!
 * @brief Creates an object-type Property builder object with a specified name and default value..
 * @param name The name of the Property.
 * @param defaultValue The Property object default value.
 *
 * The Property Value type is `ctObject`. Object properties cannot be have any metadata other than
 * their name, description, and default value configured. The PropertyObject default value can only
 * be a base PropertyObject type (not a descendant type).
 *
 * If the defaultValue is not specified, it will automatically be configured to an empty Property Object.
 */
inline PropertyBuilderPtr ObjectPropertyBuilder(const StringPtr& name, const PropertyObjectPtr& defaultValue = nullptr)
{
    PropertyBuilderPtr obj(ObjectPropertyBuilder_Create(name, defaultValue));
    return obj;
}

/*!
 * @brief Creates a function- or procedure-type Property object. Requires the a CallableInfo object
 * to specify the argument type/count and function return type.
 * @param name The name of the Property.
 * @param callableInfo Information about the callable argument type/count and return type.
 * @param visible If true, the Property is visible. Can be an EvalValue.
 *
 * The Property Value type is `ctFunction` or `ctProc`, depending on if `callableInfo` contains information
 * on the return type or not. Note that the visible parameter can be an EvalValue.
 */
inline PropertyPtr FunctionProperty(const StringPtr& name,
                                          const CallableInfoPtr& callableInfo,
                                          const BooleanPtr& visible = true)
{
    PropertyPtr obj(FunctionProperty_Create(name, callableInfo, visible));
    return obj;
}

/*!
 * @brief Creates a function- or procedure-type Property builder object. Requires the a CallableInfo object
 * to specify the argument type/count and function return type.
 * @param name The name of the Property.
 * @param callableInfo Information about the callable argument type/count and return type.
 *
 * The Property Value type is `ctFunction` or `ctProc`, depending on if `callableInfo` contains information
 * on the return type or not.
 */
inline PropertyBuilderPtr FunctionPropertyBuilder(const StringPtr& name,
                                                  const CallableInfoPtr& callableInfo)
{
    PropertyBuilderPtr obj(FunctionPropertyBuilder_Create(name, callableInfo));
    return obj;
}

/*!
 * @brief Creates a Reference Property object that points at a property specified in the `referencedProperty`
 * parameter.
 * @param name The name of the Property.
 * @param referencedPropertyEval The evaluation expression that evaluates to another property.
 */
inline PropertyPtr ReferenceProperty(const StringPtr& name, const EvalValuePtr& referencedPropertyEval)
{
    PropertyPtr obj(ReferenceProperty_Create(name, referencedPropertyEval));
    return obj;
}

/*!
 * @brief Creates a Reference Property builder object that points at a property specified in the `referencedProperty`
 * parameter.
 * @param name The name of the Property.
 * @param referencedPropertyEval The evaluation expression that evaluates to another property.
 */
inline PropertyBuilderPtr ReferencePropertyBuilder(const StringPtr& name, const EvalValuePtr& referencedPropertyEval)
{
    PropertyBuilderPtr obj(ReferencePropertyBuilder_Create(name, referencedPropertyEval));
    return obj;
}

/*!
 * @brief Creates a Selection Property object with a list of selection values. The default value
 * is an integer index into the default selected value.
 * @param name The name of the Property.
 * @param selectionValues The list of selectable values.
 * @param defaultValue The default index into the list of selection values.
 * @param visible If true, the Property is visible. Can be an EvalValue.
 *
 * The Property Value type is `ctInt`.
 */
inline PropertyPtr SelectionProperty(const StringPtr& name,
                                           const ListPtr<IBaseObject>& selectionValues,
                                           const IntegerPtr& defaultValue,
                                           const BooleanPtr& visible = true)
{
    PropertyPtr obj(SelectionProperty_Create(name, selectionValues, defaultValue, visible));
    return obj;
}

/*!
 * @brief Creates a Selection Property builder object with a list of selection values. The default value
 * is an integer index into the default selected value.
 * @param name The name of the Property.
 * @param selectionValues The list of selectable values.
 * @param defaultValue The default index into the list of selection values.
 *
 * The Property Value type is `ctInt`.
 */
inline PropertyBuilderPtr SelectionPropertyBuilder(const StringPtr& name,
                                           const ListPtr<IBaseObject>& selectionValues,
                                           const IntegerPtr& defaultValue)
{
    PropertyBuilderPtr obj(SelectionPropertyBuilder_Create(name, selectionValues, defaultValue));
    return obj;
}

/*!
 * @brief Creates a Selection Property object with a dictionary of selection values. The default value
 * is an integer key into the provided dictionary.
 * @param name The name of the Property.
 * @param selectionValues The dictionary of selectable values. The key type must be `ctInt`.
 * @param defaultValue The default key into the list of selection values.
 * @param visible If true, the Property is visible. Can be an EvalValue.
 *
 * The Property Value type is `ctInt`. The key type of the Selection values dictionary must be `ctInt`.
 */
inline PropertyPtr SparseSelectionProperty(const StringPtr& name,
                                                 const DictPtr<Int, IBaseObject>& selectionValues,
                                                 const IntegerPtr& defaultValue,
                                                 const BooleanPtr& visible = true)
{
    PropertyPtr obj(SparseSelectionProperty_Create(name, selectionValues, defaultValue, visible));
    return obj;
}

/*!
 * @brief Creates a Selection Property builder object with a dictionary of selection values. The default value
 * is an integer key into the provided dictionary.
 * @param name The name of the Property.
 * @param selectionValues The dictionary of selectable values. The key type must be `ctInt`.
 * @param defaultValue The default key into the list of selection values.
 *
 * The Property Value type is `ctInt`. The key type of the Selection values dictionary must be `ctInt`.
 */
inline PropertyBuilderPtr SparseSelectionPropertyBuilder(const StringPtr& name,
                                                 const DictPtr<Int, IBaseObject>& selectionValues,
                                                 const IntegerPtr& defaultValue)
{
    PropertyBuilderPtr obj(SparseSelectionPropertyBuilder_Create(name, selectionValues, defaultValue));
    return obj;
}

/*!
 * @brief Creates a Struct Property object with a default value and its visible state.
 * @param name The name of the Property.
 * @param defaultValue The default structure value.
 * @param visible If true, the Property is visible. Can be an EvalValue.
 *
 * The Property Value type is `ctStruct`.
 */
inline PropertyPtr StructProperty(const StringPtr& name, const StructPtr& defaultValue, const BooleanPtr& visible = true)
{
    PropertyPtr obj(StructProperty_Create(name, defaultValue, visible));
    return obj;
}

/*!
 * @brief Creates a Struct Property builder object with a specified name and default value.
 * @param name The name of the Property.
 * @param defaultValue The default structure value.
 *
 * The Property Value type is `ctStruct`.
 */
inline PropertyBuilderPtr StructPropertyBuilder(const StringPtr& name, const StructPtr& defaultValue)
{
    PropertyBuilderPtr obj(StructPropertyBuilder_Create(name, defaultValue));
    return obj;
}

/*!
 * @brief Creates an Enumeration Property object with a default value and its visible state.
 * @param name The name of the Property.
 * @param defaultValue The default enumeration value.
 * @param visible If true, the Property is visible. Can be an EvalValue.
 *
 * The Property Value type is `ctEnumeration`.
 */
inline PropertyPtr EnumerationProperty(const StringPtr& name, const EnumerationPtr& defaultValue, const BooleanPtr& visible = true)
{
    PropertyPtr obj(EnumerationProperty_Create(name, defaultValue, visible));
    return obj;
}

/*!
 * @brief Creates an Enumeration Property builder object with a specified name and default value.
 * @param name The name of the Property.
 * @param defaultValue The default enumeration value.
 *
 * The Property Value type is `ctEnumeration`.
 */
inline PropertyBuilderPtr EnumerationPropertyBuilder(const StringPtr& name, const EnumerationPtr& defaultValue)
{
    PropertyBuilderPtr obj(EnumerationPropertyBuilder_Create(name, defaultValue));
    return obj;
}

/*!
 * @brief Creates a Property using Builder
 * @param builder Property Builder
 */
inline PropertyPtr PropertyFromBuilder(const PropertyBuilderPtr& builder)
{
    PropertyPtr obj(PropertyFromBuilder_Create(builder));
    return obj;
}

inline PropertyBuilderPtr PropertyBuilderCopy(const PropertyPtr& property)
{
    PropertyBuilderPtr obj(PropertyBuilderFromExisting_Create(property));
    return obj;
}

/*!@}*/

END_NAMESPACE_OPENDAQ
