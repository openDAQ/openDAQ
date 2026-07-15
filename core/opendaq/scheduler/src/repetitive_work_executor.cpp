#include <opendaq/repetitive_work_executor.h>
#include <opendaq/scheduler_errors.h>
#include <opendaq/work_repetitive_internal.h>
#include <opendaq/custom_log.h>

#include <coretypes/intfs.h>
#include <coretypes/validation.h>

#include <sstream>

BEGIN_NAMESPACE_OPENDAQ

RepetitiveWorkExecutor::RepetitiveWorkExecutor(LoggerComponentPtr loggerComponent)
    : loggerComponent(std::move(loggerComponent))
{
}

void RepetitiveWorkExecutor::logExecutionError() const
{
    ListPtr<IErrorInfo> errorInfos;
    daqGetErrorInfoList(&errorInfos);
    if (!errorInfos.assigned())
        return;

    std::ostringstream errorStream;
    bool firstMessage = true;
    for (const auto& errorInfo : errorInfos)
    {
        StringPtr message;
        if (OPENDAQ_FAILED(errorInfo->getMessage(&message)) || !message.assigned())
            continue;

        errorStream << (firstMessage ? "" : "\n") << message;
        firstMessage = false;

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

    if (!firstMessage)
        LOG_W("Error executing work: {}", errorStream.str());
}

bool isRepetitiveWorkCanceled(const WorkRepetitivePtr& work)
{
    if (!work.assigned())
        return false;

    Bool canceled = false;
    if (OPENDAQ_FAILED(work->isCanceled(&canceled)))
        return false;

    return canceled;
}

ErrCode RepetitiveWorkExecutor::schedule(IWorkRepetitive* work)
{
    OPENDAQ_PARAM_NOT_NULL(work);

    std::lock_guard<std::mutex> lock(mutex);
    if (entriesByWork.find(work) != entriesByWork.end())
        return DAQ_MAKE_ERROR_INFO(OPENDAQ_ERR_INVALIDPARAMETER, "Work is already scheduled repetitively");

    auto entry = std::make_shared<RepetitiveWorkEntry>();
    entry->work = WorkRepetitivePtr::Borrow(work);
    entry->nextRun = std::chrono::steady_clock::now();
    entriesByWork.emplace(work, entry);
    return OPENDAQ_SUCCESS;
}

std::shared_ptr<RepetitiveWorkEntry> RepetitiveWorkExecutor::find(IWorkRepetitive* work) const
{
    std::lock_guard<std::mutex> lock(mutex);
    const auto it = entriesByWork.find(work);
    if (it == entriesByWork.end())
        return nullptr;

    return it->second;
}

std::vector<std::shared_ptr<RepetitiveWorkEntry>> RepetitiveWorkExecutor::entries() const
{
    std::lock_guard<std::mutex> lock(mutex);
    std::vector<std::shared_ptr<RepetitiveWorkEntry>> result;
    result.reserve(entriesByWork.size());
    for (const auto& [_, entry] : entriesByWork)
        result.push_back(entry);
    return result;
}

void RepetitiveWorkExecutor::finishCancel(const std::shared_ptr<RepetitiveWorkEntry>& entry, const ScheduleAfterFn& scheduleAfter)
{
    IWorkRepetitive* key = nullptr;
    {
        std::lock_guard<std::mutex> lock(entry->idleMutex);
        if (entry->idleNotified.exchange(true))
            return;

        key = entry->work.getObject();
        entry->idleCv.notify_all();
    }

    WorkPtr afterCallback;
    const auto internal = entry->work.asPtrOrNull<IWorkRepetitiveInternal>(true);
    if (internal.assigned())
    {
        const ErrCode err = internal->getOnStopCallback(&afterCallback);
        if (OPENDAQ_FAILED(err))
            daqClearErrorInfo();
    }

    {
        std::lock_guard<std::mutex> lock(mutex);
        if (key != nullptr)
            entriesByWork.erase(key);
    }

    if (afterCallback.assigned() && scheduleAfter)
        scheduleAfter(afterCallback);
}

void RepetitiveWorkExecutor::waitUntilIdle(const std::shared_ptr<RepetitiveWorkEntry>& entry)
{
    std::unique_lock<std::mutex> lock(entry->idleMutex);
    entry->idleCv.wait(lock, [&] { return entry->idleNotified.load(); });
}

void RepetitiveWorkExecutor::cancelAll()
{
    std::vector<std::shared_ptr<RepetitiveWorkEntry>> activeEntries;
    {
        std::lock_guard<std::mutex> lock(mutex);
        activeEntries.reserve(entriesByWork.size());
        for (const auto& [_, entry] : entriesByWork)
        {
            entry->work.cancel();
            activeEntries.push_back(entry);
        }
    }

    for (const auto& entry : activeEntries)
        waitUntilIdle(entry);
}

ErrCode RepetitiveWorkExecutor::execute(const WorkRepetitivePtr& work)
{
    if (!work.assigned())
        return DAQ_MAKE_ERROR_INFO(OPENDAQ_ERR_INVALIDPARAMETER, "Work must not be null");

    const ErrCode err = work.asPtr<IWork>()->execute();
    if (OPENDAQ_FAILED(err))
        logExecutionError();

    return err;
}

ErrCode RepetitiveWorkExecutor::executeRepetitively(const WorkRepetitivePtr& work, Bool* repeatAfter)
{
    OPENDAQ_PARAM_NOT_NULL(repeatAfter);
    *repeatAfter = False;

    if (!work.assigned())
        return DAQ_MAKE_ERROR_INFO(OPENDAQ_ERR_INVALIDPARAMETER, "Work must not be null");

    const ErrCode err = work->executeRepetitively(repeatAfter);
    if (OPENDAQ_FAILED(err))
        logExecutionError();

    return err;
}

END_NAMESPACE_OPENDAQ
