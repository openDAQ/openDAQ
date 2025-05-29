#include <pybind11/gil.h>
#include <pybind11/pybind11.h>

#include <coretypes/ctutils.h>
#include <coretypes/event_handler.h>
#include <coretypes/event_handler_ptr.h>

#include "py_core_types/py_queued_event_handler.h"
#include "py_core_types/py_event_queue.h"

#include <queue>
#include <mutex>
#include <functional>

BEGIN_NAMESPACE_OPENDAQ

namespace
{
    std::queue<std::function<void()>> callbackQueue;
    std::mutex callbackQueueMutex;
}

void enqueuePythonEvent(PyQueuedEventHandler* eventHandler, daq::ObjectPtr<daq::IBaseObject> sender, daq::ObjectPtr<daq::IEventArgs> eventArgs)
{
    if (eventHandler == nullptr)
        return;

    IEventHandler* eventHandlerInterface;
    const auto err = eventHandler->borrowInterface(IEventHandler::Id, reinterpret_cast<void**>(&eventHandlerInterface));
    checkErrorInfo(err);
    EventHandlerPtr eventHandlerPtr = eventHandlerInterface;

    std::lock_guard<std::mutex> lock(callbackQueueMutex);
    callbackQueue.push([eventHandler
                        , eventHandlerPtr = std::move(eventHandlerPtr)
                        , sender = std::move(sender)
                        , eventArgs = std::move(eventArgs)] 
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

END_NAMESPACE_OPENDAQ
