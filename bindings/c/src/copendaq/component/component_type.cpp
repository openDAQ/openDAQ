//------------------------------------------------------------------------------
// <auto-generated>
//     This code was generated by a tool.
//
//     Changes to this file may cause incorrect behavior and will be lost if
//     the code is regenerated.
//
//     RTGen (CGenerator v0.7.0) on 03.06.2025 22:06:54.
// </auto-generated>
//------------------------------------------------------------------------------

#include <copendaq/component/component_type.h>

#include <opendaq/opendaq.h>

#include <copendaq_private.h>

const daqIntfID DAQ_COMPONENT_TYPE_INTF_ID = { daq::IComponentType::Id.Data1, daq::IComponentType::Id.Data2, daq::IComponentType::Id.Data3, daq::IComponentType::Id.Data4_UInt64 };

daqErrCode daqComponentType_getId(daqComponentType* self, daqString** id)
{
    return reinterpret_cast<daq::IComponentType*>(self)->getId(reinterpret_cast<daq::IString**>(id));
}

daqErrCode daqComponentType_getName(daqComponentType* self, daqString** name)
{
    return reinterpret_cast<daq::IComponentType*>(self)->getName(reinterpret_cast<daq::IString**>(name));
}

daqErrCode daqComponentType_getDescription(daqComponentType* self, daqString** description)
{
    return reinterpret_cast<daq::IComponentType*>(self)->getDescription(reinterpret_cast<daq::IString**>(description));
}

daqErrCode daqComponentType_createDefaultConfig(daqComponentType* self, daqPropertyObject** defaultConfig)
{
    return reinterpret_cast<daq::IComponentType*>(self)->createDefaultConfig(reinterpret_cast<daq::IPropertyObject**>(defaultConfig));
}

daqErrCode daqComponentType_getModuleInfo(daqComponentType* self, daqModuleInfo** info)
{
    return reinterpret_cast<daq::IComponentType*>(self)->getModuleInfo(reinterpret_cast<daq::IModuleInfo**>(info));
}
