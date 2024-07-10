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
#include <opendaq/connection.h>

BEGIN_NAMESPACE_OPENDAQ

/*!
 * @ingroup opendaq_signals
 * @addtogroup opendaq_signal Signal
 * @{
 */

/*!
 * @brief Internal functions of a signal containing event methods that are called on certain
 * events. Eg. when a signal is connected to an input port, or when a signal is used as a domain
 * signal of another.
 */
DECLARE_OPENDAQ_INTERFACE(ISignalEvents, IBaseObject)
{
    /*!
     * @brief Notifies the signal that it has been connected to an input port forming a new connection.
     * @param connection The formed connection.
     */
    virtual ErrCode INTERFACE_FUNC listenerConnected(IConnection* connection) = 0;

    /*!
     * @brief Notifies the signal that it has been disconnected from an input port with the given connection.
     * @param connection The connection that was broken.
     */
    virtual ErrCode INTERFACE_FUNC listenerDisconnected(IConnection* connection) = 0;

    /*!
     * @brief Notifies the signal that it is being used as a domain signal by the signal passed as the function
     * argument.
     * @param signal The callee signal on which the domain signal reference has been set.
     */
    virtual ErrCode INTERFACE_FUNC domainSignalReferenceSet(ISignal* signal) = 0;

    /*!
     * @brief Notifies the signal that it is no longer being used as a domain signal by the signal passed as
     * the function argument.
     * @param signal The callee signal on which the domain signal reference has been removed.
     */
    virtual ErrCode INTERFACE_FUNC domainSignalReferenceRemoved(ISignal* signal) = 0;
};
/*!@}*/

END_NAMESPACE_OPENDAQ
