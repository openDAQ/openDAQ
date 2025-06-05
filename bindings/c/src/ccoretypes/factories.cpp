#include "ccoretypes/factories.h"

#include <opendaq/opendaq.h>

#include "private/c_event_handler_impl.h"

daqErrCode daqSerializer_createJsonSerializer(daqSerializer** obj, daqBool pretty)
{
    daq::ISerializer* ptr = nullptr;
    daqErrCode err = daq::createJsonSerializer(&ptr, pretty);
    *obj = reinterpret_cast<daqSerializer*>(ptr);
    return err;
}

daqErrCode daqEventHandler_createEventHandler(daqEventHandler** obj, daqEventCall call)
{
    daq::IEventHandler* ptr = nullptr;
    daqErrCode err = daq::createObjectForwarding<daq::IEventHandler, daq::CEventHandlerImpl>(&ptr, call);
    *obj = reinterpret_cast<daqEventHandler*>(ptr);
    return err;
}