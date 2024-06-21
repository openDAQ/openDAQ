/*
 * Copyright 2022-2024 Blueberry d.o.o.
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
#include <coretypes/stringobject.h>
#include <opendaq/instance.h>
#include <opendaq/config_provider.h>
#include <coreobjects/authentication_provider.h>
#include <opendaq/discovery_server.h>

BEGIN_NAMESPACE_OPENDAQ

/*!
 * @ingroup opendaq_devices
 * @addtogroup opendaq_instance InstanceBuilder
 * @{
 */

/*#
 * [interfaceLibrary(IPropertyObject, "coreobjects")]
 * [interfaceLibrary(IAuthenticationProvider, "coreobjects")]
 */

/*!
 * @brief Builder component of Instance objects. Contains setter methods to configure the Instance parameters, such as Context (Logger, Scheduler, ModuleManager) and RootDevice.
 * Contains a  `build` method that builds the Instance object.
 *
 * The InstanceBuilder provides a fluent interface for setting various configuration options for an Instance
 * object before its creation. It allows customization of the logger, module manager, scheduler and root device. 
 * Once configured, calling the `build` method returns a fully initialized Instance object with the specified settings.
 * 
 * @subsection Configuration Methods:
 * The InstanceBuilder provides the following configuration methods:
 *
 * - **Logger:** The custom Logger for the Instance. This logger will be used for logging messages related to the Instance and its components. 
 *   If a custom logger is set, the `Logger sink` will be ignored since it is only used with the default Instance logger.
 *   If custom logger was not set, builder will generate Instance with default logger.
 *
 * - **Global log level:** The Logger global log level for the Instance. All log messages with a severity
 *   level equal to or higher than the specified level will be processed. Default log level is LogLevel::Default
 *
 * - **Component log level:** The Logger level for a specific component of the Instance. Log messages related to
 *   that component will be processed according to the specified log level. By default, each component uses the global log level.  
 *
 * - **Logger sink:** The logger sink to the default Instance logger. This sink will be responsible for processing log messages, 
 *   such as writing them to a file or sending them to a remote server. If `Logger` has been set, configuring of the 'Logger sink' has no effect in building Instance.
 *   If logger sinks has not been configure, the Instance uses 'default sinks'.
 *
 * - **Module manager:** The custom ModuleManager for the Instance. When configured, the default module manager path will be ignored. 
 *   If module manager has not configured, the Instance uses built in manager
 *
 * - **Module path:** The path for the default ModuleManager of the Instance. If a custom module manager has not been set, 
 *   this path will be used to load modules. Default module path is empty string
 *
 * - **Scheduler:** The custom scheduler for the Instance. If set, the number of worker threads will be ignored.
 *   If scheduler has not been configured, the Instance uses default schduler. 
 *
 * - **Scheduler worker num:** The number of worker threads in the scheduler of the Instance. if a scheduler has not been set and worker num is 0,
 *   which considers as maximum number of concurrent threads.
 *
 * - **Default root device local ID:** The local id of the default client root device. Has no effect if `Root device` has been congigured.
 *
 * - **Default root device info:** The information of default root device of the Instance such as serial number. Has no effect if `Root device` has been congigured.
 *
 * - **Root device:** The connection string of a device that replaces the default openDAQ root device (virtual client). 
 *   When the instance is created a connection to the device with the given connection string will be established, and the device will be placed at the root of the component tree structure.
 *   When configured, the `Default root device local ID` and `Default root device info` will be ignored.
 *
 * - **Sink log level:** The sink logger level of the default Instance logger. This level is ignored if a custom logger has been configured.
 * 
 * - **Config provider:** The config provider is expanding the local options of instance builder from json file or command lines.
 *   If value was set before, provider will override this with new one.
 * 
 * - **Module options:** Local options dictionary of instance builder has `modules` key which contains custom values for modules.
 *   By default this dictionary is empty, but can be populated from json file, env variables or command line arguments.
 */
