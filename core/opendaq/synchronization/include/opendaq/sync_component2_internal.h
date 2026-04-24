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

BEGIN_NAMESPACE_OPENDAQ

/*!
 * @ingroup opendaq_synchronization_path
 * @addtogroup opendaq_sync_component2 Sync Component 2
 * @{
 */

/*!
 * @brief Internal interface for synchronization component 2 operations.
 */
DECLARE_OPENDAQ_INTERFACE(ISyncComponent2Internal, IBaseObject)
{
    /*!
     * @brief Adds an interface to the synchronization component 2.
     * @param syncInterface The sync interface to be added.
     */
    virtual ErrCode INTERFACE_FUNC addInterface(ISyncInterface* syncInterface) = 0;
};
/*!@}*/

END_NAMESPACE_OPENDAQ
