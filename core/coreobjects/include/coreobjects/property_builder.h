/*
 * Copyright 2022-2024 Blueberry d.o.o.
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
#include <coreobjects/property.h>

BEGIN_NAMESPACE_OPENDAQ

struct IPropertyObject;
struct IEvalValue;

/*!
 * @ingroup objects_property
 * @addtogroup objects_property_obj PropertyBuilder
 * @{
 */

/*#
 * [interfaceLibrary(INumber, CoreTypes)]
 * [interfaceSmartPtr(IBoolean, BooleanPtr, "<coretypes/boolean_factory.h>")]
 * [interfaceSmartPtr(IStruct, StructPtr, "<coretypes/struct_ptr.h>")]
 */

/*!
 * @brief The builder interface of Properties. Allows for construction of Properties through the `build`
 * method.
 *
 * Contains setters for the Property fields. The setters take as parameters openDAQ objects, even if
 * the value must always evaluate to, for example, a boolean. This allows for EvalValue objects to be
 * set instead of a static value.
 *
 * The EvalValue objects can evaluate to Boolean, String, List, Unit, and Property types. and can thus be
 * used when such types are expected from the getters.
 *
 * The Property can be built by calling the `build` method.
 */
DECLARE_OPENDAQ_INTERFACE(IPropertyBuilder, IBaseObject)
{    
    /*!
     * @brief Builds and returns a Property using the currently set values of the Builder.
     * @param[out] property The built property.
     */
    virtual ErrCode INTERFACE_FUNC build(IProperty** property) = 0;

    // [returnSelf]
    /*!
     * @brief Sets the Value type of the Property. Values written to the corresponding Property value
     * must be of the same type.
     * @param type The value type.
     */
    virtual ErrCode INTERFACE_FUNC setValueType(CoreType type) = 0;

    /*!
     * @brief Gets the Value type of the Property.
     * @param[out] type The value type.
     */
    virtual ErrCode INTERFACE_FUNC getValueType(CoreType* type) = 0;

    // [returnSelf]
    /*!
     * @brief Sets the Name of the Property. The names of Properties in a Property object must be unique.
     * The name is used as the key to the corresponding Property value when getting/setting the value.
     * @param name The Name of the Property.
     */
    virtual ErrCode INTERFACE_FUNC setName(IString* name) = 0;

    /*!
     * @brief Gets the Name of the Property.
     * @param[out] name The Name of the Property.
     */
    virtual ErrCode INTERFACE_FUNC getName(IString** name) = 0;
    
    // [returnSelf]
    /*!
     * @brief Sets the short string Description of the Property.
     * @param description The Description of the Property.
     */
    virtual ErrCode INTERFACE_FUNC setDescription(IString* description) = 0;

    /*!
     * @brief Gets the short string Description of the Property.
     * @param[out] description The Description of the Property.
     */
    virtual ErrCode INTERFACE_FUNC getDescription(IString** description) = 0;
    
    // [returnSelf, polymorphic(unit)]
    /*!
     * @brief Sets the Unit of the Property.
     * @param unit The Unit of the Property.
     */
    virtual ErrCode INTERFACE_FUNC setUnit(IUnit* unit) = 0;

    /*!
     * @brief Gets the Unit of the Property.
     * @param[out] unit The Unit of the Property.
     */
    virtual ErrCode INTERFACE_FUNC getUnit(IUnit** unit) = 0;
    
    // [returnSelf, polymorphic(min)]
    /*!
     * @brief Sets the Minimum value of the Property. Available only if the Value type is `ctInt` or `ctFloat`.
     * @param min The Minimum value of the Property.
     */
    virtual ErrCode INTERFACE_FUNC setMinValue(INumber* min) = 0;

    /*!
     * @brief Gets the Minimum value of the Property. Available only if the Value type is `ctInt` or `ctFloat`.
     * @param[out] min The Minimum value of the Property.
     */
    virtual ErrCode INTERFACE_FUNC getMinValue(INumber** min) = 0;
    
    // [returnSelf, polymorphic(max)]
    /*!
     * @brief Sets the Maximum value of the Property. Available only if the Value type is `ctInt` or `ctFloat`.
     * @param max The Maximum value of the Property.
     */
    virtual ErrCode INTERFACE_FUNC setMaxValue(INumber* max) = 0;

    /*!
     * @brief Gets the Maximum value of the Property. Available only if the Value type is `ctInt` or `ctFloat`.
     * @param[out] max The Maximum value of the Property.
     */
    virtual ErrCode INTERFACE_FUNC getMaxValue(INumber** max) = 0;
    
    // [returnSelf, polymorphic(value)]
    /*!
     * @brief Sets the Default value of the Property. The Default value must always be configured for a Property to be
     * in a valid state. Exceptions are Function/Procedure and Reference properties.
     * The function will freeze default value if it is freezable. 
     * @param value The Default value of the Property.
     */
    virtual ErrCode INTERFACE_FUNC setDefaultValue(IBaseObject* value) = 0;

    /*!
     * @brief Gets the Default value of the Property.
     * @param[out] value The Default value of the Property.
     */
    virtual ErrCode INTERFACE_FUNC getDefaultValue(IBaseObject** value) = 0;

    // [templateType(values, IBaseObject), returnSelf, polymorphic(values)]
    /*!
     * @brief Sets the list of Suggested values. Contains values that are the optimal settings for the corresponding
     * Property value. These values, however, are not enforced when setting a new Property value.
     * @param values The Suggested values of the Property.
     */
    virtual ErrCode INTERFACE_FUNC setSuggestedValues(IList* values) = 0;

    // [templateType(values, IBaseObject)]
    /*!
     * @brief Gets the list of Suggested values. Contains values that are the optimal gettings for the corresponding
     * Property value. These values, however, are not enforced when getting a new Property value.
     * @param[out] values The Suggested values of the Property.
     */
    virtual ErrCode INTERFACE_FUNC getSuggestedValues(IList** values) = 0;
    
    // [returnSelf, polymorphic(visible)]
    /*!
     * @brief Used to determine whether the property is visible or not.
     * @param visible True if the Property is visible; false otherwise.
     */
    virtual ErrCode INTERFACE_FUNC setVisible(IBoolean* visible) = 0;

    /*!
     * @brief Used to determine whether the property is visible or not.
     * @param[out] visible True if the Property is visible; false otherwise.
     */
    virtual ErrCode INTERFACE_FUNC getVisible(IBoolean** visible) = 0;
    
    // [returnSelf, polymorphic(readOnly)]
    /*!
     * @brief Used to determine whether the Property is a read-only property or not.
     * @param readOnly True if the Property is a read-only property; false otherwise.
     *
     * Read-only Property values can still be modified by using the `PropertyObjectProtected` interface methods.
     */
    virtual ErrCode INTERFACE_FUNC setReadOnly(IBoolean* readOnly) = 0;

    /*!
     * @brief Used to determine whether the Property is a read-only property or not.
     * @param[out] readOnly True if the Property is a read-only property; false otherwise.
     */
    virtual ErrCode INTERFACE_FUNC getReadOnly(IBoolean** readOnly) = 0;
    
    // [returnSelf, polymorphic(values)]
    /*!
     * @brief Sets the list or dictionary of selection values. If the list/dictionary is not empty, the property
     * is a Selection property, and must have the Value type `ctInt`.
     * @param values The list/dictionary of possible selection values.
     */
    virtual ErrCode INTERFACE_FUNC setSelectionValues(IBaseObject* values) = 0;

    /*!
     * @brief Gets the list or dictionary of selection values.
     * @param[out] values The list/dictionary of possible selection values.
     */
    virtual ErrCode INTERFACE_FUNC getSelectionValues(IBaseObject** values) = 0;

    // [templateType(property, IProperty), returnSelf]
    /*!
     * @brief Sets the referenced property. If set, all getters except for the `Name`, `Referenced property`, and
     * `Is referenced` getters will return the value of the `Referenced property`.
     * @param propertyEval The referenced property.
     *
     * If the Property is not bound to a Property object this call will not be able to return the Referenced property.
     */
    virtual ErrCode INTERFACE_FUNC setReferencedProperty(IEvalValue* propertyEval) = 0;
    
    // [templateType(property, IProperty)]
    /*!
     * @brief Gets the referenced property.
     * @param[out] propertyEval The referenced property.
     */
    virtual ErrCode INTERFACE_FUNC getReferencedProperty(IEvalValue** propertyEval) = 0;
    // [returnSelf, polymorphic(validator)]
    /*!
     * @brief Sets the validator of the Property.
     * @param validator The validator.
     *
     * Used to validate whether a value written to the corresponding Property value is valid or not.
     */
    virtual ErrCode INTERFACE_FUNC setValidator(IValidator* validator) = 0;

    /*!
     * @brief Gets the validator of the Property.
     * @param[out] validator The validator.
     */
    virtual ErrCode INTERFACE_FUNC getValidator(IValidator** validator) = 0;
    
    // [returnSelf, polymorphic(coercer)]
    /*!
     * @brief Sets the coercer of the Property.
     * @param coercer The coercer.
     *
     * Used to coerce a value written to the corresponding Property value to the constraints specified by the coercer.
     */
    virtual ErrCode INTERFACE_FUNC setCoercer(ICoercer* coercer) = 0;

    /*!
     * @brief Gets the coercer of the Property.
     * @param[out] coercer The coercer.
     */
    virtual ErrCode INTERFACE_FUNC getCoercer(ICoercer** coercer) = 0;
    
    // [returnSelf, polymorphic(callable)]
    /*!
     * @brief Sets the Callable information objects of the Property that specifies the argument and return types
     * of the callable object stored as the Property value.
     * @param callable The Callable info object.
     */
    virtual ErrCode INTERFACE_FUNC setCallableInfo(ICallableInfo* callable) = 0;
    
    /*!
     * @brief Gets the Callable information objects of the Property that specifies the argument and return types
     * of the callable object stored as the Property value.
     * @param[out] callable The Callable info object.
     */
    virtual ErrCode INTERFACE_FUNC getCallableInfo(ICallableInfo** callable) = 0;

    // [templateType(event, "PropertyObjectPtr, PropertyValueEventArgsPtr"), returnSelf]
    /*!
     * @brief Sets a custom on-write event. Used mostly when cloning properties.
     * @param event The on-write event.
     */
    virtual ErrCode INTERFACE_FUNC setOnPropertyValueWrite(IEvent* event) = 0;

    // [templateType(event, IPropertyObject, IPropertyValueEventArgs)]
    /*!
     * @brief Gets a custom on-write event. Used mostly when cloning properties.
     * @param[out] event The on-write event.
     */
    virtual ErrCode INTERFACE_FUNC getOnPropertyValueWrite(IEvent** event) = 0;

    // [templateType(event, "PropertyObjectPtr, PropertyValueEventArgsPtr"), returnSelf]
    /*!
     * @brief Sets a custom on-read event. Used mostly when cloning properties.
     * @param event The on-read event.
     */
    virtual ErrCode INTERFACE_FUNC setOnPropertyValueRead(IEvent * event) = 0;

    // [templateType(event, IPropertyObject, IPropertyValueEventArgs)]
    /*!
     * @brief Gets a custom on-read event. Used mostly when cloning properties.
     * @param[out] event The on-read event.
     */
    virtual ErrCode INTERFACE_FUNC getOnPropertyValueRead(IEvent** event) = 0;
};

