//------------------------------------------------------------------------------
// <auto-generated>
//     This code was generated by a tool.
//
//     Changes to this file may cause incorrect behavior and will be lost if
//     the code is regenerated.
//
//     RTGen (CGenerator v0.5.0) on 01.04.2025 17:01:52.
// </auto-generated>
//------------------------------------------------------------------------------

#include "ccoretypes/number.h"

#include <opendaq/opendaq.h>

#include "copendaq_private.h"

const IntfID NUMBER_INTF_ID = { daq::INumber::Id.Data1, daq::INumber::Id.Data2, daq::INumber::Id.Data3, daq::INumber::Id.Data4_UInt64 };

ErrCode Number_getFloatValue(Number* self, Float* value)
{
    return reinterpret_cast<daq::INumber*>(self)->getFloatValue(value);
}

ErrCode Number_getIntValue(Number* self, Int* value)
{
    return reinterpret_cast<daq::INumber*>(self)->getIntValue(value);
}
