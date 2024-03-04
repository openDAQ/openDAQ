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
#include <opendaq/module.h>
#include <opendaq/device_ptr.h>
#include <opendaq/function_block_ptr.h>
#include <opendaq/module_manager_ptr.h>
#include <opendaq/context_ptr.h>

#include <coretypes/intfs.h>
#include <coretypes/version_info_ptr.h>
#include <coretypes/validation.h>

#include <opendaq/logger_ptr.h>
#include <opendaq/logger_component_ptr.h>
#include <opendaq/streaming_ptr.h>
#include <opendaq/streaming_info_ptr.h>

#include <opendaq/custom_log.h>

BEGIN_NAMESPACE_OPENDAQ

class Module : public ImplementationOf<IModule>
{
public:

    /*!
     * @brief Retrieves the module version information.
     * @param[out] moduleVersion The semantic version information.
     */
    ErrCode INTERFACE_FUNC getVersionInfo(IVersionInfo** moduleVersion) override
    {
        OPENDAQ_PARAM_NOT_NULL(moduleVersion);

        *moduleVersion = version.addRefAndReturn();
        return OPENDAQ_SUCCESS;
    }

    /*!
     * @brief Gets the module name.
     * @param[out] moduleName The module name.
     */
    ErrCode INTERFACE_FUNC getName(IString** moduleName) override
    {
        OPENDAQ_PARAM_NOT_NULL(moduleName);

        *moduleName = name.addRefAndReturn();
        return OPENDAQ_SUCCESS;
    }

    /*!
     * @brief Gets the module id.
     * @param[out] id The module id.
     */
    ErrCode INTERFACE_FUNC getId(IString** moduleId) override
    {
        OPENDAQ_PARAM_NOT_NULL(moduleId);

        *moduleId = id.addRefAndReturn();
        return OPENDAQ_SUCCESS;
    }

    /*!
     * @brief Returns a list of known devices info.
     * The implementation can start discovery in background and only return the results in this function.
     * @param[out] availableDevices The list of known devices information.
     */
    ErrCode INTERFACE_FUNC getAvailableDevices(IList** availableDevices) override
    {
        OPENDAQ_PARAM_NOT_NULL(availableDevices);

        ListPtr<IDeviceInfo> availableDevicesPtr;
        ErrCode errCode = wrapHandlerReturn(this, &Module::onGetAvailableDevices, availableDevicesPtr);

        *availableDevices = availableDevicesPtr.detach();
        return errCode;
    }

    /*!
     * @brief Returns a dictionary of known and available device types this module can create.
     * @param[out] deviceTypes The dictionary of known device types.
     */
    ErrCode INTERFACE_FUNC getAvailableDeviceTypes(IDict** deviceTypes) override
    {
        OPENDAQ_PARAM_NOT_NULL(deviceTypes);

        DictPtr<IString, IDeviceType> types;
        ErrCode errCode = wrapHandlerReturn(this, &Module::onGetAvailableDeviceTypes, types);

        *deviceTypes = types.detach();
        return errCode;
    }

    /*!
     * @brief Checks if connection string can be used to connect to devices supported by this module.
     * @param connectionString Typically a connection string usually has a well known prefix, such as `opc.tcp//`.
     * Connection strings could simply be devices such as `obsidian` when openDAQ SDK is running on DAQ device hardware.
     * @param accepted Whether this module supports the @p connectionString.
     */
    ErrCode INTERFACE_FUNC acceptsConnectionParameters(Bool* accepted, IString* connectionString, IPropertyObject* config) override
    {
        OPENDAQ_PARAM_NOT_NULL(accepted);
        OPENDAQ_PARAM_NOT_NULL(connectionString);

        bool accepts;
        ErrCode errCode = wrapHandlerReturn(this, &Module::onAcceptsConnectionParameters, accepts, connectionString, config);

        *accepted = accepts;
        return errCode;
    }

    /*!
     * @brief Creates a device object that can communicate with the device described in the specified connection string.
     * The device object is not automatically added as a sub-device of the caller, but only returned by reference.
     * @param connectionString Describes the connection info of the device to connect to.
     * @param parent The parent component/device to which the device attaches.
     * @param[out] device The device object created to communicate with and control the device.
     */
    ErrCode INTERFACE_FUNC createDevice(IDevice** device, IString* connectionString, IComponent* parent, IPropertyObject* config) override
    {
        OPENDAQ_PARAM_NOT_NULL(connectionString);
        OPENDAQ_PARAM_NOT_NULL(device);

        DevicePtr createdDevice;
        ErrCode errCode = wrapHandlerReturn(this, &Module::onCreateDevice, createdDevice, connectionString, parent, config);

        *device = createdDevice.detach();
        return errCode;
    }

