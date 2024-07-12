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
#include <coreobjects/property.h>
#include <coretypes/type_manager.h>
#include <coretypes/event.h>
#include <coreobjects/permission_manager.h>

BEGIN_NAMESPACE_OPENDAQ


/*!
 * @ingroup objects_property_object
 * @addtogroup objects_property_object_obj PropertyObject
 * @{
 */

/*#
 * [templated(defaultAliasName: PropertyObjectPtr)]
 * [interfaceSmartPtr(IPropertyObject, GenericPropertyObjectPtr)]
 * [interfaceSmartPtr(IPermissionManager, PermissionManagerPtr, "<coreobjects/permission_manager_ptr.h>")]
 */

/*!
 * @brief A container of Properties and their corresponding Property values.
 *
 * The Property object acts as a container of properties that can be inherited from a specified Property object class,
 * or added to the Property object after its creation. The property names must be unique within any given Property object,
 * as no duplicates are allowed.
 *
 * The Property object class name is specified when constructing the Property object, and the class itself is obtained from
 * the Type manager, which, at that point, must already contain a class with the specified name.
 *
 * Each Property defines a set of metadata, specifying what value type it represents, as well as additional information that
 * is used when displaying an user interface, such as whether or not a Property is visible, or read-only. The Property can
 * also limit the set of valid values by specifying a minimum/maximum value, or validation/coercion expressions.
 *
 * In addition to properties, a Property object holds a dictionary of Property values, where the key is a Property's name,
 * and the value is the current Property value. When setting the value of a property, the value is written into the said dictionary.
 * Correspondingly, when reading the value of a property, it is read from the dictionary as well. If the value is not present in
 * the dictionary, the default value of the Property is read instead.
 *
 * Property values must match the Value, Item, and Key type of the corresponding Property. If the Property expects an integer type,
 * only objects of which core type is equal to ctInt can be written to said Property. The Item type represents the type of items
 * expected in lists and dictionaries (when the value type is ctList or ctDict), while the Key type represents the type of keys
 * expected in a dictionary (when the value type is ctDict). Notable, when the Property expects object type values, only other
 * base Property objects can be written to said property. Any classes that inherit the Property object class are not valid Property
 * values.
 *
 * Property objects can be frozen. When frozen, their set of Properties and Property values can no longer be modified.
 */