/*!@}*/

/*!
 * @addtogroup objects_property_obj_factories Factories
 * @{
 */

/*!
 * @brief Creates an Property builder object with only the name field configured.
 * @param name The name of the Property.
 *
 * The default Value type is `ctUndefined`.
 */
OPENDAQ_DECLARE_CLASS_FACTORY_WITH_INTERFACE(
    LIBRARY_FACTORY, PropertyBuilder, IPropertyBuilder,
    IString*, name
)

/*!
 * @brief Creates a boolean Property builder object with a specified name and default value.
 * @param name The name of the Property.
 * @param defaultValue The boolean default value. Can be an EvalValue.
 *
 * The Property Value type is `ctBool`. Note that the defaultValue parameter can be EvalValue.
 */
OPENDAQ_DECLARE_CLASS_FACTORY_WITH_INTERFACE(
    LIBRARY_FACTORY, BoolPropertyBuilder, IPropertyBuilder,
    IString*, name,
    IBoolean*, defaultValue
)

/*!
 * @brief Creates an integer Property builder object with a specified name and default value.
 * @param name The name of the Property.
 * @param defaultValue The integer default value. Can be an EvalValue.
 *
 * The Property Value type is `ctInt`. Note that the defaultValue parameter can be EvalValue.
 */
OPENDAQ_DECLARE_CLASS_FACTORY_WITH_INTERFACE(
    LIBRARY_FACTORY, IntPropertyBuilder, IPropertyBuilder,
    IString*, name,
    IInteger*, defaultValue
)

