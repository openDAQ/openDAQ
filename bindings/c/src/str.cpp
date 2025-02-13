#include "str.h"

#include <opendaq/opendaq.h>

void String_getCharPtr(String* self, ConstCharPtr* value)
{
    reinterpret_cast<daq::IString*>(self)->getCharPtr(value);
}

void String_getLength(String* self, SizeT* size)
{
    reinterpret_cast<daq::IString*>(self)->getLength(size);
}

void String_create(String** str, ConstCharPtr cstr)
{
    *str = reinterpret_cast<String*>(daq::String_Create(cstr));
}
