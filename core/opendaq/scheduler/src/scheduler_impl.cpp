#include <opendaq/scheduler_impl.h>
#include <opendaq/scheduler_errors.h>

#include <opendaq/awaitable_impl.h>

#include <opendaq/custom_log.h>

#include <opendaq/task_internal.h>
#include <opendaq/task_ptr.h>
#include <opendaq/work_factory.h>
#include <coretypes/function_ptr.h>
#include <coretypes/validation.h>
#include <utility>
#include <opendaq/thread_name.h>

class CustomWorkerInterface : public tf::WorkerInterface
{
public:
    void scheduler_prologue(tf::Worker& worker) override
    {
        daqNameThread(fmt::format("Scheduler{}", worker.id()).c_str());
    }

    void scheduler_epilogue(tf::Worker& worker, std::exception_ptr ptr) override
    {
    };
};

BEGIN_NAMESPACE_OPENDAQ
MainThreadLoop::MainThreadLoop(const LoggerPtr& logger)
{
    this->loggerComponent = logger.getOrAddComponent("MainThreadLoop");
}

MainThreadLoop::~MainThreadLoop()
{
    stop();
}

ErrCode MainThreadLoop::stop()
{
    return this->scheduleTask(Work([this]
    {
        std::lock_guard<std::mutex> lock(mutex);
        this->running = false;
    }));
}

bool MainThreadLoop::executeWork(const WorkPtr& work)
{
    Bool repeatAfter = False;
    ErrCode errCode;
    
    if (auto workPtr = work.asPtrOrNull<IWorkRepetitive>(true); workPtr.assigned())
        errCode = workPtr->executeRepetitively(&repeatAfter);
    else
        errCode = work->execute();
    
    if (OPENDAQ_FAILED(errCode))
    {
        ListPtr<IErrorInfo> errorInfos;
        daqGetErrorInfoList(&errorInfos);
        if (errorInfos.assigned())
        {
            std::ostringstream errorStream;
            bool firstMessage = true;
            for (const auto& errorInfo : errorInfos)
            {
                StringPtr message;
                errCode = errorInfo->getMessage(&message);
                if (OPENDAQ_FAILED(errCode))
                    continue;

                if (message.assigned())
                {
                    errorStream << (firstMessage ? "" : "\n") << message;
                    firstMessage = false;
                }

                #ifndef NDEBUG
                    ConstCharPtr fileName = nullptr;
                    Int fileLine = -1;

                    errorInfo->getFileName(&fileName);
                    errorInfo->getFileLine(&fileLine);
                    if (fileName != nullptr)
                    {
                        errorStream << " [ File " << fileName;
                        if (fileLine != -1)
                            errorStream << ":" << fileLine;
                        errorStream << " ]";
                    }
                #endif                        
            }
            LOG_W("Error executing work: {}", errorStream.str());
        }
    }
    return repeatAfter; 
}

void MainThreadLoop::runIteration(std::unique_lock<std::mutex>& lock)
{
    std::list<WorkPtr> currentWork;
    currentWork.swap(workQueue);
    lock.unlock();

    for (auto it = currentWork.begin(); it != currentWork.end();)
    {
        if (executeWork(*it))
            ++it;
        else
            it = currentWork.erase(it);
    }

    lock.lock();
    workQueue.splice(workQueue.end(), currentWork);
}

ErrCode MainThreadLoop::runIteration()
{
    std::unique_lock<std::mutex> lock(mutex);
    if (this->running)
        return DAQ_MAKE_ERROR_INFO(OPENDAQ_ERR_INVALID_OPERATION, "Main thread event loop is already running");

    this->running = true;
    runIteration(lock);
    this->running = false;
    return OPENDAQ_SUCCESS;
}

ErrCode MainThreadLoop::run(SizeT loopTime)
{
    if (loopTime == 0)
        loopTime = 1;

    std::unique_lock<std::mutex> lock(mutex);
    if (this->running)
        return DAQ_MAKE_ERROR_INFO(OPENDAQ_ERR_INVALID_OPERATION, "Main thread event loop is already running");

    this->running = true;
    const auto waitTime = std::chrono::milliseconds(loopTime);
    auto waitUntil = std::chrono::steady_clock::now() + waitTime;

    while (this->running)
    {
        cv.wait_until(lock, waitUntil, [] { return false; });
        waitUntil += waitTime;
        runIteration(lock);
    }

    this->running = false;
    return OPENDAQ_SUCCESS;
}

bool MainThreadLoop::isRunning() const
{
    std::lock_guard<std::mutex> lock(mutex);
    return running;
}

ErrCode MainThreadLoop::scheduleTask(IWork* work)
{
    OPENDAQ_PARAM_NOT_NULL(work);

    {
        std::lock_guard<std::mutex> lock(mutex);
        workQueue.emplace_back(work);
    }

    cv.notify_one();
    return OPENDAQ_SUCCESS;
}

SchedulerImpl::SchedulerImpl(LoggerPtr logger, SizeT numWorkers, Bool useMainLoop)
    : stopped(false)
    , logger(std::move(logger))
    , loggerComponent( this->logger.assigned()
                          ? this->logger.getOrAddComponent("Scheduler")
                          : throw ArgumentNullException("Logger must not be null"))
    , executor(std::make_unique<tf::Executor>(numWorkers < 1 ? std::thread::hardware_concurrency() : numWorkers,
               std::make_shared<CustomWorkerInterface>()))
{
    if (useMainLoop)
        mainThreadWorker = std::make_unique<MainThreadLoop>(this->logger);
    LOG_D("Starting scheduler with {} workers.", executor->num_workers())
}

