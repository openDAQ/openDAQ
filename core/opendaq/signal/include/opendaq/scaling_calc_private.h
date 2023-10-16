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
#include <coretypes/baseobject.h>

BEGIN_NAMESPACE_OPENDAQ

/*!
 * @ingroup opendaq_data_descriptor
 * @addtogroup opendaq_scaling Scaling
 * @{
 */

/*!
 * @brief Internal functions used by openDAQ core. This interface should never be used in
 * client SDK or module code.
 */
DECLARE_OPENDAQ_INTERFACE(IScalingCalcPrivate, IBaseObject)
{
    /*!
     * @brief Scales the packet data.
     * @param data Pointer to the packet data.
     * @param sampleCount The number of samples in the packet.
     * @returns A pointer to the scaled data.
     */
    virtual void* INTERFACE_FUNC scaleData(void* data, SizeT sampleCount) const = 0;

    /*!
     * @brief Scales the packet data.
     * @param data Pointer to the packet data.
     * @param sampleCount The number of samples in the packet.
     * @param[out] A pointer to the scaled data.
     */
    virtual void INTERFACE_FUNC scaleData(void* data, SizeT sampleCount, void** output) const = 0;

    /*!
     * @brief Checks whether the Scaling Calculator is available for packet or not.
     * @return True if the Scaling Calculator is initialized within the implementation; false otherwise.
     */
    virtual bool INTERFACE_FUNC hasScalingCalc() const = 0;
};
/*!@}*/

END_NAMESPACE_OPENDAQ
