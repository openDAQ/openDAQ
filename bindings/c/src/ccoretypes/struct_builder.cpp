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

#include "ccoretypes/struct_builder.h"

#include <opendaq/opendaq.h>

#include "copendaq_private.h"

const IntfID STRUCT_BUILDER_INTF_ID = { daq::IStructBuilder::Id.Data1, daq::IStructBuilder::Id.Data2, daq::IStructBuilder::Id.Data3, daq::IStructBuilder::Id.Data4_UInt64 };

ErrCode StructBuilder_build(StructBuilder* self, Struct** struct_)
{
    return reinterpret_cast<daq::IStructBuilder*>(self)->build(reinterpret_cast<daq::IStruct**>(struct_));
}

ErrCode StructBuilder_getStructType(StructBuilder* self, StructType** type)
{
    return reinterpret_cast<daq::IStructBuilder*>(self)->getStructType(reinterpret_cast<daq::IStructType**>(type));
}

ErrCode StructBuilder_getFieldNames(StructBuilder* self, List** names)
{
    return reinterpret_cast<daq::IStructBuilder*>(self)->getFieldNames(reinterpret_cast<daq::IList**>(names));
}

ErrCode StructBuilder_setFieldValues(StructBuilder* self, List* values)
{
    return reinterpret_cast<daq::IStructBuilder*>(self)->setFieldValues(reinterpret_cast<daq::IList*>(values));
}

ErrCode StructBuilder_getFieldValues(StructBuilder* self, List** values)
{
    return reinterpret_cast<daq::IStructBuilder*>(self)->getFieldValues(reinterpret_cast<daq::IList**>(values));
}

ErrCode StructBuilder_set(StructBuilder* self, String* name, BaseObject* field)
{
    return reinterpret_cast<daq::IStructBuilder*>(self)->set(reinterpret_cast<daq::IString*>(name), reinterpret_cast<daq::IBaseObject*>(field));
}

ErrCode StructBuilder_get(StructBuilder* self, String* name, BaseObject** field)
{
    return reinterpret_cast<daq::IStructBuilder*>(self)->get(reinterpret_cast<daq::IString*>(name), reinterpret_cast<daq::IBaseObject**>(field));
}

ErrCode StructBuilder_hasField(StructBuilder* self, String* name, Bool* contains)
{
    return reinterpret_cast<daq::IStructBuilder*>(self)->hasField(reinterpret_cast<daq::IString*>(name), contains);
}

ErrCode StructBuilder_getAsDictionary(StructBuilder* self, Dict** dictionary)
{
    return reinterpret_cast<daq::IStructBuilder*>(self)->getAsDictionary(reinterpret_cast<daq::IDict**>(dictionary));
}

ErrCode StructBuilder_createStructBuilder(StructBuilder** obj, String* name, TypeManager* typeManager)
{
    daq::IStructBuilder* ptr = nullptr;
    ErrCode err = daq::createStructBuilder(&ptr, reinterpret_cast<daq::IString*>(name), reinterpret_cast<daq::ITypeManager*>(typeManager));
    *obj = reinterpret_cast<StructBuilder*>(ptr);
    return err;
}

ErrCode StructBuilder_createStructBuilderFromStruct(StructBuilder** obj, Struct* struct_)
{
    daq::IStructBuilder* ptr = nullptr;
    ErrCode err = daq::createStructBuilderFromStruct(&ptr, reinterpret_cast<daq::IStruct*>(struct_));
    *obj = reinterpret_cast<StructBuilder*>(ptr);
    return err;
}
