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

#include <coretypes/search_filter.h>
#include <coretypes/stringobject.h>
#include <coretypes/listobject.h>

BEGIN_NAMESPACE_OPENDAQ

/*!
 * @ingroup objects_property
 * @addtogroup objects_property_filter Property filter
 * @{
 */

/*!
 * @ingroup objects_property_filter
 * @addtogroup objects_property_filter_factories Factories
 * @{
 */

/*!
 * @brief Creates a search filter that accepts only visible properties.
 */
OPENDAQ_DECLARE_CLASS_FACTORY_WITH_INTERFACE(LIBRARY_FACTORY, VisiblePropertyFilter, ISearchFilter)

/*!
 * @brief Creates a search filter that accepts only read-only properties.
 */
OPENDAQ_DECLARE_CLASS_FACTORY_WITH_INTERFACE(LIBRARY_FACTORY, ReadOnlyPropertyFilter, ISearchFilter)

/*!
 * @brief Creates a search filter that accepts properties with the specified name.
 * @param name The name of the accepted properties.
 */
OPENDAQ_DECLARE_CLASS_FACTORY_WITH_INTERFACE(LIBRARY_FACTORY, NamePropertyFilter, ISearchFilter, IString*, name)

/*!@}*/

END_NAMESPACE_OPENDAQ