SchedulerImpl::~SchedulerImpl()
{
    logger.removeComponent("Scheduler");
    if (!stopped)
        SchedulerImpl::stop();
}

ErrCode SchedulerImpl::waitAll()
{
    LOGP_I("Waiting for all current tasks to complete")
    executor->wait_for_all();

    return OPENDAQ_SUCCESS;
}

ErrCode SchedulerImpl::stop()
{
    LOGP_T("Stop requested")
    stopped = true;

    LOGP_T("Stopping scheduler")
    executor.reset();

    LOGP_T("Stopping main thread worker")
    mainThreadWorker.reset();

    LOGP_T("Stopped")
    return OPENDAQ_SUCCESS;
}

ErrCode SchedulerImpl::checkAndPrepare(const IBaseObject* work, IAwaitable** awaitable)
{
    OPENDAQ_PARAM_NOT_NULL(work);
    OPENDAQ_PARAM_NOT_NULL(awaitable);

    if (stopped)
        return DAQ_MAKE_ERROR_INFO(OPENDAQ_ERR_SCHEDULER_STOPPED);

    return OPENDAQ_SUCCESS;
}


ErrCode SchedulerImpl::scheduleFunction(IFunction* function, IAwaitable** awaitable)
{
    ErrCode errCode = checkAndPrepare(function, awaitable);
    OPENDAQ_RETURN_IF_FAILED(errCode);

    auto scheduled = createWithImplementation<IAwaitable, AwaitableFunc>(
        executor->async([func = FunctionPtr(function)]() mutable
            {
                return func(nullptr);
            })
        );

    *awaitable = scheduled.detach();
    return OPENDAQ_SUCCESS;
}

ErrCode SchedulerImpl::scheduleWork(IWork* work)
{
    OPENDAQ_PARAM_NOT_NULL(work);

    if (stopped)
        return DAQ_MAKE_ERROR_INFO(OPENDAQ_ERR_SCHEDULER_STOPPED);

    executor->silent_async([work = WorkPtr(work)]()
    {
        work->execute();
    });

    return OPENDAQ_SUCCESS;
}

ErrCode SchedulerImpl::scheduleGraph(ITaskGraph* graph, IAwaitable** awaitable)
{
    ErrCode errCode = checkAndPrepare(graph, awaitable);
    OPENDAQ_RETURN_IF_FAILED(errCode);

    auto scheduled = TaskPtr::Borrow(graph).asPtrOrNull<ITaskInternal>(true);
    if (scheduled == nullptr)
        return DAQ_MAKE_ERROR_INFO(OPENDAQ_ERR_NOT_SUPPORTED);

    auto flow = scheduled->getFlow();
    if (flow == nullptr)
        return DAQ_MAKE_ERROR_INFO(OPENDAQ_ERR_NOT_SUPPORTED);

    auto awaitable_ = createWithImplementation<IAwaitable, AwaitableImpl<void>>(executor->run(*flow));
    *awaitable = awaitable_.detach();

    return OPENDAQ_SUCCESS;
}

ErrCode SchedulerImpl::isMultiThreaded(Bool* multiThreaded)
{
    OPENDAQ_PARAM_NOT_NULL(multiThreaded);

    *multiThreaded = executor->num_workers() > 1;
    return OPENDAQ_SUCCESS;
}

ErrCode SchedulerImpl::runMainLoop(SizeT loopTime)
{
    if (!mainThreadWorker)
        return DAQ_MAKE_ERROR_INFO(OPENDAQ_ERR_NOT_SUPPORTED, "Main thread worker is not set");
    
    return mainThreadWorker->run(loopTime);
}

ErrCode SchedulerImpl::isMainLoopSet(Bool* isSet)
{
    OPENDAQ_PARAM_NOT_NULL(isSet);
    *isSet = mainThreadWorker != nullptr;
    return OPENDAQ_SUCCESS;
}

ErrCode SchedulerImpl::stopMainLoop()
{
    if (!mainThreadWorker)
        return DAQ_MAKE_ERROR_INFO(OPENDAQ_ERR_NOT_SUPPORTED, "Main thread worker is not set");
    
    return mainThreadWorker->stop();
}

ErrCode SchedulerImpl::runMainLoopIteration()
{
    if (!mainThreadWorker)
        return DAQ_MAKE_ERROR_INFO(OPENDAQ_ERR_NOT_SUPPORTED, "Main thread worker is not set");
    
    return mainThreadWorker->runIteration();
}

ErrCode SchedulerImpl::scheduleWorkOnMainLoop(IWork* work)
{
    OPENDAQ_PARAM_NOT_NULL(work);
    if (!mainThreadWorker)
        return DAQ_MAKE_ERROR_INFO(OPENDAQ_ERR_NOT_SUPPORTED, "Main thread worker is not set");
    return mainThreadWorker->scheduleTask(work);
}

OPENDAQ_DEFINE_CLASS_FACTORY(
    LIBRARY_FACTORY, Scheduler,
    ILogger*, logger,
    SizeT, numWorkers
)

OPENDAQ_DEFINE_CLASS_FACTORY_WITH_INTERFACE_AND_CREATEFUNC(
    LIBRARY_FACTORY, Scheduler,
    IScheduler, createSchedulerWithMainLoop,
    ILogger*, logger,
    SizeT, numWorkers,
    Bool, useMainLoop
)

END_NAMESPACE_OPENDAQ