    /*!
     * @brief Returns a dictionary of known and available function blocks this module can create.
     * @param[out] functionBlockTypes The dictionary of known function blocks types.
     */
    ErrCode INTERFACE_FUNC getAvailableFunctionBlockTypes(IDict** functionBlockTypes) override
    {
        OPENDAQ_PARAM_NOT_NULL(functionBlockTypes);

        DictPtr<IString, IFunctionBlockType> types;
        ErrCode errCode = wrapHandlerReturn(this, &Module::onGetAvailableFunctionBlockTypes, types);

        *functionBlockTypes = types.detach();
        return errCode;
    }

    /*!
     * @brief Creates and returns a function block with the specified id.
     * The function block is not automatically added to the FB list of the caller.
     * @param id The id of the function block to create. Ids can be retrieved by calling `getAvailableFunctionBlockTypes()`.
     * @param parent The parent component/folder/device to which the device attaches.
     * @param localId The local id of the function block.
     * @param config Function block configuration. In case of a null value, implementation should use default configuration.
     * @param[out] functionBlock The created function block.
     */
    ErrCode INTERFACE_FUNC createFunctionBlock(IFunctionBlock** functionBlock, IString* id, IComponent* parent, daq::IString* localId, IPropertyObject* config = nullptr) override
    {
        OPENDAQ_PARAM_NOT_NULL(id);
        OPENDAQ_PARAM_NOT_NULL(functionBlock);

        FunctionBlockPtr block;
        ErrCode errCode = wrapHandlerReturn(this, &Module::onCreateFunctionBlock, block, id, parent, localId, config);

        *functionBlock = block.detach();
        return errCode;
    }

    /*!
     * @brief Returns a dictionary of known and available server types this module can create.
     * @param[out] serverTypes The dictionary of known server types information.
     */
    ErrCode INTERFACE_FUNC getAvailableServerTypes(IDict** serverTypes) override
    {
        OPENDAQ_PARAM_NOT_NULL(serverTypes);

        DictPtr<IString, IServerType> types;
        ErrCode errCode = wrapHandlerReturn(this, &Module::onGetAvailableServerTypes, types);

        *serverTypes = types.detach();
        return errCode;
    }


    /*!
     * @brief Creates and returns a server with the specified serverType.
     * @param serverTypeId The id of the server to create. Ids can be retrieved by calling `getAvailableServerTypes()`.
     * @param config Server configuration. In case of a null value, implementation should use default configuration.
     * @param rootDevice Root device.
     * @param[out] server The created server.
     */
    ErrCode INTERFACE_FUNC createServer(daq::IServer** server, IString* serverTypeId, daq::IDevice* rootDevice, IPropertyObject* config) override
    {
        OPENDAQ_PARAM_NOT_NULL(serverTypeId);
        OPENDAQ_PARAM_NOT_NULL(rootDevice);
        OPENDAQ_PARAM_NOT_NULL(server);
        
        ServerPtr serverInstance;
        ErrCode errCode = wrapHandlerReturn(this, &Module::onCreateServer, serverInstance, serverTypeId, config, rootDevice);

        *server = serverInstance.detach();
        return errCode;
    }

    /*!
     * @brief Verifies whether the provided connection string or config object can be used to establish a streaming connection
     * supported by this module. If the connection string is not assigned, it checks if the config object
     * is valid and complete enough to generate a connection string.
     * @param[out] accepted Whether this module supports the @p connectionString or @p config.
     * @param connectionString Typically a connection string usually has a well known prefix, such as `daq.wss//`.
     * @param config A configuration info object that contains streaming type ID and additional parameters.
     * The configuration info is used to generate a connection string if it is not present.
     */
    ErrCode INTERFACE_FUNC acceptsStreamingConnectionParameters(Bool* accepted, IString* connectionString, IStreamingInfo* config = nullptr) override
    {
        OPENDAQ_PARAM_NOT_NULL(accepted);
        if (connectionString == nullptr && config == nullptr)
            return makeErrorInfo(
                OPENDAQ_ERR_ARGUMENT_NULL,
                "At least one parameter connection string or config should be provided for streaming"
            );

        bool accepts;
        ErrCode errCode = wrapHandlerReturn(this, &Module::onAcceptsStreamingConnectionParameters, accepts, connectionString, config);

        *accepted = accepts;
        return errCode;
    }

    /*!
     * @brief Creates and returns a streaming object using the specified connection string or config info object.
     * @param connectionString Typically a connection string usually has a well known prefix, such as `daq.wss//`.
     * @param config Streaming configuration info.
     * @param[out] streaming The created streaming object.
     */
    ErrCode INTERFACE_FUNC createStreaming(IStreaming** streaming, IString* connectionString, IStreamingInfo* config) override
    {
        OPENDAQ_PARAM_NOT_NULL(streaming);
        if (connectionString == nullptr && config == nullptr)
            return makeErrorInfo(
                OPENDAQ_ERR_ARGUMENT_NULL,
                "At least one parameter connection string or config should be provided for streaming"
            );

        StreamingPtr streamingInstance;
        ErrCode errCode = wrapHandlerReturn(this, &Module::onCreateStreaming, streamingInstance, connectionString, config);

        *streaming = streamingInstance.detach();
        return errCode;
    }

    // Helpers

