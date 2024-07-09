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
#include <opendaq/task_ptr.h>
#include <opendaq/task_graph_ptr.h>

BEGIN_NAMESPACE_OPENDAQ

/*!
 * @ingroup opendaq_task
 * @addtogroup opendaq_task_factories Factories
 * @{
 */

/*!
 * @brief Creates a dependency graph (directed acyclic graph) of tasks that can be scheduled for execution on a Scheduler
 * with the specified @p work that is always executed before any other task on the graph.
 * @param work The root callable that is always executed before any other tasks on the graph.
 * @param name The graph name used in diagnostics.
 * @return An instance of a dependency graph used for scheduling.
 */
inline TaskPtr TaskGraph(ProcedurePtr work, StringPtr name)
{
    TaskPtr obj(TaskGraph_Create(work, name));
    return obj;
}
/*!
 * @brief Creates a dependency graph (directed acyclic graph) of tasks that can be scheduled for execution on a Scheduler.
 * @param name The graph name used in diagnostics.
 * @return An instance of a dependency graph used for scheduling.
 */
inline TaskPtr TaskGraph(StringPtr name)
{
    TaskPtr obj(TaskGraph_Create(nullptr, name));
    return obj;
}

/*!
 * @brief A packaged callback with possible continuations and dependencies that can
 * be arranged in a dependency graph (directed acyclic graph).
 * @param work The callback to execute when the dependencies complete.
 * @param name The name used for diagnostics.
 * @return An instance of a task on a dependency graph.
 */
inline TaskPtr Task(ProcedurePtr work, StringPtr name = "")
{
    TaskPtr obj(Task_Create(work, name));
    return obj;
}

/*!@}*/

END_NAMESPACE_OPENDAQ
