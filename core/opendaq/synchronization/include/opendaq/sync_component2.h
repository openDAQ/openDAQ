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

#include <coretypes/baseobject.h>
#include <opendaq/sync_interface.h>
#include <coretypes/stringobject.h>
#include <coretypes/dictobject.h>

BEGIN_NAMESPACE_OPENDAQ

/*!
 * @ingroup opendaq_synchronization_path
 * @addtogroup opendaq_sync_component2 Sync Component 2
 * @{
 */

/*!
 * @brief Interface representing a Synchronization Component 2 in a Test & Measurement system.
 */
DECLARE_OPENDAQ_INTERFACE(ISyncComponent2, IBaseObject)
{
    /*!
     * @brief Retrieves the selected sync source interface.
     * @param[out] selectedSource The selected sync source interface.
     */
    virtual ErrCode INTERFACE_FUNC getSelectedSource(ISyncInterface** selectedSource) = 0;

    /*!
     * @brief Sets the selected sync source interface by name.
     * @param selectedSourceName The name of the selected sync source interface.
     */
    virtual ErrCode INTERFACE_FUNC setSelectedSource(IString* selectedSourceName) = 0;

    /*!
     * @brief Gets whether the source is synced.
     * @param[out] synced True if the source is synced; false otherwise.
     */
    virtual ErrCode INTERFACE_FUNC getSourceSynced(Bool* synced) = 0;

    /*!
     * @brief Gets the reference domain ID of the source.
     * @param[out] referenceDomainId The reference domain ID string.
     */
    virtual ErrCode INTERFACE_FUNC getSourceReferenceDomainId(IString** referenceDomainId) = 0;

    // [templateType(interfaces, IString, ISyncInterface)]
    /*!
     * @brief Retrieves the dictionary of interfaces associated with this synchronization component.
     * @param[out] interfaces Dictionary of interfaces associated with this component, where keys are interface names (IString) and values are sync interfaces (ISyncInterface).
     */
    virtual ErrCode INTERFACE_FUNC getInterfaces(IDict** interfaces) = 0;
};
/*!@}*/

OPENDAQ_DECLARE_CLASS_FACTORY_WITH_INTERFACE(
    LIBRARY_FACTORY, SyncComponent2, ISyncComponent2,
    IContext*, context,
    IComponent*, parent,
    IString*, localId
)

END_NAMESPACE_OPENDAQ
