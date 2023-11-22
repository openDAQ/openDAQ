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

#pragma once
#include <coretypes/coretypes.h>
#include <coreobjects/callable_info.h>
#include <coreobjects/coercer.h>
#include <coreobjects/validator.h>
#include <coreobjects/unit.h>

BEGIN_NAMESPACE_OPENDAQ

struct IPropertyObject;
struct IEvalValue;
struct IPropertyBuilder;

/*!
 * @ingroup objects_property
 * @addtogroup objects_property_obj Property
 * @{
 */

/*#
 * [templated(defaultAliasName: PropertyPtr)]
 * [interfaceSmartPtr(IProperty, GenericPropertyPtr)]
 * [interfaceSmartPtr(IPropertyObject, PropertyObjectPtr, "<coreobjects/property_ptr.fwd_declare.h>")]
 * [interfaceSmartPtr(IPropertyValueEventArgs, PropertyValueEventArgsPtr, "<coretypes/event_wrapper.h>")]
 * [interfaceSmartPtr(IBoolean, BooleanPtr, "<coretypes/boolean_factory.h>")]
 * [interfaceSmartPtr(IStruct, StructPtr, "<coretypes/struct_ptr.h>")]
 * [interfaceSmartPtr(IStructType, StructTypePtr, "<coretypes/struct_type_ptr.h>")]
 * [interfaceLibrary(INumber, CoreTypes)]
 */

