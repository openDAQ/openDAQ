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
#include <coreobjects/search_filter_factory.h>
#include <opendaq/search_filter.h>

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

/*!@}*/

END_NAMESPACE_OPENDAQ_SEARCH
