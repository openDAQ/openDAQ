//------------------------------------------------------------------------------
// <auto-generated>
//     This code was generated by a tool.
//
//     Changes to this file may cause incorrect behavior and will be lost if
//     the code is regenerated.
//
//     RTGen (CGenerator v0.5.0) on 31.03.2025 16:56:23.
// </auto-generated>
//------------------------------------------------------------------------------

#include "ccoretypes/event.h"

#include <opendaq/opendaq.h>

#include "copendaq_private.h"

const IntfID EVENT_INTF_ID = { daq::IEvent::Id.Data1, daq::IEvent::Id.Data2, daq::IEvent::Id.Data3, daq::IEvent::Id.Data4_UInt64 };

ErrCode Event_addHandler(Event* self, EventHandler* eventHandler)
{
    return reinterpret_cast<daq::IEvent*>(self)->addHandler(reinterpret_cast<daq::IEventHandler*>(eventHandler));
}

ErrCode Event_removeHandler(Event* self, EventHandler* eventHandler)
{
    return reinterpret_cast<daq::IEvent*>(self)->removeHandler(reinterpret_cast<daq::IEventHandler*>(eventHandler));
}

ErrCode Event_trigger(Event* self, BaseObject* sender, EventArgs* args)
{
    return reinterpret_cast<daq::IEvent*>(self)->trigger(reinterpret_cast<daq::IBaseObject*>(sender), reinterpret_cast<daq::IEventArgs*>(args));
}

ErrCode Event_clear(Event* self)
{
    return reinterpret_cast<daq::IEvent*>(self)->clear();
}

ErrCode Event_getSubscriberCount(Event* self, SizeT* count)
{
    return reinterpret_cast<daq::IEvent*>(self)->getSubscriberCount(count);
}

ErrCode Event_getSubscribers(Event* self, List** subscribers)
{
    return reinterpret_cast<daq::IEvent*>(self)->getSubscribers(reinterpret_cast<daq::IList**>(subscribers));
}

ErrCode Event_mute(Event* self)
{
    return reinterpret_cast<daq::IEvent*>(self)->mute();
}

ErrCode Event_unmute(Event* self)
{
    return reinterpret_cast<daq::IEvent*>(self)->unmute();
}

ErrCode Event_muteListener(Event* self, EventHandler* eventHandler)
{
    return reinterpret_cast<daq::IEvent*>(self)->muteListener(reinterpret_cast<daq::IEventHandler*>(eventHandler));
}

ErrCode Event_unmuteListener(Event* self, EventHandler* eventHandler)
{
    return reinterpret_cast<daq::IEvent*>(self)->unmuteListener(reinterpret_cast<daq::IEventHandler*>(eventHandler));
}

ErrCode Event_createEvent(Event** obj)
{
    daq::IEvent* ptr = nullptr;
    ErrCode err = daq::createEvent(&ptr);
    *obj = reinterpret_cast<Event*>(ptr);
    return err;
}
