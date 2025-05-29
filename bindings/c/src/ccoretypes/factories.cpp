#include "ccoretypes/factories.h"

#include <opendaq/opendaq.h>

#include "private/c_event_handler_impl.h"

ErrCode Serializer_createJsonSerializer(Serializer** obj, Bool pretty)
{
    daq::ISerializer* ptr = nullptr;
    ErrCode err = daq::createJsonSerializer(&ptr, pretty);
    *obj = reinterpret_cast<Serializer*>(ptr);
    return err;
}

ErrCode EventHandler_createEventHandler(EventHandler** obj, EventCall call)
{
    daq::IEventHandler* ptr = nullptr;
    ErrCode err = daq::createObjectForwarding<daq::IEventHandler, daq::CEventHandlerImpl>(&ptr, call);
    *obj = reinterpret_cast<EventHandler*>(ptr);
    return err;
}