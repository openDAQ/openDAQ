/*
 * Copyright 2022-2024 openDAQ d. o. o.
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
     * @brief Sets the port of the device
     * @param port The port of the device
     */
    virtual ErrCode INTERFACE_FUNC setPort(IInteger* port) = 0;
};

OPENDAQ_DECLARE_CLASS_FACTORY_WITH_INTERFACE(
    LIBRARY_FACTORY, ServerCapability, IServerCapabilityConfig,
    IString*, protocolId,
    IString*, protocolName,
    ProtocolType, protocolType
)

END_NAMESPACE_OPENDAQ
