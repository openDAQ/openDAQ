//------------------------------------------------------------------------------
// <auto-generated>
//     This code was generated by a tool.
//
//     Changes to this file may cause incorrect behavior and will be lost if
//     the code is regenerated.
//
//     RTGen (CGenerator v0.7.0) on 03.06.2025 17:18:00.
// </auto-generated>
//------------------------------------------------------------------------------

#include <ccoretypes/serializer.h>

#include <opendaq/opendaq.h>

#include <copendaq_private.h>

const daqIntfID DAQ_SERIALIZER_INTF_ID = { daq::ISerializer::Id.Data1, daq::ISerializer::Id.Data2, daq::ISerializer::Id.Data3, daq::ISerializer::Id.Data4_UInt64 };

daqErrCode daqSerializer_startTaggedObject(daqSerializer* self, daqSerializable* obj)
{
    return reinterpret_cast<daq::ISerializer*>(self)->startTaggedObject(reinterpret_cast<daq::ISerializable*>(obj));
}

daqErrCode daqSerializer_startObject(daqSerializer* self)
{
    return reinterpret_cast<daq::ISerializer*>(self)->startObject();
}

daqErrCode daqSerializer_endObject(daqSerializer* self)
{
    return reinterpret_cast<daq::ISerializer*>(self)->endObject();
}

daqErrCode daqSerializer_startList(daqSerializer* self)
{
    return reinterpret_cast<daq::ISerializer*>(self)->startList();
}

daqErrCode daqSerializer_endList(daqSerializer* self)
{
    return reinterpret_cast<daq::ISerializer*>(self)->endList();
}

daqErrCode daqSerializer_getOutput(daqSerializer* self, daqString** serialized)
{
    return reinterpret_cast<daq::ISerializer*>(self)->getOutput(reinterpret_cast<daq::IString**>(serialized));
}

daqErrCode daqSerializer_key(daqSerializer* self, daqConstCharPtr string)
{
    return reinterpret_cast<daq::ISerializer*>(self)->key(static_cast<daq::ConstCharPtr>(string));
}

daqErrCode daqSerializer_keyStr(daqSerializer* self, daqString* name)
{
    return reinterpret_cast<daq::ISerializer*>(self)->keyStr(reinterpret_cast<daq::IString*>(name));
}

daqErrCode daqSerializer_keyRaw(daqSerializer* self, daqConstCharPtr string, daqSizeT length)
{
    return reinterpret_cast<daq::ISerializer*>(self)->keyRaw(static_cast<daq::ConstCharPtr>(string), length);
}

daqErrCode daqSerializer_writeInt(daqSerializer* self, daqInt integer)
{
    return reinterpret_cast<daq::ISerializer*>(self)->writeInt(integer);
}

daqErrCode daqSerializer_writeBool(daqSerializer* self, daqBool boolean)
{
    return reinterpret_cast<daq::ISerializer*>(self)->writeBool(boolean);
}

daqErrCode daqSerializer_writeFloat(daqSerializer* self, daqFloat real)
{
    return reinterpret_cast<daq::ISerializer*>(self)->writeFloat(real);
}

daqErrCode daqSerializer_writeString(daqSerializer* self, daqConstCharPtr string, daqSizeT length)
{
    return reinterpret_cast<daq::ISerializer*>(self)->writeString(static_cast<daq::ConstCharPtr>(string), length);
}

daqErrCode daqSerializer_writeNull(daqSerializer* self)
{
    return reinterpret_cast<daq::ISerializer*>(self)->writeNull();
}

daqErrCode daqSerializer_reset(daqSerializer* self)
{
    return reinterpret_cast<daq::ISerializer*>(self)->reset();
}

daqErrCode daqSerializer_isComplete(daqSerializer* self, daqBool* complete)
{
    return reinterpret_cast<daq::ISerializer*>(self)->isComplete(complete);
}

daqErrCode daqSerializer_getUser(daqSerializer* self, daqBaseObject** user)
{
    return reinterpret_cast<daq::ISerializer*>(self)->getUser(reinterpret_cast<daq::IBaseObject**>(user));
}

daqErrCode daqSerializer_setUser(daqSerializer* self, daqBaseObject* user)
{
    return reinterpret_cast<daq::ISerializer*>(self)->setUser(reinterpret_cast<daq::IBaseObject*>(user));
}

daqErrCode daqSerializer_getVersion(daqSerializer* self, daqInt* version)
{
    return reinterpret_cast<daq::ISerializer*>(self)->getVersion(version);
}
