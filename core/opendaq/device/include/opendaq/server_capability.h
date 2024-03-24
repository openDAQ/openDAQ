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
#include <opendaq/context.h>
#include <coretypes/common.h>
#include <coretypes/stringobject.h>
#include <coretypes/enumeration.h>
#include <coreobjects/property_object.h>

BEGIN_NAMESPACE_OPENDAQ

/*!
 * @ingroup opendaq_devices
 * @addtogroup opendaq_server_capbility Server capbility
 * @{
 */

/*#
 * [interfaceSmartPtr(IEnumeration, EnumerationPtr, "<coretypes/enumeration_ptr.h>")]
 * [interfaceLibrary(IPropertyObject, "coreobjects")]
 * [interfaceSmartPtr(IPropertyObject, GenericPropertyObjectPtr, "<coreobjects/property_object_ptr.h>")]
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
 * - protocol type (e.g., "Structure&Streaming," "Streaming"), 
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

    /*!
     * @brief Gets the name of the protocol supported by the device.
     * @param[out] protocolName The name of the protocol (e.g., "openDAQ Native Streaming", "openDAQ OpcUa", "openDAQ WebsocketTcp Streaming").
     */
    virtual ErrCode INTERFACE_FUNC getProtocolName(IString** protocolName) = 0;

    /*!
     * @brief Gets the type of protocol supported by the device.
     * @param[out] type The type of protocol (Enumeration value reflecting protocol type: "Structure&Streaming", "Structure", "Streaming", "ServerStreaming", "Unknown").
     */
    virtual ErrCode INTERFACE_FUNC getProtocolType(IEnumeration** type) = 0;

    /*!
     * @brief Gets the type of connection supported by the device.
     * @param[out] type The type of connection (e.g., "IPv4", "IPv6").
     */
    virtual ErrCode INTERFACE_FUNC getConnectionType(IString** type) = 0;

    /*!
     * @brief Gets the client update method supported by the device.
     * @param[out] enabled The client update method (Boolean value indicating if core events are enabled for communication between server and client device).
     */
    virtual ErrCode INTERFACE_FUNC getCoreEventsEnabled(Bool* enabled) = 0;
};
/*!@}*/

END_NAMESPACE_OPENDAQ
