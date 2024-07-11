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
#include <coreobjects/property_object_class.h>
#include <coretypes/type_manager.h>

BEGIN_NAMESPACE_OPENDAQ

/*!
 * @ingroup objects_property_object
 * @addtogroup objects_property_object_class PropertyObjectClassConfig
 * @{
 */

/*#
 * [interfaceLibrary(ITypeManager, CoreTypes)]
 * [interfaceSmartPtr(IWeakRef, WeakRefPtr, "<coretypes/weakrefptr.h>")]
 */
/*!
 * @brief The builder interface of Property object classes. Allows for their modification and building of
 * Property object classes.
 *
 * The configuration interface allows for modifying the list of properties, the class's name, parent, and the
 * sorting order of properties. To build the Class, the `build` method is used.
 */
DECLARE_OPENDAQ_INTERFACE(IPropertyObjectClassBuilder, IBaseObject)
{
    /*!
     * @brief Builds and returns a Property object class using the currently set values of the Builder.
     * @param[out] propertyObjectClass The built Property object class.
     */
    virtual ErrCode INTERFACE_FUNC build(IPropertyObjectClass** propertyObjectClass) = 0;

    // [returnSelf]
    /*!
     * @brief Sets the name of the property class.
     * @param className The name of the class.
     */
    virtual ErrCode INTERFACE_FUNC setName(IString* className) = 0;

    /*!
     * @brief Gets the name of the property class.
     * @param[out] className The name of the class.
     */
    virtual ErrCode INTERFACE_FUNC getName(IString** className) = 0;
    
    // [returnSelf]
    /*!
     * @brief Gets the name of the parent of the property class.
     * @param parentName The parent class's name.
     */
    virtual ErrCode INTERFACE_FUNC setParentName(IString* parentName) = 0;

    /*!
     * @brief Gets the name of the parent of the property class.
     * @param[out] parentName The parent class's name.
     */
    virtual ErrCode INTERFACE_FUNC getParentName(IString** parentName) = 0;
    
    // [returnSelf]
    /*!
     * @brief Adds a property to the class.
     * @param property The property to be added.
     * @retval OPENDAQ_ERR_ALREADYEXISTS if a property with the same name already added to the class.
     * @retval OPENDAQ_ERR_INVALIDTYPE if the property is an object type and is not atomic.
     *
     * The default value of object-type properties that are added to a class are frozen once added.
     */
    virtual ErrCode INTERFACE_FUNC addProperty(IProperty* property) = 0;

    // [templateType(properties, IString, IProperty)]
    /*!
     * @brief Gets the dictionary of properties
     * @param[out] properties dictionary of properties
     */
    virtual ErrCode INTERFACE_FUNC getProperties(IDict** properties) = 0;
    
    // [returnSelf]
    /*!
     * @brief Removes a property with the given name from the class.
     * @param propertyName The name of the property to be removed.
     * @retval OPENDAQ_ERR_NOTFOUND if the property with `propertyName` is not a member of the class.
     */
    virtual ErrCode INTERFACE_FUNC removeProperty(IString* propertyName) = 0;

    // [elementType(orderedPropertyNames, IString), returnSelf]
    /*!
     * @brief Sets a custom order of properties as defined in the list of property names.
     * @param orderedPropertyNames A list of names of properties. The order of the list is applied to the class's properties.
     *
     * The list should contain names of properties available in the class. When retrieving the class's properties, they will
     * be sorted in the order in which the names appear in the provided list. Any class properties not in the custom order are
     * kept in insertion order at the end of the class's list of properties.
     */
    virtual ErrCode INTERFACE_FUNC setPropertyOrder(IList* orderedPropertyNames) = 0;

    // [elementType(orderedPropertyNames, IString)]
    /*!
     * @brief Gets a custom order of properties as defined in the list of property names.
     * @param[out] orderedPropertyNames A list of names of properties. The order of the list is applied to the class's properties.
     */
    virtual ErrCode INTERFACE_FUNC getPropertyOrder(IList** orderedPropertyNames) = 0;

    /*!
     * @brief Gets a type manager
     * @param[out] manager a type manager
     */
    virtual ErrCode INTERFACE_FUNC getManager(ITypeManager** manager) = 0;
};

/*!@}*/

/*!
 * @addtogroup objects_property_object_class_factories Factories
 * @{
 */

/*!
 * @brief Creates a property object class configuration object with a given name.
 * @param name The name of the class.
 */
OPENDAQ_DECLARE_CLASS_FACTORY_WITH_INTERFACE(
    LIBRARY_FACTORY, PropertyObjectClassBuilder, IPropertyObjectClassBuilder,
    IString*, name
)

/*!
 * @brief Creates a Property object class configuration object with a given name, and a reference to the Type manager.
 * @param manager The Property object class manager object.
 * @param name The name of the class.
 */
OPENDAQ_DECLARE_CLASS_FACTORY_WITH_INTERFACE(
    LIBRARY_FACTORY, PropertyObjectClassBuilderWithManager, IPropertyObjectClassBuilder,
    ITypeManager*, manager,
    IString*, name
)

/*!@}*/

END_NAMESPACE_OPENDAQ
