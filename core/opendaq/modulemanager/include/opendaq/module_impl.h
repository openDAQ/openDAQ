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
#include <opendaq/module.h>
#include <opendaq/device_ptr.h>
#include <opendaq/function_block_ptr.h>
#include <opendaq/module_manager_ptr.h>
#include <opendaq/context_ptr.h>
#include <coreobjects/property_object_factory.h>
#include <coretypes/intfs.h>
#include <coretypes/validation.h>
#include <opendaq/logger_ptr.h>
#include <opendaq/logger_component_ptr.h>
#include <opendaq/streaming_ptr.h>
#include <opendaq/streaming_type_ptr.h>
#include <opendaq/server_capability_config_ptr.h>
#include <opendaq/custom_log.h>
#include <opendaq/module_info_factory.h>
#include <opendaq/component_type_private.h>

BEGIN_NAMESPACE_OPENDAQ
class Module : public ImplementationOf<IModule>
{
public:

    /*!
     * @brief Retrieves the module information.
     * @param[out] info The module information.
     */
    ErrCode INTERFACE_FUNC getModuleInfo(IModuleInfo** info) override
    {
        OPENDAQ_PARAM_NOT_NULL(info);

        *info = moduleInfo.addRefAndReturn();
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

        for (const auto& type : types)
        {
            auto componentTypePrivate = type.second.asPtr<IComponentTypePrivate>();
            componentTypePrivate->setModuleInfo(this->moduleInfo);
        }

        *deviceTypes = types.detach();
        return errCode;
    }

    /*!
     * @brief Creates a device object that can communicate with the device described in the specified connection string.
     * The device object is not automatically added as a sub-device of the caller, but only returned by reference.
     * @param connectionString Describes the connection info of the device to connect to. 
     * If connection string starts with `daq://`, module chooses the optimal server capability based on protocol type
     * @param parent The parent component/device to which the device attaches.
     * @param[out] device The device object created to communicate with and control the device.
     */
    ErrCode INTERFACE_FUNC createDevice(IDevice** device, IString* connectionString, IComponent* parent, IPropertyObject* config) override
    {
        OPENDAQ_PARAM_NOT_NULL(connectionString);
        OPENDAQ_PARAM_NOT_NULL(device);

        DictPtr<IString, IDeviceType> types;
        ErrCode errCode = wrapHandlerReturn(this, &Module::onGetAvailableDeviceTypes, types);
        if (OPENDAQ_FAILED(errCode) && errCode != OPENDAQ_ERR_NOTIMPLEMENTED)
            return errCode;

        ComponentTypePtr deviceType;
        const StringPtr prefix = getPrefixFromConnectionString(connectionString);
        if (prefix.assigned() && prefix.getLength() != 0)
        {
            for (const auto& [_, type] : types)
            {
                if (type.getConnectionStringPrefix() == prefix)
                {
                    deviceType = type;
                    break;
                }
            }
        }

        DevicePtr createdDevice;
        errCode = wrapHandlerReturn(this, &Module::onCreateDevice, createdDevice, connectionString, parent, mergeConfig(config, deviceType));

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

        for (const auto& type : types)
        {
            auto componentTypePrivate = type.second.asPtr<IComponentTypePrivate>();
            componentTypePrivate->setModuleInfo(this->moduleInfo);
        }

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
        
        DictPtr<IString, IComponentType> types;
        ErrCode errCode = wrapHandlerReturn(this, &Module::onGetAvailableFunctionBlockTypes, types);
        if (OPENDAQ_FAILED(errCode) && errCode != OPENDAQ_ERR_NOTIMPLEMENTED)
            return errCode;

        ComponentTypePtr type;
        if (types.assigned() && types.hasKey(id))
            type = types.get(id);

        FunctionBlockPtr block;
        errCode = wrapHandlerReturn(this, &Module::onCreateFunctionBlock, block, id, parent, localId, mergeConfig(config, type));

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

        for (const auto& type : types)
        {
            auto componentTypePrivate = type.second.asPtr<IComponentTypePrivate>();
            componentTypePrivate->setModuleInfo(this->moduleInfo);
        }

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
        OPENDAQ_PARAM_NOT_NULL(server);

        DictPtr<IString, IComponentType> types;
        ErrCode errCode = wrapHandlerReturn(this, &Module::onGetAvailableServerTypes, types);
        if (OPENDAQ_FAILED(errCode) && errCode != OPENDAQ_ERR_NOTIMPLEMENTED)
            return errCode;
        
        ComponentTypePtr type;
        if (types.assigned() && types.hasKey(serverTypeId))
            type = types.get(serverTypeId);

        ServerPtr serverInstance;
        errCode = wrapHandlerReturn(this, &Module::onCreateServer, serverInstance, serverTypeId, mergeConfig(config, type), rootDevice);

        *server = serverInstance.detach();
        return errCode;
    }

