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
#include <coretypes/number_ptr.h>

BEGIN_NAMESPACE_OPENDAQ

/*!
 * @ingroup opendaq_data_descriptor
 * @addtogroup opendaq_scaling Data rule
 * @{
 */

/*!
 * @brief Internal functions used by openDAQ core. This interface should never be used in
 * client SDK or module code.
 */
DECLARE_OPENDAQ_INTERFACE(IDataRuleCalcPrivate, IBaseObject)
{
    /*!
     * @brief Calculates the data according to the rule.
     * @param packetOffset Packet offset.
     * @param sampleCount The number of samples in the packet.
     * @returns A pointer to the calculated data.
     */
    virtual void* INTERFACE_FUNC calculateRule(const NumberPtr& packetOffset, SizeT sampleCount, void* input, SizeT inputSize) const = 0;

    /*!
     * @brief Calculates the data according to the rule.
     * @param packetOffset Packet offset.
     * @param sampleCount The number of samples in the packet.
     * @param[out] output A pointer to the calculated data.
     */
    virtual void INTERFACE_FUNC calculateRule(const NumberPtr& packetOffset, SizeT sampleCount, void* input, SizeT inputSize, void** output)
        const = 0;

    /*!
     * @brief Calculates the sample according to the rule.
     * @param packetOffset Packet offset.
     * @param sampleIndex The index of sample in the packet.
     * @returns A pointer to the calculated data.
     */
    virtual void* INTERFACE_FUNC calculateSample(const NumberPtr& packetOffset, SizeT sampleIndex, void* input, SizeT inputSize) const = 0;

    /*!
     * @brief Calculates the sample according to the rule.
     * @param packetOffset Packet offset.
     * @param sampleIndex The index of sample in the packet.
     * @param[out] output A pointer to the calculated data.
     */
    virtual void INTERFACE_FUNC calculateSample(const NumberPtr& packetOffset, SizeT sampleIndex, void* input, SizeT inputSize, void** output)
        const = 0;

    /*!
     * @brief Checks whether the Data Rule Calculator is available for packet or not.
     * @returns True if the Data Rule Calculator is initialized within the implementation; false otherwise.
     */
    virtual Bool INTERFACE_FUNC hasDataRuleCalc() const = 0;
};
/*!@}*/

END_NAMESPACE_OPENDAQ
