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
#include <opendaq/device_domain.h>
#include <opendaq/device_info.h>
#include <opendaq/function_block.h>
#include <coreobjects/property_object.h>
#include <coretypes/listobject.h>
#include <opendaq/device_type.h>
#include <opendaq/streaming.h>

BEGIN_NAMESPACE_OPENDAQ

/*#
 * [interfaceSmartPtr(IFolder, GenericFolderPtr, "<opendaq/folder_ptr.h>")]
 * [templated(defaultAliasName: DevicePtr)]
 * [interfaceSmartPtr(IDevice, GenericDevicePtr)]
 * [interfaceSmartPtr(IPropertyObject, PropertyObjectPtr, "<coreobjects/property_object.h>")]
 */

/*!
 * @ingroup opendaq_devices
 * @addtogroup opendaq_device Device
 * @{
 */

/*!
 * @brief Represents an openDAQ device. The device contains a list of signals and physical channels.
 * Some devices support adding function blocks, or connecting to devices. The list of available
 * function blocks/devices can be obtained via the `getAvailable` functions, and added
 * via the `add` functions.
 *
 * Devices can be split up into three different types, with each devices supporting one or more:
 * 1. Physical devices
 *    Physical devices provide access to physical channels. They measure real-world data and send
 *    it via packets through output signals of channels. The list of channels can be obtained via
 *    `getChannels` as a flat list.
 * 2. Client devices
 *    Client devices can connect to other devices via their supported connection protocol. openDAQ
 *    natively supports connecting to TMS devices via its openDAQ OpcUa Client Module. A list of available
 *    devices a client device can connect to can be obtained via `getAvailableDevices`. The
 *    `addDevice` is used to connect to/add a device.
 * 3. Function block devices
 *    Function block devices provide a dictionary of available function block types that can be added to them
 *    and configured. The calculation of function blocks is done on the device itself. The dictionary
 *    of available function block types can be obtained via `getAvailableFunctionBlockTypes`. They
 *    can then be added via `addFunctionBlock`.
 *
 * All devices also provide access to their Device information, containing metadata such as the
 * device's serial number, location... They can also be queried for their current domain values
 * (time) through its device domain.
 *
 * As each device is a property object, a device has access to all Property object methods, allowing
 * each device to expose a list of custom properties such as sample rate, scaling factor and many
 * others. By default, openDAQ devices have the UserName and Location string Properties.
 */
