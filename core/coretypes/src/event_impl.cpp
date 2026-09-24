#include <coretypes/event_impl.h>
#include <algorithm>

BEGIN_NAMESPACE_OPENDAQ

namespace
{
    // Lets removeHandler() and clear() find the calls on their own thread, because waiting for those would deadlock.
    // The calls of all events form a list through trigger()'s stack frames, innermost first.
    struct Call
    {
        const void* handler;
        const Call* outer;
    };

    thread_local const Call* innermostCall = nullptr;

    SizeT countCallsOnThisThread(const void* handler)
    {
        SizeT count = 0;
        for (const Call* call = innermostCall; call != nullptr; call = call->outer)
        {
            if (call->handler == handler)
                ++count;
        }
        return count;
    }
}

// Ends the record of a call when it goes out of scope, so that a handler that throws can't leave removeHandler() waiting forever.
class EventImpl::CallInProgress
{
public:
    CallInProgress(EventImpl& event, Handler& handler)
        : event(event)
        , handler(handler)
        , call{&handler, innermostCall}
    {
        handler.inFlight.fetch_add(1);
        innermostCall = &call;
    }

    ~CallInProgress()
    {
        innermostCall = call.outer;
        event.finishCall(handler);
    }

    // Prevent copying as innermostCall points into this object.
    CallInProgress(const CallInProgress&) = delete;
    CallInProgress& operator=(const CallInProgress&) = delete;

private:
    EventImpl& event;
    Handler& handler;
    const Call call;
};

EventImpl::Handler::Handler(EventHandler eventHandler, SizeT hashCode)
    : eventHandler(std::move(eventHandler))
    , hashCode(hashCode)
{
}

EventImpl::EventImpl()
    : handlers(std::make_shared<const HandlerList>())
{
}

ErrCode EventImpl::addHandler(IEventHandler* eventHandler)
{
    OPENDAQ_PARAM_NOT_NULL(eventHandler);

    SizeT hashCode;
    const ErrCode errCode = eventHandler->getHashCode(&hashCode);
    OPENDAQ_RETURN_IF_FAILED(errCode);

    std::scoped_lock lock(sync);

    if (frozen)
    {
        return DAQ_MAKE_ERROR_INFO(OPENDAQ_ERR_FROZEN);
    }

    auto newHandlers = std::make_shared<HandlerList>(*handlers);
    newHandlers->push_back(HandlerEntry{
        std::make_shared<Handler>(EventHandler(eventHandler), hashCode),
        false
    });

    handlers = std::move(newHandlers);
    return OPENDAQ_SUCCESS;
}

ErrCode EventImpl::removeHandler(IEventHandler* eventHandler)
{
    OPENDAQ_PARAM_NOT_NULL(eventHandler);

    SizeT hashCode;
    const ErrCode errCode = eventHandler->getHashCode(&hashCode);
    OPENDAQ_RETURN_IF_FAILED(errCode);

    std::unique_lock lock(sync);

    if (frozen)
    {
        return DAQ_MAKE_ERROR_INFO(OPENDAQ_ERR_FROZEN);
    }

    const auto iterator = std::find_if(handlers->begin(),
                                       handlers->end(),
                                       [hashCode](const HandlerEntry& entry)
                                       {
                                           return entry.handler->hashCode == hashCode;
                                       });
    if (iterator == handlers->end())
    {
        return OPENDAQ_SUCCESS;
    }

    const std::vector<std::shared_ptr<Handler>> removedHandlers{iterator->handler};
    iterator->handler->removed = true;

    auto newHandlers = std::make_shared<HandlerList>(*handlers);
    newHandlers->erase(newHandlers->begin() + (iterator - handlers->begin()));
    handlers = std::move(newHandlers);

    // After this returns, the handler isn't running on another thread and won't be called again
    // so the object it belongs to can then be destroyed.
    waitForCallsOnOtherThreads(lock, removedHandlers);
    return OPENDAQ_SUCCESS;
}

ErrCode EventImpl::clear()
{
    std::unique_lock lock(sync);

    if (frozen)
    {
        return DAQ_MAKE_ERROR_INFO(OPENDAQ_ERR_FROZEN);
    }

    std::vector<std::shared_ptr<Handler>> removedHandlers;
    removedHandlers.reserve(handlers->size());
    for (const auto& entry : *handlers)
    {
        entry.handler->removed = true;
        removedHandlers.push_back(entry.handler);
    }
    handlers = std::make_shared<const HandlerList>();

    waitForCallsOnOtherThreads(lock, removedHandlers);
    return OPENDAQ_SUCCESS;
}

ErrCode EventImpl::getSubscriberCount(SizeT* count)
{
    std::scoped_lock lock(sync);

    *count = handlers->size();

    return OPENDAQ_SUCCESS;
}

ErrCode EventImpl::getSubscribers(IList** subscribers)
{
    OPENDAQ_PARAM_NOT_NULL(subscribers);

    std::scoped_lock lock(sync);

    auto list = List<IEventHandler>();

    for (const auto& entry : *handlers)
        list.pushBack(entry.handler->eventHandler);

    *subscribers = list.detach();
    return OPENDAQ_SUCCESS;
}

