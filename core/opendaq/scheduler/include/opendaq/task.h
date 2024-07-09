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
#include <coretypes/stringobject.h>
#include <coretypes/procedure.h>

BEGIN_NAMESPACE_OPENDAQ

/*!
 * @ingroup opendaq_scheduler_components
 * @addtogroup opendaq_task Task
 * @{
 */

/*!
 *  @brief A packaged callback with possible continuations and dependencies that can
 *  be arranged in a dependency graph (directed acyclic graph). The task is not executed directly but only
 *  when the graph is scheduled for execution and all dependencies have been satisfied.
 */

/*#
 * [templated(defaultAliasName: TaskPtr)]
 * [interfaceSmartPtr(ITask, GenericTaskPtr)]
 */
DECLARE_OPENDAQ_INTERFACE(ITask, IBaseObject)
{
    /*!
     * @brief Gets the task name.
     * @param[out] name The task name.
     */
    virtual ErrCode INTERFACE_FUNC getName(IString** name) = 0;

    /*!
     * @brief Sets the task name that is used in diagnostics.
     * @param name The new task name.
     */
    virtual ErrCode INTERFACE_FUNC setName(IString* name) = 0;

    /*!
     * @brief Sets the continuation to only execute after this task completes.
     * Be careful of forming cycles as tasks whose dependencies cannot be satisfied will never execute.
     * @param continuation The task that should wait for this one to complete.
     * @retval OPENDAQ_ERR_NOT_SUPPORTED If the @p continuation implementation was not crated by the scheduler library.
     */
    // [ignore(Wrapper)]
    virtual ErrCode INTERFACE_FUNC then(ITask* continuation) = 0;
};
/*!@}*/

OPENDAQ_DECLARE_CLASS_FACTORY(LIBRARY_FACTORY, Task, IProcedure*, work, IString*, name)

END_NAMESPACE_OPENDAQ
