//------------------------------------------------------------------------------
// <auto-generated>
//     This code was generated by a tool.
//
//     Changes to this file may cause incorrect behavior and will be lost if
//     the code is regenerated.
//
//     RTGen (CGenerator v0.5.0) on 31.03.2025 16:56:32.
// </auto-generated>
//------------------------------------------------------------------------------

#include "ccoretypes/struct.h"

#include <opendaq/opendaq.h>

#include "copendaq_private.h"

const IntfID STRUCT_INTF_ID = { daq::IStruct::Id.Data1, daq::IStruct::Id.Data2, daq::IStruct::Id.Data3, daq::IStruct::Id.Data4_UInt64 };

ErrCode Struct_getStructType(Struct* self, StructType** type)
{
    return reinterpret_cast<daq::IStruct*>(self)->getStructType(reinterpret_cast<daq::IStructType**>(type));
}

ErrCode Struct_getFieldNames(Struct* self, List** names)
{
    return reinterpret_cast<daq::IStruct*>(self)->getFieldNames(reinterpret_cast<daq::IList**>(names));
}

ErrCode Struct_getFieldValues(Struct* self, List** values)
{
    return reinterpret_cast<daq::IStruct*>(self)->getFieldValues(reinterpret_cast<daq::IList**>(values));
}

ErrCode Struct_get(Struct* self, String* name, BaseObject** field)
{
    return reinterpret_cast<daq::IStruct*>(self)->get(reinterpret_cast<daq::IString*>(name), reinterpret_cast<daq::IBaseObject**>(field));
}

ErrCode Struct_getAsDictionary(Struct* self, Dict** dictionary)
{
    return reinterpret_cast<daq::IStruct*>(self)->getAsDictionary(reinterpret_cast<daq::IDict**>(dictionary));
}

ErrCode Struct_hasField(Struct* self, String* name, Bool* contains)
{
    return reinterpret_cast<daq::IStruct*>(self)->hasField(reinterpret_cast<daq::IString*>(name), contains);
}

ErrCode Struct_createStruct(Struct** obj, String* name, Dict* fields, TypeManager* typeManager)
{
    daq::IStruct* ptr = nullptr;
    ErrCode err = daq::createStruct(&ptr, reinterpret_cast<daq::IString*>(name), reinterpret_cast<daq::IDict*>(fields), reinterpret_cast<daq::ITypeManager*>(typeManager));
    *obj = reinterpret_cast<Struct*>(ptr);
    return err;
}

ErrCode Struct_createStructFromBuilder(Struct** obj, StructBuilder* builder)
{
    daq::IStruct* ptr = nullptr;
    ErrCode err = daq::createStructFromBuilder(&ptr, reinterpret_cast<daq::IStructBuilder*>(builder));
    *obj = reinterpret_cast<Struct*>(ptr);
    return err;
}
