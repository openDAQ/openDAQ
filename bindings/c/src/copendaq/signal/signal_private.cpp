//------------------------------------------------------------------------------
// <auto-generated>
//     This code was generated by a tool.
//
//     Changes to this file may cause incorrect behavior and will be lost if
//     the code is regenerated.
//
//     RTGen (CGenerator v0.7.0) on 03.06.2025 22:07:56.
// </auto-generated>
//------------------------------------------------------------------------------

#include <copendaq/signal/signal_private.h>

#include <opendaq/opendaq.h>

#include <copendaq_private.h>

const daqIntfID DAQ_SIGNAL_PRIVATE_INTF_ID = { daq::ISignalPrivate::Id.Data1, daq::ISignalPrivate::Id.Data2, daq::ISignalPrivate::Id.Data3, daq::ISignalPrivate::Id.Data4_UInt64 };

daqErrCode daqSignalPrivate_clearDomainSignalWithoutNotification(daqSignalPrivate* self)
{
    return reinterpret_cast<daq::ISignalPrivate*>(self)->clearDomainSignalWithoutNotification();
}

daqErrCode daqSignalPrivate_enableKeepLastValue(daqSignalPrivate* self, daqBool enabled)
{
    return reinterpret_cast<daq::ISignalPrivate*>(self)->enableKeepLastValue(enabled);
}

daqErrCode daqSignalPrivate_getSignalSerializeId(daqSignalPrivate* self, daqString** serializeId)
{
    return reinterpret_cast<daq::ISignalPrivate*>(self)->getSignalSerializeId(reinterpret_cast<daq::IString**>(serializeId));
}

daqErrCode daqSignalPrivate_getKeepLastValue(daqSignalPrivate* self, daqBool* keepLastValue)
{
    return reinterpret_cast<daq::ISignalPrivate*>(self)->getKeepLastValue(keepLastValue);
}
