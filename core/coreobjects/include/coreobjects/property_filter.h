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

#pragma once
#include <coreobjects/property.h>

BEGIN_NAMESPACE_OPENDAQ

/*!
 * @ingroup objects_property
 * @addtogroup objects_property_filter Property filter
 * @{
 */

/*!
 * @brief Property search filter that should be passed to property search function of property object to filter
 * out unwanted results.
 *
 * Each filter defines an "accepts property" function.
 *
 * Accepts property defines whether or not the property being evaluated as part of a search method should be included
 * in the resulting output.
 *
 */
DECLARE_OPENDAQ_INTERFACE(IPropertyFilter, IBaseObject)
{
    /*!
     * @brief Defines whether or not the property should be included in the search results
     * @param property The property being evaluated.
     * @param[out] accepts True of the property is to be included in the results; false otherwise.
     */
    virtual ErrCode INTERFACE_FUNC acceptsProperty(IProperty* property, Bool* accepts) = 0;
};

/*!@}*/

/*!
 * @ingroup objects_property_filter
 * @addtogroup objects_property_filter_factories Factories
 * @{
 */

/*!
 * @brief Creates a search filter that accepts only visible properties.
 */
OPENDAQ_DECLARE_CLASS_FACTORY_WITH_INTERFACE(LIBRARY_FACTORY, VisiblePropertyFilter, IPropertyFilter)

/*!
 * @brief Creates a search filter that accepts only read-only properties.
 */
OPENDAQ_DECLARE_CLASS_FACTORY_WITH_INTERFACE(LIBRARY_FACTORY, ReadOnlyPropertyFilter, IPropertyFilter)

/*!
 * @brief Creates a search filter that accepts properties of the specified type.
 * @param type The type of accepted properties.
 */
OPENDAQ_DECLARE_CLASS_FACTORY_WITH_INTERFACE(LIBRARY_FACTORY, TypePropertyFilter, IPropertyFilter, const CoreType&, type)

/*!
 * @brief Creates a search filter that accepts properties with the specified name.
 * @param name The name of the accepted properties.
 */
OPENDAQ_DECLARE_CLASS_FACTORY_WITH_INTERFACE(LIBRARY_FACTORY, NamePropertyFilter, IPropertyFilter, IString*, name)

/*!
 * @brief Creates a search filter that accepts all properties.
 */
OPENDAQ_DECLARE_CLASS_FACTORY_WITH_INTERFACE(LIBRARY_FACTORY, AnyPropertyFilter, IPropertyFilter)

/*!
 * @brief Creates a "conjunction" search filter that combines 2 filters, accepting a property only if both filters accept it.
 * @param left The first argument of the conjunction operation.
 * @param right The second argument of the conjunction operation.
 */
OPENDAQ_DECLARE_CLASS_FACTORY_WITH_INTERFACE(LIBRARY_FACTORY, AndPropertyFilter, IPropertyFilter, IPropertyFilter*, left, IPropertyFilter*, right)

/*!
 * @brief Creates a "disjunction" search filter that combines 2 filters, accepting a property if any of the two filters accepts it.
 * @param left The first argument of the disjunction operation.
 * @param right The second argument of the disjunction operation.
 */
OPENDAQ_DECLARE_CLASS_FACTORY_WITH_INTERFACE(LIBRARY_FACTORY, OrPropertyFilter, IPropertyFilter, IPropertyFilter*, left, IPropertyFilter*, right)

/*!
 * @brief Creates a search filter that negates the "accepts property" result of the filter provided as construction argument.
 * @param filter The filter of which results should be negated.
 */
OPENDAQ_DECLARE_CLASS_FACTORY_WITH_INTERFACE(LIBRARY_FACTORY, NotPropertyFilter, IPropertyFilter, IPropertyFilter*, filter)

/*!
 * @brief Creates a custom search filter with a user-defined "accepts" function.
 * @param acceptsFunction The function to be called when "accepts property" is called. Should return `true` or `false`.
 */
OPENDAQ_DECLARE_CLASS_FACTORY_WITH_INTERFACE(LIBRARY_FACTORY, CustomPropertyFilter, IPropertyFilter, IFunction*, acceptsFunction)

/*!
 * @brief Creates a search filter that indicates that the property search method should recursively called for the nested object-properties.
 * This filter constructor should always be the final filter wrapper, and should not be used as a constructor argument
 * for another filter.
 * @param filter The filter to be wrapped with a "recursive" flag.
 */
OPENDAQ_DECLARE_CLASS_FACTORY_WITH_INTERFACE(LIBRARY_FACTORY, RecursivePropertyFilter, IPropertyFilter, IPropertyFilter*, filter)

/*!@}*/

END_NAMESPACE_OPENDAQ
