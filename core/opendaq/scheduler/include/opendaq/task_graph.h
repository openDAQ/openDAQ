/*
 * Copyright 2022-2024 openDAQ d.o.o.
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
#include <opendaq/task.h>

BEGIN_NAMESPACE_OPENDAQ

/*!
 * @ingroup opendaq_scheduler_components
 * @addtogroup opendaq_task TaskGraph
 * @{
 */

/*!
 * @brief A dependency graph (directed acyclic graph) of tasks that can be scheduled for execution on a Scheduler.
 */
/*#
 * [interfaceSmartPtr(ITask, GenericTaskPtr)]
 */
DECLARE_OPENDAQ_INTERFACE(ITaskGraph, ITask)
{
};
/*!@}*/

OPENDAQ_DECLARE_CLASS_FACTORY(LIBRARY_FACTORY, TaskGraph, IProcedure*, work, IString*, name)

END_NAMESPACE_OPENDAQ
