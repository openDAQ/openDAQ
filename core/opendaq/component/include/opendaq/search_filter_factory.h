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

inline SearchFilterPtr Visible()
{
    SearchFilterPtr obj(VisibleSearchFilter_Create());
    return obj;
}

inline SearchFilterPtr RequireTags(const ListPtr<IString>& requiredTags)
{
    SearchFilterPtr obj(RequiredTagsSearchFilter_Create(requiredTags));
    return obj;
}

inline SearchFilterPtr ExcludeTags(const ListPtr<IString>& excludedTags)
{
    SearchFilterPtr obj(ExcludedTagsSearchFilter_Create(excludedTags));
    return obj;
}

inline SearchFilterPtr SearchId(const IntfID searchId)
{
    SearchFilterPtr obj(SearchIdSearchFilter_Create(searchId));
    return obj;
}

inline SearchFilterPtr Any()
{
    SearchFilterPtr obj(AnySearchFilter_Create());
    return obj;
}

inline SearchFilterPtr And(const SearchFilterPtr& left, const SearchFilterPtr& right)
{
    SearchFilterPtr obj(AndSearchFilter_Create(left, right));
    return obj;
}

inline SearchFilterPtr Or(const SearchFilterPtr& left, const SearchFilterPtr& right)
{
    SearchFilterPtr obj(OrSearchFilter_Create(left, right));
    return obj;
}

inline SearchFilterPtr Not(const SearchFilterPtr& filter)
{
    SearchFilterPtr obj(NotSearchFilter_Create(filter));
    return obj;
}

inline SearchFilterPtr Custom(const FunctionPtr& acceptsFunction, const FunctionPtr& visitFunction)
{
    SearchFilterPtr obj(CustomSearchFilter_Create(acceptsFunction, visitFunction));
    return obj;
}

inline SearchFilterPtr Recursive(const SearchFilterPtr& filter)
{
    SearchFilterPtr obj(RecursiveSearchFilter_Create(filter));
    return obj;
}

END_NAMESPACE_OPENDAQ_SEARCH
