//------------------------------------------------------------------------------
// <auto-generated>
//     This code was generated by a tool.
//
//     Changes to this file may cause incorrect behavior and will be lost if
//     the code is regenerated.
//
//     RTGen (CGenerator v0.1.0) on 12.03.2025 16:25:34.
// </auto-generated>
//------------------------------------------------------------------------------

#include "ccoretypes/serializer.h"

#include <opendaq/opendaq.h>

ErrCode Serializer_startTaggedObject(Serializer* self, Serializable* obj)
{
    return reinterpret_cast<daq::ISerializer*>(self)->startTaggedObject(reinterpret_cast<daq::ISerializable*>(obj));
}

ErrCode Serializer_startObject(Serializer* self)
{
    return reinterpret_cast<daq::ISerializer*>(self)->startObject();
}

ErrCode Serializer_endObject(Serializer* self)
{
    return reinterpret_cast<daq::ISerializer*>(self)->endObject();
}

ErrCode Serializer_startList(Serializer* self)
{
    return reinterpret_cast<daq::ISerializer*>(self)->startList();
}

ErrCode Serializer_endList(Serializer* self)
{
    return reinterpret_cast<daq::ISerializer*>(self)->endList();
}

ErrCode Serializer_getOutput(Serializer* self, String** serialized)
{
    return reinterpret_cast<daq::ISerializer*>(self)->getOutput(reinterpret_cast<daq::IString**>(serialized));
}

ErrCode Serializer_key(Serializer* self, ConstCharPtr string)
{
    return reinterpret_cast<daq::ISerializer*>(self)->key(string);
}

ErrCode Serializer_keyStr(Serializer* self, String* name)
{
    return reinterpret_cast<daq::ISerializer*>(self)->keyStr(reinterpret_cast<daq::IString*>(name));
}

ErrCode Serializer_keyRaw(Serializer* self, ConstCharPtr string, SizeT length)
{
    return reinterpret_cast<daq::ISerializer*>(self)->keyRaw(string, length);
}

ErrCode Serializer_writeInt(Serializer* self, Int integer)
{
    return reinterpret_cast<daq::ISerializer*>(self)->writeInt(integer);
}

ErrCode Serializer_writeBool(Serializer* self, Bool boolean)
{
    return reinterpret_cast<daq::ISerializer*>(self)->writeBool(boolean);
}

ErrCode Serializer_writeFloat(Serializer* self, Float real)
{
    return reinterpret_cast<daq::ISerializer*>(self)->writeFloat(real);
}

ErrCode Serializer_writeString(Serializer* self, ConstCharPtr string, SizeT length)
{
    return reinterpret_cast<daq::ISerializer*>(self)->writeString(string, length);
}

ErrCode Serializer_writeNull(Serializer* self)
{
    return reinterpret_cast<daq::ISerializer*>(self)->writeNull();
}

ErrCode Serializer_reset(Serializer* self)
{
    return reinterpret_cast<daq::ISerializer*>(self)->reset();
}

ErrCode Serializer_isComplete(Serializer* self, Bool* complete)
{
    return reinterpret_cast<daq::ISerializer*>(self)->isComplete(complete);
}

ErrCode Serializer_getUser(Serializer* self, BaseObject** user)
{
    return reinterpret_cast<daq::ISerializer*>(self)->getUser(reinterpret_cast<daq::IBaseObject**>(user));
}

ErrCode Serializer_setUser(Serializer* self, BaseObject* user)
{
    return reinterpret_cast<daq::ISerializer*>(self)->setUser(reinterpret_cast<daq::IBaseObject*>(user));
}
