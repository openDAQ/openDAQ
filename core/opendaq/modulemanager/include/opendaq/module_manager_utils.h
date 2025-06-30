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
#include <opendaq/module.h>

BEGIN_NAMESPACE_OPENDAQ

/*#
 * [interfaceLibrary(IPropertyObject, "coreobjects")]
 * [interfaceSmartPtr(IInteger, IntegerPtr, "<coretypes/integer.h>")]
 * [interfaceSmartPtr(IErrorInfo, ObjectPtr<IErrorInfo>, "<coretypes/errorinfo.h>")]
 */

/*!
 * @ingroup opendaq_modules
 * @addtogroup opendaq_module_manager Module manager utils
 * @{
 */

DECLARE_OPENDAQ_INTERFACE(IModuleManagerUtils, IBaseObject)
{
    
    /*!
     * @brief Returns a list of known devices info.
     * The implementation can start discovery in background and only return the results in this function.
     * @param[out] availableDevices The list of known devices information.
     *
     * Contains information on devices available in all loaded modules.
     */
    // [elementType(availableDevices, IDeviceInfo)]
    virtual ErrCode INTERFACE_FUNC getAvailableDevices(IList** availableDevices) = 0;

    /*!
     * @brief Returns a dictionary of known and available device types this module can create.
     * @param[out] deviceTypes The dictionary of known device types.
     *
     * Contains information on devices available in all loaded modules.
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
     *
     * Iterates through all loaded modules and creates a device with the first module that accepts the provided connection string.
     */
    virtual ErrCode INTERFACE_FUNC createDevice(IDevice** device, IString* connectionString, IComponent* parent, IPropertyObject* config = nullptr) = 0;

    /*!
     * @brief Returns a dictionary of known and available function block types this module can create.
     * @param[out] functionBlockTypes The dictionary of known function block types.
     *
     * Contains information on function blocks available in all loaded modules.
     */
    // [templateType(functionBlockTypes, IString, IFunctionBlockType)]
    virtual ErrCode INTERFACE_FUNC getAvailableFunctionBlockTypes(IDict** functionBlockTypes) = 0;

    /*!
     * @brief Creates and returns a function block with the specified id.
     * The function block is not automatically added to the FB list of the caller.
     * @param id The id of the function block to create. Ids can be retrieved by calling `getAvailableFunctionBlockTypes()`.
     * @param parent The parent component/device to which the function block attaches.
     * @param config Function block configuration. In case of a null value, implementation should use default configuration.
     * @param localId Custom local ID for the function block. Overrides the "LocalId" property of the "config" object, if present.
     * @param[out] functionBlock The created function block.
     * 
     * Iterates through all loaded modules and creates a function block with the first module that accepts the provided connection string.
     * The local ID is equal to the name of the function block type with a "_n" suffix, where "n" is an integer, equal to that of the greatest
     * integer suffix amongst the function blocks of the same function block type already added to a given parent. The initial value of "n" is 0.
     * A custom local ID can be provided by adding a "LocalId" string property to the `config` property object input parameter, or by providing the
     * localId string argument.
     */
    virtual ErrCode INTERFACE_FUNC createFunctionBlock(IFunctionBlock** functionBlock, IString* id, IComponent* parent, IPropertyObject* config = nullptr, IString* localId = nullptr) = 0;
    
    /*!
     * @brief Creates a streaming object using the specified connection string and config object.
     * @param[out] streaming The created streaming object.
     * @param connectionString Describes the connection parameters of the streaming.
     * @param config A configuration object that contains parameters used to configure a streaming connection in the form of key-value pairs.
     *
     * Iterates through all loaded modules and creates a streaming connection with the first module that accepts the provided connection string.
     */
    virtual ErrCode INTERFACE_FUNC createStreaming(IStreaming** streaming, IString* connectionString, IPropertyObject* config = nullptr) = 0;
    // [templateType(streamingTypes, IString, IStreamingType)]
    virtual ErrCode INTERFACE_FUNC getAvailableStreamingTypes(IDict** streamingTypes) = 0;

    virtual ErrCode INTERFACE_FUNC createDefaultAddDeviceConfig(IPropertyObject** defaultConfig) = 0;

    /*!
     * @brief Creates and returns a server with the provided serverType and configuration.
     * @param serverTypeId Type id of the server. Can be obtained from its corresponding Server type object.
     * @param rootDevice The root device
     * @param serverConfig Config of the server. Can be created from its corresponding Server type object.
     * In case of a null value, it will use the default configuration.
     * @param[out] server The created server.
     *
     * Iterates through all loaded modules and creates a server with the first module that accepts the provided serverTypeId.
     * The servers folder of the root device is automatically assigned as parent for created server component.
     * The local ID of created server component is equal to the name of the server type.
     */
    virtual ErrCode INTERFACE_FUNC createServer(IServer** server, IString* serverTypeId, IDevice* rootDevice, IPropertyObject* serverConfig = nullptr) = 0;

    /*!
     * @brief Initiates the modification of IP configuration parameters for a specified network interface associated with a target device.
     * @param iface The name of the network interface whose IP configuration parameters need to be updated.
     * @param manufacturer The manufacturer's name identifying the device owning the network interface.
     * @param serialNumber The serial number of the device owning the network interface.
     * @param config A property object containing the configuration parameters to be applied.
     *
     * The manufacturer name and serial number are used to uniquely identify the target device. Once the config modification is invoked,
     * the new config parameters are advertised via multicast to all devices in the subnet. The target device compares the received
     * identification parameters with its own and, if they match, attempts to apply the new configuration parameters for the specified interface.
     */
    virtual ErrCode INTERFACE_FUNC changeIpConfig(IString* iface, IString* manufacturer, IString* serialNumber, IPropertyObject* config) = 0;

    /*!
     * @brief Attempts to retrieve the current IP configuration parameters for a specified network interface associated with a target device.
     * @param iface The name of the network interface whose IP configuration parameters are to be retrieved.
     * @param manufacturer The manufacturer's name identifying the device owning the network interface.
     * @param serialNumber The serial number of the device owning the network interface.
     * @param[out] config A property object where the retrieved configuration parameters are stored.
     *
     * The manufacturer name and serial number are used to uniquely identify the target device. The method queries the current
     * IP configuration parameters of the specified network interface via multicast addressing query to all devices in the subnet.
     * The target device compares the received identification parameters with its own and, if they match, attempts to retrieve
     * the currently active configuration parameters for the specified interface.
     */
    virtual ErrCode INTERFACE_FUNC requestIpConfig(IString* iface, IString* manufacturer, IString* serialNumber, IPropertyObject** config) = 0;

    /*!
     * @brief Completes the DeviceInfo's ServerCapabilities of existing device with the information obtained from device discovery.
     * @param device The device whose ServerCapabilities should be completed.
     */
    virtual ErrCode INTERFACE_FUNC completeDeviceCapabilities(IDevice* device) = 0;

    // [templateType(devices, IString, IDevice)]
    // [templateType(connectionArgs, IString, IPropertyObject)]
    // [templateType(errCodes, IString, IInteger)]
    // [templateType(errorInfos, IString, IErrorInfo)]
    /*!
     * @brief Creates multiple device objects in parallel using the specified connection strings. Each device is created concurrently.
     * None of the created device object are automatically added as a sub-device of the caller, but only returned by reference.
     *
     * @param[out] devices A dictionary which maps each connection string to the corresponding created device object.
     * If a device creation attempt fails, the value will be `nullptr` for that entry.
     *
     * @param connectionArgs A dictionary where each key is a connection string identifying the target device
     * (e.g., IPv4/IPv6), and each value is a configuration object that customizes the connection.
     * The configuration may specify parameters such as maximum sample rate, communication port, number of channels,
     * or other device-specific settings. A `nullptr` value indicates that the default configuration should be used.
     *
     * @param parent The parent component/device to which the created devices attach.
     *
     * @param[in,out] errCodes An optional dictionary to populate error codes for failed connections.
     * For each failed connection, the key is the connection string, and the value contains error code.
     *
     * @param[in,out] errorInfos An optional dictionary to populate error info details for failed connections.
     * For each failed connection, the key is the connection string, and the value contains error info object.
     *
     * @return OPENDAQ_PARTIAL_SUCCESS if at least one device was successfully created, but not all of them;
     *         OPENDAQ_ERR_GENERALERROR if no devices were created.
     */
    virtual ErrCode INTERFACE_FUNC createDevices(IDict** devices, IDict* connectionArgs, IComponent* parent, IDict* errCodes = nullptr, IDict* errorInfos = nullptr) = 0;
};
/*!@}*/

END_NAMESPACE_OPENDAQ
