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
#include <coretypes/function.h>
#include <coretypes/listobject.h>
#include <coretypes/stringobject.h>

BEGIN_NAMESPACE_OPENDAQ

/*!
 * @ingroup opendaq_utility
 * @addtogroup opendaq_tags Tags
 * @{
 */

/*!
 * @brief List of string tags. Provides helpers to get and search the list of tags.
 *
 * Tags provide a view into an underlying list of tags. The list can be retrieved via
 * `getList`, and inspected through `contains` and `query`.
 *
 * To manipulate the list of tags, the add/remove tag functions can be used. The Tags
 * object can only be modified if the object is not locked by the owning Component.
 */
DECLARE_OPENDAQ_INTERFACE(ITags, IBaseObject)
{
    // [elementType(value, IString)]
    /*!
     * @brief Gets the list of all tags in the list.
     * @param[out] value The list of tag strings.
     */
    virtual ErrCode INTERFACE_FUNC getList(IList** value) = 0;
    /*!
     * @brief Checks whether a tag is present in the list of tags.
     * @param name The name of the tag being checked.
     * @param[out] value True if a tag is found; false otherwise.
     */
    virtual ErrCode INTERFACE_FUNC contains(IString* name, Bool* value) = 0;
    /*!
     * @brief Queries the list of tags, creating an EvalValue expression from the `query` string.
     * Returns true if the list of tags matches the query, false otherwise.
     * @param query The query string. I.e. "tag1 || (tag2 && tag3)"
     * @param[out] value The result of the query
     */
    virtual ErrCode INTERFACE_FUNC query(IString* query, Bool* value) = 0;
};
/*!@}*/


OPENDAQ_DECLARE_CLASS_FACTORY_WITH_INTERFACE(LIBRARY_FACTORY, Tags, ITags)

END_NAMESPACE_OPENDAQ
