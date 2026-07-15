#include <opendaq/periodic_work_manager.h>
#include <opendaq/scheduler_impl.h>
#include <opendaq/scheduler_errors.h>
#include <opendaq/repetitive_work_executor.h>

#include <coretypes/intfs.h>
#include <coretypes/validation.h>

#include <thread>

BEGIN_NAMESPACE_OPENDAQ

PeriodicWorkManager::PeriodicWorkManager(SchedulerImpl& scheduler,
                                         tf::Executor& executor,
                                         LoggerComponentPtr loggerComponent)
    : scheduler(scheduler)
    , loggerComponent(std::move(loggerComponent))
    , executor(this->loggerComponent)
    , tfExecutor(executor)
{
}

PeriodicWorkManager::~PeriodicWorkManager()
{
    stop();
}

void PeriodicWorkManager::stop()
{
    if (stopped.exchange(true))
        return;

    executor.cancelAll();
}

ErrCode PeriodicWorkManager::schedule(IWorkRepetitive* work)
{
    OPENDAQ_PARAM_NOT_NULL(work);

    if (stopped)
        return DAQ_MAKE_ERROR_INFO(OPENDAQ_ERR_SCHEDULER_STOPPED);

    const ErrCode err = executor.schedule(work);
    OPENDAQ_RETURN_IF_FAILED(err);

    startEntryLoop(executor.find(work));
    return OPENDAQ_SUCCESS;
}

void PeriodicWorkManager::startEntryLoop(const std::shared_ptr<RepetitiveWorkEntry>& entry)
{
    bool expected = false;
    if (!entry->loopRunning.compare_exchange_strong(expected, true))
        return;

    tfExecutor.silent_async([this, entry]() { runEntryLoop(entry); });
}

void PeriodicWorkManager::runEntryLoop(const std::shared_ptr<RepetitiveWorkEntry>& entry)
{
    SizeT intervalMs = 0;
    entry->work->getIntervalMs(&intervalMs);

    const bool hasInterval = intervalMs > 0;
    const auto interval = std::chrono::milliseconds(intervalMs);

    while (!stopped.load() && !isRepetitiveWorkCanceled(entry->work))
    {
        if (hasInterval)
        {
            const auto now = std::chrono::steady_clock::now();
            if (now < entry->nextRun)
                std::this_thread::sleep_until(entry->nextRun);

            if (stopped.load() || isRepetitiveWorkCanceled(entry->work))
                break;
        }

        if (isRepetitiveWorkCanceled(entry->work))
            break;

        executor.execute(entry->work);

        if (hasInterval)
        {
            entry->nextRun += interval;
            const auto afterExecute = std::chrono::steady_clock::now();
            if (entry->nextRun < afterExecute)
                entry->nextRun = afterExecute;
        }
    }

    entry->loopRunning = false;
    executor.finishCancel(entry, [this](const WorkPtr& callback) {
        if (stopped || !callback.assigned())
            return;

        const ErrCode errCode = scheduler.scheduleWork(callback);
        checkErrorInfoExcept(errCode, OPENDAQ_ERR_SCHEDULER_STOPPED);
    });
}

END_NAMESPACE_OPENDAQ
