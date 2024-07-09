/*
 * Copyright 2022-2024 openDAQ d. o. o.
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
 * @ingroup opendaq_components
 * @addtogroup opendaq_components_removable Removable
 * @{
 */

/*!
 * @brief Allows the component to be notified when it is removed.
 */
DECLARE_OPENDAQ_INTERFACE(IRemovable, IBaseObject)
{
    /*!
     * @brief Notifies the component that it is being removed.
     *
     * Call `remove` on the component to mark it as removed. It's up to the implementation
     * to define what is does on the act of removal. Basic implementation of `Component`
     * will switch it to inactive state and it cannot be activated again.
     */
    virtual ErrCode INTERFACE_FUNC remove() = 0;

    /*!
     * @brief Returns True if component was removed.
     * @param[out] removed True if component was removed; otherwise False.
     */
    virtual ErrCode INTERFACE_FUNC isRemoved(Bool* removed) = 0;

};
/*!@}*/

END_NAMESPACE_OPENDAQ
