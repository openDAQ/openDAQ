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
#include <coretypes/stringobject.h>
#include <opendaq/instance.h>

BEGIN_NAMESPACE_OPENDAQ

/*!
 * @ingroup opendaq_devices
 * @addtogroup opendaq_instance InstanceBuilder
 * @{
 */

/*!
 * @brief Builder component of Instance objects. Contains setter methods to configure the Instance parameters, and a
 * `build` method that builds the Instance object.
 */
DECLARE_OPENDAQ_INTERFACE(IInstanceBuilder, IBaseObject)
{
    /*!
     * @brief Builds and returns a Instance object using the currently set values of the Builder.
     * @param[out] instance The built Instance.
     */
    virtual ErrCode INTERFACE_FUNC build(IInstance** instance) = 0;

    // [returnSelf]
    /*!
     * @brief Sets the Logger of Instance
     * @param logger The Logger of Instance
     */
    virtual ErrCode INTERFACE_FUNC setLogger(ILogger* logger) = 0;

    /*!
     * @brief Gets the Logger of Instance
     * @param[out] logger Tthe Logger of Instance
     */
    virtual ErrCode INTERFACE_FUNC getLogger(ILogger** logger) = 0;

    // [returnSelf]
    /*!
     * @brief Sets the Logger global level of Instance
     * @param logLevel The Logger global level of Instance
     */
    virtual ErrCode INTERFACE_FUNC setGlobalLogLevel(LogLevel logLevel) = 0;

    /*!
     * @brief Gets the Logger global level of Instance
     * @param[out] logLevel The Logger global level of Instance
     */
    virtual ErrCode INTERFACE_FUNC getGlobalLogLevel(LogLevel* logLevel) = 0;

    // [returnSelf]
    /*!
     * @brief Sets the Logger level of Instance component
     * @param component The name of Instance component
     * @param logLevel The log level of Instance component
     */
    virtual ErrCode INTERFACE_FUNC setComponentLogLevel(IString* component, LogLevel logLevel) = 0;

    // [returnSelf]
    /*!
     * @brief Sets the sink logger level of Instance
     * @param sink The sink logger of Instance
     * @param logLevel The sink logger level of Instancee
     */
    virtual ErrCode INTERFACE_FUNC setSinkLogLevel(ILoggerSink* sink, LogLevel logLevel) = 0;

    // [returnSelf]
    /*!
     * @brief Sets the path for default ModuleManager of Instance. This method would be ignored if was called setModuleManager method
     * @param path The path for default ModuleManager of Instance
     */
    virtual ErrCode INTERFACE_FUNC setModulePath(IString* path) = 0;

    // [returnSelf]
    /*!
     * @brief Sets the ModuleManager of Instance.
     * @param moduleManager The ModuleManager of Instance
     */
    virtual ErrCode INTERFACE_FUNC setModuleManager(IModuleManager* moduleManager) = 0;

    /*!
     * @brief Gets the ModuleManager of Instance.
     * @param[out] moduleManager The ModuleManager of Instance
     */
    virtual ErrCode INTERFACE_FUNC getModuleManager(IModuleManager** moduleManager) = 0;

    // [returnSelf]
    /*!
     * @brief Sets the module manager of Instance
     * @param moduleManager The module manager of Instance
     */
    virtual ErrCode INTERFACE_FUNC setScheduler(IScheduler* scheduler) = 0;

    /*!
     * @brief Gets the module manager of Instance
     * @param[out] moduleManager The module manager of Instance
     */
    virtual ErrCode INTERFACE_FUNC getScheduler(IScheduler** scheduler) = 0;

    // [returnSelf]
    /*!
     * @brief Sets the option of Instance
     * @param option The name of option of Instance
     * @param option The value of option of Instance
     */
    virtual ErrCode INTERFACE_FUNC setOption(IString* option, IBaseObject* value) = 0;

    // [templateType(options, IString, IBaseObject)]
    /*!
     * @brief Gets dictionary of options of Instance
     * @param[out] option The dictionary of options of Instance
     */
    virtual ErrCode INTERFACE_FUNC getOptions(IDict** options) = 0;

    // [returnSelf]
    /*!
     * @brief Sets the local ID of Instance
     * @param rootDevice The local ID of Instance
     */
    virtual ErrCode INTERFACE_FUNC setInstanceLocalId(IString* localId) = 0;

    /*!
     * @brief Gets the local ID of Instance
     * @param[out] rootDevice The local ID of Instance
     */
    virtual ErrCode INTERFACE_FUNC getInstanceLocalId(IString** localId) = 0;

    // [returnSelf]
    /*!
     * @brief Sets the root device of Instance
     * @param rootDevice The root device of Instance
     */
    virtual ErrCode INTERFACE_FUNC setRootDevice(IDevice* rootDevice) = 0;

    /*!
     * @brief Gets the root device of Instance
     * @param[out] rootDevice The root device of Instance
     */
    virtual ErrCode INTERFACE_FUNC getRootDevice(IDevice** rootDevice) = 0;

    // [returnSelf]
    /*!
     * @brief Sets the default device of Instance
     * @param deviceInfo The device info of default device of Instance
     */
    virtual ErrCode INTERFACE_FUNC setDefaultRootDeviceInfo(IDeviceInfo* deviceInfo) = 0;

    /*!
     * @brief Gets the default device info of Instance
     * @param defaultDevice The default device info of Instance
     */
    virtual ErrCode INTERFACE_FUNC getDefaultRootDeviceInfo(IDeviceInfo** deviceInfo) = 0;
};
/*!@}*/

/*!
 * @ingroup opendaq_instance
 * @addtogroup opendaq_instance_factories Factories
 * @{
 */

// /*!
//  * @brief Creates a InstanceConfig with no parameters configured.
//  */
// OPENDAQ_DECLARE_CLASS_FACTORY_WITH_INTERFACE(LIBRARY_FACTORY, InstanceBuilder, IInstanceBuilder)

// /*!
//  * @brief InstanceConfig copy factory that creates a configurable Instance object from a possibly non-configurable Instance.
//  * @param instanceToCopy The Instance of which configuration should be copied.
//  */
// OPENDAQ_DECLARE_CLASS_FACTORY_WITH_INTERFACE(
//     LIBRARY_FACTORY, InstanceBuilderFromExisting, IInstanceBuilder,
//     IInstance*, instanceToCopy
// )

/*!@}*/

END_NAMESPACE_OPENDAQ
