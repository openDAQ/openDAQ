#include <coretypes/event_impl.h>
#include <algorithm>

BEGIN_NAMESPACE_OPENDAQ

EventImpl::EventImpl()
{
    handlers.reserve(5);
}

ErrCode EventImpl::addHandler(IEventHandler* eventHandler)
{
    if (eventHandler == nullptr)
        return OPENDAQ_ERR_ARGUMENT_NULL;

    if (frozen)
    {
        return OPENDAQ_ERR_FROZEN;
    }

    handlers.emplace_back(Handler{ eventHandler, false });
    return OPENDAQ_SUCCESS;
}

ErrCode EventImpl::removeHandler(IEventHandler* eventHandler)
{
    if (eventHandler == nullptr)
        return OPENDAQ_ERR_ARGUMENT_NULL;

    if (frozen)
    {
        return OPENDAQ_ERR_FROZEN;
    }

    SizeT hashCode;
    ErrCode errCode = eventHandler->getHashCode(&hashCode);

    if (OPENDAQ_FAILED(errCode))
    {
        return errCode;
    }

    const ConstIterator iterator = std::find_if(handlers.begin(),
                                                handlers.end(),
                                                [hashCode](const Handler& lhs)
                                                {
                                                    return lhs.eventHandler.getHashCode() == hashCode;
                                                });

    if (iterator != handlers.cend())
    {
        handlers.erase(iterator);
    }
    return OPENDAQ_SUCCESS;
}

ErrCode EventImpl::clear()
{
    if (frozen)
    {
        return OPENDAQ_ERR_FROZEN;
    }

    handlers.clear();

    return OPENDAQ_SUCCESS;
}

ErrCode EventImpl::getSubscriberCount(SizeT* count)
{
    *count = handlers.size();

    return OPENDAQ_SUCCESS;
}

ErrCode EventImpl::trigger(IBaseObject* sender, IEventArgs* args)
{
    if (muted)
    {
        return OPENDAQ_SUCCESS;
    }

    for (const Handler& handler : handlers)
    {
        if (!handler.muted)
        {
            const ErrCode errCode = handler.eventHandler->handleEvent(sender, args);
            if (OPENDAQ_FAILED(errCode))
                return errCode;
            continue;
        }

    }

    return OPENDAQ_SUCCESS;
}

ErrCode EventImpl::freeze()
{
    if (frozen)
        return  OPENDAQ_IGNORED;

    frozen = true;
    return OPENDAQ_SUCCESS;
}

ErrCode EventImpl::isFrozen(Bool* isFrozen) const
{
    *isFrozen = frozen;

    return OPENDAQ_SUCCESS;
}

ErrCode EventImpl::toString(CharPtr* str)
{
    return daqDuplicateCharPtr("Event", str);
}

ErrCode EventImpl::mute()
{
    muted = true;
    return OPENDAQ_SUCCESS;
}

ErrCode EventImpl::unmute()
{
    muted = false;
    return OPENDAQ_SUCCESS;
}

ErrCode EventImpl::muteListener(IEventHandler* eventHandler)
{
    return setMuted(eventHandler, true);
}

ErrCode EventImpl::unmuteListener(IEventHandler* eventHandler)
{
    return setMuted(eventHandler, false);
}

ErrCode EventImpl::setMuted(IEventHandler* eventHandler, bool muted)
{
    if (eventHandler == nullptr)
        return OPENDAQ_ERR_ARGUMENT_NULL;

    SizeT hashCode;
    ErrCode errCode = eventHandler->getHashCode(&hashCode);

    if (OPENDAQ_FAILED(errCode))
        return errCode;

    const Iterator iterator = std::find_if(handlers.begin(),
        handlers.end(),
        [hashCode](const Handler& lhs)
    {
        return lhs.eventHandler.getHashCode() == hashCode;
    });

    if (iterator != handlers.end())
        iterator->muted = muted;

    return OPENDAQ_SUCCESS;
}

OPENDAQ_DEFINE_CLASS_FACTORY(LIBRARY_FACTORY, Event)

END_NAMESPACE_OPENDAQ
