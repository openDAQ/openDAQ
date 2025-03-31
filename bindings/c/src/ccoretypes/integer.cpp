//------------------------------------------------------------------------------
// <auto-generated>
//     This code was generated by a tool.
//
//     Changes to this file may cause incorrect behavior and will be lost if
//     the code is regenerated.
//
//     RTGen (CGenerator v0.5.0) on 31.03.2025 16:56:26.
// </auto-generated>
//------------------------------------------------------------------------------

#include "ccoretypes/integer.h"

#include <opendaq/opendaq.h>

#include "copendaq_private.h"

const IntfID INTEGER_INTF_ID = { daq::IInteger::Id.Data1, daq::IInteger::Id.Data2, daq::IInteger::Id.Data3, daq::IInteger::Id.Data4_UInt64 };

ErrCode Integer_getValue(Integer* self, Int* value)
{
    return reinterpret_cast<daq::IInteger*>(self)->getValue(value);
}

ErrCode Integer_equalsValue(Integer* self, Int value, Bool* equals)
{
    return reinterpret_cast<daq::IInteger*>(self)->equalsValue(value, equals);
}

ErrCode Integer_createInteger(Integer** obj, Int value)
{
    daq::IInteger* ptr = nullptr;
    ErrCode err = daq::createInteger(&ptr, value);
    *obj = reinterpret_cast<Integer*>(ptr);
    return err;
}
