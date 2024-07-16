/*
 * Copyright 2022-2024 openDAQ d.o.o.
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
#include <coretypes/baseobject.h>

BEGIN_NAMESPACE_OPENDAQ

/*!
 * @ingroup opendaq_data_descriptor
 * @addtogroup opendaq_data_rule Rule private
 * @{
 */

/*!
 * @brief Private rule interface implemented by Dimension rules, Data rules and Scaling. Allows for parameter verification.
 */
DECLARE_OPENDAQ_INTERFACE(IRulePrivate, IBaseObject)
{
    /*!
     * @brief Checks whether the parameters are valid and returns an appropriate error code if not.
     * @retval OPENDAQ_ERR_INVALID_PARAMETERS If the parameters are invalid for the specific rule type.
     */
    virtual ErrCode INTERFACE_FUNC verifyParameters() = 0;
};
/*!@}*/

END_NAMESPACE_OPENDAQ
