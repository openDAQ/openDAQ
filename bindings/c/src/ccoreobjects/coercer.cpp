//------------------------------------------------------------------------------
// <auto-generated>
//     This code was generated by a tool.
//
//     Changes to this file may cause incorrect behavior and will be lost if
//     the code is regenerated.
//
//     RTGen (CGenerator v0.7.0) on 03.06.2025 22:05:10.
// </auto-generated>
//------------------------------------------------------------------------------

#include <ccoreobjects/coercer.h>

#include <opendaq/opendaq.h>

#include <copendaq_private.h>

const daqIntfID DAQ_COERCER_INTF_ID = { daq::ICoercer::Id.Data1, daq::ICoercer::Id.Data2, daq::ICoercer::Id.Data3, daq::ICoercer::Id.Data4_UInt64 };

daqErrCode daqCoercer_coerce(daqCoercer* self, daqBaseObject* propObj, daqBaseObject* value, daqBaseObject** result)
{
    return reinterpret_cast<daq::ICoercer*>(self)->coerce(reinterpret_cast<daq::IBaseObject*>(propObj), reinterpret_cast<daq::IBaseObject*>(value), reinterpret_cast<daq::IBaseObject**>(result));
}

daqErrCode daqCoercer_coerceNoLock(daqCoercer* self, daqBaseObject* propObj, daqBaseObject* value, daqBaseObject** result)
{
    return reinterpret_cast<daq::ICoercer*>(self)->coerceNoLock(reinterpret_cast<daq::IBaseObject*>(propObj), reinterpret_cast<daq::IBaseObject*>(value), reinterpret_cast<daq::IBaseObject**>(result));
}

daqErrCode daqCoercer_getEval(daqCoercer* self, daqString** eval)
{
    return reinterpret_cast<daq::ICoercer*>(self)->getEval(reinterpret_cast<daq::IString**>(eval));
}

daqErrCode daqCoercer_createCoercer(daqCoercer** obj, daqString* eval)
{
    daq::ICoercer* ptr = nullptr;
    daqErrCode err = daq::createCoercer(&ptr, reinterpret_cast<daq::IString*>(eval));
    *obj = reinterpret_cast<daqCoercer*>(ptr);
    return err;
}
