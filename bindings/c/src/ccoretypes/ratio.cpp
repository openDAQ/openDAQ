//------------------------------------------------------------------------------
// <auto-generated>
//     This code was generated by a tool.
//
//     Changes to this file may cause incorrect behavior and will be lost if
//     the code is regenerated.
//
//     RTGen (CGenerator v0.1.0) on 13.03.2025 21:47:23.
// </auto-generated>
//------------------------------------------------------------------------------

#include "ccoretypes/ratio.h"

#include <opendaq/opendaq.h>

ErrCode Ratio_getNumerator(Ratio* self, Int* numerator)
{
    return reinterpret_cast<daq::IRatio*>(self)->getNumerator(numerator);
}

ErrCode Ratio_getDenominator(Ratio* self, Int* denominator)
{
    return reinterpret_cast<daq::IRatio*>(self)->getDenominator(denominator);
}

ErrCode Ratio_simplify(Ratio* self, Ratio** simplifiedRatio)
{
    return reinterpret_cast<daq::IRatio*>(self)->simplify(reinterpret_cast<daq::IRatio**>(simplifiedRatio));
}

ErrCode Ratio_createRatio(Ratio** obj, Int numerator, Int denominator)
{
    daq::IRatio* ptr = nullptr;
    ErrCode err = daq::createRatio(&ptr, numerator, denominator);
    *obj = reinterpret_cast<Ratio*>(ptr);
    return err;
}
