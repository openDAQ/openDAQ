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
#include <opendaq/signal.h>
#include <opendaq/input_port.h>

BEGIN_NAMESPACE_OPENDAQ

/*!
 * @ingroup opendaq_signal_path
 * @addtogroup opendaq_input_port Input port notifications
 * @{
 */

/*!
 * @brief Notifications object passed to the input port on construction by its owner (listener).
 *
 * Input ports invoke the notification functions within the Input port notifications object when corresponding
 * events occur. The listener can then react on those events.
 */
DECLARE_OPENDAQ_INTERFACE(IInputPortNotifications, IBaseObject)
{
    /*!
     * @brief Called when the Input port method `acceptsSignal` is called. Should return true if the signal is
     * accepted; false otherwise.
     * @param port The input port on which the method was called.
     * @param signal The signal which is being checked for acceptance.
     * @param[out] accept True if the signal is accepted; false otherwise.
     */
    virtual ErrCode INTERFACE_FUNC acceptsSignal(IInputPort* port, ISignal* signal, Bool* accept) = 0;

    /*!
     * @brief Called when a signal is connected to the input port.
     * @param port The port to which the signal was connected.
     */
    virtual ErrCode INTERFACE_FUNC connected(IInputPort* port) = 0;

    /*!
     * @brief Called when a signal is disconnected from the input port.
     * @param port The port from which a signal was disconnected.
     */
    virtual ErrCode INTERFACE_FUNC disconnected(IInputPort* port) = 0;

    /*!
     * @brief Notifies the listener of the newly received packet on the specified input-port.
     * @param port The port on which the new packet was received.
     */
    virtual ErrCode INTERFACE_FUNC packetReceived(IInputPort* port) = 0;
};
/*!@}*/

END_NAMESPACE_OPENDAQ