DECLARE_OPENDAQ_INTERFACE(IPropertyObject, IBaseObject)
{
    /*!
     * @brief Gets the name of the class the Property object was constructed with.
     * @param[out] className The class's name. Contains an empty string if the class name is not configured.
     *
     * A Property object inherits all properties of the Property object class of the same name. Such Property objects
     * have access to the Type manager from which they can retrieve its class type and its properties.
     */
    virtual ErrCode INTERFACE_FUNC getClassName(IString** className) = 0;

    /*!
     * @brief Sets the value of the Property with the given name.
     * @param propertyName The name of the Property.
     * @param value The Property value to be set. Cannot be null.
     * @retval OPENDAQ_ERR_NOTFOUND if a property with given `propertyName` is not part of the Property object.
     * @retval OPENDAQ_ERR_ACCESSDENIED if the property is Read-only.
     * @retval OPENDAQ_ERR_CONVERSIONFAILED if the `value` cannot be converted to the Value type of the Property.
     * @retval OPENDAQ_ERR_INVALIDTYPE if the `value` is a list/dictionary/object with invalid keys/items/fields.
     * @retval OPENDAQ_ERR_VALIDATE_FAILED if the Validator fails to validate the `value`.
     * @retval OPENDAQ_ERR_COERCION_FAILED if the Coercer fails to coerce the `value`.
     * @retval OPENDAQ_ERR_FROZEN if the Property object is frozen.
     * @retval OPENDAQ_ERR_IGNORED if the `value` is the same as the default, or the previously written value.
     *
     * Stores the provided `value` into an internal dictionary of property name and value pairs. This property value can
     * later be retrieved through the corresponding getter method when invoked with the same `propertyName`. The provided
     * `value` must adhere to the restrictions of the corresponding Property (that bears the name `propertyName`). If such
     * a Property is part of the Property object, the setter call will fail. Some restrictions include:
     *   - The core type of the value must match that of the Property Value type.
     *   - If the Property is a numeric type, the value must be equal or greater than the Min value, and equal or smaller
     *     than the Max value.
     *   - If the Property is Read-only, the setter will fail.
     *   - The value will be validated by the Property validator causing the setter method to fail if validation is unsuccessful.
     *   - The value will be coerced to fit the coercion expression of the Property coercer before being written into the local
     *     dictionary of property values.
     *
     * Setting the value of a Property will override either its default value or the value that was set beforehand.
     *
     * @subsection patterns Behaviour patterns of note
     *
     *   - When setting the value of a Property with the Selection values field configured (a Selection property), the `value`
     *     must be an integer type, and acts as an index/key into the list/dictionary of Selection values.
     *   - If the Property is a Reference property (the Referenced property field is configured), the `value` is actually written
     *     under the key of the referenced Property, not the one specified through the `propertyName` argument.
     *   - When setting a list or dictionary type property, the list items and dictionary keys and items must be homogeneous, and
     *     of the same type as specified by the item and key type of the Property.
     *   - Setting a Property value will invoke the corresponding `onPropertyValueWrite` event.
     *
     * @subsection value_set_child_property_objects Child Property objects
     *
     * The Property value setter allows for direct configuration of any child Property objects. To set the Property value of a
     * child Property object, the `propertyName` parameter should be of the format: "childName.propertyName". This pattern
     * can also be used to access nested properties - for example "childName1.childName2.childName3.propertyName".
     */
    virtual ErrCode INTERFACE_FUNC setPropertyValue(IString* propertyName, IBaseObject* value) = 0;

    /*!
     * @brief Gets the value of the Property with the given name.
     * @param propertyName The name of the Property.
     * @param[out] value The returned Property value.
     * @retval OPENDAQ_ERR_NOTFOUND if a property with given `propertyName` is not part of the Property object.
     * @retval OPENDAQ_ERR_INVALIDPARAMETER if attempting to get a value at an index of a non-list Property.
     * @retval OPENDAQ_ERR_OUTOFRANGE if attempting to get a value of a list Property at an out-of-bounds index.
     *
     * The value is retrieved from a local dictionary of Property values where they are stored when set. If a a value is not
     * present under the `propertyName` key, the default value of the corresponding Property is returned. If said property
     * is not part of the Property object, an error occurs.
     *
     * @subsection value_get_child_property_objects Child Property objects
     *
     * The Property value getter allows for direct retrieval of values of child Property objects. To get the Property value of a
     * child Property object, the `propertyName` parameter should be of the format: "childName.propertyName". This pattern can
     * also be used to access nested properties - for example "childName1.childName2.childName3.propertyName".
     *
     * @subsection value_get_list_properties List properties
     *
     * If the requested Property is a list-type object, an item of the list at a selected index can be retrieved instead of the list
     * itself. To do so, add a [index] suffix to the `propertyName` parameter. For example "ListProperty[1]" retrieves the 2nd item
     * stored in the list Property value of the Property named "ListProperty".
     *
     * @subsection value_get_selection_properties Selection properties
     *
     * If the requested Property has the Selection values fields configured, the Property value getter returns an index/key of the
     * selected item in the Selection values list/dictionary.
     */
    virtual ErrCode INTERFACE_FUNC getPropertyValue(IString* propertyName, IBaseObject** value) = 0;

    /*!
     * @brief Gets the selected value of the Property, if the Property is a Selection property.
     * @param propertyName The name of the Property.
     * @param[out] value The selected value.
     * @retval OPENDAQ_ERR_NOTFOUND if a Property with given `propertyName` is not part of the Property object.
     * @retval OPENDAQ_ERR_INVALIDPROPERTY if the Property either has no Selection values, or the Selection values are not a list or dictionary.
     * @retval OPENDAQ_ERR_INVALIDTYPE if the retrieved value does not match the Property's item type.
     *
     * This function serves as a shortcut to obtaining the Property value of a Property, and using it to retrieve the currently
     * selected value from the Selection values of the Property. For example, if the Selection values contain the following list
     * "["banana", "apple", "pear"]", and the corresponding Property value is set to 1, retrieving the Property selection value
     * will return the string "apple".
     */
    virtual ErrCode INTERFACE_FUNC getPropertySelectionValue(IString* propertyName, IBaseObject** value) = 0;

    /*!
     * @brief Clears the Property value from the Property object
     * @param propertyName The name of the Property of which value should be cleared.
     * @retval OPENDAQ_ERR_NOTFOUND if a Property with given `propertyName` is not part of the Property object.
     * @retval OPENDAQ_ERR_FROZEN if the Property object is frozen.
     *
     * When a Property value is set, the value is written in the internal dictionary of Property values.
     * This function will remove said value from the dictionary. If the tries to obtain the Property value of
     * a property that does not have a set Property value, then the default value is returned.
     *
     * Importantly, clearing the value of an Object-type property will release the reference of the current
     * Property object value of the property. It will then create a new clone of the Default value and set it
     * as the value of the property.
     */
    virtual ErrCode INTERFACE_FUNC clearPropertyValue(IString* propertyName) = 0;

    /*!
     * @brief Checks if the Property object contains a property named `propertyName`.
     * @param propertyName The name of the property.
     * @param[out] hasProperty True if the Property object contains the Property; false otherwise.
     */
    virtual ErrCode INTERFACE_FUNC hasProperty(IString* propertyName, Bool* hasProperty) = 0;

    /*!
     * @brief Gets the Property with the given `propertyName`.
     * @param propertyName The name of the property.
     * @param[out] property The retrieved Property.
     * @retval OPENDAQ_ERR_NOTFOUND if the Property object does not contain the requested Property.
     *
     * The property is obtained from either the Property object class of the Property object, or the
     * object's local list of properties. The Property held by the object/class is not returned directly,
     * but is instead cloned, and bound to the Property object. This allows for evaluation of Property metadata
     * that depends on other properties of the Property object.
     *
     * For example, a Property's visibility might depend on the value of another Property. To make evaluation
     * of the visibility parameter possible, the Property must be able to access its owning Property object.
     *
     * Likewise, Reference properties always point at another Property of a Property object. Again, for this to
     * be possible, the Property must be able to access its owning Property object, to be able to retrieve its
     * referenced Property.
     */
    virtual ErrCode INTERFACE_FUNC getProperty(IString* propertyName, IProperty** property) = 0;

    /*!
     * @brief Adds the property to the Property object.
     * @param property The property to be added.
     * @retval OPENDAQ_ERR_INVALIDVALUE if the property has no name.
     * @retval OPENDAQ_ERR_ALREADYEXISTS if a property with the same name is already part of the Property object.
     * @retval OPENDAQ_ERR_FROZEN if the Property object is frozen.
     *
     * The Property is frozen once added to the Property object, making it immutable. The same Property cannot
     * be added to multiple different Property objects.
     */
    virtual ErrCode INTERFACE_FUNC addProperty(IProperty* property) = 0;

    /*!
     * @brief Removes the Property named `propertyName` from the Property object.
     * @param propertyName The name of the Property to be removed.
     * @retval OPENDAQ_ERR_NOTFOUND if the Property object does not contain a Property named `propertyName`, or the
     *         Property is part of the Property object's Property object class.
     * @retval OPENDAQ_ERR_FROZEN if the Property object is frozen.
     *
     * A property can only be removed from a Property object, if it was added to the object, and not inherited from its class.
     */
    virtual ErrCode INTERFACE_FUNC removeProperty(IString* propertyName) = 0;

    // [templateType(event, IPropertyObject, IPropertyValueEventArgs)]
    /*!
     * @brief Gets the Event that is triggered whenever a Property value is written to the Property named `propertyName`.
     * @param propertyName The name of the property.
     * @param[out] event The write Event.
     * @retval OPENDAQ_ERR_NOTFOUND if the Property object does not contain a Property named `propertyName`.
     *
     * A handler can be added to the event containing a callback function which is invoked whenever the event is triggered.
     * The callback function requires two parameters - a Property object, as well as a "Property value event args" object.
     * The callback will be invoked with the Property object holding the written-to property as the first argument The second argument
     * holds an event args object that contains the written value, event type (Update), and a method of overriding the written value.
     * If the written value is overridden, the overridden value is stored in the Property object instead.
     */
    virtual ErrCode INTERFACE_FUNC getOnPropertyValueWrite(IString* propertyName, IEvent** event) = 0;

    // [templateType(event, IPropertyObject, IPropertyValueEventArgs)]
    /*!
     * @brief Gets the Event that is triggered whenever a Property value of a Property named `propertyName` is read.
     * @param propertyName The name of the property.
     * @param[out] event The read Event.
     * @retval OPENDAQ_ERR_NOTFOUND if the Property object does not contain a Property named `propertyName`.
     *
     * A handler can be added to the event containing a callback function which is invoked whenever the event is triggered.
     * The callback function requires two parameters - a Property object, as well as a "Property value event args" object.
     * The callback will be invoked with the Property object holding the read value as the first argument. The second argument
     * holds an event args object that contains the read Property value, event type (Read), and a method of overriding the read value.
     * If the read value is overridden, the overridden value is read instead.
     */
    virtual ErrCode INTERFACE_FUNC getOnPropertyValueRead(IString* propertyName, IEvent** event) = 0;

    // [elementType(properties, IProperty)]
    /*!
     * @brief Returns a list of visible properties contained in the Property object.
     * @param[out] properties The List of properties.
     *
     * A Property is visible if both the Visible parameter is set to `true`, and IsReferenced is `false`. The properties
     * are sorted in insertion order unless a custom order is specified.
     *
     * This function returns both the properties added to the Property object, as well as those of its class.
     */
    virtual ErrCode INTERFACE_FUNC getVisibleProperties(IList** properties) = 0;

    // [elementType(properties, IProperty)]
    /*!
     * @brief Returns a list of all properties contained in the Property object.
     * @param[out] properties The List of properties.
     *
     * Properties are retrieved regardless of their visibility. They are sorted in insertion order unless a custom order is specified.
     *
     * This function returns both the properties added to the Property object, as well as those of its class.
     */
    virtual ErrCode INTERFACE_FUNC getAllProperties(IList** properties) = 0;

    // [elementType(orderedPropertyNames, IString)]
    /*!
     * @brief Sets a custom order of properties as defined in the list of property names.
     * @param orderedPropertyNames A list of names of properties. The order of the list is applied to the object's properties.
     * @retval OPENDAQ_ERR_FROZEN if the Property object is frozen.
     *
     * The list should contain names of properties available in the object. When retrieving the Property object's properties, they will
     * be sorted in the order in which the names appear in the provided list. Any properties not in the custom order are
     * kept in insertion order at the end of the Property object's list of properties.
     */
    virtual ErrCode INTERFACE_FUNC setPropertyOrder(IList* orderedPropertyNames) = 0;

    /*!
     * @brief Begins batch configuration of the object.
     *
     * Batched configuration is used to apply several settings at once. To begin batch configuration, call `beginUpdate`.
     * When the `setPropertyValue` is called on the object, the changes are not immediately applied to it. When `endUpdate`
     * is called, the property values set between the `beginUpdate` and `endUpdate` method calls are
     * applied. It triggers the ˙OnPropertyWriteEvent` for each property value set, and the `OnEndUpdate` event.
     *
     * `beginUpdate` is called recursively for each child property object.
     */
    virtual ErrCode INTERFACE_FUNC beginUpdate() = 0;

    /*!
     * @brief Ends batch configuration of the object.
     *
     * Batched configuration is used to apply several settings at once. To begin batch configuration, call `beginUpdate`.
     * When the `setPropertyValue` is called on the object, the changes are not immediately applied to it. When `endUpdate`
     * is called, the property values set between the `beginUpdate` and `endUpdate` method calls are
     * applied. It triggers the ˙OnPropertyWriteEvent` for each property value set, and the `OnEndUpdate` event.
     *
     * `endUpdate` is called recursively for each child property object.
     */
    virtual ErrCode INTERFACE_FUNC endUpdate() = 0;

    // [templateType(event, IPropertyObject, IEndUpdateEventArgs)]
    /*!
     * @brief Gets the Event that is triggered whenever the batch configuration is applied.
     * @param[out] event The Event.
     *
     * A handler can be added to the event containing a callback function which is invoked whenever the event is triggered.
     * The callback function requires one parameter - a "End update value event args" object.
     * The callback will be invoked with the batch configuration is applied, i.e. from the `endUpdate` method. The first argument
     * holds an event args object that contains a list of properties updated.
     */
    virtual ErrCode INTERFACE_FUNC getOnEndUpdate(IEvent** event) = 0;

    /*!
     * @brief Gets the permission manager of property object.
     * @param[out] permissionManager The permission manager of property object.
     */
    virtual ErrCode INTERFACE_FUNC getPermissionManager(IPermissionManager** permissionManager) = 0;
};

/*!@}*/

/*!
 * @addtogroup objects_property_object_obj_factories Factories
 * @{
 */

/*!
 * @brief Creates a empty Property object with no class.
 */
OPENDAQ_DECLARE_CLASS_FACTORY(LIBRARY_FACTORY, PropertyObject)

/*!
 * @brief Creates a Property object that inherits the properties of a class added to the Type manager with the specified name.
 * @param manager The Type manager manager.
 * @param className The name of the class from which the Property object inherits its properties.
 */
OPENDAQ_DECLARE_CLASS_FACTORY_WITH_INTERFACE(
    LIBRARY_FACTORY, PropertyObjectWithClassAndManager,
    IPropertyObject,
    ITypeManager*, manager,
    IString*, className
)


/*!@}*/

END_NAMESPACE_OPENDAQ