/*!
 * @brief Defines a set of metadata that describes the values held by a Property object stored
 * under the key equal to the property's name.
 *
 * A property can be added to a Property object or a Property object class. Once added to either, the
 * Property is frozen and can no longer be changed. Adding a Property to a Property object allows for
 * its corresponding value to be get/set. Similarly, when a Property object is created using a Property
 * object class to which a Property was added, corresponding values can be get/set.
 *
 * When retrieving a Property from a Property object, the returned Property is bound to the Property object.
 * Property metadata fields can reference another Property/Value of the bound Property object. Most Property
 * fields can contain an EvalValue that is evaluated once the corresponding Property field's getter is called.
 * For more information on using EvalValues and binding Properties, see the below section "EvalValue fields and
 * Property binding".
 *
 * Below we define the types of fields available and highlight some special property types, and what fields
 * are expected for a specific type of property.
 *
 * Generally, the mandatory fields of a Property are Name, Value type, and Default value.
 *
 * @subsection objects_property_fields Property metadata fields
 *
 * A Property can define the following fields:
 *
 * - <b>Name:</b> The name of the property. Within a Property object or Property object class, no two properties
 *                can have the same name. A Property value is linked to a Property via its name.
 * - <b>Value type:</b> The type of the corresponding Property value stored in a Property object. The Default
 *                      value must also be of the same type.
 * - <b>Default value:</b> The default value of the Property. If no Property value is set on the Property object,
 *                         the value getter will return the default value.
 * - <b>Description:</b> A short string description of the property.
 * - <b>Item type:</b> If the property is a list or dictionary type, the Item type field specifies the types of
 *                     values stored in the container.
 * - <b>Key type:</b> If the property is a dictionary type, the Key type specifies the key type of the dictionary.
 * - <b>Unit:</b> The Property's unit. Eg. second, meter, volt.
 * - <b>Min value:</b> The minimum value of the Property's corresponding value. The property must be numeric for
 *                     this field to be valid.
 * - <b>Max value:</b> The maximum value of the Property's corresponding value. The property must be numeric for
 *                     this field to be valid.
 * - <b>Suggested values:</b> A list of suggested values for the property. The list allows a user to see what
 *                            values are expected for the Property. Those values, however, are not enforced. 
 * - <b>Selection values:</b> A list or dictionary of selection values. If the Selection values field is configured,
 *                            the value of the Property must be an integer that is used to index into the
 *                            list/dictionary of selection values.
 * - <b>Referenced property:</b> Reference to another property on the Property object. When the Referenced property
 *                               field is set, all getter/setter methods except for those referencing the `name` and
 *                               the `Referenced property` fields will be invoked on the referenced property instead.
 *                               This field is usually an EvalValue object, pointing to a different property depending
 *                               on the Property object's current state.
 * - <b>Is referenced:</b> If true, the property is referenced by another. Properties with the `Is referenced` field
 *                         set to true are visible only through the property referencing them and will not be included
 *                         in the list of visible properties available on the Property object.
 * - <b>Validator:</b> Validator object that contains an EvalValue expression that checks whether or not the value is
 *                     valid. See the "Validation and Coercion" section below. See the Validator documentation for
 *                     more information.
 * - <b>Coercer:</b> Coercer object that contains an EvalValue expression that coerces any written Property value to
 *                   the specified boundaries. See the Coercer documentation for more information.
 * - <b>Read-only:</b> Property values of Properties with `Read-only` set to true cannot be changed. This can be
 *                     circumvented by using a protected write available through the `PropertyObjectProtected`
 *                     interface.
 * - <b>Callable info:</b> Available only for function- and procedure-type properties. Contains information about the
 *                         parameter and return types of the function/procedure stored as the Property value.
 * - <b>On property value write:</b> Event triggered when the corresponding Property value is written to. Contains a
 *                                   reference to the Property object and allows for overriding the written value.
 * - <b>On property value read:</b> Event triggered when the corresponding Property value is read. Contains a reference
 *                                  to the Property object and allows for overriding the read value.
 *
 * @subsection objects_property_binding EvalValue fields and Property binding
 *
 * Properties that are retrieved from a Property object are bound to that Property object. This allows for a Property
 * metadata field to access other Properties and their corresponding values of the Property object. Most fields of a
 * Property can be configured with an EvalValue. The EvalValue is evaluated when the property field is retrieved, allowing
 * for its evaluated value to change depending on the state of the Property object.
 *
 * In an EvalValue expression, other Property object Properties can be referenced with the "%" symbol, while their
 * Property values can be referenced with "$". For example, setting the Visible field to be:
 * `EvalValue(If($showProp == 1))` results in the property being visible only if the Property value of "showProp"
 * is set to 1.
 *
 * @subsection objects_property_selection Selection properties
 * Selection properties are those that have the Selection values field configured with either a
 * list, or dictionary, and its Value type must be Integer. The values of the list/dictionary
 * match the Item type of the property, while the keys of the dictionary must be integers.
 * (matching the Value type).
 *
 * The Property value of a selection property represents the index or key used to retrieve the
 * Selection value from the list/dictionary. As such, the values written to the corresponding
 * Property value are always integers, but the selected value can be of any type.
 *
 * To obtain the selected value, we get the corresponding Property value, and use it as the
 * index/key to obtain the value from our list/dictionary of selection values. Alternatively,
 * the Property object provides a Selection property getter method that automaticlly performs
 * the above steps.
 *
 * Selection properties must have a default value.
 *
 * @subsection objects_property_function Function/Procedure properties
 * Function properties have the Value type Function or Procedure. Functions are callable methods
 * that have an optional return type, while procedures do not return anything. The property
 * value of a Function/Procedure property is a callable object.
 *
 * To determine the parameter count and types, as well as the return type, the Callable info
 * field must be configured. Callable info contains a list of argument types that need to
 * be passed as arguments when invoking the callable object. If the Property is a Function,
 * the Callable info field also contains the type of the variable returned by the function.
 *
 * Importantly, Function and Procedure type properties are currently not accessible through the
 * OPC UA layer. Thus, they will not appear on connected-to devices.
 *
 * Function and Procedure type properties cannot have a default value.
 *
 * @subsection objects_property_reference Reference properties
 *
 * Reference properties have the Referenced property field configured. The Referenced property
 * contains a pointer to another Property that is part of the same Property object. On such
 * properties, all Property field getters except for the Name, Is referenced, Referenced
 * property, Value type, Key type, and Item type return the metadata fields of the referenced
 * Property. Similarly, the Property object value getters and setters get/set the value of the
 * referenced property.
 * 
 * The Referenced property field is configured with an EvalValue that most often switches
 * between different properties depending on the value of another property. For example the
 * `EvalValue` string "switch($switchProp, 0, %prop1, 1, %prop2)" reads the value of the
 * property named "switchProp" and references the property named "prop1" if the value is 0. If
 * the value is 1, it references "prop2" instead.
 *
 * A Property can be referenced by only one Property within a Property object.
 *
 * Reference properties can only have the Name and Referenced property fields configured. Their
 * Value type is always undefined.
 *
 * @subsection objects_property_object_type Object-type properties
 *
 * Object type properties have the Value type Object. These kinds of properties allow for
 * Properties to be grouped and represented in a hierarchy of nested Property objects. A value
 * of an object-type Property can only be a base Property object. Objects such as Devices or
 * Function blocks that are descendants of the Property object class cannot be set as the
 * Property value.
 * 
 * Object type properties can only have their Name, Description and Default value configured,
 * where the Default value is mandatory.
 *
 * Object Properties behave slightly differently in that their Default Values do
 * not get frozen when the Property is added to a Property Object (they do get frozen when
 * added to a Property Object Class, or if the Object Property is Read-only).
 * As such, Properties of the Default Value Property Object can be modified even after being
 * added to a Property Object.
 *
 * @subsection objects_property_containers Container-type properties
 *
 * Container type properties have the Value type List or Dictionary and must be homogeneous -
 * they can only have the keys and values of the same type. Their Key and Item types are
 * configured to match that of the Property's Default value. Any new Property value must
 * adhere to the original key and item type.
 *
 * Containers cannot contain Object-type values, Container-type values (List, Dictionary),
 * or Function-type values. Same applies for they Key type of dictionary objects.
 * 
 * Importantly, Container-type properties cannot have empty default values as of now. If the
 * default values are empty, the Key and Item type deduction will not work properly,
 * evaluating the types to be undefined.
 *
 * Container properties must have a default value.
 *
 * @subsection objects_property_numbers Numerical properties
 *
 * Numerical properties represent numbers. Their values can be either Integers or Floating
 * point numbers, depending on the configured Value type. Numerical properties can have
 * the Min and Max value fields configured to limit the range of values accepted.
 *
 * Additionally, Numerical properties they can have a list of Suggested values, indicating the
 * expected values for the Property. Note that the Property system does not enforce that a
 * Property value matches a number in the list of Suggested values.
 *
 * Numerical properties must have a default value.
 *
 * @subsection objects_property_structs Struct properties
 *
 * Struct properties represent structures of pre-defined key-value pairs. A Structure property
 * has Value type `ctStruct`. It stores the Structure type of the Struct given as the default value.
 * New Struct values must have the same Struct type as the default.
 */
