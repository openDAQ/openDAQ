/*
 * Copyright 2022-2024 Blueberry d.o.o.
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

#include <opendaq/component.h>
#include <opendaq/search_filter.h>

BEGIN_NAMESPACE_OPENDAQ

/*!
 * @ingroup opendaq_components
 * @addtogroup opendaq_folder Folder
 * @{
 */

/*#
 * [interfaceSmartPtr(IComponent, GenericComponentPtr, "<opendaq/component_ptr.h>")]
 * [templated(defaultAliasName: FolderPtr)]
 * [interfaceSmartPtr(IFolder, GenericFolderPtr)]
 */


/*!
 * @brief Acts as a container for other components
 *
 * Other components use the folder component to organize the children components, 
 * such as channels, signals, function blocks, etc.
 */
DECLARE_OPENDAQ_INTERFACE(IFolder, IComponent)
{
    // [elementType(items, IComponent)]
    /*!
     * @brief Gets the list of the items in the folder.
     * @param searchFilter Provides an optional filter that filters out unwanted components and allows for recursion.
     * @param[out] items The list of the items.
     *
     * If searchFilter is not provided, the returned list contains only immediate children with visible set to `true`.
     */
    virtual ErrCode INTERFACE_FUNC getItems(IList** items, ISearchFilter* searchFilter = nullptr) = 0;

    /*!
     * @brief Returns True if the folder is empty.
     * @param[out] empty True if the folder is empty.
     */
    virtual ErrCode INTERFACE_FUNC isEmpty(Bool* empty) = 0;

    /*!
     * @brief Returns True if the folder has an item with local ID.
     * @param localId The local ID of the item.
     * @param[out] value True if the folder contains item with local ID.
     */
    virtual ErrCode INTERFACE_FUNC hasItem(IString* localId, Bool* value) = 0;

    // [templateType(item, IComponent)]
    /*!
     * @brief Gets the item component with the specified localId.
     * @param localId The local id of the child component.
     * @param[out] item The item component.
     * @retval OPENDAQ_SUCCESS if succeeded.
     * @retval OPENDAQ_NOT_FOUND if folder with the specified ID not found.
     */
    virtual ErrCode INTERFACE_FUNC getItem(IString* localId, IComponent** item) = 0;
};
/*!@}*/

END_NAMESPACE_OPENDAQ
