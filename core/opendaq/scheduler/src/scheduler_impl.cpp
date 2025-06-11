#include <opendaq/scheduler_impl.h>
#include <opendaq/scheduler_errors.h>

#include <opendaq/awaitable_impl.h>

#include <opendaq/custom_log.h>

#include <opendaq/task_internal.h>
#include <opendaq/task_ptr.h>
#include <opendaq/work_ptr.h>
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

SchedulerImpl::SchedulerImpl(LoggerPtr logger, SizeT numWorkers)
    : stopped(false)
    , logger(std::move(logger))
    , loggerComponent( this->logger.assigned()
                          ? this->logger.getOrAddComponent("Scheduler")
                          : throw ArgumentNullException("Logger must not be null"))
    , executor(std::make_unique<tf::Executor>(numWorkers < 1 ? std::thread::hardware_concurrency() : numWorkers,
               std::make_shared<CustomWorkerInterface>()))
    , mainThreadWorker(std::make_unique<MainThreadWorker>())
{
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

std::size_t SchedulerImpl::getWorkerCount() const
{
    return executor->num_workers();
}

ErrCode SchedulerImpl::mainLoop()
{    
    mainThreadWorker->run();
    return OPENDAQ_SUCCESS;
}

ErrCode SchedulerImpl::proccessMainThreadTasks()
{
    mainThreadWorker->runIteration();
    return OPENDAQ_SUCCESS;
}

ErrCode SchedulerImpl::scheduleWorkOnMainThread(IWork* work)
{
    OPENDAQ_PARAM_NOT_NULL(work);
    return mainThreadWorker->execute(work);
}

OPENDAQ_DEFINE_CLASS_FACTORY(
    LIBRARY_FACTORY, Scheduler,
    ILogger*, logger,
    SizeT, numWorkers
)

END_NAMESPACE_OPENDAQ
