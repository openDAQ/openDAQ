/*
 * Copyright 2022-2026 openDAQ d.o.o.
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
#include <coretypes/common.h>

#include <atomic>
#include <functional>

BEGIN_NAMESPACE_OPENDAQ

/*!
 * @brief Internal to MultiReader2: the ports notify on the sending thread, which only marks its port and, when the reader is
 * idle, schedules one task that ingests every marked port. A burst of packets on any number of ports costs one task, and the
 * sending thread never takes a reader lock. With ingestTaskEnabled false every port notifies through the scheduler and each
 * notification ingests its own port, one task per packet.
 */
namespace multi_reader2
{

constexpr bool ingestTaskEnabled = true;

class IngestTask
{
public:
    // On the sending thread, after the packet is queued and its port marked: true when the caller schedules a run
    bool notify();

    // The scheduled run: drains once; true when packets came in meanwhile and the caller schedules the next run. A drain
    // that throws leaves the task idle and the exception to the caller
    bool run(const std::function<void()>& drain);

    // A run that could not be scheduled: the next notification schedules again
    void cancel();

private:
    enum State : int
    {
        Idle,
        Scheduled,
        Running,
        RunningDirty
    };
    std::atomic<int> state{Idle};
};
}

END_NAMESPACE_OPENDAQ
