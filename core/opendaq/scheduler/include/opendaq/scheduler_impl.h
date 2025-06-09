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


class MainThreadWorker
{
public:
    MainThreadWorker()
        :stop(false)
    {
    }
    
    ~MainThreadWorker()
    {
        {
            std::lock_guard<std::mutex> lock(mutex);
            stop = true;
        }
        cv.notify_all();
    }

    void start()
    {
        while(true)
        {
            std::unique_lock<std::mutex> lock(mutex);
            cv.wait(lock, [this] { return stop || !workQueue.empty(); });
            if (stop)
                return;
            auto work = std::move(workQueue.front());
            workQueue.pop_front();
            lock.unlock();
            work->execute();
            lock.lock();
        }
    }

    ErrCode execute(IWork* work)
    {
        OPENDAQ_PARAM_NOT_NULL(work);
        {
            std::lock_guard<std::mutex> lock(mutex);
            workQueue.push_back(WorkPtr(work));
        }
        cv.notify_one();
        return OPENDAQ_SUCCESS;
    }

private:
    std::mutex mutex;
    std::condition_variable cv;
    std::deque<WorkPtr> workQueue;
    bool stop = false;
};

class SchedulerImpl final : public ImplementationOf<IScheduler>
{
public:
    explicit SchedulerImpl(LoggerPtr logger, SizeT numWorkers);
    ~SchedulerImpl() override;

    ErrCode INTERFACE_FUNC scheduleFunction(IFunction* function, IAwaitable** awaitable) override;
    ErrCode INTERFACE_FUNC scheduleWork(IWork* work) override;
    ErrCode INTERFACE_FUNC scheduleGraph(ITaskGraph* graph, IAwaitable** awaitable) override;
    ErrCode INTERFACE_FUNC isMultiThreaded(Bool* multiThreaded) override;

    ErrCode INTERFACE_FUNC stop() override;
    ErrCode INTERFACE_FUNC waitAll() override;

    ErrCode INTERFACE_FUNC mainLoop() override;
    ErrCode INTERFACE_FUNC isMainLoopRunning(Bool* running) override;
    ErrCode INTERFACE_FUNC scheduleWorkOnMainThread(IWork* work) override;


    [[nodiscard]] std::size_t getWorkerCount() const;

private:
    ErrCode checkAndPrepare(const IBaseObject* work, IAwaitable** awaitable);

    bool stopped;
    LoggerPtr logger;
    LoggerComponentPtr loggerComponent;

    std::unique_ptr<tf::Executor> executor;

    std::unique_ptr<MainThreadWorker> mainThreadWorker;
};

END_NAMESPACE_OPENDAQ
