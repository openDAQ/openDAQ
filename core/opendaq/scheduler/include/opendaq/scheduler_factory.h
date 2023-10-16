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
#include <opendaq/scheduler_ptr.h>
#include <opendaq/logger_ptr.h>

BEGIN_NAMESPACE_OPENDAQ

/*!
 * @ingroup opendaq_scheduler
 * @addtogroup opendaq_scheduler_factories Factories
 * @{
 */

/*!
 * @brief Creates an instance of a Scheduler with the specified amount of @p numWorker threads.
 * @param numWorkers The amount of worker threads. If @c 0 then maximum number of concurrent threads supported by the implementation is used.
 * @returns A Scheduler instance with the specified amount of worker threads.
 */
inline SchedulerPtr Scheduler(LoggerPtr logger, SizeT numWorkers = 0)
{
    SchedulerPtr obj(Scheduler_Create(logger, numWorkers));
    return obj;
}

/*!@}*/

END_NAMESPACE_OPENDAQ
