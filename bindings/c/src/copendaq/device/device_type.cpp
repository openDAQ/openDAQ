//------------------------------------------------------------------------------
// <auto-generated>
//     This code was generated by a tool.
//
//     Changes to this file may cause incorrect behavior and will be lost if
//     the code is regenerated.
//
//     RTGen (CGenerator v0.7.0) on 03.06.2025 22:07:08.
// </auto-generated>
//------------------------------------------------------------------------------

#include <copendaq/device/device_type.h>

#include <opendaq/opendaq.h>

#include <copendaq_private.h>

const daqIntfID DAQ_DEVICE_TYPE_INTF_ID = { daq::IDeviceType::Id.Data1, daq::IDeviceType::Id.Data2, daq::IDeviceType::Id.Data3, daq::IDeviceType::Id.Data4_UInt64 };

daqErrCode daqDeviceType_getConnectionStringPrefix(daqDeviceType* self, daqString** prefix)
{
    return reinterpret_cast<daq::IDeviceType*>(self)->getConnectionStringPrefix(reinterpret_cast<daq::IString**>(prefix));
}

daqErrCode daqDeviceType_createDeviceType(daqDeviceType** obj, daqString* id, daqString* name, daqString* description, daqPropertyObject* defaultConfig, daqString* prefix)
{
    daq::IDeviceType* ptr = nullptr;
    daqErrCode err = daq::createDeviceType(&ptr, reinterpret_cast<daq::IString*>(id), reinterpret_cast<daq::IString*>(name), reinterpret_cast<daq::IString*>(description), reinterpret_cast<daq::IPropertyObject*>(defaultConfig), reinterpret_cast<daq::IString*>(prefix));
    *obj = reinterpret_cast<daqDeviceType*>(ptr);
    return err;
}
