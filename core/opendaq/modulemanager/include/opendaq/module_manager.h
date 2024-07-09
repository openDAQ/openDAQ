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
#include <opendaq/module.h>
#include <coretypes/listobject.h>

BEGIN_NAMESPACE_OPENDAQ

/*!
 * @ingroup opendaq_modules
 * @addtogroup opendaq_module_manager Module manager
 * @{
 */

/*!
 * @brief Loads all available modules in a implementation-defined manner.
 * User can also side-load custom modules via `addModule` call.
 */
DECLARE_OPENDAQ_INTERFACE(IModuleManager, IBaseObject)
{
    // [elementType(modules, IModule)]
    /*!
     * @brief Retrieves all modules known to the manager. Whether they were found or side-loaded.
     * @param[out] modules A list of known modules.
     */
    virtual ErrCode INTERFACE_FUNC getModules(IList** modules) = 0;

    /*!
     * @brief Side-load a custom module in run-time from memory that was not found by default.
     * @param module The module to add.
     * @retval OPENDAQ_ERR_DUPLICATEITEM When an identical @p module was already added.
     */
    virtual ErrCode INTERFACE_FUNC addModule(IModule* module) = 0;

    /*!
     * @brief Loads all modules from the directory path specified during manager construction. The
     * Context is passed to all loaded modules for internal use.
     * @param context The Context containing the Logger, Scheduler, Property Object Class Manager and Module Manager
     */
    virtual ErrCode INTERFACE_FUNC loadModules(IContext* context) = 0;
};
/*!@}*/

OPENDAQ_DECLARE_CLASS_FACTORY(
    LIBRARY_FACTORY, ModuleManager,
    IString*, path)

OPENDAQ_DECLARE_CLASS_FACTORY_WITH_INTERFACE(LIBRARY_FACTORY,
    ModuleManagerMultiplePaths, IModuleManager,
    IList*, paths)

END_NAMESPACE_OPENDAQ