/*!
 * @brief Creates a floating point value Property builder object with a specified name and default value.
 * @param name The name of the Property.
 * @param defaultValue The float default value. Can be an EvalValue
 *
 * The Property Value type is `ctFloat`. Note that the defaultValue parameter can be EvalValue.
 */
OPENDAQ_DECLARE_CLASS_FACTORY_WITH_INTERFACE(
    LIBRARY_FACTORY, FloatPropertyBuilder, IPropertyBuilder,
    IString*, name,
    IFloat*, defaultValue
)

/*!
 * @brief Creates a string Property builder object with a specified name and default value.
 * @param name The name of the Property.
 * @param defaultValue The integer default value. Can be an EvalValue.
 *
 * The Property Value type is `ctString`. Note that the defaultValue parameter can be EvalValue.
 */
OPENDAQ_DECLARE_CLASS_FACTORY_WITH_INTERFACE(
    LIBRARY_FACTORY, StringPropertyBuilder, IPropertyBuilder,
    IString*, name,
    IString*, defaultValue
)

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
OPENDAQ_DECLARE_CLASS_FACTORY_WITH_INTERFACE(
    LIBRARY_FACTORY, ListPropertyBuilder, IPropertyBuilder,
    IString*, name,
    IList*, defaultValue
)

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
OPENDAQ_DECLARE_CLASS_FACTORY_WITH_INTERFACE(
    LIBRARY_FACTORY, DictPropertyBuilder, IPropertyBuilder,
    IString*, name,
    IDict*, defaultValue
)

