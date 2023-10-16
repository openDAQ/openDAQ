#include "opcuaclient/taskprocessor/opcuataskprocessor.h"

#include <opcuashared/opcualog.h>

BEGIN_NAMESPACE_OPENDAQ_OPCUA

using namespace daq::utils;

OpcUaTaskProcessor::OpcUaTaskProcessor(const OpcUaClientPtr& client)
    : ThreadEx()
    , client(client)
    , prevStatus(UA_STATUSCODE_GOOD)
{
    setPriority(ThreadExPriority::timeCritical);
}

OpcUaTaskProcessor::~OpcUaTaskProcessor()
{
    OpcUaTaskProcessor::stop();
}

bool OpcUaTaskProcessor::isConnected()
{
    return connected;
}

void OpcUaTaskProcessor::start()
{
    connected = client->isConnected();
    ThreadEx::start();
}

void OpcUaTaskProcessor::stop()
{
    terminate();
    cvWait.notify_one();
    ThreadEx::stop();
}

void OpcUaTaskProcessor::executeTask(const OpcUaTaskType& task)
{
    auto future = executeTaskAwait(task);
    future.get();
}

void OpcUaTaskProcessor::executeTask(std::promise<void>& promise, const OpcUaTaskType& task) const
{
    try
    {
        task(*client);
        promise.set_value();
    }
    catch (...)
    {
        try
        {
            promise.set_exception(std::current_exception());
        }
        catch (...)  // set_exception() may throw too
        {
        }
    }
}

bool OpcUaTaskProcessor::connect()
{
    bool result = false;
    executeTask([&result](OpcUaClient& client) { result = client.connect(); });
    return result;
}

void OpcUaTaskProcessor::setConnectionTimeout(uint32_t timeoutMs)
{
    executeTask([timeoutMs](OpcUaClient& client) { client.setTimeout(timeoutMs); });
}

const OpcUaClientPtr& OpcUaTaskProcessor::getClient() const noexcept
{
    return client;
}

std::future<void> OpcUaTaskProcessor::executeTaskAwait(const OpcUaTaskType& task)
{
    if (std::this_thread::get_id() == executingThreadId)  // to prevent deadlock. if its called inside task
    {
        std::promise<void> promise;
        executeTask(promise, task);
        return promise.get_future();
    }
    else
    {
        std::lock_guard guard(mutex);
        queue.push(std::make_pair(std::promise<void>(), task));
        return queue.back().first.get_future();
    }
}

OpcUaTaskProcessor::QueueItemType OpcUaTaskProcessor::getNextTask()
{
    std::lock_guard guard(mutex);
    OpcUaTaskProcessor::QueueItemType task;
    if (!queue.empty())
    {
        task = std::move(queue.front());
        queue.pop();
    }

    return task;
}

void OpcUaTaskProcessor::execute()
{
    executingThreadId = std::this_thread::get_id();
    setThreadName("OpcUaTaskProcessor_Normal");
    while (!terminated)
    {
        QueueItemType task = getNextTask();
        if (task.second)
        {
            executeTask(task.first, task.second);
        }
        else
        {
            static constexpr std::chrono::milliseconds timeout(1);
            UA_StatusCode status = client->iterate(timeout);

            if (OPCUA_STATUSCODE_FAILED(status) && prevStatus != status)  // don't log repeating errors
                LOGE << "OpcUa error occured during client iterate " << OPCUA_STATUSCODE_LOG_MESSAGE(status);

            prevStatus = status;

            if (OPCUA_STATUSCODE_FAILED(status))  // If client disconnects, we don't wait on UA_SELECT. So wait here
            {
                auto uniqueLock = std::unique_lock<std::mutex>(waitMutex);
                cvWait.wait_for(uniqueLock, std::chrono::milliseconds(timeout));
            }
        }

        connected = client->isConnected();
    }
}

END_NAMESPACE_OPENDAQ_OPCUA
