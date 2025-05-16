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
#include <coretypes/search_filter_factory.h>
#include <coreobjects/search_filter.h>
#include <coretypes/string_ptr.h>

BEGIN_NAMESPACE_OPENDAQ_SEARCH

namespace properties
{

/*!
 * @ingroup objects_property_search_filter
 * @addtogroup objects_property_search_filter_factories Factories
 * @{
 */

/*!
 * @brief Creates a search filter that accepts only visible properties. "Visit children" returns `true`
 * only if the property being evaluated is visible.
 */
inline SearchFilterPtr Visible()
{
    SearchFilterPtr obj(VisiblePropertyFilter_Create());
    return obj;
}

/*!
 * @brief Creates a search filter that accepts only read-only properties.
 */
inline SearchFilterPtr ReadOnly()
{
    SearchFilterPtr obj(ReadOnlyPropertyFilter_Create());
    return obj;
}

/*!
 * @brief Creates a search filter that accepts properties whose names match the specified pattern.
 * @param regex A regular expression pattern used to match property names.
 */
inline SearchFilterPtr Name(const StringPtr& regex)
{
    SearchFilterPtr obj(NamePropertyFilter_Create(regex));
    return obj;
}

/*!@}*/

}

END_NAMESPACE_OPENDAQ_SEARCH