/*!
 * @brief Creates a ratio Property builder object with a specified name and default value.
 * @param name The name of the Property.
 * @param defaultValue The ratio default value.
 *
 * The Property Value type is `ctRatio`.
 *
 * TODO: defaultValue can be an EvalValue once ratios are supported.
 */
OPENDAQ_DECLARE_CLASS_FACTORY_WITH_INTERFACE(
    LIBRARY_FACTORY, RatioPropertyBuilder, IPropertyBuilder,
    IString*, name,
    IRatio*, defaultValue
)

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
OPENDAQ_DECLARE_CLASS_FACTORY_WITH_INTERFACE(
    LIBRARY_FACTORY, ObjectPropertyBuilder, IPropertyBuilder,
    IString*, name,
    IPropertyObject*, defaultValue
)

/*!
 * @brief Creates a Reference Property builder object that points at a property specified in the `referencedProperty`
 * parameter.
 * @param name The name of the Property.
 * @param referencedPropertyEval The evaluation expression that evaluates to another property.
 */
OPENDAQ_DECLARE_CLASS_FACTORY_WITH_INTERFACE(
    LIBRARY_FACTORY, ReferencePropertyBuilder, IPropertyBuilder,
    IString*, name,
    IEvalValue*, referencedPropertyEval
)

/*!
 * @brief Creates a function- or procedure-type Property builder object. Requires the a CallableInfo object
 * to specify the argument type/count and function return type.
 * @param name The name of the Property.
 * @param callableInfo Information about the callable argument type/count and return type.
 *
 * The Property Value type is `ctFunction` or `ctProc`, depending on if `callableInfo` contains information
 * on the return type or not.
 */
OPENDAQ_DECLARE_CLASS_FACTORY_WITH_INTERFACE(
    LIBRARY_FACTORY, FunctionPropertyBuilder, IPropertyBuilder,
    IString*, name,
    ICallableInfo*, callableInfo
)

/*!
 * @brief Creates a Selection Property builder object with a list of selection values. The default value
 * is an integer index into the default selected value.
 * @param name The name of the Property.
 * @param selectionValues The list of selectable values.
 * @param defaultValue The default index into the list of selection values.
 *
 * The Property Value type is `ctInt`.
 */
OPENDAQ_DECLARE_CLASS_FACTORY_WITH_INTERFACE(
    LIBRARY_FACTORY, SelectionPropertyBuilder, IPropertyBuilder,
    IString*, name,
    IList*, selectionValues,
    IInteger*, defaultValue
)

/*!
 * @brief Creates a Selection Property builder object with a dictionary of selection values. The default value
 * is an integer key into the provided dictionary.
 * @param name The name of the Property.
 * @param selectionValues The dictionary of selectable values. The key type must be `ctInt`.
 * @param defaultValue The default key into the list of selection values.
 *
 * The Property Value type is `ctInt`. The key type of the Selection values dictionary must be `ctInt`.
 */
OPENDAQ_DECLARE_CLASS_FACTORY_WITH_INTERFACE(
    LIBRARY_FACTORY, SparseSelectionPropertyBuilder, IPropertyBuilder,
    IString*, name,
    IDict*, selectionValues,
    IInteger*, defaultValue
)

/*!
 * @brief Creates a Struct Property builder object with a specified name and default value.
 * @param name The name of the Property.
 * @param defaultValue The default structure value.
 *
 * The Property Value type is `ctStruct`.
 */
OPENDAQ_DECLARE_CLASS_FACTORY_WITH_INTERFACE(
    LIBRARY_FACTORY, StructPropertyBuilder, IPropertyBuilder,
    IString*, name,
    IStruct*, defaultValue
)

/*!
 * @brief Creates an Enumeration Property builder object with a specified name and default value.
 * @param name The name of the Property.
 * @param defaultValue The default structure value.
 *
 * The Property Value type is `ctEnumeration`.
 */
OPENDAQ_DECLARE_CLASS_FACTORY_WITH_INTERFACE(
    LIBRARY_FACTORY, EnumerationPropertyBuilder, IPropertyBuilder,
    IString*, name,
    IEnumeration*, defaultValue
)

/*!@}*/

END_NAMESPACE_OPENDAQ
