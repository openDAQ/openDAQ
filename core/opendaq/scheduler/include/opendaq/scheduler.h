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
#include <opendaq/awaitable.h>
#include <opendaq/work.h>
#include <opendaq/task_graph.h>
#include <opendaq/logger.h>
#include <coretypes/listobject.h>
#include <coretypes/procedure.h>
#include <coretypes/function.h>

BEGIN_NAMESPACE_OPENDAQ

/*!
 * @ingroup opendaq_scheduler_components
 * @addtogroup opendaq_scheduler Scheduler
 * @{
 */

/*!
 * @brief A thread-pool scheduler that supports scheduling one-off functions as well as
 * dependency graphs.
 */
DECLARE_OPENDAQ_INTERFACE(IScheduler, IBaseObject)
{
    /*!
     * @brief Schedules the specified @p work function to run on the thread-pool.
     * The call does not block but immediately returns an @p awaitable that represents
     * the asynchronous execution. It can be waited upon and queried for status and result.
     * @param function The function to schedule for execution.
     * @param[out] awaitable The object representing the state and result of the execution.
     * @retval OPENDAQ_ERR_SCHEDULER_STOPPED when the scheduler already stopped and is not accepting any more work.
     */
    virtual ErrCode INTERFACE_FUNC scheduleFunction(IFunction* function, IAwaitable** awaitable) = 0;

    /*!
     * @brief Schedules the specified work callback to run on the thread-pool.
     * The call does not block.
     * @param work The function to schedule for execution.
     * @retval OPENDAQ_ERR_SCHEDULER_STOPPED when the scheduler already stopped and is not accepting any more work.
     *
     * Work is a lightweight callback that returns no value and accepts no procedure. It has less overhead than
     * function. The function does not return awaitable object.
     */
    virtual ErrCode INTERFACE_FUNC scheduleWork(IWork* work) = 0;

    /*!
     * @brief Schedules the specified dependency @p graph to run on the thread-pool.
     * The call does not block but immediately returns an @p awaitable that represents
     * the asynchronous execution. It can be waited upon and queried for status and result.
     * <b>Any exceptions that occur during the graph execution are silently ignored.</b>
     * @param graph The dependency graph (acyclic directed graph) to schedule.
     * @param[out] awaitable The object representing the state and result of the execution.
     * @retval OPENDAQ_ERR_SCHEDULER_STOPPED when the scheduler already stopped and is not accepting any more work.
     */
    virtual ErrCode INTERFACE_FUNC scheduleGraph(ITaskGraph* graph, IAwaitable** awaitable) = 0;

    /*!
     * @brief Cancels all outstanding work and waits for the remaining to complete.
     * After this point the scheduler does not allow any new work or graphs for scheduling.
     */
    virtual ErrCode INTERFACE_FUNC stop() = 0;

    /*!
     * @brief Waits fo all current scheduled work and tasks to complete.
     */
    virtual ErrCode INTERFACE_FUNC waitAll() = 0;

    /*!
     * @brief Returns whether more than one worker thread is used.
     * @param[out] multiThreaded Returns @c true if more that one worker thread is used by the scheduler.
     */
    virtual ErrCode INTERFACE_FUNC isMultiThreaded(Bool* multiThreaded) = 0;
};
/*!@}*/

OPENDAQ_DECLARE_CLASS_FACTORY(
    LIBRARY_FACTORY, Scheduler,
    ILogger*, logger,
    SizeT, numWorkers
)

END_NAMESPACE_OPENDAQ
