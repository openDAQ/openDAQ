#include <pybind11/gil.h>
#include <pybind11/pybind11.h>

#include <coretypes/ctutils.h>
#include <coretypes/event_handler.h>
#include <coretypes/event_handler_ptr.h>

#include "py_core_types/py_event_queue.h"

std::weak_ptr<PyEventQueue> eventQueueWeakPtr;

std::shared_ptr<PyEventQueue> PyEventQueue::Create()
{
    auto existing = eventQueueWeakPtr.lock();
    if (existing)
    {
        return existing;
    }

    auto queue = std::shared_ptr<PyEventQueue>(new PyEventQueue());
    eventQueueWeakPtr = queue;
    return queue;
}

std::weak_ptr<PyEventQueue> PyEventQueue::GetWeak()
{
    return eventQueueWeakPtr;
}

void PyEventQueue::enqueueEvent(daq::IPythonQueuedEventHandler* eventHandler, daq::IBaseObject* sender, daq::IEventArgs* eventArgs)
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

void PyEventQueue::processEvents()
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

void PyEventQueue::clearQueue()
{
    std::lock_guard<std::mutex> lock(callbackQueueMutex);
    callbackQueue = {};
}

pybind11::class_<PyEventQueue, std::shared_ptr<PyEventQueue>> declarePyEventQueue(pybind11::module_ m)
{
    return pybind11::class_<PyEventQueue, std::shared_ptr<PyEventQueue>>(m, "EventQueue");
}

void definePyEventQueue(pybind11::module_ m, pybind11::class_<PyEventQueue, std::shared_ptr<PyEventQueue>> cls)
{
    cls.def("enqueue_event", &PyEventQueue::enqueueEvent);
    cls.def("process_events", &PyEventQueue::processEvents);
    cls.def("clear", &PyEventQueue::clearQueue);
}