ErrCode EventImpl::trigger(IBaseObject* sender, IEventArgs* args)
{
    if (muted)
    {
        return OPENDAQ_SUCCESS;
    }

    const auto currentHandlers = getHandlers();
    for (const auto& entry : *currentHandlers)
    {
        if (entry.muted)
            continue;

        // Record the call before checking whether the handler was removed, so that no call starts after removeHandler() returns.
        // removeHandler() does it in the opposite order: it marks the handler removed, then checks for calls.
        // Either this skips the handler, or removeHandler() waits for this call.
        Handler& handler = *entry.handler;
        const CallInProgress callInProgress(*this, handler);
        if (handler.removed.load())
            continue;

        const ErrCode errCode = handler.eventHandler->handleEvent(sender, args);
        OPENDAQ_RETURN_IF_FAILED(errCode);
    }

    return OPENDAQ_SUCCESS;
}

std::shared_ptr<const EventImpl::HandlerList> EventImpl::getHandlers() const
{
    std::scoped_lock lock(sync);
    return handlers;
}

void EventImpl::finishCall(Handler& handler)
{
    handler.inFlight.fetch_sub(1);

    // Paired with waitForCallsOnOtherThreads(): the waiter announces itself before it checks inFlight.
    // Either this wakes the waiter, or the waiter sees that the call has ended.
    if (waitingForCalls.load() > 0)
    {
        std::scoped_lock lock(sync);
        callFinished.notify_all();
    }
}

void EventImpl::waitForCallsOnOtherThreads(std::unique_lock<daq::mutex>& lock, const std::vector<std::shared_ptr<Handler>>& removedHandlers)
{
    ++waitingForCalls;
    callFinished.wait(lock, [&removedHandlers]()
    {
        // A call on this thread is the handler removing itself, or a handler it called removing it.
        // Waiting for such a call would deadlock.
        return std::all_of(
            removedHandlers.begin(),
            removedHandlers.end(),
            [](const std::shared_ptr<Handler>& handler)
            {
                return handler->inFlight.load() <= countCallsOnThisThread(handler.get());
            });
    });
    --waitingForCalls;
}

ErrCode EventImpl::freeze()
{
    std::scoped_lock lock(sync);

    if (frozen)
        return OPENDAQ_IGNORED;

    frozen = true;
    return OPENDAQ_SUCCESS;
}

ErrCode EventImpl::isFrozen(Bool* isFrozen) const
{
    std::scoped_lock lock(sync);

    *isFrozen = frozen;

    return OPENDAQ_SUCCESS;
}

ErrCode EventImpl::toString(CharPtr* str)
{
    return daqDuplicateCharPtr("Event", str);
}

ErrCode EventImpl::mute()
{
    std::scoped_lock lock(sync);

    muted = true;
    return OPENDAQ_SUCCESS;
}

ErrCode EventImpl::unmute()
{
    std::scoped_lock lock(sync);

    muted = false;
    return OPENDAQ_SUCCESS;
}

ErrCode EventImpl::muteListener(IEventHandler* eventHandler)
{
    std::scoped_lock lock(sync);

    return setMuted(eventHandler, true);
}

ErrCode EventImpl::unmuteListener(IEventHandler* eventHandler)
{
    std::scoped_lock lock(sync);

    return setMuted(eventHandler, false);
}

ErrCode EventImpl::setMuted(IEventHandler* eventHandler, bool muted)
{
    OPENDAQ_PARAM_NOT_NULL(eventHandler);

    SizeT hashCode;
    ErrCode errCode = eventHandler->getHashCode(&hashCode);

    OPENDAQ_RETURN_IF_FAILED(errCode);

    const auto iterator = std::find_if(handlers->begin(),
                                       handlers->end(),
                                       [hashCode](const HandlerEntry& entry)
                                       {
                                           return entry.handler->hashCode == hashCode;
                                       });

    if (iterator != handlers->end() && iterator->muted != muted)
    {
        auto newHandlers = std::make_shared<HandlerList>(*handlers);
        (*newHandlers)[iterator - handlers->begin()].muted = muted;
        handlers = std::move(newHandlers);
    }

    return OPENDAQ_SUCCESS;
}

ErrCode EventImpl::clone(IBaseObject** cloned)
{
    OPENDAQ_PARAM_NOT_NULL(cloned);

    std::scoped_lock lock(sync);

    auto* newEvent = new(std::nothrow) EventImpl();
    if (newEvent == nullptr)
        return OPENDAQ_ERR_NOMEMORY;

    auto clonedHandlers = std::make_shared<HandlerList>();
    for (const auto& entry : *handlers)
    {
        clonedHandlers->push_back(HandlerEntry{
            std::make_shared<Handler>(entry.handler->eventHandler, entry.handler->hashCode),
            entry.muted
        });
    }

    newEvent->handlers = std::move(clonedHandlers);

    ErrCode errCode = newEvent->queryInterface(IBaseObject::Id, reinterpret_cast<void**>(cloned));
    if (errCode == OPENDAQ_ERR_NOINTERFACE)
        return DAQ_MAKE_ERROR_INFO(errCode);
    return errCode;
}

OPENDAQ_DEFINE_CLASS_FACTORY(LIBRARY_FACTORY, Event)

END_NAMESPACE_OPENDAQ
