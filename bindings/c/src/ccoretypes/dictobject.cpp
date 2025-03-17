//------------------------------------------------------------------------------
// <auto-generated>
//     This code was generated by a tool.
//
//     Changes to this file may cause incorrect behavior and will be lost if
//     the code is regenerated.
//
//     RTGen (CGenerator v0.1.0) on 13.03.2025 21:47:30.
// </auto-generated>
//------------------------------------------------------------------------------

#include "ccoretypes/dictobject.h"

#include <opendaq/opendaq.h>

ErrCode Dict_get(Dict* self, BaseObject* key, BaseObject** value)
{
    return reinterpret_cast<daq::IDict*>(self)->get(reinterpret_cast<daq::IBaseObject*>(key), reinterpret_cast<daq::IBaseObject**>(value));
}

ErrCode Dict_set(Dict* self, BaseObject* key, BaseObject* value)
{
    return reinterpret_cast<daq::IDict*>(self)->set(reinterpret_cast<daq::IBaseObject*>(key), reinterpret_cast<daq::IBaseObject*>(value));
}

ErrCode Dict_remove(Dict* self, BaseObject* key, BaseObject** value)
{
    return reinterpret_cast<daq::IDict*>(self)->remove(reinterpret_cast<daq::IBaseObject*>(key), reinterpret_cast<daq::IBaseObject**>(value));
}

ErrCode Dict_deleteItem(Dict* self, BaseObject* key)
{
    return reinterpret_cast<daq::IDict*>(self)->deleteItem(reinterpret_cast<daq::IBaseObject*>(key));
}

ErrCode Dict_clear(Dict* self)
{
    return reinterpret_cast<daq::IDict*>(self)->clear();
}

ErrCode Dict_getCount(Dict* self, SizeT* size)
{
    return reinterpret_cast<daq::IDict*>(self)->getCount(size);
}

ErrCode Dict_hasKey(Dict* self, BaseObject* key, Bool* hasKey)
{
    return reinterpret_cast<daq::IDict*>(self)->hasKey(reinterpret_cast<daq::IBaseObject*>(key), hasKey);
}

ErrCode Dict_getKeyList(Dict* self, List** keys)
{
    return reinterpret_cast<daq::IDict*>(self)->getKeyList(reinterpret_cast<daq::IList**>(keys));
}

ErrCode Dict_getValueList(Dict* self, List** values)
{
    return reinterpret_cast<daq::IDict*>(self)->getValueList(reinterpret_cast<daq::IList**>(values));
}

ErrCode Dict_getKeys(Dict* self, Iterable** iterable)
{
    return reinterpret_cast<daq::IDict*>(self)->getKeys(reinterpret_cast<daq::IIterable**>(iterable));
}

ErrCode Dict_getValues(Dict* self, Iterable** iterable)
{
    return reinterpret_cast<daq::IDict*>(self)->getValues(reinterpret_cast<daq::IIterable**>(iterable));
}

ErrCode Dict_createDict(Dict** obj)
{
    daq::IDict* ptr = nullptr;
    ErrCode err = daq::createDict(&ptr);
    *obj = reinterpret_cast<Dict*>(ptr);
    return err;
}

/*
ErrCode Dict_createDictWithExpectedTypes(Dict** obj, IntfID keyType, IntfID valueType)
{
    daq::IDict* ptr = nullptr;
    ErrCode err = daq::createDictWithExpectedTypes(&ptr, keyType, valueType);
    *obj = reinterpret_cast<Dict*>(ptr);
    return err;
}
*/
