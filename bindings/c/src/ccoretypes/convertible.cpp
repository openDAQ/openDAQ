//------------------------------------------------------------------------------
// <auto-generated>
//     This code was generated by a tool.
//
//     Changes to this file may cause incorrect behavior and will be lost if
//     the code is regenerated.
//
//     RTGen (CGenerator v0.1.0) on 13.03.2025 21:47:16.
// </auto-generated>
//------------------------------------------------------------------------------

#include "ccoretypes/convertible.h"

#include <opendaq/opendaq.h>

ErrCode Convertible_toFloat(Convertible* self, Float* val)
{
    return reinterpret_cast<daq::IConvertible*>(self)->toFloat(val);
}

ErrCode Convertible_toInt(Convertible* self, Int* val)
{
    return reinterpret_cast<daq::IConvertible*>(self)->toInt(val);
}

ErrCode Convertible_toBool(Convertible* self, Bool* val)
{
    return reinterpret_cast<daq::IConvertible*>(self)->toBool(val);
}
