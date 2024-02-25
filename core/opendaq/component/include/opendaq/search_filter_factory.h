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
#include <opendaq/search_filter_ptr.h>

BEGIN_NAMESPACE_OPENDAQ_SEARCH

/*!
 * @ingroup opendaq_components_search_filter
 * @addtogroup opendaq_components_search_filter_factories Factories
 * @{
 */

/*!
 * @brief Creates a search filter that accepts only visible components. "Visit children" returns `true`
 * only if the component being evaluated is visible.
 */
inline SearchFilterPtr Visible()
{
    SearchFilterPtr obj(VisibleSearchFilter_Create());
    return obj;
}

/*!
 * @brief Creates a search filter that accepts components that have all the required tags. "Visit children"
 * always returns `true`.
 * @param requiredTags A list of strings containing the tags that a component must have to be accepted.
 */
inline SearchFilterPtr RequireTags(const ListPtr<IString>& requiredTags)
{
    SearchFilterPtr obj(RequiredTagsSearchFilter_Create(requiredTags));
    return obj;
}

/*!
 * @brief Creates a search filter that accepts components that do not have any of the excluded tags. "Visit children"
 * always returns `true`.
 * @param excludedTags A list of strings containing the tags that a component must not have to be accepted.
 */
inline SearchFilterPtr ExcludeTags(const ListPtr<IString>& excludedTags)
{
    SearchFilterPtr obj(ExcludedTagsSearchFilter_Create(excludedTags));
    return obj;
}

/*!
 * @brief Creates a search filter that accepts components that implement the interface with the given interface ID. "Visit children"
 * always returns `true`.
 * @param intfId The interface ID that should be implemented by accepted components.
 */
inline SearchFilterPtr InterfaceId(const IntfID& intfId)
{
    SearchFilterPtr obj(InterfaceIdSearchFilter_Create(intfId));
    return obj;
}

/*!
 * @brief Creates a search filter that accepts components with the specified local ID. "Visit children"
 * always returns `true`.
 * @param localId The local ID of the accepted components.
 */
inline SearchFilterPtr LocalId(const StringPtr& localId)
{
    SearchFilterPtr obj(LocalIdSearchFilter_Create(localId));
    return obj;
}

/*!
 * @brief Creates a search filter that accepts all components. "Visit children" always returns `true`.
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
 * @brief Creates a "disjunction" search filter that combines 2 filters, accepting a component if any of the two filters accepts it.
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
 * @brief Creates a search filter that negates the "accepts component" result of the filter provided as construction argument.
 * Does not negate the "visit children" result.
 * @param filter The filter of which results should be negated.
 */
inline SearchFilterPtr Not(const SearchFilterPtr& filter)
{
    SearchFilterPtr obj(NotSearchFilter_Create(filter));
    return obj;
}

/*!
 * @brief Creates a custom search filter with a user-defined "accepts" and "visit children" function.
 * @param acceptsFunction The function to be called when "accepts component" is called. Should return `true` or `false`.
 * @param visitFunction The function to be called when "visit children" is called. Should return `true` or `false`.
 */
inline SearchFilterPtr Custom(const FunctionPtr& acceptsFunction, const FunctionPtr& visitFunction = nullptr)
{
    SearchFilterPtr obj(CustomSearchFilter_Create(acceptsFunction, visitFunction));
    return obj;
}

/*!
 * @brief Creates a search filter that indicates that the tree traversal method should recursively search the tree.
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
