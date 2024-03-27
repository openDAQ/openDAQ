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

#include <opendaq/folder.h>

BEGIN_NAMESPACE_OPENDAQ

/*!
 * @ingroup opendaq_components
 * @addtogroup opendaq_folder Folder
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
};
/*!@}*/

/*!
 * @ingroup opendaq_folder
 * @addtogroup opendaq_folder_factories Folder
 * @{
 */

/*!
 * @brief Creates a folder.
 * @param context The Context. Most often the creating function-block/device passes its own Context to the Folder.
 * @param parent The parent component.
 * @param localId The local ID of the component.
 */
OPENDAQ_DECLARE_CLASS_FACTORY_WITH_INTERFACE(
    LIBRARY_FACTORY, Folder, IFolderConfig,
    IContext*, context,
    IComponent*, parent,
    IString*, localId)

/*!
 * @brief Creates a folder with an interface ID that must be implemented by its children.
 * @param context The Context. Most often the creating function-block/device passes its own Context to the Folder.
 * @param itemType The ID of interface that child objects of the folder must implement.
 * @param parent The parent component.
 * @param localId The local ID of the component.
 */
OPENDAQ_DECLARE_CLASS_FACTORY_WITH_INTERFACE(
    LIBRARY_FACTORY, FolderWithItemType, IFolderConfig,
    IntfID, itemType,
    IContext*, context,
    IComponent*, parent,
    IString*, localId)

/*!
 * @brief Creates an IO folder.
 * @param context The Context. Most often the creating function-block/device passes its own Context to the Folder.
 * @param parent The parent component.
 * @param localId The local ID of the parent.
 *
 * IO folders are folder created by device and may contain only channels and other IO folders.
 */
OPENDAQ_DECLARE_CLASS_FACTORY_WITH_INTERFACE(
    LIBRARY_FACTORY, IoFolder, IFolderConfig,
    IContext*, context,
    IComponent*, parent,
    IString*, localId)

/*!@}*/

END_NAMESPACE_OPENDAQ
