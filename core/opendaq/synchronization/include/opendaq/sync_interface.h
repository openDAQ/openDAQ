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
#include <coretypes/stringobject.h>

BEGIN_NAMESPACE_OPENDAQ

/*!
 * @ingroup opendaq_synchronization_path
 * @addtogroup opendaq_sync_interface Sync Interface
 * @{
 */

/*!
 * @brief Interface representing a Synchronization Interface.
 */
DECLARE_OPENDAQ_INTERFACE(ISyncInterface, IBaseObject)
{
    /*!
     * @brief Gets the name of the synchronization interface.
     * @param[out] name The name of the synchronization interface.
     */
    virtual ErrCode INTERFACE_FUNC getName(IString** name) = 0;

    /*!
     * @brief Gets whether the synchronization interface is synced.
     * @param[out] synced True if the interface is synced; false otherwise.
     */
    virtual ErrCode INTERFACE_FUNC getSynced(Bool* synced) = 0;

    /*!
     * @brief Gets the reference domain ID of the synchronization interface.
     * @param[out] referenceDomainId The reference domain ID string.
     */
    virtual ErrCode INTERFACE_FUNC getReferenceDomainId(IString** referenceDomainId) = 0;
};
/*!@}*/

END_NAMESPACE_OPENDAQ
