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
#include <coreobjects/property.h>
#include <coreobjects/property_object.h>
#include <opendaq/server_capability.h>

BEGIN_NAMESPACE_OPENDAQ

/*#
 * [interfaceLibrary(IProperty, "coreobjects")]
 * [interfaceLibrary(IServerCapability, "coreobjects")]
 * [interfaceSmartPtr(IServerCapability, GenericPropertyObjectPtr, "<coreobjects/property_object_ptr.h>")]
 */

DECLARE_OPENDAQ_INTERFACE(IServerCapabilityConfig, IServerCapability)
{
    // [returnSelf]
    /*!
     * @brief Sets the connection string of device with current protocol
     * @param connectionString The connection string of device
     */
    virtual ErrCode INTERFACE_FUNC setPrimaryConnectionString(IString* connectionString) = 0;

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
     * @brief Sets the type of connection
     * @param type The type of connection
     */
    virtual ErrCode INTERFACE_FUNC setConnectionType(IString* type) = 0;

    // [returnSelf]
    /*!
     * @brief Sets the client update method
     * @param type The client update method
     */
    virtual ErrCode INTERFACE_FUNC setCoreEventsEnabled(Bool enabled) = 0;
};

OPENDAQ_DECLARE_CLASS_FACTORY_WITH_INTERFACE(
    LIBRARY_FACTORY, ServerCapability, IServerCapabilityConfig,
    IString*, protocolName,
    ProtocolType, protocolType
)

OPENDAQ_DECLARE_CLASS_FACTORY_WITH_INTERFACE(
    LIBRARY_FACTORY, ServerStreamingCapability, IServerCapabilityConfig,
    IString*, protocolId
)

END_NAMESPACE_OPENDAQ
