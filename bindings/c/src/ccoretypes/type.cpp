//------------------------------------------------------------------------------
// <auto-generated>
//     This code was generated by a tool.
//
//     Changes to this file may cause incorrect behavior and will be lost if
//     the code is regenerated.
//
//     RTGen (CGenerator v0.5.0) on 01.04.2025 17:01:58.
// </auto-generated>
//------------------------------------------------------------------------------

#include "ccoretypes/type.h"

#include <opendaq/opendaq.h>

#include "copendaq_private.h"

const IntfID TYPE_INTF_ID = { daq::IType::Id.Data1, daq::IType::Id.Data2, daq::IType::Id.Data3, daq::IType::Id.Data4_UInt64 };

ErrCode Type_getName(Type* self, String** typeName)
{
    return reinterpret_cast<daq::IType*>(self)->getName(reinterpret_cast<daq::IString**>(typeName));
}
