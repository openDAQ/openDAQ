//------------------------------------------------------------------------------
// <auto-generated>
//     This code was generated by a tool.
//
//     Changes to this file may cause incorrect behavior and will be lost if
//     the code is regenerated.
//
//     RTGen (CGenerator v0.1.0) on 25.03.2025 01:13:23.
// </auto-generated>
//------------------------------------------------------------------------------

#include "ccoretypes/complex_number.h"

#include <opendaq/opendaq.h>

const IntfID COMPLEX_NUMBER_INTF_ID = { daq::IComplexNumber::Id.Data1, daq::IComplexNumber::Id.Data2, daq::IComplexNumber::Id.Data3, daq::IComplexNumber::Id.Data4_UInt64 };

/*
ErrCode ComplexNumber_getValue(ComplexNumber* self, ComplexFloat64* value)
{
    return reinterpret_cast<daq::IComplexNumber*>(self)->getValue(value);
}
*/

/*
ErrCode ComplexNumber_equalsValue(ComplexNumber* self, ComplexFloat64 value, Bool* equal)
{
    return reinterpret_cast<daq::IComplexNumber*>(self)->equalsValue(value, equal);
}
*/

ErrCode ComplexNumber_getReal(ComplexNumber* self, Float* real)
{
    return reinterpret_cast<daq::IComplexNumber*>(self)->getReal(real);
}

ErrCode ComplexNumber_getImaginary(ComplexNumber* self, Float* imaginary)
{
    return reinterpret_cast<daq::IComplexNumber*>(self)->getImaginary(imaginary);
}

ErrCode ComplexNumber_createComplexNumber(ComplexNumber** obj, Float real, Float imaginary)
{
    daq::IComplexNumber* ptr = nullptr;
    ErrCode err = daq::createComplexNumber(&ptr, real, imaginary);
    *obj = reinterpret_cast<ComplexNumber*>(ptr);
    return err;
}
