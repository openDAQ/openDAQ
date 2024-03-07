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
#include <coretypes/common.h>
#include <coretypes/stringobject.h>

BEGIN_NAMESPACE_OPENDAQ

enum class ProtocolType : uint32_t
{
    Streaming = 0,
    Structure,
    Both,
    Unknown = 0xffff
};

enum class ConnectionType : uint32_t
{
    Ipv4 = 0,
    Ipv6,
    Unknown = 0xffff
};

DECLARE_OPENDAQ_INTERFACE(IDeviceCapability, IBaseObject)
{
    /*!
     * @brief Gets the connection string of device with current protocol
     * @param[out] connectionString The connection string of device
     */
    virtual ErrCode INTERFACE_FUNC getConnectionString(IString** connectionString) = 0;

    /*!
     * @brief Gets the name of protocol
     * @param[out] protocolName The name of protocol
     */
    virtual ErrCode INTERFACE_FUNC getProtocolName(IString** protocolName) = 0;

    /*!
     * @brief Gets the type of protocol
     * @param[out] type The type of protocol
     */
    virtual ErrCode INTERFACE_FUNC getProtocolType(ProtocolType* type) = 0;

    /*!
     * @brief Gets the type of connection
     * @param[out] type The type of connection
     */
    virtual ErrCode INTERFACE_FUNC getConnectionType(ConnectionType* type) = 0;
};

OPENDAQ_DECLARE_CLASS_FACTORY_WITH_INTERFACE(LIBRARY_FACTORY, DeviceCapability, IDeviceCapability,
    IString*, connectionString,
    IString*, protocolName,
    ProtocolType, protocolType, 
    ConnectionType, connectionType
    )

END_NAMESPACE_OPENDAQ
