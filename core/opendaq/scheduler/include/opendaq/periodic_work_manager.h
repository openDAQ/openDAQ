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
#include <opendaq/repetitive_work_executor.h>

#include <taskflow/taskflow.hpp>

#include <atomic>
#include <memory>

BEGIN_NAMESPACE_OPENDAQ

class SchedulerImpl;

/*!
 * @brief Runs repetitive work on the scheduler thread pool.
 */
class PeriodicWorkManager final
{
public:
    PeriodicWorkManager(SchedulerImpl& scheduler, tf::Executor& executor, LoggerComponentPtr loggerComponent);
    ~PeriodicWorkManager();

    PeriodicWorkManager(const PeriodicWorkManager&) = delete;
    PeriodicWorkManager& operator=(const PeriodicWorkManager&) = delete;

    ErrCode schedule(IWorkRepetitive* work);
    void stop();

private:
    void startEntryLoop(const std::shared_ptr<RepetitiveWorkEntry>& entry);
    void runEntryLoop(const std::shared_ptr<RepetitiveWorkEntry>& entry);

    SchedulerImpl& scheduler;
    LoggerComponentPtr loggerComponent;
    RepetitiveWorkExecutor executor;
    tf::Executor& tfExecutor;

    std::atomic<bool> stopped{false};
};

END_NAMESPACE_OPENDAQ
