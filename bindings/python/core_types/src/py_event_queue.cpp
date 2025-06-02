#include <pybind11/gil.h>
#include <pybind11/pybind11.h>

#include <coretypes/ctutils.h>
#include <coretypes/event_handler.h>
#include <coretypes/event_handler_ptr.h>

#include "py_core_types/py_queued_event_handler_impl.h"
#include "py_core_types/py_event_queue.h"

#include <queue>
#include <mutex>
#include <functional>


namespace
{
    std::queue<std::function<void()>> callbackQueue;
    std::mutex callbackQueueMutex;
}

void enqueuePythonEvent(daq::IPythonQueuedEventHandler* eventHandler, daq::IBaseObject* sender, daq::IEventArgs* eventArgs)
{
    if (eventHandler == nullptr)
        return;

    daq::ObjectPtr<daq::IPythonQueuedEventHandler> eventHandlerPtr(eventHandler);
    daq::ObjectPtr<daq::IBaseObject> senderPtr(sender);
    daq::ObjectPtr<daq::IEventArgs> eventArgsPtr(eventArgs);

    std::lock_guard<std::mutex> lock(callbackQueueMutex);
    callbackQueue.push([eventHandler = std::move(eventHandlerPtr),
                        sender = std::move(senderPtr),
                        eventArgs = std::move(eventArgsPtr)] 
    {
        eventHandler->dispatch(sender, eventArgs);
    });
}

void processPythonEventFromQueue()
{
    std::queue<std::function<void()>> localQueue;

    {
        std::lock_guard<std::mutex> lock(callbackQueueMutex);
        std::swap(localQueue, callbackQueue);
    }

    while (!localQueue.empty())
    {
        auto cb = std::move(localQueue.front());
        localQueue.pop();
        cb();
    }
}

void clearPythonEventQueue()
{
    std::lock_guard<std::mutex> lock(callbackQueueMutex);
    callbackQueue = {};
}
