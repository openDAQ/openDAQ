/*
 * Copyright 2022-2024 Blueberry d.o.o.
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
#include <coretypes/number.h>

BEGIN_NAMESPACE_OPENDAQ

/*#
 * [interfaceLibrary(INumber, CoreTypes)]
 */

/*!
 * @ingroup opendaq_data_descriptor
 * @addtogroup opendaq_range Range
 * @{
 */

/*!
 * @brief Describes a range of values between the `lowValue` and `highValue` boundaries.
 * 
 * Range objects implement the Struct methods internally and are Core type `ctStruct`.
 */
DECLARE_OPENDAQ_INTERFACE(IRange, IBaseObject)
{
    /*!
     * @brief Gets the lower boundary value of the range.
     */
    virtual ErrCode INTERFACE_FUNC getLowValue(INumber** value) = 0;
    /*!
     * @brief Gets the upper boundary value of the range.
     */
    virtual ErrCode INTERFACE_FUNC getHighValue(INumber** value) = 0;
};
/*!@}*/

/*!
 * @ingroup opendaq_range
 * @addtogroup opendaq_range_factories Factories
 * @{
 */

/*!
 * @brief Creates a range object with specified low and high boundary values.
 * @param lowValue The lower boundary of the range.
 * @param highValue The upper boundary of the range.
 * @retval OPENDAQ_ERR_RANGE_BOUNDARIES_INVALID if lowValue > highValue.
 */
OPENDAQ_DECLARE_CLASS_FACTORY(LIBRARY_FACTORY, Range, INumber*, lowValue, INumber*, highValue)

/*!@}*/

END_NAMESPACE_OPENDAQ
