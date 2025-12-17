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
#include <opendaq/scheduler.h>
#include <coretypes/intfs.h>
#include <coretypes/impl.h>
#include <opendaq/logger_ptr.h>
#include <opendaq/logger_component_ptr.h>
#include <opendaq/awaitable_ptr.h>
#include <opendaq/task_flow.h>
#include <opendaq/work_ptr.h>

BEGIN_NAMESPACE_OPENDAQ

class MainThreadLoop
{
public:
    MainThreadLoop(const LoggerPtr& logger);
    ~MainThreadLoop();

    ErrCode stop();
    ErrCode runIteration();
    ErrCode run(SizeT loopTime);
    bool isRunning() const;
    ErrCode scheduleTask(IWork* work);

    MainThreadLoop(const MainThreadLoop&) = delete;
    MainThreadLoop& operator=(const MainThreadLoop&) = delete;

private:
    bool executeWork(const WorkPtr& work);

    void runIteration(std::unique_lock<std::mutex>& lock);

    LoggerComponentPtr loggerComponent;

    mutable std::mutex mutex;
    std::condition_variable cv;
    std::list<WorkPtr> workQueue;
    bool running{ false };
};

class SchedulerImpl final : public ImplementationOf<IScheduler>
{
public:
    explicit SchedulerImpl(LoggerPtr logger, SizeT numWorkers, Bool useMainLoop = false);
    ~SchedulerImpl() override;

    ErrCode INTERFACE_FUNC scheduleFunction(IFunction* function, IAwaitable** awaitable) override;
    ErrCode INTERFACE_FUNC scheduleWork(IWork* work) override;
    ErrCode INTERFACE_FUNC scheduleGraph(ITaskGraph* graph, IAwaitable** awaitable) override;
    ErrCode INTERFACE_FUNC isMultiThreaded(Bool* multiThreaded) override;

    ErrCode INTERFACE_FUNC stop() override;
    ErrCode INTERFACE_FUNC waitAll() override;

    ErrCode INTERFACE_FUNC runMainLoop(SizeT loopTime) override;
    ErrCode INTERFACE_FUNC isMainLoopSet(Bool* isSet) override;
    ErrCode INTERFACE_FUNC stopMainLoop() override;
    ErrCode INTERFACE_FUNC runMainLoopIteration() override;
    ErrCode INTERFACE_FUNC scheduleWorkOnMainLoop(IWork* work) override;

    [[nodiscard]] std::size_t getWorkerCount() const;

private:
    ErrCode checkAndPrepare(const IBaseObject* work, IAwaitable** awaitable);

    bool stopped;
    LoggerPtr logger;
    LoggerComponentPtr loggerComponent;

    std::unique_ptr<tf::Executor> executor;

    std::unique_ptr<MainThreadLoop> mainThreadWorker;
};


inline std::size_t SchedulerImpl::getWorkerCount() const
{
    return executor->num_workers();
}

END_NAMESPACE_OPENDAQ
