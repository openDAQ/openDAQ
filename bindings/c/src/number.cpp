#include "number.h"

#include <opendaq/opendaq.h>

void Number_getFloatValue(Number* self, Float* value)
{
    reinterpret_cast<daq::INumber*>(self)->getFloatValue(value);
}

void Number_getIntValue(Number* self, Int* value)
{
    reinterpret_cast<daq::INumber*>(self)->getIntValue(value);
}