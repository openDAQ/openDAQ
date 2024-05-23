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
#include <opendaq/module_manager.h>
#include <opendaq/discovery_service.h>

BEGIN_NAMESPACE_OPENDAQ

/*!
 * @ingroup opendaq_utility
 * @addtogroup opendaq_context Context
 * @{
 */

/*!
 * @brief Internal Context interface used for transferring the Module Manager reference to a new owner.
 */

DECLARE_OPENDAQ_INTERFACE(IContextInternal, IBaseObject)
{
    /*!
     * @brief Gets the Module Manager. Moves the strong reference to the manager to the first
     * caller and retains a weak reference internally.
     * @param[out] manager The module manager.
     *
     * Returns a nullptr on subsequent invocations, and if the manager is not assigned.
     */
    virtual ErrCode INTERFACE_FUNC moveModuleManager(IModuleManager** manager) = 0;
};
/*!@}*/

END_NAMESPACE_OPENDAQ