DECLARE_OPENDAQ_INTERFACE(IProperty, IBaseObject)
{
    /*!
     * @brief Gets the Value type of the Property. Values written to the corresponding Property value
     * must be of the same type.
     * @param[out] type The value type.
     */
    virtual ErrCode INTERFACE_FUNC getValueType(CoreType* type) = 0;

    /*!
     * @brief Gets the Key type of the Property. Configured only if the Value type is `ctDict`. If so,
     * the key type of the dictionary Property values must match the Property's Key type.
     * @param[out] type The Key type of dictionary properties.
     */
    virtual ErrCode INTERFACE_FUNC getKeyType(CoreType* type) = 0;

    /*!
     * @brief Gets the Item type of the Property. Configured only if the Value type is `ctDict` or `ctList`. If so,
     * the item types of the list/dictionary must match the Property's Item type.
     * @param[out] type The Item type of list/dictionary properties.
     */
    virtual ErrCode INTERFACE_FUNC getItemType(CoreType* type) = 0;

    /*!
     * @brief Gets the Name of the Property. The names of Properties in a Property object must be unique.
     * The name is used as the key to the corresponding Property value when getting/setting the value.
     * @param[out] name The Name of the Property.
     */
    virtual ErrCode INTERFACE_FUNC getName(IString** name) = 0;

    /*!
     * @brief Gets the short string Description of the Property.
     * @param[out] description The Description of the Property.
     */
    virtual ErrCode INTERFACE_FUNC getDescription(IString** description) = 0;

    /*!
     * @brief Gets the Unit of the Property.
     * @param[out] unit The Unit of the Property.
     */
    virtual ErrCode INTERFACE_FUNC getUnit(IUnit** unit) = 0;

    /*!
     * @brief Gets the Minimum value of the Property. Available only if the Value type is `ctInt` or `ctFloat`.
     * @param[out] min The Minimum value of the Property.
     */
    virtual ErrCode INTERFACE_FUNC getMinValue(INumber** min) = 0;

    /*!
     * @brief Gets the Maximum value of the Property. Available only if the Value type is `ctInt` or `ctFloat`.
     * @param[out] max The Maximum value of the Property.
     */
    virtual ErrCode INTERFACE_FUNC getMaxValue(INumber** max) = 0;

    /*!
     * @brief Gets the Default value of the Property. The Default value must always be configured for a Property to be
     * in a valid state. Exceptions are Function/Procedure and Reference properties.
     * @param[out] value The Default value of the Property.
     */
    virtual ErrCode INTERFACE_FUNC getDefaultValue(IBaseObject** value) = 0;

    // [templateType(values, IBaseObject)]
    /*!
     * @brief Gets the list of Suggested values. Contains values that are the optimal settings for the corresponding
     * Property value. These values, however, are not enforced when setting a new Property value.
     * @param[out] values The Suggested values of the Property.
     */
    virtual ErrCode INTERFACE_FUNC getSuggestedValues(IList** values) = 0;

    /*!
     * @brief Used to determine whether the property is visible or not.
     * @param[out] visible True if the Property is visible; false otherwise.
     */
    virtual ErrCode INTERFACE_FUNC getVisible(Bool* visible) = 0;

    /*!
     * @brief Used to determine whether the Property is a read-only property or not.
     * @param[out] readOnly True if the Property is a read-only property; false otherwise.
     *
     * Read-only Property values can still be modified by using the `PropertyObjectProtected` interface methods.
     */
    virtual ErrCode INTERFACE_FUNC getReadOnly(Bool* readOnly) = 0;

    /*!
     * @brief Gets the list or dictionary of selection values. If the list/dictionary is not empty, the property
     * is a Selection property, and must have the Value type `ctInt`.
     * @param[out] values The list/dictionary of possible selection values.
     */
    virtual ErrCode INTERFACE_FUNC getSelectionValues(IBaseObject** values) = 0;

    // [templateType(property, IProperty)]
    /*!
     * @brief Gets the referenced property. If set, all getters except for the `Name`, `Referenced property`, and
     * `Is referenced` getters will return the value of the `Referenced property`.
     * @param[out] property The referenced property.
     *
     * If the Property is not bound to a Property object this call will not be able to return the Referenced property.
     */
    virtual ErrCode INTERFACE_FUNC getReferencedProperty(IProperty** property) = 0;

    /*!
     * @brief Used to determine whether the Property is referenced by another property.
     * @param[out] isReferenced True if the Property is referenced by another property; false otherwise.
     */
    virtual ErrCode INTERFACE_FUNC getIsReferenced(Bool* isReferenced) = 0;

    /*!
     * @brief Gets the validator of the Property.
     * @param[out] validator The validator.
     *
     * Used to validate whether a value written to the corresponding Property value is valid or not.
     */
    virtual ErrCode INTERFACE_FUNC getValidator(IValidator** validator) = 0;

    /*!
     * @brief Gets the coercer of the Property.
     * @param[out] coercer The coercer.
     *
     * Used to coerce a value written to the corresponding Property value to the constraints specified by the coercer.
     */
    virtual ErrCode INTERFACE_FUNC getCoercer(ICoercer** coercer) = 0;

    /*!
     * @brief Gets the Callable information objects of the Property that specifies the argument and return types
     * of the callable object stored as the Property value.
     * @param[out] callable The Callable info object.
     */
    virtual ErrCode INTERFACE_FUNC getCallableInfo(ICallableInfo** callable) = 0;

    /*!
     * @brief Gets the Struct type object of the Property, if the Property is a Struct property.
     * @param[out] structType The Struct type of the Struct Property.
     */
    virtual ErrCode INTERFACE_FUNC getStructType(IStructType** structType) = 0;

    // [templateType(event, IPropertyObject, IPropertyValueEventArgs)]
    /*!
     * @brief Gets the event object that is triggered when a value is written to the corresponding Property value.
     * @param[out] event The On-write event.
     *
     * The event arguments contain a reference to the property object, as well as a function allowing for the written
     * value to be overridden.
     */
    virtual ErrCode INTERFACE_FUNC getOnPropertyValueWrite(IEvent** event) = 0;

    // [templateType(event, IPropertyObject, IPropertyValueEventArgs)]
    /*!
     * @brief Gets the event object that is triggered when the corresponding Property value is read.
     * @param[out] event The On-read event.
     *
     * The event arguments contain a reference to the property object, as well as a function allowing for the read
     * value to be overridden.
     */
    virtual ErrCode INTERFACE_FUNC getOnPropertyValueRead(IEvent** event) = 0;
};
/*!@}*/

