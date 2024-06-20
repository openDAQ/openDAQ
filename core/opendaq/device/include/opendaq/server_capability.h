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
#include <coretypes/common.h>
#include <coretypes/stringobject.h>
#include <opendaq/address_info.h>

BEGIN_NAMESPACE_OPENDAQ

/*!
 * @ingroup opendaq_devices
 * @addtogroup opendaq_server_capability Server capability
 * @{
 */

enum class ProtocolType: uint32_t
{
    Unknown = 0,
    Configuration,
    Streaming,
    ConfigurationAndStreaming,
};

/*#
 * [templated(defaultAliasName: ServerCapabilityPtr)]
 * [interfaceSmartPtr(IServerCapability, GenericServerCapabilityPtr)]
 * [interfaceSmartPtr(IEnumeration, EnumerationPtr, "<coretypes/enumeration_ptr.h>")]
 * [interfaceLibrary(IPropertyObject, "coreobjects")]
 * [interfaceSmartPtr(IPropertyObject, GenericPropertyObjectPtr, "<coreobjects/property_object_ptr.h>")]
 * [interfaceSmartPtr(IInteger, IntegerPtr, "<coretypes/integer.h>")]
 */

/*!
 * @brief Represents standard information about a server's capability to support various protocols. 
 * The Server Capability object functions as a Property Object, facilitating the inclusion of custom properties of String, Int, Bool, or Float types.
 * This interface serves to store essential details regarding the supported protocol by a device. 
 * It adheres to a standardized set of properties, including methods to retrieve information such as 
 * the connection string, protocol name, protocol type, connection type, and core events enabled.
 * 
 * Additional String, Int, Bool, or Float-type properties can be added using the appropriate Property Object "add property" method. 
 * However, other property types are considered invalid for this interface.
 * 
 * The Server Capability object conforms to a standardized format, ensuring compatibility with communication standards. 
 * For instance, it provides methods to retrieve details like 
 * - the connection string (URL), 
 * - protocol name (e.g., "openDAQ Native Streaming," "openDAQ OpcUa"), 
 * - protocol type (e.g., "Configuration&Streaming," "Streaming"), 
 * - connection type (e.g., IPv4, IPv6),
 * - core events enabled (indicating communication mode).
*/
DECLARE_OPENDAQ_INTERFACE(IServerCapability, IPropertyObject)
{
    /*!
     * @brief Gets the connection string of the device with the current protocol.
     * @param[out] connectionString The connection string of the device (URL to connect).
     */
    virtual ErrCode INTERFACE_FUNC getConnectionString(IString** connectionString) = 0;

    // [templateType(connectionStrings, IString)]
    /*!
     * @brief Gets the connection string of the device with the current protocol.
     * @param[out] connectionStrings The connection string of the device (URL to connect).
     */
    virtual ErrCode INTERFACE_FUNC getConnectionStrings(IList** connectionStrings) = 0;

    /*!
     * @brief Gets the name of the protocol supported by the device.
     * @param[out] protocolName The name of the protocol (e.g., "OpenDAQNativeStreaming", "openDAQ OpcUa", "openDAQ LT Streaming").
     */
    virtual ErrCode INTERFACE_FUNC getProtocolName(IString** protocolName) = 0;

    /*!
     * @brief Gets the id of the protocol supported by the device. Should not contain spaces or special characters except for '_' and '-'.
     * @param[out] protocolId The id of the protocol.
     */
    virtual ErrCode INTERFACE_FUNC getProtocolId(IString** protocolId) = 0;

    /*!
     * @brief Gets the type of protocol supported by the device.
     * @param[out] type The type of protocol (Enumeration value reflecting protocol type: "ConfigurationAndStreaming", "Configuration", "Streaming", "ServerStreaming", "Unknown").
     */
    virtual ErrCode INTERFACE_FUNC getProtocolType(ProtocolType* type) = 0;

    /*!
     * @brief Gets the prefix of the connection string (eg. "daq.nd" or "daq.opcua")
     * @param prefix The connection string prefix
     */
    virtual ErrCode INTERFACE_FUNC getPrefix(IString** prefix) = 0;

    /*!
     * @brief Gets the type of connection supported by the device.
     * @param[out] type The type of connection (e.g., "TCP/IP").
     */
    virtual ErrCode INTERFACE_FUNC getConnectionType(IString** type) = 0;

    /*!
     * @brief Gets the client update method supported by the device.
     * @param[out] enabled The client update method (Boolean value indicating if core events are enabled for communication between server and client device).
     */
    virtual ErrCode INTERFACE_FUNC getCoreEventsEnabled(Bool* enabled) = 0;

    // [templateType(addresses, IString)]
    /*!
     * @brief Gets the device's list of addresses with the current protocol.
     * @param[out] addresses The device's list of addresses (hosts)
     */
    virtual ErrCode INTERFACE_FUNC getAddresses(IList** addresses) = 0;

    /*!
     * @brief Gets the port of the device with the current protocol.
     * @param[out] port The port of the device.
     */
    virtual ErrCode INTERFACE_FUNC getPort(IInteger** port) = 0;
    
    // [templateType(addressInfo, IAddressInfo)]
    /*!
     * @brief Gets the list of address information objects.
     * @param[out] addressInfo The list of address information objects.
     *
     * Address information duplicates the connection string and address as available on the Server Capability object.
     * Additionally, it provides information on what type of address it is (e.g., IPv4, IPv6), as well as whether the address is reachable.
     */
    virtual ErrCode INTERFACE_FUNC getAddressInfo(IList** addressInfo) = 0;
};
/*!@}*/

END_NAMESPACE_OPENDAQ
