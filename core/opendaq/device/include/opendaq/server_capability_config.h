/*
 * Copyright 2022-2026 openDAQ d.o.o.
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
#include <coreobjects/property.h>
#include <coreobjects/property_object.h>
#include <opendaq/server_capability.h>

BEGIN_NAMESPACE_OPENDAQ

/*#
 * [interfaceLibrary(IProperty, "coreobjects")]
 * [interfaceSmartPtr(IServerCapability, GenericServerCapabilityPtr)]
 * [interfaceSmartPtr(IInteger, IntegerPtr, "<coretypes/integer.h>")]
 * [interfaceLibrary(ITransportOption, "opendaq")]
 * [interfaceSmartPtr(ITransportOption, TransportOptionPtr, "<opendaq/transport_option_ptr.h>")]
 */

DECLARE_OPENDAQ_INTERFACE(IServerCapabilityConfig, IServerCapability)
{
    // [returnSelf]
    /*!
     * @brief Sets the connection string of device with current protocol
     * @param connectionString The connection string of device
     */
    virtual ErrCode INTERFACE_FUNC setConnectionString(IString* connectionString) = 0;

    // [returnSelf]
    /*!
     * @brief Sets the connection string of device with current protocol
     * @param connectionString The connection string of device
     */
    virtual ErrCode INTERFACE_FUNC addConnectionString(IString* connectionString) = 0;

    // [returnSelf]
    /*!
     * @brief Sets the ID of protocol group. Empty protocol group ID means that the protocol is not part of any group.
     * @param protocolGroupId The ID of protocol group
     */
    virtual ErrCode INTERFACE_FUNC setProtocolGroupId(IString* protocolGroupId) = 0;

    // [returnSelf]
    /*!
     * @brief Sets the ID of protocol
     * @param protocolId The ID of protocol
     */
    virtual ErrCode INTERFACE_FUNC setProtocolId(IString* protocolId) = 0;

    // [returnSelf]
    /*!
     * @brief Sets the name of protocol
     * @param protocolName The name of protocol
     */
    virtual ErrCode INTERFACE_FUNC setProtocolName(IString* protocolName) = 0;

    // [returnSelf]
    /*!
     * @brief Sets the type of protocol
     * @param type The type of protocol
     */
    virtual ErrCode INTERFACE_FUNC setProtocolType(ProtocolType type) = 0;
    
    // [returnSelf]
    /*!
     * @brief Sets the prefix of the connection string (eg. "daq.nd" or "daq.opcua")
     * @param prefix The connection string prefix
     */
    virtual ErrCode INTERFACE_FUNC setPrefix(IString* prefix) = 0;

    // [returnSelf]
    /*!
     * @brief Sets the type of connection
     * @param type The type of connection
     */
    virtual ErrCode INTERFACE_FUNC setConnectionType(IString* type) = 0;

    // [returnSelf]
    /*!
     * @brief Sets the boolean flag indicating whether the server capability supports core event propagation to clients
     * @param enabled True if core events are enabled; false otherwise
     */
    virtual ErrCode INTERFACE_FUNC setCoreEventsEnabled(Bool enabled) = 0;

    // [returnSelf]
    /*!
     * @brief Sets the device's address
     * @param[out] address The device's address
     */
    virtual ErrCode INTERFACE_FUNC addAddress(IString* address) = 0;

    // [returnSelf]
    /*!
     * @brief Sets the port of the default transport.
     * @param port The port of the device
     *
     * @deprecated Use setAvailableTransportOptions instead.
     * When options are set, the first (default) one is replaced by a copy at the new port, keeping the order.
     * Without options, only the port is stored.
     */
    virtual ErrCode INTERFACE_FUNC setPort(IInteger* port) = 0;
    
    // [returnSelf]
    virtual ErrCode INTERFACE_FUNC addAddressInfo(IAddressInfo* addressInfo) = 0;

    // [returnSelf]
    /*!
     * @brief Sets the protocol version
     * @param version The protocol version
     */
    virtual ErrCode INTERFACE_FUNC setProtocolVersion(IString* version) = 0;

    // [returnSelf]
    /*!
     * @brief Sets the security level of the protocol.
     * @param securityLevel The security level of the protocol.
     *
     * Could be '-1' if the protocol does not define any security level or if the security level is not known,
     * '0' if the protocol is not secure, and positive integer values for different security levels defined by the protocol.
     * The higher the value, the more secure the protocol is considered to be.
     */
    virtual ErrCode INTERFACE_FUNC setProtocolSecurityLevel(IInteger* securityLevel) = 0;

    // [returnSelf]
    /*!
     * @brief Adds a method id to the list of supported authentication methods. No-op when `methodId` is
     * already present.
     * @param methodId The authentication method id to add.
     */
    virtual ErrCode INTERFACE_FUNC addAuthenticationMethod(IString* methodId) = 0;

    // [templateType(transportOptions, IString, ITransportOption), returnSelf]
    /*!
     * @brief Sets the transport options the server accepts connections over, with the port it serves each on.
     * @param transportOptions The served options, keyed by their own names, the server's preferred first in dict
     * iteration order.
     * @retval OPENDAQ_ERR_INVALIDTYPE if a key is not a string or a value is not a transport option.
     * @retval OPENDAQ_ERR_INVALIDVALUE if a key is not the name of its option.
     *
     * Replaces any previously set options. The first one becomes the default, and its port is the one returned by
     * getPort; that port is also stored as the port read by older clients. An empty dictionary leaves that port
     * unchanged.
     */
    virtual ErrCode INTERFACE_FUNC setAvailableTransportOptions(IDict* transportOptions) = 0;

    // [returnSelf]
    /*!
     * @brief Adds a transport option the server accepts connections over, after the ones already set.
     * @param transportOption The served option, with the port it is served on.
     * @retval OPENDAQ_ERR_ALREADYEXISTS if an option of the same name is already set.
     *
     * The first option added becomes the default, and sets the port read by older clients.
     */
    virtual ErrCode INTERFACE_FUNC addAvailableTransportOption(ITransportOption* transportOption) = 0;
};

OPENDAQ_DECLARE_CLASS_FACTORY_WITH_INTERFACE(
    LIBRARY_FACTORY, ServerCapability, IServerCapabilityConfig,
    IString*, protocolId,
    IString*, protocolName,
    ProtocolType, protocolType
)

END_NAMESPACE_OPENDAQ
