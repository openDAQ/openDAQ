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
#include <opendaq/folder.h>
#include <opendaq/server_type.h>
#include <opendaq/function_block.h>
#include <coreobjects/property_object.h>
#include <opendaq/streaming.h>

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
 * [interfaceLibrary(ISearchFilter, "coretypes")]
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
 *
 * Function blocks added to a server configure or extend what it serves, and live and die with the server. Their output
 * signals belong to the server, not to the device: a device's recursive signal search does not return them, and servers
 * that stream device signals do not advertise them to their clients.
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

    /*!
     * @brief Gets a streaming source associated with server.
     * @param[out] streaming The streaming object that represents the server’s client-to-device streaming source.
     *
     * The streaming object is assigned on servers that support the generalized client-to-device streaming mechanism.
     * The object is expected to be associated with mirrored server-side copies of client signals and is responsible for receiving
     * their data sent from the client to the device.
     */
    virtual ErrCode INTERFACE_FUNC getStreaming(IStreaming** streaming) = 0;

    /*!
     * @brief Disables the server to be discovered by the clients.
     */
    virtual ErrCode INTERFACE_FUNC disableDiscovery() = 0;

    /*!
     * @brief Gets the type the server was created from.
     * @param[out] type The server type.
     *
     * A server whose module creates it from an ID alone returns a type with that ID and the Unspecified server kind.
     */
    virtual ErrCode INTERFACE_FUNC getServerType(IServerType** type) = 0;

    // [elementType(functionBlocks, IFunctionBlock)]
    /*!
     * @brief Gets the list of function blocks added to the server.
     * @param searchFilter Provides an optional filter that filters out unwanted components and allows for recursion.
     * @param[out] functionBlocks The list of added function blocks.
     *
     * If searchFilter is not provided, the returned list contains only visible function blocks and does not include those of
     * child function blocks.
     */
    virtual ErrCode INTERFACE_FUNC getFunctionBlocks(IList** functionBlocks, ISearchFilter* searchFilter = nullptr) = 0;

    // [templateType(functionBlockTypes, IString, IFunctionBlockType)]
    /*!
     * @brief Gets the function block types the server can add, containing their description.
     * @param[out] functionBlockTypes A dictionary of available function block types; empty when the server supports none.
     *
     * A server offers only its own function block types, never the general function block types of the instance.
     */
    virtual ErrCode INTERFACE_FUNC getAvailableFunctionBlockTypes(IDict** functionBlockTypes) = 0;

    /*!
     * @brief Creates and adds a function block to the server with the provided type ID and returns it.
     * @param[out] functionBlock The added function block.
     * @param typeId The ID of one of the types returned by getAvailableFunctionBlockTypes.
     * @param config A config object to configure a function block with custom settings specific to that function block type.
     * @retval OPENDAQ_ERR_NOTSUPPORTED The server supports no function blocks.
     */
    virtual ErrCode INTERFACE_FUNC addFunctionBlock(IFunctionBlock** functionBlock, IString* typeId, IPropertyObject* config = nullptr) = 0;

    /*!
     * @brief Removes the function block provided as argument, disconnecting its signals and input ports.
     * @param functionBlock The function block to be removed.
     */
    virtual ErrCode INTERFACE_FUNC removeFunctionBlock(IFunctionBlock* functionBlock) = 0;
};
/*!@}*/

END_NAMESPACE_OPENDAQ
