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

#include <opendaq/logger_component_ptr.h>
#include <opendaq/work_ptr.h>
#include <opendaq/work_repetitive_ptr.h>

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <functional>
#include <memory>
#include <mutex>
#include <unordered_map>
#include <vector>

BEGIN_NAMESPACE_OPENDAQ

struct RepetitiveWorkEntry
{
    WorkRepetitivePtr work;
    std::chrono::steady_clock::time_point nextRun{};

    std::atomic<bool> loopRunning{false};
    std::atomic<bool> idleNotified{false};
    std::atomic<bool> inFlight{false};

    std::mutex idleMutex;
    std::condition_variable idleCv;
};

class RepetitiveWorkExecutor
{
public:
    using ScheduleAfterFn = std::function<void(const WorkPtr&)>;

    explicit RepetitiveWorkExecutor(LoggerComponentPtr loggerComponent);

    ErrCode schedule(IWorkRepetitive* work);

    std::shared_ptr<RepetitiveWorkEntry> find(IWorkRepetitive* work) const;
    std::vector<std::shared_ptr<RepetitiveWorkEntry>> entries() const;
    void finishCancel(const std::shared_ptr<RepetitiveWorkEntry>& entry, const ScheduleAfterFn& scheduleAfter);
    void waitUntilIdle(const std::shared_ptr<RepetitiveWorkEntry>& entry);
    void cancelAll();

    ErrCode execute(const WorkRepetitivePtr& work);
    ErrCode executeRepetitively(const WorkRepetitivePtr& work, Bool* repeatAfter);
    void logExecutionError() const;

private:
    LoggerComponentPtr loggerComponent;
    mutable std::mutex mutex;
    std::unordered_map<IWorkRepetitive*, std::shared_ptr<RepetitiveWorkEntry>> entriesByWork;
};

bool isRepetitiveWorkCanceled(const WorkRepetitivePtr& work);

END_NAMESPACE_OPENDAQ
