//------------------------------------------------------------------------------
// <auto-generated>
//     This code was generated by a tool.
//
//     Changes to this file may cause incorrect behavior and will be lost if
//     the code is regenerated.
//
//     RTGen (CGenerator v0.7.0) on 03.06.2025 22:07:02.
// </auto-generated>
//------------------------------------------------------------------------------

#include <copendaq/device/address_info_private.h>

#include <opendaq/opendaq.h>

#include <copendaq_private.h>

const daqIntfID DAQ_ADDRESS_INFO_PRIVATE_INTF_ID = { daq::IAddressInfoPrivate::Id.Data1, daq::IAddressInfoPrivate::Id.Data2, daq::IAddressInfoPrivate::Id.Data3, daq::IAddressInfoPrivate::Id.Data4_UInt64 };

daqErrCode daqAddressInfoPrivate_setReachabilityStatusPrivate(daqAddressInfoPrivate* self, daqAddressReachabilityStatus addressReachability)
{
    return reinterpret_cast<daq::IAddressInfoPrivate*>(self)->setReachabilityStatusPrivate(static_cast<daq::AddressReachabilityStatus>(addressReachability));
}
