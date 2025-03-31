//------------------------------------------------------------------------------
// <auto-generated>
//     This code was generated by a tool.
//
//     Changes to this file may cause incorrect behavior and will be lost if
//     the code is regenerated.
//
//     RTGen (CGenerator v0.5.0) on 31.03.2025 16:56:21.
// </auto-generated>
//------------------------------------------------------------------------------

/*
 * Copyright 2022-2025 openDAQ d.o.o.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#pragma once

#ifdef __cplusplus
extern "C"
{
#endif

#include "ccommon.h"

    typedef struct ComplexNumber ComplexNumber;

    EXPORTED extern const IntfID COMPLEX_NUMBER_INTF_ID;

/*
    ErrCode EXPORTED ComplexNumber_getValue(ComplexNumber* self, ComplexFloat64* value);
*/
/*
    ErrCode EXPORTED ComplexNumber_equalsValue(ComplexNumber* self, ComplexFloat64 value, Bool* equal);
*/
    ErrCode EXPORTED ComplexNumber_getReal(ComplexNumber* self, Float* real);
    ErrCode EXPORTED ComplexNumber_getImaginary(ComplexNumber* self, Float* imaginary);
    ErrCode EXPORTED ComplexNumber_createComplexNumber(ComplexNumber** obj, Float real, Float imaginary);

#ifdef __cplusplus
}
#endif