/*!
 * @brief Creates a boolean Property object with a default value and Visible state.
 * @param name The name of the Property.
 * @param defaultValue The boolean default value. Can be an EvalValue.
 * @param visible If true, the Property is visible. Can be an EvalValue.
 *
 * The Property Value type is `ctBool`. Note that the defaultValue and visible parameters can be EvalValues.
 */
OPENDAQ_DECLARE_CLASS_FACTORY_WITH_INTERFACE(
    LIBRARY_FACTORY, BoolProperty, IProperty,
    IString*, name,
    IBoolean*, defaultValue,
    IBoolean*, visible
)

/*!
 * @brief Creates an integer Property object with a default value and Visible state.
 * @param name The name of the Property.
 * @param defaultValue The integer default value. Can be an EvalValue.
 * @param visible If true, the Property is visible. Can be an EvalValue.
 *
 * The Property Value type is `ctInt`. Note that the defaultValue and visible parameters can be EvalValues.
 */
OPENDAQ_DECLARE_CLASS_FACTORY_WITH_INTERFACE(
    LIBRARY_FACTORY, IntProperty, IProperty,
    IString*, name,
    IInteger*, defaultValue,
    IBoolean*, visible
)

/*!
 * @brief Creates a floating point value Property object with a default value and Visible state.
 * @param name The name of the Property.
 * @param defaultValue The float default value. Can be an EvalValue.
 * @param visible If true, the Property is visible. Can be an EvalValue.
 *
 * The Property Value type is `ctFloat`. Note that the defaultValue and visible parameters can be EvalValues.
 */
