//------------------------------------------------------------------------------
// <auto-generated>
//     This code was generated by a tool.
//
//     Changes to this file may cause incorrect behavior and will be lost if
//     the code is regenerated.
//
//     RTGen (CGenerator v0.1.0) on 13.03.2025 21:47:18.
// </auto-generated>
//------------------------------------------------------------------------------

#include "ccoretypes/enumeration_type.h"

#include <opendaq/opendaq.h>

ErrCode EnumerationType_getEnumeratorNames(EnumerationType* self, List** names)
{
    return reinterpret_cast<daq::IEnumerationType*>(self)->getEnumeratorNames(reinterpret_cast<daq::IList**>(names));
}

ErrCode EnumerationType_getAsDictionary(EnumerationType* self, Dict** dictionary)
{
    return reinterpret_cast<daq::IEnumerationType*>(self)->getAsDictionary(reinterpret_cast<daq::IDict**>(dictionary));
}

ErrCode EnumerationType_getEnumeratorIntValue(EnumerationType* self, String* name, Int* value)
{
    return reinterpret_cast<daq::IEnumerationType*>(self)->getEnumeratorIntValue(reinterpret_cast<daq::IString*>(name), value);
}

ErrCode EnumerationType_getCount(EnumerationType* self, SizeT* count)
{
    return reinterpret_cast<daq::IEnumerationType*>(self)->getCount(count);
}

ErrCode EnumerationType_createEnumerationType(EnumerationType** obj, String* typeName, List* enumeratorNames, Int firstEnumeratorIntValue)
{
    daq::IEnumerationType* ptr = nullptr;
    ErrCode err = daq::createEnumerationType(&ptr, reinterpret_cast<daq::IString*>(typeName), reinterpret_cast<daq::IList*>(enumeratorNames), firstEnumeratorIntValue);
    *obj = reinterpret_cast<EnumerationType*>(ptr);
    return err;
}

ErrCode EnumerationType_createEnumerationTypeWithValues(EnumerationType** obj, String* typeName, Dict* enumerators)
{
    daq::IEnumerationType* ptr = nullptr;
    ErrCode err = daq::createEnumerationTypeWithValues(&ptr, reinterpret_cast<daq::IString*>(typeName), reinterpret_cast<daq::IDict*>(enumerators));
    *obj = reinterpret_cast<EnumerationType*>(ptr);
    return err;
}
