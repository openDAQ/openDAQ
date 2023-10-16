/*
 * Copyright 2022-2023 Blueberry d.o.o.
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
#include <coretypes/complex_number_ptr.h>
#include <coretypes/struct_type_factory.h>
#include <coretypes/simple_type_factory.h>

BEGIN_NAMESPACE_OPENDAQ

inline ComplexNumberPtr ComplexNumber()
{
    return ComplexNumberPtr(ComplexNumber_Create(0, 0));
}

inline ComplexNumberPtr ComplexNumber(const Float real, const Float imaginary)
{
    return ComplexNumberPtr(ComplexNumber_Create(real, imaginary));
}

inline StructTypePtr ComplexNumberStructType()
{
    return StructType("complexNumber",
                      List<IString>("real", "imaginary"),
                      List<IFloat>(0.0, 0.0),
                      List<IType>(SimpleType(ctFloat), SimpleType(ctFloat)));
}

END_NAMESPACE_OPENDAQ
