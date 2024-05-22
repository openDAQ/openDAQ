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
#include <opendaq/context.h>
#include <opendaq/input_port.h>
#include <opendaq/task_graph.h>
#include <coretypes/factory.h>

BEGIN_NAMESPACE_OPENDAQ

struct IFunctionBlock;
struct IInputPortNotifications;

/*!
 * @brief Represents how the input port should handle the packet-enqueued notification.
 */
enum class PacketReadyNotification
{
    None,                   ///< Ignore the notification.
    SameThread,             ///< Call the listener in the same thread the notification was received.
    Scheduler,              ///< Call the listener asynchronously or in another thread.
    SchedulerQueueWasEmpty  ///< Call the listener asynchronously or in another thread only if connection packet queue was empty
};

 /*!
 * @ingroup opendaq_signal_path
 * @addtogroup opendaq_input_port Input port
 * @{
 */

/*#
 * [interfaceSmartPtr(IInputPort, GenericInputPortPtr)]
 */

/*!
 * @brief The configuration component of input ports. Provides access to Input port owners
 * to internal components of the input port.
 */
DECLARE_OPENDAQ_INTERFACE(IInputPortConfig, IInputPort)
{
    /*!
     * @brief Sets the input-ports response to the packet enqueued notification.
     */
    virtual ErrCode INTERFACE_FUNC setNotificationMethod(PacketReadyNotification method) = 0;

    /*!
     * @brief Gets called when a packet was enqueued in a connection.
     * @param queueWasEmpty True if queue was empty before packet was enqueued.
     */
    virtual ErrCode INTERFACE_FUNC notifyPacketEnqueued(Bool queueWasEmpty) = 0;

    /*!
     * @brief Gets called when a packet was enqueued in a connection.
     *
     * The notification is called on the same thread that enqueued the packet.
     */
    virtual ErrCode INTERFACE_FUNC notifyPacketEnqueuedOnThisThread() = 0;

    /*!
     * @brief Set the object receiving input-port related events and notifications.
     */
    virtual ErrCode INTERFACE_FUNC setListener(IInputPortNotifications* port) = 0;

    /*!
     * @brief Get a custom data attached to the object.
     */
    virtual ErrCode INTERFACE_FUNC getCustomData(IBaseObject** customData) = 0;

    /*!
     * @brief Set a custom data attached to the object.
     */
    virtual ErrCode INTERFACE_FUNC setCustomData(IBaseObject* customData) = 0;

    /*!
     * @brief Sets requires signal flag of the input port.
     * @param requiresSignal True if the input port requires a signal to be connected; false otherwise.
     *
     * If an input port requires a signal, then the input port must have a signal connected otherwise
     * the owner of the input port (function block) should report an error.
     */
    virtual ErrCode INTERFACE_FUNC setRequiresSignal(Bool requiresSignal) = 0;

    /*!
     * @brief Returns the state of gap checking requested by the input port.
     * @param gapCheckingEnabled true if gap checking is requested by the input port.
     */
    virtual ErrCode INTERFACE_FUNC getGapCheckingEnabled(Bool* gapCheckingEnabled) = 0;
};
/*!@}*/

OPENDAQ_DECLARE_CLASS_FACTORY_WITH_INTERFACE(
    LIBRARY_FACTORY,
    InputPort,
    IInputPortConfig,
    IContext*, context,
    IComponent*, parent,
    IString*, localId,
    Bool, gapChecking
)

END_NAMESPACE_OPENDAQ
