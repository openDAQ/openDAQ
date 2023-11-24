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
#include <opendaq/data_descriptor.h>
#include <opendaq/component.h>

BEGIN_NAMESPACE_OPENDAQ

/*#
 * [interfaceSmartPtr(IComponent, GenericComponentPtr, "<opendaq/component_ptr.h>")]
 * [interfaceSmartPtr(IConnection, ObjectPtr<IConnection>)]
 * [templated(defaultAliasName: SignalPtr)]
 * [interfaceSmartPtr(ISignal, GenericSignalPtr)]
 */

/*!
 * @ingroup opendaq_signals
 * @addtogroup opendaq_signal Signal
 * @{
 */

/*!
 * @brief A signal with an unique ID that sends event/data packets through connections to input ports
 * the signal is connected to.
 *
 * A signal features an unique ID within a given device. It sends data along its signal path to all
 * connected input ports, if its set to be active via its active property.
 *
 * A signal is visible, and its data is streamed to all clients connected to a device only if its public
 * property is set to `True`.
 *
 * Each signal has a domain descriptor, which is set by the owner of the signal - most often a function
 * block or device. The descriptor defines all properties of the signal, such as its name, description
 * and data structure.
 *
 * Signals can have a reference to another signal, which is used for determining the domain data. The domain
 * signal outputs data at the same rate as the signal itself, providing domain (most often time - timestamps)
 * information for each sample sent along the signal path. Each value packet sent by a signal thus contains
 * a reference to another data packet containing domain data (if the signal is associated with another domain signal).
 *
 * Additionally, a list of related signals can be defined, containing any signals relevant to interpreting the
 * signal data.
 *
 * To get the list of connections to input ports of the signal, `getConnections` can be used.
 */
DECLARE_OPENDAQ_INTERFACE(ISignal, IComponent)
{
    /*!
     * @brief Returns true if the signal is public; false otherwise.
     * @param[out] isPublic True if the signal is public; false otherwise.
     *
     * Public signals are visible to clients connected to the device, and are streamed.
     */
    virtual ErrCode INTERFACE_FUNC getPublic(Bool* isPublic) = 0;

    /*!
     * @brief Sets the signal to be either public or private.
     * @param isPublic If false, the signal is set to private; if true, the signal is set to be public.
     *
     * Public signals are visible to clients connected to the device, and are streamed.
     */
    virtual ErrCode INTERFACE_FUNC setPublic(Bool isPublic) = 0;

    /*!
     * @brief Gets the signal's data descriptor.
     * @param[out] descriptor The signal's data descriptor.
     *
     * The descriptor contains metadata about the signal, providing information about its name, description,...
     * and defines how the data in it's packet's buffers should be interpreted.
     */
    virtual ErrCode INTERFACE_FUNC getDescriptor(IDataDescriptor** descriptor) = 0;

    /*!
     * @brief Gets the signal that carries its domain data.
     * @param[out] signal The domain signal.
     *
     * The domain signal contains domain (most often time) data that is used to interpret a signal's data in
     * a given domain. It has the same sampling rate as the signal.
     */
    virtual ErrCode INTERFACE_FUNC getDomainSignal(ISignal** signal) = 0;

    // [elementType(signals, ISignal)]
    /*!
     * @brief Gets a list of related signals.
     * @param[out] signals The list of related signals.
     *
     * Signals within the related signals list are facilitate the interpretation of a given signal's data, or
     * are otherwise relevant when working with the signal.
     */
    virtual ErrCode INTERFACE_FUNC getRelatedSignals(IList** signals) = 0;

    // [elementType(connections, IConnection)]
    /*!
     * @brief Gets the list of connections to input ports formed by the signal.
     * @param[out] connections The list of connections.
     */
    virtual ErrCode INTERFACE_FUNC getConnections(IList** connections) = 0;

    /*!
     * @brief Sets the name of the signal.
     * @param name The name of the signal.
     *
     * The name property of the signal is user configurable.
     */
    virtual ErrCode INTERFACE_FUNC setName(IString* name) = 0;

    /*!
     * @brief Sets the description of the signal.
     * @param description The description of the signal.
     *
     * The description property of the signal is user configurable. openDAQ SDK does not use nor
     * interpret the description in any way.
     */
    virtual ErrCode INTERFACE_FUNC setDescription(IString* description) = 0;

    /*!
     * @brief Gets the description of the signal.
     * @param[out] description The description of the signal.
     *
     * The description property of the signal is user configurable. openDAQ SDK does not use nor
     * interpret the description in any way.
     */
    virtual ErrCode INTERFACE_FUNC getDescription(IString** description) = 0;

    /*!
     * @brief Returns true if the signal is streamed; false otherwise.
     * @param[out] streamed True if the signal is streamed; false otherwise.
     *
     * A streamed signal receives packets from a streaming server and forwards packets on the signal path.
     * Method always sets `streamed` parameter to False if the signal is local to the current Instance.
     */
    virtual ErrCode INTERFACE_FUNC getStreamed(Bool* streamed) = 0;

    /*!
     * @brief Sets the signal to be either streamed or not.
     * @param streamed The new streamed state of the signal.
     *
     * A streamed signal receives packets from a streaming server and forwards packets on the signal path.
     * Setting the "Streamed" flag has no effect if the signal is local to the current Instance.
     * Method returns OPENDAQ_IGNORED if that is the case.
     */
    virtual ErrCode INTERFACE_FUNC setStreamed(Bool streamed) = 0;
};
/*!@}*/

END_NAMESPACE_OPENDAQ