    /*!
     * @brief Retrieves information about known devices.
     * The implementation can start discovery in background and only return the results in this function.
     * @returns The list of known devices information.
     */
    virtual ListPtr<IDeviceInfo> onGetAvailableDevices()
    {
        return List<IDeviceInfo>();
    }

    /*!
     * @brief Retrieves a dictionary of known and available device types this module can create.
     * @returns A dictionary of known device types information.
     */
    virtual DictPtr<IString, IDeviceType> onGetAvailableDeviceTypes()
    {
        return Dict<IString, IDeviceType>();
    }

    /*!
     * @brief Checks if connection string can be used to connect to devices supported by this module.
     * @param connectionString Typically a connection string usually has a well known prefix, such as `opc.tcp//`.
     * Connection strings could simply be devices such as `obsidian` when openDAQ SDK is running on DAQ device hardware.
     * @param config A configuration object that contains connection parameters in the form of key-value pairs. The configuration
     * is used and applied when connecting to a device. This method check if provided config object is valid.
     * @returns Whether this module supports the @p connectionString and @p config is valid.
     */
    virtual bool onAcceptsConnectionParameters(const StringPtr& connectionString, const PropertyObjectPtr& config)
    {
        return false;
    }

    /*!
     * @brief Creates a device object that can communicate with the device described in the specified connection string.
     * The device object is not automatically added as a sub-device of the caller, but only returned by reference.
     * @param connectionString Describes the connection info of the device to connect to.
     * @param parent The parent component/device to which the device attaches.
     * @param config A configuration object that contains parameters used to configure a device in the form of key-value pairs.
     * @returns The device object created to communicate with and control the device.
     */
    virtual DevicePtr onCreateDevice(const StringPtr& connectionString, const ComponentPtr& parent, const PropertyObjectPtr& config)
    {
        return nullptr;
    }

    /*!
     * @brief Retrieves a dictionary of known and available function blocks this module can create.
     * @returns A dictionary of known function blocks information.
     */
    virtual DictPtr<IString, IFunctionBlockType> onGetAvailableFunctionBlockTypes()
    {
        return Dict<IString, IFunctionBlockType>();
    }

    /*!
     * @brief Creates and returns a function block with the specified id.
     * The function block is not automatically added to the FB list of the caller.
     * @param id The id of the function block to create. Ids can be retrieved by calling `getAvailableFunctionBlockTypes()`.
     * @param parent The parent component/folder/device to which the device attaches.
     * @returns The created function block.
     */
    virtual FunctionBlockPtr onCreateFunctionBlock(const StringPtr& id, const ComponentPtr& parent, const StringPtr& localId, const PropertyObjectPtr& config)
    {
        throw NotFoundException();
    }

    /*!
     * @brief Retrieves a dictionary of known and available server types this module can create.
     * @returns The dictionary of known server types information.
     */
    virtual DictPtr<IString, IServerType> onGetAvailableServerTypes()
    {
        return Dict<IString, IServerType>();
    }

    virtual ServerPtr onCreateServer(StringPtr serverType, PropertyObjectPtr serverConfig, DevicePtr rootDevice)
    {
        return nullptr;
    }

    virtual bool onAcceptsStreamingConnectionParameters(const StringPtr& connectionString, const StreamingInfoPtr& config)
    {
        return false;
    }

    virtual StreamingPtr onCreateStreaming(const StringPtr& connectionString, const StreamingInfoPtr& config)
    {
        return nullptr;
    }

    StreamingPtr createStreamingFromAnotherModule(const StringPtr& connectionString, const StreamingInfoPtr& config)
    {
        StreamingPtr streaming = nullptr;
        ModuleManagerPtr moduleManager = context.getModuleManager();
        for (const auto module : moduleManager.getModules())
        {
            bool accepted{};
            try
            {
                accepted = module.acceptsStreamingConnectionParameters(connectionString, config);
            }
            catch(NotImplementedException&)
            {
                LOG_I("{}: acceptsStreamingConnectionParameters not implemented", module.getName())
                accepted = false;
            }
            catch(const std::exception& e)
            {
                LOG_W("{}: acceptsStreamingConnectionParameters failed: {}", module.getName(), e.what())
                accepted = false;
            }

            if (accepted)
            {
                streaming = module.createStreaming(connectionString, config);
            }
        }
        return streaming;
    }

protected:
    StringPtr name;
    StringPtr id;
    VersionInfoPtr version;

    ContextPtr context;

    LoggerPtr logger;
    LoggerComponentPtr loggerComponent;

    Module(StringPtr name, VersionInfoPtr version, ContextPtr context, StringPtr id = nullptr)
        : name(std::move(name))
        , id (std::move(id))
        , version(std::move(version))
        , context(std::move(context))
        , logger(this->context.getLogger())
        , loggerComponent( this->logger.assigned()
                              ? this->logger.getOrAddComponent(this->name.assigned() ? this->name : "UnknownModule" )
                              : throw ArgumentNullException("Logger must not be null"))
    {
    }
};

END_NAMESPACE_OPENDAQ
