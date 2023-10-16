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

#include <opendaq/folder.h>

BEGIN_NAMESPACE_OPENDAQ

/*!
 * @ingroup opendaq_components
 * @addtogroup opendaq_component Folder
 * @{
 */

/*#
 * [interfaceSmartPtr(IFolder, GenericFolderPtr, "<opendaq/folder_ptr.h>")]
 * [templated(defaultAliasName: FolderConfigPtr)]
 * [interfaceSmartPtr(IFolderConfig, GenericFolderConfigPtr)]
 */

/*!
 * @brief Allows write access to folder.
 *
 * Provides methods to add and remove items to the folder.
 */
DECLARE_OPENDAQ_INTERFACE(IFolderConfig, IFolder)
{
    /*!
     * @brief Adds a component to the folder.
     * @param item The component.
     */
    virtual ErrCode INTERFACE_FUNC addItem(IComponent* item) = 0;

    /*!
     * @brief Removes the item from the folder.
     * @param item The item component.
     */
    virtual ErrCode INTERFACE_FUNC removeItem(IComponent* item) = 0;

    /*!
     * @brief Removes the item from the folder using local id of the component.
     * @param localId The local id of the component.
     */
    virtual ErrCode INTERFACE_FUNC removeItemWithLocalId(IString* localId) = 0;

    /*!
     * @brief Removes all items from the folder.
     */
    virtual ErrCode INTERFACE_FUNC clear() = 0;

    /*!
     * @brief Sets the name of the folder.
     * @param name The name of the folder.
     *
     * If the name is not set, the local ID is used as default value.
     */
    virtual ErrCode INTERFACE_FUNC setName(IString* name) = 0;
};
/*!@}*/

OPENDAQ_DECLARE_CLASS_FACTORY_WITH_INTERFACE(
    LIBRARY_FACTORY, Folder, IFolderConfig,
    IContext*, context,
    IComponent*, parent,
    IString*, localId
)

OPENDAQ_DECLARE_CLASS_FACTORY_WITH_INTERFACE(
    LIBRARY_FACTORY, FolderWithItemType, IFolderConfig,
    IntfID, itemType,
    IContext*, context,
    IComponent*, parent,
    IString*, localId)

OPENDAQ_DECLARE_CLASS_FACTORY_WITH_INTERFACE(
    LIBRARY_FACTORY, IoFolder, IFolderConfig,
    IContext*, context,
    IComponent*, parent,
    IString*, localId)

END_NAMESPACE_OPENDAQ
