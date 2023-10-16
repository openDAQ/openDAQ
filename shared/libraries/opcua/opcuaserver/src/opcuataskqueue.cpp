#include "opcuaserver/opcuataskqueue.h"
#include "opcuashared/opcua.h"

BEGIN_NAMESPACE_OPENDAQ_OPCUA

void OpcUaTaskQueue::push(Function&& func)
{
    {
        std::lock_guard<std::mutex> guard(mutex_);
        q_.emplace_back(func);
    }
    cv_.notify_one();
}

bool OpcUaTaskQueue::pop(Function& func)
{
    std::lock_guard<std::mutex> guard(mutex_);
    if (q_.empty())
    {
        return false;
    }
    else
    {
        func = q_.front();
        q_.pop_front();
        return true;
    }
}

bool OpcUaTaskQueue::pop(Function& func, const unsigned int timeout_ms)
{
    std::unique_lock lock(mutex_);
    if (timeout_ms == infinity)
    {
        cv_.wait(lock, [& q = q_]() { return !q.empty(); });
    }
    else
    {
        cv_.wait_for(lock, std::chrono::milliseconds(timeout_ms), [& q = q_]() { return !q.empty(); });
    }

    if (q_.empty())
    {
        return false;
    }
    else
    {
        func = q_.front();
        q_.pop_front();
        return true;
    }
}

void OpcUaTaskQueue::processTaskQueue()
{
    OpcUaTaskQueue::Function func;

    while (pop(func))
    {
        func();
    }
}

END_NAMESPACE_OPENDAQ_OPCUA
