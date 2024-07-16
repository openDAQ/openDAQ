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
#include <coretypes/stringobject.h>

BEGIN_NAMESPACE_OPENDAQ

/*!
 * @ingroup opendaq_utility
 * @addtogroup opendaq_tags Tags private
 * @{
 */

/*!
 * @brief Private interface to component tags. Allows for adding/removing tags.
 *
 * Modifying the tags of a component might have unintended sideffects and should in most cases only be done
 * by the component owner module.
 */
DECLARE_OPENDAQ_INTERFACE(ITagsPrivate, IBaseObject)
{
    /*!
     * @brief Adds a new tag to the list.
     * @param name The name of the tag to be added.
     * @retval OPENDAQ_IGNORED if a node with the `name` is already in the list of tags.
     */
    virtual ErrCode INTERFACE_FUNC add(IString* name) = 0;

    /*!
     * @brief Removes a new tag from the list.
     * @param name The name of the tag to be removed.
     * @retval OPENDAQ_IGNORED if a node with the `name` is not in the list of tags.
     */
    virtual ErrCode INTERFACE_FUNC remove(IString* name) = 0;

    // [templateType(tags, IString)]
    /*!
     * @brief Replaces all tags.
     * @param tags The new list of tags.
     */
    virtual ErrCode INTERFACE_FUNC replace(IList* tags) = 0;
};

/*!@}*/

END_NAMESPACE_OPENDAQ
