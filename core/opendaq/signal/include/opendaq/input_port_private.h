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
#include <opendaq/signal.h>
#include <opendaq/input_port_notifications.h>

BEGIN_NAMESPACE_OPENDAQ

/*!
 * @ingroup opendaq_signal_path
 * @addtogroup opendaq_input_port Input port
 * @{
 */

/*!
 * @brief Internal functions used by openDAQ core. This interface should never be used in
 * client SDK or module code.
 */
DECLARE_OPENDAQ_INTERFACE(IInputPortPrivate, IBaseObject)
{
    /*!
     * @brief Disconnects the signal without notification to the signal.
     */
    virtual ErrCode INTERFACE_FUNC disconnectWithoutSignalNotification() = 0;

    /*!
     * @brief Connects the signal to the input port, forming a Connection.
     * @param signal The signal to be connected to the input port.
     *
     * On connect, an event packet is enqueued in the connection. This method schedules the
     * `onPacketReceived` notification instead of invoking it on the same thread.
     */
    virtual ErrCode INTERFACE_FUNC connectSignalSchedulerNotification(ISignal* signal) = 0;
};
/*!@}*/

END_NAMESPACE_OPENDAQ
