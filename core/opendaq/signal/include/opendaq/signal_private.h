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
 * @ingroup opendaq_signals
 * @addtogroup opendaq_signal Signal
 * @{
 */

/*!
 * @brief Internal functions used by openDAQ core. This interface should never be used in
 * client SDK or module code.
 */
DECLARE_OPENDAQ_INTERFACE(ISignalPrivate, IBaseObject)
{
    /*!
     * @brief Sets the domain signal to null without notifying the domain signal
     */
    virtual ErrCode INTERFACE_FUNC clearDomainSignalWithoutNotification() = 0;

    /*!
     * @brief Enable or disable keeping last data packet which is using by Signal method `getLastValue` 
     * @param enabled Option for enabling method getLastValue
     */
    virtual ErrCode INTERFACE_FUNC enableKeepLastValue(Bool enabled) = 0;

    /*!
     * @brief Gets the signal serilized id. In local device the serilized id matached the signal global id. For remote id it is the signal id in the remote device.
     * @param[out] serializeId The signal serilized id.
     */
    virtual ErrCode INTERFACE_FUNC getSignalSerializeId(IString** serializeId) = 0;

    /*!
     * @brief Returns True if last value calculation is enabled on the signal.
     * @param[out] keepLastValue True if enabled.
     */
    virtual ErrCode INTERFACE_FUNC getKeepLastValue(Bool* keepLastValue) = 0;

    /*!
     * @brief Sends a packet through all connections of the signal, acquiring a recursive lock instead of an acquisition lock.
     * @param packet The packet to be sent.
     */
    virtual ErrCode INTERFACE_FUNC sendPacketRecursiveLock(IPacket* packet) = 0;
};
/*!@}*/

END_NAMESPACE_OPENDAQ
