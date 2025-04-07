//------------------------------------------------------------------------------
// <auto-generated>
//     This code was generated by a tool.
//
//     Changes to this file may cause incorrect behavior and will be lost if
//     the code is regenerated.
//
//     RTGen (CGenerator v0.5.0) on 01.04.2025 17:01:53.
// </auto-generated>
//------------------------------------------------------------------------------

#include "ccoretypes/serialized_list.h"

#include <opendaq/opendaq.h>

#include "copendaq_private.h"

const IntfID SERIALIZED_LIST_INTF_ID = { daq::ISerializedList::Id.Data1, daq::ISerializedList::Id.Data2, daq::ISerializedList::Id.Data3, daq::ISerializedList::Id.Data4_UInt64 };

ErrCode SerializedList_readSerializedObject(SerializedList* self, SerializedObject** plainObj)
{
    return reinterpret_cast<daq::ISerializedList*>(self)->readSerializedObject(reinterpret_cast<daq::ISerializedObject**>(plainObj));
}

ErrCode SerializedList_readSerializedList(SerializedList* self, SerializedList** list)
{
    return reinterpret_cast<daq::ISerializedList*>(self)->readSerializedList(reinterpret_cast<daq::ISerializedList**>(list));
}

ErrCode SerializedList_readList(SerializedList* self, BaseObject* context, Function* factoryCallback, List** list)
{
    return reinterpret_cast<daq::ISerializedList*>(self)->readList(reinterpret_cast<daq::IBaseObject*>(context), reinterpret_cast<daq::IFunction*>(factoryCallback), reinterpret_cast<daq::IList**>(list));
}

ErrCode SerializedList_readObject(SerializedList* self, BaseObject* context, Function* factoryCallback, BaseObject** obj)
{
    return reinterpret_cast<daq::ISerializedList*>(self)->readObject(reinterpret_cast<daq::IBaseObject*>(context), reinterpret_cast<daq::IFunction*>(factoryCallback), reinterpret_cast<daq::IBaseObject**>(obj));
}

ErrCode SerializedList_readString(SerializedList* self, String** string)
{
    return reinterpret_cast<daq::ISerializedList*>(self)->readString(reinterpret_cast<daq::IString**>(string));
}

ErrCode SerializedList_readBool(SerializedList* self, Bool* boolean)
{
    return reinterpret_cast<daq::ISerializedList*>(self)->readBool(boolean);
}

ErrCode SerializedList_readFloat(SerializedList* self, Float* real)
{
    return reinterpret_cast<daq::ISerializedList*>(self)->readFloat(real);
}

ErrCode SerializedList_readInt(SerializedList* self, Int* integer)
{
    return reinterpret_cast<daq::ISerializedList*>(self)->readInt(integer);
}

ErrCode SerializedList_getCount(SerializedList* self, SizeT* size)
{
    return reinterpret_cast<daq::ISerializedList*>(self)->getCount(size);
}

ErrCode SerializedList_getCurrentItemType(SerializedList* self, CoreType* size)
{
    return reinterpret_cast<daq::ISerializedList*>(self)->getCurrentItemType(reinterpret_cast<daq::CoreType*>(size));
}
