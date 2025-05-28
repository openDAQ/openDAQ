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
#include <coretypes/search_filter_ptr.h>
#include <coretypes/function_ptr.h>

BEGIN_NAMESPACE_OPENDAQ_SEARCH

/*!
 * @ingroup types_search_filter
 * @addtogroup types_search_filter_factories Factories
 * @{
 */

/*!
 * @brief Creates a search filter that accepts all objects. "Visit children" always returns `true`.
 */
inline SearchFilterPtr Any()
{
    SearchFilterPtr obj(AnySearchFilter_Create());
    return obj;
}

/*!
 * @brief Creates a "conjunction" search filter that combines 2 filters, accepting a component only if both filters accept it.
 * "Visit children" returns `true` only if both filters do so.
 * @param left The first argument of the conjunction operation.
 * @param right The second argument of the conjunction operation.
 */
inline SearchFilterPtr And(const SearchFilterPtr& left, const SearchFilterPtr& right)
{
    SearchFilterPtr obj(AndSearchFilter_Create(left, right));
    return obj;
}

/*!
 * @brief Creates a "disjunction" search filter that combines 2 filters, accepting an object if any of the two filters accepts it.
 * "Visit children" returns `true` if any of the two filters accepts does so.
 * @param left The first argument of the disjunction operation.
 * @param right The second argument of the disjunction operation.
 */
inline SearchFilterPtr Or(const SearchFilterPtr& left, const SearchFilterPtr& right)
{
    SearchFilterPtr obj(OrSearchFilter_Create(left, right));
    return obj;
}

/*!
 * @brief Creates a search filter that negates the "accepts object" result of the filter provided as construction argument.
 * Does not negate the "visit children" result.
 * @param filter The filter of which results should be negated.
 */
inline SearchFilterPtr Not(const SearchFilterPtr& filter)
{
    SearchFilterPtr obj(NotSearchFilter_Create(filter));
    return obj;
}

/*!
 * @brief Creates a custom search filter with a user-defined "accepts object" and "visit children" function.
 * @param acceptsFunction The function to be called when "accepts component" is called. Should return `true` or `false`.
 * @param visitFunction The function to be called when "visit children" is called. Should return `true` or `false`.
 */
inline SearchFilterPtr Custom(const FunctionPtr& acceptsFunction, const FunctionPtr& visitFunction = nullptr)
{
    SearchFilterPtr obj(CustomSearchFilter_Create(acceptsFunction, visitFunction));
    return obj;
}

/*!
 * @brief Creates a search filter that indicates that the search method should recursively search through the object's child elements.
 * This filter constructor should always be the final filter wrapper, and should not be used as a constructor argument
 * for another filter.
 * @param filter The filter to be wrapped with a "recursive" flag.
 */
inline SearchFilterPtr Recursive(const SearchFilterPtr& filter)
{
    SearchFilterPtr obj(RecursiveSearchFilter_Create(filter));
    return obj;
}

/*!@}*/

END_NAMESPACE_OPENDAQ_SEARCH
