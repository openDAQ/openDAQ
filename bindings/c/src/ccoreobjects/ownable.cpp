//------------------------------------------------------------------------------
// <auto-generated>
//     This code was generated by a tool.
//
//     Changes to this file may cause incorrect behavior and will be lost if
//     the code is regenerated.
//
//     RTGen (CGenerator v0.5.0) on 01.04.2025 17:02:06.
// </auto-generated>
//------------------------------------------------------------------------------

#include "ccoreobjects/ownable.h"

#include <opendaq/opendaq.h>

#include "copendaq_private.h"

const IntfID OWNABLE_INTF_ID = { daq::IOwnable::Id.Data1, daq::IOwnable::Id.Data2, daq::IOwnable::Id.Data3, daq::IOwnable::Id.Data4_UInt64 };

ErrCode Ownable_setOwner(Ownable* self, PropertyObject* owner)
{
    return reinterpret_cast<daq::IOwnable*>(self)->setOwner(reinterpret_cast<daq::IPropertyObject*>(owner));
}