DECLARE_OPENDAQ_INTERFACE(IInstanceBuilder, IBaseObject)
{
    /*!
     * @brief Builds and returns an Instance object using the currently set values of the Builder.
     * @param[out] instance The built Instance.
     */
    virtual ErrCode INTERFACE_FUNC build(IInstance** instance) = 0;

    // [returnSelf]
    /*!
     * @brief Populates internal options dictionary with values from set config provider
     * @param configProvider The configuration provider
     */
    virtual ErrCode INTERFACE_FUNC addConfigProvider(IConfigProvider* configProvider) = 0;

    // [returnSelf]
    /*!
     * @brief Sets the Context object of the instance. This overwrites other context related
     * settings such as logger, scheduler and module manager settings.
     * @param context The Context object for instance.
     */
    virtual ErrCode INTERFACE_FUNC setContext(IContext* context) = 0;

    /*!
     * @brief Returns a context object of the instance.
     * @param[out] context The Context object of the instance.
     */
    virtual ErrCode INTERFACE_FUNC getContext(IContext** context) = 0;

    // [returnSelf]
    /*!
     * @brief Sets the custom Logger for the Instance. This logger will be used for logging messages related to the Instance and its components. 
     * When configured, the `Logger sink` will be ignored, as it is in use only with the default Instance logger.
     * @param logger The custom Logger of Instance
     */
    virtual ErrCode INTERFACE_FUNC setLogger(ILogger* logger) = 0;

    /*!
     * @brief Gets the Logger of the Instance. Returns nullptr if custom logger has not been set
     * @param[out] logger The Logger of Instance
     */
    virtual ErrCode INTERFACE_FUNC getLogger(ILogger** logger) = 0;

    // [returnSelf]
    /*!
     * @brief Sets the Logger global log level for the Instance. All log messages with a severity
     * level equal to or higher than the specified level will be processed.
     * @param logLevel The Logger global level of Instance
     */
    virtual ErrCode INTERFACE_FUNC setGlobalLogLevel(LogLevel logLevel) = 0;

    /*!
     * @brief Gets the default Logger global level of Instance
     * @param[out] logLevel The Logger global level of Instance. Returns LogLevel::Default, If global log level has not been set
     */
    virtual ErrCode INTERFACE_FUNC getGlobalLogLevel(LogLevel* logLevel) = 0;

    // [returnSelf]
    /*!
     * @brief Sets The Logger level for a specific component of the Instance. Log messages related to
     * that component will be processed according to the specified log level.
     * @param component The name of Instance component
     * @param logLevel The log level of Instance component
     */
    virtual ErrCode INTERFACE_FUNC setComponentLogLevel(IString* component, LogLevel logLevel) = 0;
    
    // [templateType(components, IString, INumber)]
    /*!
     * @brief Gets the dictionary of component names and log level which will be added to logger components
     * @param[out] components The dictionary of component names and log level
     */
    virtual ErrCode INTERFACE_FUNC getComponentsLogLevel(IDict** components) = 0;
    
    // [returnSelf]
    /*!
     * @brief Adds the logger sink of the default Instance logger. If Logger has been set, configuring of the Logger sink has no effect in building Instance.
     * @param sink The logger sink of the default Instance logger
     */
    virtual ErrCode INTERFACE_FUNC addLoggerSink(ILoggerSink* sink) = 0;

    // [returnSelf]
    /*!
     * @brief Sets the sink logger level of the default Instance logger. If Logger has been set, configuring of the Logger sink has no effect in building Instance.
     * @param sink The sink logger of the default Instance logger
     * @param logLevel The sink logger level of the default Instance logger
     */
    virtual ErrCode INTERFACE_FUNC setSinkLogLevel(ILoggerSink* sink, LogLevel logLevel) = 0;

    // [elementType(sinks, ILoggerSink)]
    /*!
     * @brief Gets the list of logger sinks for the default Instance logger.
     * @param[out] sinks The list of logger sinks of the default Instance logger
     */
    virtual ErrCode INTERFACE_FUNC getLoggerSinks(IList** sinks) = 0;

    // [returnSelf]
    /*!
     * @brief Sets the path for the default ModuleManager of the Instance. If Module manager has been set, configuring of Module path has no effect in building Instance.
     * @param path The path for the default ModuleManager of Instance
     */
    virtual ErrCode INTERFACE_FUNC setModulePath(IString* path) = 0;

    /*!
     * @brief Gets the path for the default ModuleManager of Instance.
     * @param[out] path The path for the default ModuleManager of Instance. Returns empty string, If module path has not been set
     */
    virtual ErrCode INTERFACE_FUNC getModulePath(IString** path) = 0;

    // [returnSelf]
    /*!
     * @brief Add the path for the default ModuleManager of the Instance. If Module manager has been set, configuring of Module path has no effect in building Instance.
     * @param path The path for the default ModuleManager of Instance
     */
    virtual ErrCode INTERFACE_FUNC addModulePath(IString* path) = 0;
    
    // [elementType(paths, IString)]
    /*!
     * @brief Get the list of paths for the default ModuleManager of the Instance. If Module manager has been set, configuring of Module path has no effect in building Instance.
     * @param paths The paths for the default ModuleManager of Instance
     */
    virtual ErrCode INTERFACE_FUNC getModulePathsList(IList** paths) = 0;

    // [returnSelf]
    /*!
     * @brief Sets The custom ModuleManager for the Instance.
     * @param moduleManager The custom ModuleManager of Instance
     */
    virtual ErrCode INTERFACE_FUNC setModuleManager(IModuleManager* moduleManager) = 0;

    /*!
     * @brief Gets the custom ModuleManager of Instance
     * @param[out] moduleManager The ModuleManager of Instance. Returns nullptr, if custom ModuleManager has not been set
     */
    virtual ErrCode INTERFACE_FUNC getModuleManager(IModuleManager** moduleManager) = 0;

    // [returnSelf]
    /*!
     * @brief Sets the AuthenticationProvider for the Instance.
     * @param authenticationProvider The AuthenticationProvider for the Instance
     */
    virtual ErrCode INTERFACE_FUNC setAuthenticationProvider(IAuthenticationProvider* authenticationProvider) = 0;

    /*!
     * @brief Gets the AuthenticationProvider of Instance
     * @param[out] authenticationProvider The AuthenticationProvider of Instance.
     */
    virtual ErrCode INTERFACE_FUNC getAuthenticationProvider(IAuthenticationProvider** authenticationProvider) = 0;

    // [returnSelf]
    /*!
     * @brief Sets the number of worker threads in the scheduler of the Instance. If Scheduler has been set, configuring of Scheduler worker num has no effect in building Instance.
     * @param numWorkers The amount of worker threads in the scheduler of Instance. If @c is 0, then the amount of workers is the maximum number of concurrent threads supported by the implementation.
     */
    virtual ErrCode INTERFACE_FUNC setSchedulerWorkerNum(SizeT numWorkers) = 0;

    /*!
     * @brief Gets the amount of worker threads in the scheduler of Instance.
     * @param[out] numWorkers The amount of worker threads in the scheduler of Instance. Returns 0, if worker num has not been set
     */
    virtual ErrCode INTERFACE_FUNC getSchedulerWorkerNum(SizeT* numWorkers) = 0;

    // [returnSelf]
    /*!
     * @brief Sets the custom scheduler of Instance
     * @param scheduler The custom scheduler of Instance
     */
    virtual ErrCode INTERFACE_FUNC setScheduler(IScheduler* scheduler) = 0;

    /*!
     * @brief Gets the custom scheduler of Instance
     * @param[out] scheduler The custom scheduler of Instance. Returns nullptr, if custom Scheduler has not been set.
     */
    virtual ErrCode INTERFACE_FUNC getScheduler(IScheduler** scheduler) = 0;

    // [returnSelf]
    /*!
     * @brief Sets the local id for default device. Has no effect if `Root device` has been congigured.
     * @param localId The default root device local id
     */
    virtual ErrCode INTERFACE_FUNC setDefaultRootDeviceLocalId(IString* localId) = 0;

    /*!
     * @brief Gets the default root device local id
     * @param[out] localId The default root device local id. Returns empty string id default root device local is has not been set.
     */
    virtual ErrCode INTERFACE_FUNC getDefaultRootDeviceLocalId(IString** localId) = 0;

    // [returnSelf]
    /*!
    * @brief Sets the connection string for a device that replaces the default openDAQ root device. 
    * When the instance is created, a connection to the device with the given connection string will be established, 
    * and the device will be placed at the root of the component tree structure.
    * @param connectionString The connection string for the root device of the Instance.
    * @param config A config object to configure a client device. This object can contain properties like max sample rate,
    * port to use for 3rd party communication, number of channels to generate, or other device specific settings. In case
    * of nullptr, a default configuration is used.
    */
    virtual ErrCode INTERFACE_FUNC setRootDevice(IString* connectionString, IPropertyObject* config = nullptr) = 0;

    /*!
     * @brief Gets the connection string for the default root device of Instance.
     * @param[out] connectionString The connection string for the root device of Instance. Returns nullptr, if root device connection string has not been set.
     */
    virtual ErrCode INTERFACE_FUNC getRootDevice(IString** connectionString) = 0;

    /*!
     * @brief Gets the configuration property object for the default root device of Instance.
     * @param[out] config The configuraton property object for the root device of Instance. Returns nullptr, for the default
     * configuration property object.
     */
    virtual ErrCode INTERFACE_FUNC getRootDeviceConfig(IPropertyObject** config) = 0;

    // [returnSelf]
    /*!
     * @brief Sets the default device info of Instance. If device info has been set, method getInfo of Instance will return set device info if Root Device has not been set
     * @param deviceInfo The device info of the default device of Instance
     */
    virtual ErrCode INTERFACE_FUNC setDefaultRootDeviceInfo(IDeviceInfo* deviceInfo) = 0;

    /*!
     * @brief Gets the default device info of Instance
     * @param deviceInfo The default device info of Instance. Returns nullptr, if default device info has not been set.
     */
    virtual ErrCode INTERFACE_FUNC getDefaultRootDeviceInfo(IDeviceInfo** deviceInfo) = 0;

    // [templateType(options, IString, IBaseObject)]
    /*!
     * @brief Gets the dictionary of instance options
     * @param[out] options The dictionary of instance options
     */
    virtual ErrCode INTERFACE_FUNC getOptions(IDict** options) = 0;

    // [returnSelf]
    /*!
     * @brief Allows enabling or disabling standard configuration providers, including JsonConfigProvider, based on the specified flag.
     * @param flag Boolean flag indicating whether to enable (true) or disable (false) standard config providers.
     */
    virtual ErrCode INTERFACE_FUNC enableStandardProviders(Bool flag) = 0;

    // [elementType(serverNames, IString)]
    /*!
     * @brief Gets the dictionary of discovery servers
     * @param[out] serverNames The dictionary of discovery server names
     */
    virtual ErrCode INTERFACE_FUNC getDiscoveryServers(IList** serverNames) = 0;

    // [returnSelf]
    /*!
     * @brief Adds a discovery server to the context
     * @param serverName The discovery server to add
     *
     * openDAQ supports the "mdns" server by default, but must be added to the instance builder to be enabled.
     */
    virtual ErrCode INTERFACE_FUNC addDiscoveryServer(IString* serverName) = 0;
};
/*!@}*/

/*!
 * @ingroup opendaq_instance
 * @addtogroup opendaq_instance_factories Factories
 * @{
 */

/*!
 * @brief Creates a InstanceBuilder with no parameters configured.
 */
OPENDAQ_DECLARE_CLASS_FACTORY_WITH_INTERFACE(LIBRARY_FACTORY, InstanceBuilder, IInstanceBuilder)
/*!@}*/

END_NAMESPACE_OPENDAQ
