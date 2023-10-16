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
#include <opendaq/signal.h>
#include <opendaq/component.h>

BEGIN_NAMESPACE_OPENDAQ

struct IConnection;

/*#
 * [interfaceSmartPtr(IComponent, GenericComponentPtr, "<opendaq/component_ptr.h>")]
 * [templated(defaultAliasName: InputPortPtr)]
 * [interfaceSmartPtr(IInputPort, GenericInputPortPtr)]
 */

/*!
 * @ingroup opendaq_signal_path
 * @addtogroup opendaq_input_port Input port
 * @{
 */

/*!
 * @brief Signals accepted by input ports can be connected, forming a connection between the input port
 * and signal, through which Packets can be sent.
 *
 * Any openDAQ object which wishes to receive signal data must create an input port and connect it to said
 * signals. Such objects are for example function blocks, and readers.
 *
 * An input port can filter out incompatible signals by returning false when such a signal is passed as argument
 * to `acceptsSignal`, and also rejects the signals when they are passed as argument to `connect`.
 *
 * Depending on the configuration, an input port might not require a signal to be connected, returning false
 * when `requiresSignal` is called. Such input ports are usually a part of function blocks that do not require
 * a given signal to perform calculations.
 */
DECLARE_OPENDAQ_INTERFACE(IInputPort, IComponent)
{
    /*!
     * @brief Returns true if the signal can be connected to the input port; false otherwise.
     * @param signal The signal being evaluated for compatibility.
     * @param[out] accepts True if the signal can be connected; false otherwise.
     * @retval OPENDAQ_ERR_NOTASSIGNED if the accepted signal criteria is not defined by the input port.
     */
    virtual ErrCode INTERFACE_FUNC acceptsSignal(ISignal* signal, Bool* accepts) = 0;

    /*!
     * @brief Connects the signal to the input port, forming a Connection.
     * @param signal The signal to be connected to the input port.
     * @retval OPENDAQ_ERR_SIGNAL_NOT_ACCEPTED if the signal is not accepted.
     * @retval OPENDAQ_ERR_NOTASSIGNED if the accepted signal criteria is not defined by the input port.
     *
     * The signal is notified of the connection formed between it and the input port.
     */
    virtual ErrCode INTERFACE_FUNC connect(ISignal* signal) = 0;

    /*!
     * @brief Disconnects the signal from the input port.
     */
    virtual ErrCode INTERFACE_FUNC disconnect() = 0;

    /*!
     * @brief Gets the signal connected to the input port.
     * @param[out] signal The signal connected to the input port.
     */
    virtual ErrCode INTERFACE_FUNC getSignal(ISignal** signal) = 0;

    /*!
     * @brief Returns true if the input port requires a signal to be connected; false otherwise.
     * @param[out] requiresSignal True if the input port requires a signal to be connected; false otherwise.
     */
    virtual ErrCode INTERFACE_FUNC getRequiresSignal(Bool* requiresSignal) = 0;

    /*!
     * @brief Gets the Connection object formed between the Signal and Input port.
     * @param[out] connection The Connection object.
     */
    virtual ErrCode INTERFACE_FUNC getConnection(IConnection** connection) = 0;
};
/*!@}*/

END_NAMESPACE_OPENDAQ
