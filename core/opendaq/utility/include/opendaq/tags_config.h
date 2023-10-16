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
#include <opendaq/tags.h>

BEGIN_NAMESPACE_OPENDAQ

/*!
 * @ingroup opendaq_utility
 * @addtogroup opendaq_tags Tags
 * @{
 */

/*#
 * [interfaceSmartPtr(ITags, GenericTagsPtr)]
 */

/*!
 * The config component of Tags objets. Allows users to add and remove tags from the list of tags.
 *
 * The implementation of `config` also implements the `freeze` function that freezes the object making it
 * immutable. Once frozen, all method calls fail as the list of tags can no longer be modified.
 */
DECLARE_OPENDAQ_INTERFACE(ITagsConfig, ITags)
{
    /*!
     * @brief Adds a new tag to the list.
     * @param name The name of the tag to be added.
     * @retval OPENDAQ_ERR_DUPLICATEITEM if a node with the `name` is already in the list of tags.
     */
    virtual ErrCode INTERFACE_FUNC add(IString* name) = 0;
    /*!
     * @brief Removes a new tag from the list.
     * @param name The name of the tag to be removed.
     * @retval OPENDAQ_ERR_NOTFOUND if a node with the `name` is not in the list of tags.
     */
    virtual ErrCode INTERFACE_FUNC remove(IString* name) = 0;
};
/*!@}*/

OPENDAQ_DECLARE_CLASS_FACTORY_WITH_INTERFACE(LIBRARY_FACTORY, Tags, ITagsConfig)
OPENDAQ_DECLARE_CLASS_FACTORY_WITH_INTERFACE(
    LIBRARY_FACTORY, TagsFromExisting, ITagsConfig,
    ITags*, tagsToCopy
)

END_NAMESPACE_OPENDAQ
