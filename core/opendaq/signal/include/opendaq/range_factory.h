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
#include <opendaq/range_ptr.h>
#include <coretypes/struct_type_factory.h>
#include <coretypes/simple_type_factory.h>

BEGIN_NAMESPACE_OPENDAQ

/*!
 * @ingroup opendaq_range
 * @addtogroup opendaq_range_factories Factories
 * @{
 */

/*!
 * @brief Creates a range object with specified low and high boundary values.
 * @param lowValue The lower boundary of the range.
 * @param highValue The upper boundary of the range.
 * @throws RangeBoundariesInvalidException if lowValue > highValue.
 */
inline RangePtr Range(const NumberPtr& lowValue, const NumberPtr& highValue)
{
    RangePtr obj(Range_Create(lowValue, highValue));
    return obj;
}

/*!
 * @brief Creates the Struct type object that defines the Range struct.
 */
inline StructTypePtr RangeStructType()
{
    return StructType("Range",
                      List<IString>("LowValue", "HighValue"),
                      List<INumber>(0, 1),
                      List<IType>(SimpleType(ctFloat), SimpleType(ctFloat)));
}
/*!@}*/

END_NAMESPACE_OPENDAQ
