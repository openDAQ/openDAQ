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
#include <coretypes/baseobject.h>

BEGIN_NAMESPACE_OPENDAQ

/*!
 * @ingroup opendaq_packets
 * @addtogroup opendaq_deleter Deleter
 * @{
 */

/*!
 * @brief Callback which is called when external memory is no longer needed and can be freed.
 *
 * This interface is used with blueberry packets that are created with external memory. Provider
 * of external memory is responsible to provide a custom deleter, which is called when the packet is
 * destroyed.
 */
DECLARE_OPENDAQ_INTERFACE(IDeleter, IBaseObject)
{
    /*!
     * @brief Deletes or frees the memory associated with `address` parameter.
     * @param[out] address The address of the external memory.
     */
    virtual ErrCode INTERFACE_FUNC deleteMemory(void* address) = 0;
};

/*!@}*/

END_NAMESPACE_OPENDAQ