DECLARE_OPENDAQ_INTERFACE(IDevice, IFolder)
{
    /*!
     * @brief Gets the device info. It contains data about the device such as the device's
     * serial number, location, and connection string.
     * @param[out] info The device info.
     */
    virtual ErrCode INTERFACE_FUNC getInfo(IDeviceInfo** info) = 0;

    /*!
     * @brief Gets the device's domain data. It allows for querying the device for its
     * domain (time) values.
     * @param[out] domain The device domain.
     */
    virtual ErrCode INTERFACE_FUNC getDomain(IDeviceDomain** domain) = 0;

    // [templateType(inputsOutputsFolder, IFolder)]
    /*!
     * @brief Gets a folder containing channels. 
     * @param[out] inputsOutputsFolder The folder that contains channels.
     *
     * The InputsOutputs folder can contain other folders that themselves contain channels.
     */
    virtual ErrCode INTERFACE_FUNC getInputsOutputsFolder(IFolder** inputsOutputsFolder) = 0;

    // [templateType(customComponents, IComponent)]
    /*!
     * @brief Gets a list of all components/folders in a device that are not titled 'io', 'sig', 'dev' or 'fb'
     * @param[out] customComponents The list of custom components.
     */
    virtual ErrCode INTERFACE_FUNC getCustomComponents(IList** customComponents) = 0;

    // [elementType(signals, ISignal)]
    /*!
     * @brief Gets a list of the device's signals.
     * @param searchFilter Provides an optional filter that filters out unwanted components and allows for recursion.
     * @param[out] signals The flat list of signals.
     *
     * If searchFilter is not provided, the returned list contains only visible signals and does not include those of
     * child function blocks, devices, or channels.
     *
     * Device signals are most often domain signals shared by other signals that belong to channels and/or function blocks.
     */
    virtual ErrCode INTERFACE_FUNC getSignals(IList** signals, ISearchFilter* searchFilter = nullptr) = 0;

    // [elementType(signals, ISignal)]
    /*!
     * @brief Gets a list of the signals that belong to the device.
     * @param searchFilter Provides an optional filter that filters out unwanted components and allows for recursion.
     * @param[out] signals The flat list of signals.
     *
     * The list includes visible signals that belong to visible channels, function blocks, or sub devices
     * of the device.
     */
    virtual ErrCode INTERFACE_FUNC getSignalsRecursive(IList** signals, ISearchFilter* searchFilter = nullptr) = 0;

    // [elementType(channels, IChannel)]
    /*!
     * @brief Gets a flat list of the device's physical channels.
     * @param searchFilter Provides an optional filter that filters out unwanted components and allows for recursion.
     * @param[out] channels The flat list of channels.
     *
     * If searchFilter is not provided, the returned list contains only visible channels and does not include those of
     * child devices.
     */
    virtual ErrCode INTERFACE_FUNC getChannels(IList** channels, ISearchFilter* searchFilter = nullptr) = 0;

    // [elementType(channels, IChannel)]
    /*!
     * @brief Gets a flat list of the device's physical channels. Also finds all visible channels of visible child devices
     * @param searchFilter Provides an optional filter that filters out unwanted components and allows for recursion.
     * @param[out] channels The flat list of channels.
     */
    virtual ErrCode INTERFACE_FUNC getChannelsRecursive(IList** channels, ISearchFilter* searchFilter = nullptr) = 0;

    // [elementType(devices, IDevice)]
    /*!
     * @brief Gets a list of child devices that the device is connected to.
     * @param searchFilter Provides an optional filter that filters out unwanted components and allows for recursion.
     * @param[out] devices The list of devices.
     *
     * If searchFilter is not provided, the returned list contains only visible devices and does not include those of
     * child devices.
     */
    virtual ErrCode INTERFACE_FUNC getDevices(IList** devices, ISearchFilter* searchFilter = nullptr) = 0;

    // [elementType(availableDevices, IDeviceInfo)]
    /*!
     * @brief Gets a list of available devices, containing their Device Info.
     * @param[out] availableDevices The list of available devices.
     *
     * The getAvailableDevices most often runs a discovery client, querying for available devices that
     * a device module can connect to. The replies are formed into Device Info objects and inserted to the
     * list of available devices.
     */
    virtual ErrCode INTERFACE_FUNC getAvailableDevices(IList** availableDevices) = 0;

    // [templateType(deviceTypes, IString, IDeviceType)]
    /*!
     * @brief Get a dictionary of available device types as <IString, IDeviceType> pairs
     * @param[out] deviceTypes The dictionary of available device types.
     */
    virtual ErrCode INTERFACE_FUNC getAvailableDeviceTypes(IDict** deviceTypes) = 0;

    // [templateType(device, IDevice)]
    /*!
     * @brief Connects to a device at the given connection string and returns it.
     * @param[out] device The added device.
     * @param connectionString The connection string containing the address of the device. In example an
     * IPv4/IPv6 address. The connection string can be found in the Device Info objects returned by
     * `getAvailableDevices`.
     * @param config A config object to configure a client device. This object can contain properties like max sample rate,
     * port to use for 3rd party communication, number of channels to generate, or other device specific settings. Can be
     * created from its corresponding Device type object. In case of a null value, it will use the default configuration.
     */
    virtual ErrCode INTERFACE_FUNC addDevice(IDevice** device, IString* connectionString, IPropertyObject* config = nullptr) = 0;

    // [templateType(device, IDevice)]
    /*!
     * @brief Disconnects from the device provided as argument and removes it from the internal list of devices.
     * @param device The device to be removed.
     */
    virtual ErrCode INTERFACE_FUNC removeDevice(IDevice* device) = 0;

    // [elementType(functionBlocks, IFunctionBlock)]
    /*!
     * @brief Gets the list of added function blocks.
     * @param searchFilter Provides an optional filter that filters out unwanted components and allows for recursion.
     * @param[out] functionBlocks The list of added function blocks.
     *
     * If searchFilter is not provided, the returned list contains only visible function blocks and does not include those of
     * child function blocks, devices, or channels.
     */
    virtual ErrCode INTERFACE_FUNC getFunctionBlocks(IList** functionBlocks, ISearchFilter* searchFilter = nullptr) = 0;

    // [templateType(functionBlockTypes, IString, IFunctionBlockType)]
    /*!
     * @brief Gets all function block types that are supported by the device, containing their description.
     * @param[out] functionBlockTypes A dictionary of available function block types.
     */
    virtual ErrCode INTERFACE_FUNC getAvailableFunctionBlockTypes(IDict** functionBlockTypes) = 0;

    /*!
     * @brief Creates and adds a function block to the device with the provided unique ID and returns it.
     * @param[out] functionBlock The added function block.
     * @param typeId The unique ID of the function block. Can be obtained from its corresponding Function Block Info
     * object.
     * @param config A config object to configure a function block with custom settings specific to that function block type.
     */
    virtual ErrCode INTERFACE_FUNC addFunctionBlock(IFunctionBlock** functionBlock, IString* typeId, IPropertyObject* config = nullptr) = 0;

    /*!
     * @brief Removes the function block provided as argument, disconnecting its signals and input ports.
     * @param functionBlock The function block to be removed.
     */
    virtual ErrCode INTERFACE_FUNC removeFunctionBlock(IFunctionBlock* functionBlock) = 0;

    /*!
     * @brief Saves the configuration of the device to string.
     * @param[out] configuration Serialized configuration of the device.
     */
    virtual ErrCode INTERFACE_FUNC saveConfiguration(IString** configuration) = 0;

    /*!
     * @brief Loads the configuration of the device from string.
     * @param configuration Serialized configuration of the device.
     */
    virtual ErrCode INTERFACE_FUNC loadConfiguration(IString* configuration) = 0;

    /*!
     * @brief Gets the number of ticks passed since the device's absolute origin.
     * @param[out] ticks The number of ticks.
     *
     * To scale the ticks into a domain unit, the Device's Domain should be used.
     */
    virtual ErrCode INTERFACE_FUNC getTicksSinceOrigin(UInt* ticks) = 0;

    /*!
     * @brief Connects to a streaming at the given connection string, adds it as a streaming source of device
     * and returns created streaming object.
     * @param[out] streaming The added streaming source.
     * @param connectionString The connection string containing the address of the streaming. In example an
     * IPv4/IPv6 address. The connection string can be found in the Server Capability objects returned by
     * `getInfo().getServerCapabilities()`.
     * @param config A config object to configure a streaming connection. This object can contain properties like
     * various connection timeouts or other streaming protocol specific settings. Can be created from its corresponding
     * Streaming type object. In case of a null value, it will use the default configuration.
     */
    virtual ErrCode INTERFACE_FUNC addStreaming(IStreaming** streaming, IString* connectionString, IPropertyObject* config = nullptr) = 0;

    /*!
     * @brief Creates config object that can be used when adding a device. Contains Device and Streaming default configuration
     * for all available Device/Streaming types. Also contains general add-device configuration settings.
     * @param[out] defaultConfig The configuration object containing default settings for adding a device.
     *
     * The default config object is organized to always have 3 object-type properties:
     *   - "General" Contains general properties such as "AutomaticallyConnectStreaming"
     *   - "Device": Contains a child object-type property for each available device type, with the key of each property
     *     being the ID of the device type. These can be configured to customize the `addDevice` call when using
     *     connecting to the selected device type (eg. via the native or OPC UA protocols).
     *   - "Streaming": Same as device, but used to configure each individual streaming connection established
     *     when calling `addDevice`.
     */
    virtual ErrCode INTERFACE_FUNC createDefaultAddDeviceConfig(IPropertyObject** defaultConfig) = 0;
};
/*!@}*/

END_NAMESPACE_OPENDAQ
