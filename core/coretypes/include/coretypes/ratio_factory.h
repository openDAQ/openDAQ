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
#include <coretypes/common.h>
#include <coretypes/ratio_ptr.h>
#include <coretypes/simplified_ratio_ptr.h>
#include <coretypes/ctutils.h>
#include <coretypes/struct_type_factory.h>
#include <coretypes/simple_type_factory.h>

BEGIN_NAMESPACE_OPENDAQ

inline RatioPtr Ratio(Int num, Int den)
{
    return RatioPtr(Ratio_Create(num, den));
}

inline SimplifiedRatioPtr SimplifiedRatio(Int num, Int den)
{
    simplify(num, den);
    return SimplifiedRatioPtr(Ratio_Create(num, den));
}

inline StructTypePtr RatioStructType()
{
    return StructType("ratio",
                      List<IString>("numerator", "denominator"),
                      List<Int>(0, 1),
                      List<IType>(SimpleType(ctInt), SimpleType(ctInt)));
}

END_NAMESPACE_OPENDAQ
