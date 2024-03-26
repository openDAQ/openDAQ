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
#include <opendaq/scheduler_errors.h>
#include <coretypes/exceptions.h>

BEGIN_NAMESPACE_OPENDAQ

class SchedulerException : public DaqException
{
public:
    using DaqException::DaqException;
};

#define DEFINE_MODULE_MANAGER_EXCEPTION(excName, errCode, excMsg) DEFINE_EXCEPTION_BASE(daq::SchedulerException, excName, errCode, excMsg)

/*
 * Should be in the order of the error's numerical value
 */

DEFINE_MODULE_MANAGER_EXCEPTION(SchedulerUnknown, OPENDAQ_ERR_SCHEDULER_UNKNOWN, "Unknown scheduler exception")
DEFINE_MODULE_MANAGER_EXCEPTION(SchedulerStopped, OPENDAQ_ERR_SCHEDULER_STOPPED, "Scheduler already stopped")
DEFINE_MODULE_MANAGER_EXCEPTION(NotEnoughTasks, OPENDAQ_ERR_NOT_ENOUGH_TASKS, "Not enough tasks provided for this operation")
DEFINE_MODULE_MANAGER_EXCEPTION(EmptyAwaitable, OPENDAQ_ERR_EMPTY_AWAITABLE, "Awaitable has no state")

END_NAMESPACE_OPENDAQ