OPENDAQ_DECLARE_CLASS_FACTORY_WITH_INTERFACE(
    LIBRARY_FACTORY, FloatProperty, IProperty,
    IString*, name,
    IFloat*, defaultValue,
    IBoolean*, visible
)

/*!
 * @brief Creates a string Property object with a default value and Visible state.
 * @param name The name of the Property.
 * @param defaultValue The integer default value. Can be an EvalValue.
 * @param visible If true, the Property is visible. Can be an EvalValue.
 *
 * The Property Value type is `ctString`. Note that the defaultValue and visible parameters can be EvalValues.
 */
OPENDAQ_DECLARE_CLASS_FACTORY_WITH_INTERFACE(
    LIBRARY_FACTORY, StringProperty, IProperty,
    IString*, name,
    IString*, defaultValue,
    IBoolean*, visible
)

/*!
 * @brief Creates a list Property object with a default value and Visible state.
 * @param name The name of the Property.
 * @param defaultValue The list default value. Can be an EvalValue.
 * @param visible If true, the Property is visible. Can be an EvalValue.
 *
 * The Property Value type is `ctList`. Note that the defaultValue and visible parameters can be EvalValues.
 * The list passed as `defaultValue` must be homogeneous.
 *
 * The Property's Item type field will be set according to defaultValue list type.
 */
OPENDAQ_DECLARE_CLASS_FACTORY_WITH_INTERFACE(
    LIBRARY_FACTORY, ListProperty, IProperty,
    IString*, name,
    IList*, defaultValue,
    IBoolean*, visible
)

