#include <opendaq/scheduler_impl.h>
#include <opendaq/scheduler_errors.h>

#include <opendaq/awaitable_impl.h>

#include <opendaq/custom_log.h>

#include <opendaq/task_internal.h>
#include <opendaq/task_ptr.h>

#include <coretypes/function_ptr.h>

#include <utility>

BEGIN_NAMESPACE_OPENDAQ

SchedulerImpl::SchedulerImpl(LoggerPtr logger, SizeT numWorkers)
    : stopped(false)
    , logger(std::move(logger))
    , loggerComponent( this->logger.assigned()
                          ? this->logger.getOrAddComponent("Scheduler")
                          : throw ArgumentNullException("Logger must not be null"))
    , executor(std::make_unique<tf::Executor>(numWorkers < 1 ? std::thread::hardware_concurrency() : numWorkers))
{
    LOG_T("Starting scheduler with {} workers.", executor->num_workers())
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

    LOGP_T("Stopped")
    return OPENDAQ_SUCCESS;
}

ErrCode SchedulerImpl::checkAndPrepare(const IBaseObject* work, IAwaitable** awaitable)
{
    if (work == nullptr || awaitable == nullptr)
        return OPENDAQ_ERR_ARGUMENT_NULL;

    if (stopped)
        return OPENDAQ_ERR_SCHEDULER_STOPPED;

    return OPENDAQ_SUCCESS;
}


ErrCode SchedulerImpl::scheduleWork(IFunction* work, IAwaitable** awaitable)
{
    ErrCode errCode = checkAndPrepare(work, awaitable);
    if (OPENDAQ_FAILED(errCode))
        return errCode;

    auto scheduled = createWithImplementation<IAwaitable, AwaitableFunc>(
        executor->async(
            [func = FunctionPtr(work)]() mutable
            {
                return func(nullptr);
            })
        );

    *awaitable = scheduled.detach();
    return OPENDAQ_SUCCESS;
}

ErrCode SchedulerImpl::scheduleGraph(ITaskGraph* graph, IAwaitable** awaitable)
{
    ErrCode errCode = checkAndPrepare(graph, awaitable);
    if (OPENDAQ_FAILED(errCode))
        return errCode;

    auto scheduled = TaskPtr::Borrow(graph).asPtrOrNull<ITaskInternal>(true);
    if (scheduled == nullptr)
        return OPENDAQ_ERR_NOT_SUPPORTED;

    auto flow = scheduled->getFlow();
    if (flow == nullptr)
        return OPENDAQ_ERR_NOT_SUPPORTED;

    auto awaitable_ = createWithImplementation<IAwaitable, AwaitableImpl<void>>(executor->run(*flow));
    *awaitable = awaitable_.detach();

    return OPENDAQ_SUCCESS;
}

ErrCode SchedulerImpl::isMultiThreaded(Bool* multiThreaded)
{
    if (multiThreaded == nullptr)
    {
        return OPENDAQ_ERR_ARGUMENT_NULL;
    }

    *multiThreaded = executor->num_workers() > 1;
    return OPENDAQ_SUCCESS;
}

std::size_t SchedulerImpl::getWorkerCount() const
{
    return executor->num_workers();
}

OPENDAQ_DEFINE_CLASS_FACTORY(
    LIBRARY_FACTORY, Scheduler,
    ILogger*, logger,
    SizeT, numWorkers
)

END_NAMESPACE_OPENDAQ
