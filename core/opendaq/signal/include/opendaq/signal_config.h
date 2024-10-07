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
#include <opendaq/context.h>
#include <opendaq/signal.h>

BEGIN_NAMESPACE_OPENDAQ

/*#
 * [interfaceSmartPtr(ISignal, GenericSignalPtr)]
 * [templated(defaultAliasName: SignalConfigPtr)]
 * [interfaceSmartPtr(ISignalConfig, GenericSignalConfigPtr)]
 */

/*!
 * @ingroup opendaq_signals
 * @addtogroup opendaq_signal Signal
 * @{
 */

/*!
 * @brief The configuration component of a Signal. Allows for configuration of its properties,
 * managing its streaming sources, and sending packets through its connections.
 *
 * The Signal config is most often accessible only to the devices or function blocks that own
 * the signal. They react on property, or input signal changes to modify a signal's data descriptor,
 * and send processed/acquired data down its signal path.
 */
DECLARE_OPENDAQ_INTERFACE(ISignalConfig, ISignal)
{
    /*!
     * @brief Sets the data descriptor.
     * @param descriptor The data descriptor.
     *
     * Setting the data descriptor triggers a Descriptor changed event packet to be sent to
     * all connections of the signal. If the signal is a domain signal of another, that signal
     * also sends a Descriptor changed event to all its connections.
     */
    virtual ErrCode INTERFACE_FUNC setDescriptor(IDataDescriptor* descriptor) = 0;

    // [templateType(signal, ISignal)]
    /*!
     * @brief Sets the domain signal reference.
     * @param signal The domain signal.
     *
     * Setting a new domain signal triggers a Descriptor changed event packet to be sent to
     * all connections of the signal.
     */
    virtual ErrCode INTERFACE_FUNC setDomainSignal(ISignal* signal) = 0;

    // [elementType(signals, ISignal)]
    /*!
     * @brief Sets the list of related signals.
     * @param signals The list of related signals.
     */
    virtual ErrCode INTERFACE_FUNC setRelatedSignals(IList* signals) = 0;

    // [templateType(signal, ISignal)]
    /*!
     * @brief Adds a related signal to the list of related signals.
     * @param signal The signal to be added.
     * @retval OPENDAQ_ERR_DUPLICATEITEM if the signal is already present in the list.
     */
    virtual ErrCode INTERFACE_FUNC addRelatedSignal(ISignal* signal) = 0;

    // [templateType(signal, ISignal)]
    /*!
     * @brief Removes a signal from the list of related signal.
     * @param signal The signal to be removed.
     * @retval OPENDAQ_ERR_NOTFOUND if the signal is not part of the list.
     */
    virtual ErrCode INTERFACE_FUNC removeRelatedSignal(ISignal* signal) = 0;

    /*!
     * @brief Clears the list of related signals.
     */
    virtual ErrCode INTERFACE_FUNC clearRelatedSignals() = 0;

    /*!
     * @brief Sends a packet through all connections of the signal.
     * @param packet The packet to be sent.
     */
    virtual ErrCode INTERFACE_FUNC sendPacket(IPacket* packet) = 0;

    // [elementType(packets, IPacket)]
    /*!
     * @brief Sends multiple packets through all connections of the signal.
     * @param packets The packets to be sent.
     *
     * Sending multiple packets creates a single notification to input port.
     */
    virtual ErrCode INTERFACE_FUNC sendPackets(IList* packets) = 0;

    // [overloadFor(sendPacket), stealRef(packet)]
    /*!
     * @brief Sends a packet through all connections of the signal. Ownership of the packet is transfered.
     * @param packet The packet to be sent.
     *
     * After calling the method, the packet should not be touched again. The ownership of the packet
     * is taken by underlying connections and it could be destroyed before the function returns.
     */
    virtual ErrCode INTERFACE_FUNC sendPacketAndStealRef(IPacket* packet) = 0;

    // [elementType(packets, IPacket), overloadFor(sendPackets), stealRef(packets)]
    /*!
     * @brief Sends multiple packets through all connections of the signal. Ownership of the packets is transfered.
     * @param packet The packets to be sent.
     *
     * After calling the method, the packets should not be touched again. The ownership of the packets
     * is taken by underlying connections and they could be destroyed before the function returns.
     */
    virtual ErrCode INTERFACE_FUNC sendPacketsAndStealRef(IList* packets) = 0;
};
/*!@}*/

// [allowNull(parent), allowNull(className)]
OPENDAQ_DECLARE_CLASS_FACTORY_WITH_INTERFACE(
    LIBRARY_FACTORY, Signal, ISignalConfig,
    IContext*, context,
    IComponent*, parent,
    IString*, localId,
    IString*, className
)

// [allowNull(parent), allowNull(className)]
OPENDAQ_DECLARE_CLASS_FACTORY_WITH_INTERFACE(
    LIBRARY_FACTORY, SignalWithDescriptor, ISignalConfig,
    IContext*, context,
    IDataDescriptor*, descriptor,
    IComponent*, parent,
    IString*, localId,
    IString*, className
)

END_NAMESPACE_OPENDAQ
