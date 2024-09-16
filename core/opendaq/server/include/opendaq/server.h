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
#include <opendaq/folder.h>
#include <opendaq/server_type.h>
#include <coreobjects/property_object.h>

BEGIN_NAMESPACE_OPENDAQ

/*!
 * @ingroup structure_servers
 * @addtogroup structure_server Server
 * @{
 */

/*#
 * [interfaceSmartPtr(IFolder, GenericFolderPtr, "<opendaq/folder_ptr.h>")]
 * [templated(defaultAliasName: ServerPtr)]
 * [interfaceSmartPtr(IServer, GenericServerPtr)]
 * [interfaceLibrary(IPropertyObject, "coreobjects")]
 */

/*!
 * @brief Represents a server. The server provides access to the openDAQ device.
 * Depend of the implementation, it can support configuring the device, reading configuration, and data streaming.
 *
 * We do not make the server directly. But through the instance and module manager. For that reason, the server must be uniquely
 * defined with "ServerType". The server is then created with the current root device, context and configuration object.
 *
 * Configuration of the server can be done with custom property object.
 * The configuration object is created with the corresponding ServerType object (IServerType::createDefaultConfig method).
 * For example, with a configuration object, we can define connection timeout.
 */
DECLARE_OPENDAQ_INTERFACE(IServer, IFolder)
{
    /*!
    * @brief Stops the server. This is called when we remove the server from the Instance or Instance is closing.
    */
    virtual ErrCode INTERFACE_FUNC stop() = 0;

    /*!
    * @brief Gets the server id.
    * @param[out] serverId The server id.
    */
    virtual ErrCode INTERFACE_FUNC getId(IString** serverId) = 0;

    /*!
    * @brief Enables the server to be discovered by the clients.
    */
    virtual ErrCode INTERFACE_FUNC enableDiscovery() = 0;

    // [elementType(signals, ISignal)]
    /*!
     * @brief Gets a list of the server's signals.
     * @param[out] signals The flat list of signals.
     *
     * Server signals are most often the mirrored representations of signals that belong to the client connected to the instance via this server.
     */
    virtual ErrCode INTERFACE_FUNC getSignals(IList** signals, ISearchFilter* searchFilter = nullptr) = 0;
};
/*!@}*/

END_NAMESPACE_OPENDAQ
