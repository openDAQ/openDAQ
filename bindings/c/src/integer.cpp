#include "integer.h"

#include <opendaq/opendaq.h>

void Integer_getValue(Integer* self, Int* value)
{
    reinterpret_cast<daq::IInteger*>(self)->getValue(value);
}

void Integer_equalsValue(Integer* self, Int value, Bool* equals)
{
    reinterpret_cast<daq::IInteger*>(self)->equalsValue(value, equals);
}

void Integer_create(Integer** integer, Int value)
{
    *integer = reinterpret_cast<Integer*>(daq::Integer_Create(value));
}
