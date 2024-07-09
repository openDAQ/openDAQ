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
#include <opendaq/context.h>
#include <opendaq/device.h>
#include <opendaq/function_block.h>
#include <opendaq/server.h>
#include <opendaq/streaming.h>
#include <coretypes/listobject.h>
#include <coretypes/stringobject.h>
#include <coretypes/version_info.h>
#include <opendaq/streaming_type.h>

BEGIN_NAMESPACE_OPENDAQ

/*#
 * [interfaceLibrary(IPropertyObject, "coreobjects")]
 */

/*!
 * @ingroup opendaq_modules
 * @addtogroup opendaq_module Module
 * @{
 */

/*!
 *  @brief A module is an object that provides device and function block factories.
 *  The object is usually implemented in an external dynamic link / shared library.
 *  IModuleManager is responsible for loading all modules.
 */
DECLARE_OPENDAQ_INTERFACE(IModule, IBaseObject)
{
    /*!
     * @brief Retrieves the module version information.
     * @param[out] version The semantic version information.
     */
    virtual ErrCode INTERFACE_FUNC getVersionInfo(IVersionInfo** version) = 0;

    /*!
     * @brief Gets the module name.
     * @param[out] name The module name.
     */
    virtual ErrCode INTERFACE_FUNC getName(IString** name) = 0;

    /*!
     * @brief Gets the module id.
     * @param[out] id The module id.
     */
    virtual ErrCode INTERFACE_FUNC getId(IString** id) = 0;

    /*!
     * @brief Returns a list of known devices info.
     * The implementation can start discovery in background and only return the results in this function.
     * @param[out] availableDevices The list of known devices information.
     */
    // [elementType(availableDevices, IDeviceInfo)]
    virtual ErrCode INTERFACE_FUNC getAvailableDevices(IList** availableDevices) = 0;

    /*!
     * @brief Returns a dictionary of known and available device types this module can create.
     * @param[out] deviceTypes The dictionary of known device types.
     */
    // [templateType(deviceTypes, IString, IDeviceType)]
    virtual ErrCode INTERFACE_FUNC getAvailableDeviceTypes(IDict** deviceTypes) = 0;

    /*!
     * @brief Creates a device object that can communicate with the device described in the specified connection string.
     * The device object is not automatically added as a sub-device of the caller, but only returned by reference.
     * @param[out] device The device object created to communicate with and control the device.
     * @param connectionString Describes the connection info of the device to connect to.
     * @param parent The parent component/device to which the device attaches.
     * @param config A configuration object that contains parameters used to configure a device in the form of key-value pairs.
     */
    virtual ErrCode INTERFACE_FUNC createDevice(IDevice** device, IString* connectionString, IComponent* parent, IPropertyObject* config = nullptr) = 0;

    /*!
     * @brief Returns a dictionary of known and available function block types this module can create.
     * @param[out] functionBlockTypes The dictionary of known function block types.
     */
    // [templateType(functionBlockTypes, IString, IFunctionBlockType)]
    virtual ErrCode INTERFACE_FUNC getAvailableFunctionBlockTypes(IDict** functionBlockTypes) = 0;

    /*!
     * @brief Creates and returns a function block with the specified id.
     * The function block is not automatically added to the FB list of the caller.
     * @param id The id of the function block to create. Ids can be retrieved by calling `getAvailableFunctionBlockTypes()`.
     * @param parent The parent component/device to which the device attaches.
     * @param localId The local id of the function block.
     * @param config Function block configuration. In case of a null value, implementation should use default configuration.
     * @param[out] functionBlock The created function block.
     */
    virtual ErrCode INTERFACE_FUNC createFunctionBlock(IFunctionBlock** functionBlock, IString* id, IComponent* parent, IString* localId, IPropertyObject* config = nullptr) = 0;

    /*!
     * @brief Returns a dictionary of known and available servers types that this module can create.
     * @param[out] serverTypes The dictionary of known server types.
     */
    // [templateType(serverTypes, IString, IServerType)]
    virtual ErrCode INTERFACE_FUNC getAvailableServerTypes(IDict** serverTypes) = 0;

    /*!
     * @brief Creates and returns a server with the specified server type.
     * To prevent cyclic reference, we should not use the Instance instead of rootDevice.
     * @param serverTypeId The id of the server type to create. ServerType can be retrieved by calling `getAvailableServerTypes()`.
     * @param config Server configuration. In case of a null value, implementation should use default configuration.
     * @param rootDevice Root device.
     * @param[out] server The created server.
     */
    virtual ErrCode INTERFACE_FUNC createServer(IServer** server, IString* serverTypeId, IDevice* rootDevice, IPropertyObject* config = nullptr) = 0;


    /*!
     * @brief Creates and returns a streaming object using the specified connection string and config object.
     * @param connectionString Typically a connection string usually has a well known prefix, such as `daq.lt//`.
     * @param config A config object that contains parameters used to configure a streaming connection.
     * In case of a null value, implementation should use default configuration.
     * @param[out] streaming The created streaming object.
     */
    virtual ErrCode INTERFACE_FUNC createStreaming(IStreaming** streaming, IString* connectionString, IPropertyObject* config = nullptr) = 0;

    /*!
     * @brief Creates and returns a connection string from the specified server capability object.
     * @param serverCapability Represents the connection parameters of supported streaming or configuration protocol.
     * @param[out] connectionString The created connection string.
     * @return A non-zero error code if the @p serverCapability object is not complete enough to
     * generate a connection string.
     */
    virtual ErrCode INTERFACE_FUNC createConnectionString(IString** connectionString, IServerCapability* serverCapability) = 0;

    /*!
     * @brief Returns a dictionary of known and available streaming types that this module (client) can create.
     * @param[out] streamingTypes The dictionary of known streaming types.
     */
    // [templateType(streamingTypes, IString, IStreamingType)]
    virtual ErrCode INTERFACE_FUNC getAvailableStreamingTypes(IDict** streamingTypes) = 0;
};
/*!@}*/

END_NAMESPACE_OPENDAQ
