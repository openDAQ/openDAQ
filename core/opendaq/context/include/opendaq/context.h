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
#include <coretypes/type_manager.h>
#include <opendaq/logger.h>

BEGIN_NAMESPACE_OPENDAQ

struct IScheduler;
struct IModuleManager;

/*!
 * @ingroup opendaq_utility
 * @addtogroup opendaq_context Context
 * @{
 */

/*!
 * @brief The Context serves as a container for the Scheduler and Logger. It originates
 * at the instance, and is passed to the root device, which forwards it to components
 * such as function blocks and signals.
 *
 * Note: The context holds a strong reference to the Module Manager until  the reference is moved via the
 * ContextInternal move function. The strong reference moved to an external owner to avoid memory leaks
 * due to circular references. This is done automatically when the Context is used in the openDAQ Instance
 * constructor.
 */

/*#
 * [interfaceLibrary(ITypeManager, "coretypes")]
 */
DECLARE_OPENDAQ_INTERFACE(IContext, IBaseObject)
{
    /*!
     * @brief Gets the scheduler.
     * @param[out] scheduler The scheduler.
     */
    virtual ErrCode INTERFACE_FUNC getScheduler(IScheduler** scheduler) = 0;
    /*!
     * @brief Gets the logger.
     * @param[out] logger The logger.
     */
    virtual ErrCode INTERFACE_FUNC getLogger(ILogger** logger) = 0;
    /*!
     * @brief Gets the Module Manager as a Base Object.
     * @param[out] manager The module manager.
     */
    virtual ErrCode INTERFACE_FUNC getModuleManager(IBaseObject** manager) = 0;
    /*!
     * @brief Gets the Type Manager.
     * @param[out] manager The type manager.
     */
    virtual ErrCode INTERFACE_FUNC getTypeManager(ITypeManager** manager) = 0;
};
/*!@}*/

/*!
 * @ingroup opendaq_context
 * @addtogroup opendaq_context_factories Factories
 * @{
 */

OPENDAQ_DECLARE_CLASS_FACTORY(
    LIBRARY_FACTORY, Context,
    IScheduler*, Scheduler,
    ILogger*, Logger,
    ITypeManager*, typeManager,
    IModuleManager*, moduleManager
)

/*!@}*/

END_NAMESPACE_OPENDAQ
