/*
 * Copyright 2022-2026 openDAQ d.o.o.
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
 * @addtogroup opendaq_synchronization Synchronization
 * @{
 */

/*!
 * @brief Interface representing the Synchronization of a device in a Test & Measurement system.
 */
DECLARE_OPENDAQ_INTERFACE(ISynchronization, IBaseObject)
{
    /*!
     * @brief Gets all synchronization interfaces registered with this synchronization.
     * @param[out] interfaces A dictionary mapping interface names to the sync interfaces themselves.
     */
    // [templateType(interfaces, IString, ISyncInterface)]
    virtual ErrCode INTERFACE_FUNC getSyncInterfaces(IDict** interfaces) = 0;

    /*!
     * @brief Selects the synchronization interface with the given name as the synchronization source.
     * @param sourceName The name of the synchronization interface to select as the source.
     */
    virtual ErrCode INTERFACE_FUNC setSource(IString* sourceName) = 0;

    /*!
     * @brief Gets the currently selected synchronization source.
     * @param[out] source The currently selected synchronization interface.
     */
    virtual ErrCode INTERFACE_FUNC getSource(ISyncInterface** source) = 0;

    /*!
     * @brief Gets the reference domain IDs of all registered synchronization interfaces
     * that have one assigned.
     * @param[out] ids The list of reference domain IDs.
     */
    // [templateType(ids, IString)]
    virtual ErrCode INTERFACE_FUNC getReferenceDomainIds(IList** ids) = 0;
};
/*!@}*/

OPENDAQ_DECLARE_CLASS_FACTORY_WITH_INTERFACE(
    LIBRARY_FACTORY, Synchronization, ISynchronization,
    ITypeManager*, manager)

END_NAMESPACE_OPENDAQ
