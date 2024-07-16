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
#include <coretypes/listobject.h>
#include <coreobjects/property.h>
#include <coretypes/type_ptr.h>

BEGIN_NAMESPACE_OPENDAQ

struct IPropertyObjectClassBuilder;

/*!
 * @ingroup objects_property_object
 * @addtogroup objects_property_object_class PropertyObjectClass
 * @{
 */

/*!
 * @brief Container of properties that can be used as a base class when instantiating a Property object.
 *
 * A Property object class is defined via a name and a list of properties. For a Property object to be
 * created, using a class as its base, a class must be added to the Type manager.
 *
 * The name of the class must be unique within a given Type manager instance. The name of the Class
 * is used when choosing a template class for a Property object, as well as to define a class hierarchy.
 * A class with the Parent name configured will inherit the properties of the class with said name.
 *
 * The properties of a Property object class are, by default, sorted in insertion order. The order can,
 * however, be overridden by specifying a Property object order - a list containing the names of properties.
 * If specified, when retrieving the list of properties, they will be in the provided order.
 *
 * All Property object class objects are created as Property object class builder objects that allow for
 * customization and building of the class.
 */

/*#
 * [interfaceSmartPtr(IType, GenericTypePtr, "<coretypes/type_ptr.h>")]
 */
DECLARE_OPENDAQ_INTERFACE(IPropertyObjectClass, IType)
{
    /*!
     * @brief Gets the name of the parent of the property class.
     * @param[out] parentName The parent class's name.
     */
    virtual ErrCode INTERFACE_FUNC getParentName(IString** parentName) = 0;

    /*!
     * @brief Gets the class's property with the given name.
     * @param propertyName The property's name.
     * @param[out] property The property.
     * @retval OPENDAQ_ERR_NOTFOUND if the Property with name `propertyName` is not added to the class.
     * @retval OPENDAQ_ERR_MANAGER_NOT_ASSIGNED if the parent name is set, but the Type manager is not available.
     */
    virtual ErrCode INTERFACE_FUNC getProperty(IString* propertyName, IProperty** property) = 0;

    /*!
     * @brief Checks if the property is registered.
     * @param propertyName The property's name.
     * @param[out] hasProperty True if the property is registered, false otherwise.
     * @retval OPENDAQ_ERR_MANAGER_NOT_ASSIGNED if the parent name is set, but the Type manager is not available.
     */
    virtual ErrCode INTERFACE_FUNC hasProperty(IString* propertyName, Bool* hasProperty) = 0;

    // [elementType(properties, IProperty)]
    /*!
     * @brief Gets the list of properties added to the class.
     * @param includeInherited If true, the returned list of properties also includes the properties of the class's ancestors.
     * @param[out] properties The list of properties.
     * @retval OPENDAQ_ERR_MANAGER_NOT_ASSIGNED if the parent name is set, but the Type manager is not available.
     *
     * The properties are sorted in insertion order, unless a custom sorting order is specified for the class. Any properties
     * not listed in the custom sorting order are listed at the end of the properties list, sorted in insertion order.
     */
    virtual ErrCode INTERFACE_FUNC getProperties(Bool includeInherited, IList** properties) = 0;
};

/*!@}*/

/*!
 * @brief Creates a PropertyObjectClass using Builder
 * @param builder PropertyObjectClass Builder
 */
OPENDAQ_DECLARE_CLASS_FACTORY_WITH_INTERFACE(
    LIBRARY_FACTORY, PropertyObjectClassFromBuilder, IPropertyObjectClass,
    IPropertyObjectClassBuilder*, builder
)

END_NAMESPACE_OPENDAQ