/*!
 * @brief Creates a dictionary Property object with a default value and Visible state.
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
OPENDAQ_DECLARE_CLASS_FACTORY_WITH_INTERFACE(
    LIBRARY_FACTORY, DictProperty, IProperty,
    IString*, name,
    IDict*, defaultValue,
    IBoolean*, visible
)

/*!
 * @brief Creates a ratio Property object with a default value and Visible state.
 * @param name The name of the Property.
 * @param defaultValue The ratio default value.
 * @param visible If true, the Property is visible. Can be an EvalValue.
 *
 * The Property Value type is `ctRatio`. Note that the visible parameter can be an EvalValue.
 *
 * TODO: defaultValue can be an EvalValue once ratios are supported.
 */
OPENDAQ_DECLARE_CLASS_FACTORY_WITH_INTERFACE(
    LIBRARY_FACTORY, RatioProperty, IProperty,
    IString*, name,
    IRatio*, defaultValue,
    IBoolean*, visible
)
/*!
 * @brief Creates an object-type Property object with a default value and optional Visible state.
 * @param name The name of the Property.
 * @param defaultValue The Property object default value.
 *
 * The Property Value type is `ctObject`. Object properties cannot be have any metadata other than
 * their name, description, and default value configured. The PropertyObject default value can only
 * be a base PropertyObject type (not a descendant type).
 *
 * If the defaultValue is not specified, it will automatically be configured to an empty Property Object.
 */
OPENDAQ_DECLARE_CLASS_FACTORY_WITH_INTERFACE(
    LIBRARY_FACTORY, ObjectProperty, IProperty,
    IString*, name,
    IPropertyObject*, defaultValue
)

/*!
 * @brief Creates a Reference Property object that points at a property specified in the `referencedProperty`
 * parameter.
 * @param name The name of the Property.
 * @param referencedPropertyEval The evaluation expression that evaluates to another property.
 */
OPENDAQ_DECLARE_CLASS_FACTORY_WITH_INTERFACE(
    LIBRARY_FACTORY, ReferenceProperty, IProperty,
    IString*, name,
    IEvalValue*, referencedPropertyEval
)

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
OPENDAQ_DECLARE_CLASS_FACTORY_WITH_INTERFACE(
    LIBRARY_FACTORY, FunctionProperty, IProperty,
    IString*, name,
    ICallableInfo*, callableInfo,
    IBoolean*, visible
)

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
OPENDAQ_DECLARE_CLASS_FACTORY_WITH_INTERFACE(
    LIBRARY_FACTORY, SelectionProperty, IProperty,
    IString*, name,
    IList*, selectionValues,
    IInteger*, defaultValue,
    IBoolean*, visible
)

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
OPENDAQ_DECLARE_CLASS_FACTORY_WITH_INTERFACE(
    LIBRARY_FACTORY, SparseSelectionProperty, IProperty,
    IString*, name,
    IDict*, selectionValues,
    IInteger*, defaultValue,
    IBoolean*, visible
)

/*!
 * @brief Creates a Struct Property object with a default value and its visible state.
 * @param name The name of the Property.
 * @param defaultValue The default structure value.
 * @param visible If true, the Property is visible. Can be an EvalValue.
 *
 * The Property Value type is `ctStruct`.
 */
OPENDAQ_DECLARE_CLASS_FACTORY_WITH_INTERFACE(
    LIBRARY_FACTORY, StructProperty, IProperty,
    IString*, name,
    IStruct*, defaultValue,
    IBoolean*, visible
)

/*!
 * @brief Creates a Property using the given dictionary of Property parameters. The Dictionary contains
 * keys that correspond to the Property fields (visible, defaultValue...) in conjunction with the intended
 * values of the fields.
 * @param buildParams the Dictionary of build parameters for the Property.
 */
OPENDAQ_DECLARE_CLASS_FACTORY_WITH_INTERFACE(
    LIBRARY_FACTORY, PropertyFromBuildParams, IProperty,
    IDict*, buildParams
)

/*!
 * @brief Creates a Property using Builder
 * @param builder Property Builder
 */
OPENDAQ_DECLARE_CLASS_FACTORY_WITH_INTERFACE(
    LIBRARY_FACTORY, PropertyFromBuilder, IProperty,
    IPropertyBuilder*, builder
)

END_NAMESPACE_OPENDAQ
