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
#include <opendaq/folder_config_ptr.h>

BEGIN_NAMESPACE_OPENDAQ

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
 * @param propertyMode Enum defining whether standard properties such as "Name" and "Description" are created.
 *                     "Add" to create the default properties; "AddReadOnly" to create the properties, but configure them as "read-only";
 *                     "Skip" to skip creation.
 */
template <class TInterface = IComponent>
inline FolderConfigPtr Folder(const ContextPtr& context, const ComponentPtr& parent, const StringPtr& localId, const ComponentStandardProps propertyMode = ComponentStandardProps::Add)
{
    FolderConfigPtr obj(FolderWithItemTypeAndDefaultPropertyMode_Create(TInterface::Id, context, parent, localId, static_cast<Int>(propertyMode)));
    return obj;
}

/*!@}*/

END_NAMESPACE_OPENDAQ
