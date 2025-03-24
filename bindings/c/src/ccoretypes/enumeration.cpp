//------------------------------------------------------------------------------
// <auto-generated>
//     This code was generated by a tool.
//
//     Changes to this file may cause incorrect behavior and will be lost if
//     the code is regenerated.
//
//     RTGen (CGenerator v0.1.0) on 25.03.2025 01:13:24.
// </auto-generated>
//------------------------------------------------------------------------------

#include "ccoretypes/enumeration.h"

#include <opendaq/opendaq.h>

const IntfID ENUMERATION_INTF_ID = { daq::IEnumeration::Id.Data1, daq::IEnumeration::Id.Data2, daq::IEnumeration::Id.Data3, daq::IEnumeration::Id.Data4_UInt64 };

ErrCode Enumeration_getEnumerationType(Enumeration* self, EnumerationType** type)
{
    return reinterpret_cast<daq::IEnumeration*>(self)->getEnumerationType(reinterpret_cast<daq::IEnumerationType**>(type));
}

ErrCode Enumeration_getValue(Enumeration* self, String** value)
{
    return reinterpret_cast<daq::IEnumeration*>(self)->getValue(reinterpret_cast<daq::IString**>(value));
}

ErrCode Enumeration_getIntValue(Enumeration* self, Int* value)
{
    return reinterpret_cast<daq::IEnumeration*>(self)->getIntValue(value);
}

ErrCode Enumeration_createEnumeration(Enumeration** obj, String* name, String* value, TypeManager* typeManager)
{
    daq::IEnumeration* ptr = nullptr;
    ErrCode err = daq::createEnumeration(&ptr, reinterpret_cast<daq::IString*>(name), reinterpret_cast<daq::IString*>(value), reinterpret_cast<daq::ITypeManager*>(typeManager));
    *obj = reinterpret_cast<Enumeration*>(ptr);
    return err;
}

ErrCode Enumeration_createEnumerationWithIntValue(Enumeration** obj, String* name, Integer* value, TypeManager* typeManager)
{
    daq::IEnumeration* ptr = nullptr;
    ErrCode err = daq::createEnumerationWithIntValue(&ptr, reinterpret_cast<daq::IString*>(name), reinterpret_cast<daq::IInteger*>(value), reinterpret_cast<daq::ITypeManager*>(typeManager));
    *obj = reinterpret_cast<Enumeration*>(ptr);
    return err;
}

ErrCode Enumeration_createEnumerationWithType(Enumeration** obj, EnumerationType* type, String* value)
{
    daq::IEnumeration* ptr = nullptr;
    ErrCode err = daq::createEnumerationWithType(&ptr, reinterpret_cast<daq::IEnumerationType*>(type), reinterpret_cast<daq::IString*>(value));
    *obj = reinterpret_cast<Enumeration*>(ptr);
    return err;
}

ErrCode Enumeration_createEnumerationWithIntValueAndType(Enumeration** obj, EnumerationType* type, Integer* value)
{
    daq::IEnumeration* ptr = nullptr;
    ErrCode err = daq::createEnumerationWithIntValueAndType(&ptr, reinterpret_cast<daq::IEnumerationType*>(type), reinterpret_cast<daq::IInteger*>(value));
    *obj = reinterpret_cast<Enumeration*>(ptr);
    return err;
}
