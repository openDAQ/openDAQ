//------------------------------------------------------------------------------
// <auto-generated>
//     This code was generated by a tool.
//
//     Changes to this file may cause incorrect behavior and will be lost if
//     the code is regenerated.
//
//     RTGen (CGenerator v0.1.0) on 13.03.2025 21:47:28.
// </auto-generated>
//------------------------------------------------------------------------------

#include "ccoretypes/type.h"

#include <opendaq/opendaq.h>

ErrCode Type_getName(Type* self, String** typeName)
{
    return reinterpret_cast<daq::IType*>(self)->getName(reinterpret_cast<daq::IString**>(typeName));
}
