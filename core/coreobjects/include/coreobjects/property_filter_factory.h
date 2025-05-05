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
#include <coreobjects/property_filter_ptr.h>

BEGIN_NAMESPACE_OPENDAQ_SEARCH

namespace properties
{

/*!
 * @ingroup objects_property_filter
 * @addtogroup objects_property_filter_factories Factories
 * @{
 */

/*!
 * @brief Creates a search filter that accepts only visible properties.
 */
inline PropertyFilterPtr Visible()
{
    PropertyFilterPtr obj(VisiblePropertyFilter_Create());
    return obj;
}

/*!
 * @brief Creates a search filter that accepts only read-only properties.
 */
inline PropertyFilterPtr ReadOnly()
{
    PropertyFilterPtr obj(ReadOnlyPropertyFilter_Create());
    return obj;
}

/*!
 * @brief Creates a search filter that accepts properties of the specified type.
 * @param type The type of accepted properties.
 */
inline PropertyFilterPtr Type(const CoreType& type)
{
    PropertyFilterPtr obj(TypePropertyFilter_Create(type));
    return obj;
}

/*!
 * @brief Creates a search filter that accepts properties with the specified name.
 * @param name The name of the accepted properties.
 */
inline PropertyFilterPtr Name(const StringPtr& name)
{
    PropertyFilterPtr obj(NamePropertyFilter_Create(name));
    return obj;
}

/*!
 * @brief Creates a search filter that accepts all properties.
 */
inline PropertyFilterPtr Any()
{
    PropertyFilterPtr obj(AnyPropertyFilter_Create());
    return obj;
}

/*!
 * @brief Creates a "conjunction" search filter that combines 2 filters, accepting a property only if both filters accept it.
 * @param left The first argument of the conjunction operation.
 * @param right The second argument of the conjunction operation.
 */
inline PropertyFilterPtr And(const PropertyFilterPtr& left, const PropertyFilterPtr& right)
{
    PropertyFilterPtr obj(AndPropertyFilter_Create(left, right));
    return obj;
}

/*!
 * @brief Creates a "disjunction" search filter that combines 2 filters, accepting a property if any of the two filters accepts it.
 * @param left The first argument of the disjunction operation.
 * @param right The second argument of the disjunction operation.
 */
inline PropertyFilterPtr Or(const PropertyFilterPtr& left, const PropertyFilterPtr& right)
{
    PropertyFilterPtr obj(OrPropertyFilter_Create(left, right));
    return obj;
}

/*!
 * @brief Creates a search filter that negates the "accepts property" result of the filter provided as construction argument.
 * @param filter The filter of which results should be negated.
 */
inline PropertyFilterPtr Not(const PropertyFilterPtr& filter)
{
    PropertyFilterPtr obj(NotPropertyFilter_Create(filter));
    return obj;
}

/*!
 * @brief Creates a custom search filter with a user-defined "accepts" function.
 * @param acceptsFunction The function to be called when "accepts property" is called. Should return `true` or `false`.
 */
inline PropertyFilterPtr Custom(const FunctionPtr& acceptsFunction)
{
    PropertyFilterPtr obj(CustomPropertyFilter_Create(acceptsFunction));
    return obj;
}

///*!
// * @brief Creates a search filter that indicates that the property search method should recursively called for the nested object-properties.
// * This filter constructor should always be the final filter wrapper, and should not be used as a constructor argument
// * for another filter.
// * @param filter The filter to be wrapped with a "recursive" flag.
// */
//inline PropertyFilterPtr Recursive(const PropertyFilterPtr& filter)
//{
//    PropertyFilterPtr obj(RecursivePropertyFilter_Create(filter));
//    return obj;
//}

/*!@}*/

}

END_NAMESPACE_OPENDAQ_SEARCH
