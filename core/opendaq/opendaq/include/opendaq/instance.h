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
#include <opendaq/context.h>
#include <opendaq/device.h>
#include <opendaq/module_manager.h>
#include <opendaq/server.h>

BEGIN_NAMESPACE_OPENDAQ

struct IInstanceBuilder;

/*!
 * @ingroup opendaq_devices
 * @addtogroup opendaq_instance Instance
 * @{
 */

/*#
 * [templated(IDevice)]
 * [interfaceSmartPtr(IDevice, GenericDevicePtr, "<opendaq/device.h>")]
 * [interfaceLibrary(IPropertyObject, "coreobjects")]
 */

/*!
 * @brief The top-level openDAQ object. It acts as container for the openDAQ context and the base module manager.
 *
 * It forwards all Device and PropertyObject calls to the current root device, making the calls on the Instance
 * and root device equivalent.
 *
 * On creation, it creates a Client device - a default device implementation that can load any function blocks
 * present in the module manager search path. If the native openDAQ client-module is loaded, the Client device
 * can connect to any TMS enabled device by using the `addDevice` function. The Client is set as the root device
 * when the instance is created.
 */
DECLARE_OPENDAQ_INTERFACE(IInstance, IDevice)
{
    /*!
     * @brief Gets the Module manager.
     * @param[out] manager The module manager.
     */
  	virtual ErrCode INTERFACE_FUNC getModuleManager(IModuleManager** manager) = 0;

    /*!
     * @brief Gets the current root device.
     * @param[out] rootDevice The current root device.
     *
     * All Device calls invoked on the Instance are forwarded to the current root device.
     */
    virtual ErrCode INTERFACE_FUNC getRootDevice(IDevice** rootDevice) = 0;

    /*!
     * @brief Adds a device with the connection string as root device.
     * @param connectionString The connection string containing the address of the device.
     * @param config A config object to configure a client device. This object can contain properties like max sample rate,
     * port to use for 3rd party communication, number of channels to generate, or other device specific settings. In case
     * of nullptr, a default configuration is used.
     *
     * All Device calls invoked on the Instance are forwarded to the root device. The root device can only be set once.
     */
    virtual ErrCode INTERFACE_FUNC setRootDevice(IString* connectionString, IPropertyObject* config = nullptr) = 0;

    // [templateType(serverTypes, IString, IServerType)]
    /*!
     * @brief Get a dictionary of available server types as <IString, IServerType> pairs
     * @param[out] serverTypes The dictionary of available server types.
     */
    virtual ErrCode INTERFACE_FUNC getAvailableServerTypes(IDict** serverTypes) = 0;

    /*!
     * @brief Creates and adds a server with the provided serverType and configuration.
     * @param serverTypeId Type id of the server. Can be obtained from its corresponding Server type object.
     * @param serverConfig Config of the server. Can be created from its corresponding Server type object.
     * In case of a null value, it will use the default configuration.
     * @param[out] server The added created server.
     */
    virtual ErrCode INTERFACE_FUNC addServer(IString* serverTypeId, IPropertyObject* serverConfig, IServer** server) = 0;

    // [elementType(servers, IServer)]
    /*!
     * @brief Creates and adds streaming and "openDAQ OpcUa" servers with default configurations.
     * @param[out] servers List of added created servers.
     */
    virtual ErrCode INTERFACE_FUNC addStandardServers(IList** servers) = 0;

    /*!
     * @brief Removes the server provided as argument.
     * @param server The server to be removed.
     */
    virtual ErrCode INTERFACE_FUNC removeServer(IServer* server) = 0;

    // [elementType(servers, IServer)]
    /*!
     * @brief Get list of added servers.
     * @param[out] servers List of added servers.
     */
    virtual ErrCode INTERFACE_FUNC getServers(IList** servers) = 0;
};
/*!@}*/

/*!
 * @brief Creates an openDAQ instance.
 * @param context The context object.
 * @param localId The localID of the instance.
 *
 * openDAQ application uses instance as an entry point and a root component. The instance is the first openDAQ object
 * that is created in the application.
 *
 * The caller should provide a localID that is a string that should be unique across multiple instances. If the
 * instance sets a root device, the localID of the root device is automatically used as localID of the instance.
 *
 * The caller should provide configured module manager and context.
 */
OPENDAQ_DECLARE_CLASS_FACTORY(
    LIBRARY_FACTORY, Instance,
    IContext*, context,
    IString*, localId
)

/*!
 * @brief Creates a Instance with Builder
 * @param builder Instance Builder
 */
OPENDAQ_DECLARE_CLASS_FACTORY_WITH_INTERFACE(
    LIBRARY_FACTORY, InstanceFromBuilder, IInstance,
    IInstanceBuilder*, builder
)

 /*!
 * @brief Creates an openDAQ client.
 * @param ctx The context object.
 * @param localId The localID of the client.
 * @param defaultDeviceInfo The DeviceInfo to be used by the client device.
 * @param parent The parent component of the client.
 */
/*#
 * [factory(NoConstructor)]
 */
OPENDAQ_DECLARE_CLASS_FACTORY_WITH_INTERFACE_AND_CREATEFUNC(
    LIBRARY_FACTORY, Client, IDevice, createClient,
    IContext*, ctx,
    IString*, localId,
    IDeviceInfo*, defaultDeviceInfo,
    IComponent*, parent
)

END_NAMESPACE_OPENDAQ