    /*!
     * @brief Creates and returns a streaming object using the specified connection string and config object.
     * @param connectionString Typically a connection string usually has a well known prefix, such as `daq.lt//`.
     * @param config A config object that contains parameters used to configure a streaming connection.
     * In case of a null value, implementation should use default configuration.
     * @param[out] streaming The created streaming object.
     */
    ErrCode INTERFACE_FUNC createStreaming(IStreaming** streaming, IString* connectionString, IPropertyObject* config = nullptr) override
    {
        OPENDAQ_PARAM_NOT_NULL(streaming);
        OPENDAQ_PARAM_NOT_NULL(connectionString);
        
        DictPtr<IString, IStreamingType> types;
        ErrCode errCode = wrapHandlerReturn(this, &Module::onGetAvailableStreamingTypes, types);
        if (OPENDAQ_FAILED(errCode) && errCode != OPENDAQ_ERR_NOTIMPLEMENTED)
            return errCode;

        ComponentTypePtr streamingType;
        const StringPtr prefix = getPrefixFromConnectionString(connectionString);
        if (prefix.assigned() && prefix.getLength() != 0)
        {
            for (const auto& [_, type] : types)
            {
                if (type.getConnectionStringPrefix() == prefix)
                {
                    streamingType = type;
                    break;
                }
            }
        }

        StreamingPtr createdStreaming;
        errCode = wrapHandlerReturn(this, &Module::onCreateStreaming, createdStreaming, connectionString, mergeConfig(config, streamingType));

        *streaming = createdStreaming.detach();
        return errCode;
    }

    ErrCode INTERFACE_FUNC completeServerCapability(Bool* succeeded, IServerCapability* source, IServerCapabilityConfig* target) override
    {
        OPENDAQ_PARAM_NOT_NULL(target);
        OPENDAQ_PARAM_NOT_NULL(source);

        Bool succeededVal = false;
        ErrCode errCode = wrapHandlerReturn(this, &Module::onCompleteServerCapability, succeededVal, source, target);

        *succeeded = succeededVal;
        return errCode;
    }

    ErrCode INTERFACE_FUNC getAvailableStreamingTypes(IDict** streamingTypes) override
    {
        OPENDAQ_PARAM_NOT_NULL(streamingTypes);

        DictPtr<IString, IStreamingType> types;
        ErrCode errCode = wrapHandlerReturn(this, &Module::onGetAvailableStreamingTypes, types);

        for (const auto& type : types)
        {
            auto componentTypePrivate = type.second.asPtr<IComponentTypePrivate>();
            componentTypePrivate->setModuleInfo(this->moduleInfo);
        }

        *streamingTypes = types.detach();
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

    virtual DictPtr<IString, IStreamingType> onGetAvailableStreamingTypes()
    {
        return Dict<IString, IStreamingType>();
    }

    virtual ServerPtr onCreateServer(const StringPtr& serverType, const PropertyObjectPtr& serverConfig, const DevicePtr& rootDevice)
    {
        return nullptr;
    }

    virtual StreamingPtr onCreateStreaming(const StringPtr& connectionString, const PropertyObjectPtr& config)
    {
        return nullptr;
    }

    virtual Bool onCompleteServerCapability(const ServerCapabilityPtr& source, const ServerCapabilityConfigPtr& target)
    {
        return false;
    }

protected:
    ModuleInfoPtr moduleInfo;

    ContextPtr context;

    LoggerPtr logger;
    LoggerComponentPtr loggerComponent;

    Module(StringPtr name, VersionInfoPtr version, ContextPtr context, StringPtr id = nullptr)
        : moduleInfo(ModuleInfo(version, name, id))
        , context(std::move(context))
        , logger(this->context.getLogger())
        , loggerComponent(
              this->logger.assigned()
                  ? this->logger.getOrAddComponent(this->moduleInfo.getName().assigned() ? this->moduleInfo.getName() : "UnknownModule")
                  : throw ArgumentNullException("Logger must not be null"))
    {
    }

private:

    StringPtr getPrefixFromConnectionString(const StringPtr& connectionString) const
    {
        try
        {
            std::string str = connectionString;
            return str.substr(0, str.find("://"));
        }
        catch(...)
        {
            LOG_W("Connection string has no prefix denoted by the \"://\" delimiter")
        }

        return "";
    }

    static void populateDefaultConfig(const PropertyObjectPtr& defaultObj, const PropertyObjectPtr& userInput)
    {
        for (const auto& prop : defaultObj.getAllProperties())
            {
                const auto propName = prop.getName();

                if (userInput.hasProperty(propName))
                {
                    const auto userProp = userInput.getProperty(propName);
                    const auto defaultProp = defaultObj.getProperty(propName);

                    if (userProp.getValueType() != defaultProp.getValueType())
                        continue;

                    if (userProp.getValueType() == ctObject)
                        populateDefaultConfig(defaultProp.getValue(), userProp.getValue());
                    else
                        defaultObj.setPropertyValue(propName, userProp.getValue());
                }
            }
    }

    PropertyObjectPtr mergeConfig(const PropertyObjectPtr& userConfig, const ComponentTypePtr& type) const
    {
        
        PropertyObjectPtr configIn = userConfig.assigned() ? userConfig : PropertyObject();
        PropertyObjectPtr configOut;

        try
        {
            configOut = type.assigned() ? type.createDefaultConfig() : PropertyObject();
            populateDefaultConfig(configOut, configIn);
        }
        catch (const DaqException& e)
        {
            LOG_W("Failed to merge configuration: {}", e.what())
            return configIn;
        }
        catch (const std::exception& e)
        {
            LOG_W("Failed to merge configuration: {}", e.what())
            return configIn;
        }

        return configOut;
    }
};

END_NAMESPACE_OPENDAQ
