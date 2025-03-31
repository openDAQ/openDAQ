#include "ccoretypes/factories.h"

#include <opendaq/opendaq.h>

ErrCode Serializer_createJsonSerializer(Serializer** obj, Bool pretty)
{
    daq::ISerializer* ptr = nullptr;
    ErrCode err = daq::createJsonSerializer(&ptr, pretty);
    *obj = reinterpret_cast<Serializer*>(ptr);
    return err;
}