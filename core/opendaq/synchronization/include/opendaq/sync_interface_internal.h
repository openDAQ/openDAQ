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

BEGIN_NAMESPACE_OPENDAQ

/*!
 * @ingroup opendaq_synchronization_path
 * @addtogroup opendaq_sync_interface_internal Sync Interface Internal
 * @{
 */

/*!
 * @brief Internal interface for synchronization interface operations.
 */
DECLARE_OPENDAQ_INTERFACE(ISyncInterfaceInternal, IBaseObject)
{
    /*!
     * @brief Sets whether this interface is used as a source.
     * @param isSource True if this interface should be used as a source; false otherwise.
     */
    virtual ErrCode INTERFACE_FUNC setAsSource(Bool isSource) = 0;
};
/*!@}*/

END_NAMESPACE_OPENDAQ
