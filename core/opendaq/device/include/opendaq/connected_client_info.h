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
#include <coretypes/common.h>
#include <coretypes/stringobject.h>
#include <coreobjects/property_object.h>
#include <opendaq/server_capability.h>

BEGIN_NAMESPACE_OPENDAQ

/*!
 * @ingroup opendaq_server_capability
 * @addtogroup opendaq_connected_client_info Connected client info
 * @{
 */

/*#
 * [interfaceLibrary(IPropertyObject, "coreobjects")]
 * [interfaceSmartPtr(IPropertyObject, GenericPropertyObjectPtr, "<coreobjects/property_object_ptr.h>")]
 */
DECLARE_OPENDAQ_INTERFACE(IConnectedClientInfo, IPropertyObject)
{
    /*!
     * @brief Gets the client address string.
     * @param[out] address The client address string.
     */
    virtual ErrCode INTERFACE_FUNC getAddress(IString** address) = 0;

    /*!
     * @brief Gets the type of protocol used by the client.
     * @param[out] type The type of protocol (Enumeration value reflecting protocol type: "ConfigurationAndStreaming", "Configuration", "Streaming", "Unknown").
     */
    virtual ErrCode INTERFACE_FUNC getProtocolType(ProtocolType* type) = 0;

    /*!
     * @brief Gets the name of the protocol used by the client.
     * @param[out] protocolName The name of the protocol (e.g., "OpenDAQNativeStreaming", "OpenDAQOPCUA", "OpenDAQLTStreaming").
     */
    virtual ErrCode INTERFACE_FUNC getProtocolName(IString** protocolName) = 0;

    /*!
     * @brief Gets the type of connected configuration connection client.
     * @param[out] type The string representation of client type ("Control", "ExclusiveControl", "ViewOnly").
     */
    virtual ErrCode INTERFACE_FUNC getClientTypeName(IString** type) = 0;

    /*!
     * @brief Gets the client host name.
     * @param[out] hostName The client host name.
     */
    virtual ErrCode INTERFACE_FUNC getHostName(IString** hostName) = 0;
};

/*!@}*/

/*!
 * @ingroup opendaq_connected_client_info
 * @addtogroup opendaq_connected_client_info_factories Factories
 * @{
 */

/*!
 * @brief Creates a Connected client with no parameters configured.
 */
OPENDAQ_DECLARE_CLASS_FACTORY_WITH_INTERFACE(LIBRARY_FACTORY, ConnectedClientInfo, IConnectedClientInfo)

/*!
 * @brief Creates a Connected client info using the provided parameters.
 * @param address The address of connected client.
 * @param protocolType The type of the protocol type used by the client.
 * @param protocolName The name of the protocol name used by the client.
 * @param clientType The configuration connection client type name.
 * @param hostName The host name of connected client.
 */
OPENDAQ_DECLARE_CLASS_FACTORY_WITH_INTERFACE(
    LIBRARY_FACTORY, ConnectedClientInfoWithParams, IConnectedClientInfo,
    IString*, address,
    ProtocolType, protocolType,
    IString*, protocolName,
    IString*, clientType,
    IString*, hostName
)

/*!@}*/

END_NAMESPACE_OPENDAQ
