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
#include <opendaq/module_manager.h>
#include <opendaq/module_manager_utils.h>
#include <opendaq/context_ptr.h>
#include <opendaq/logger_ptr.h>
#include <opendaq/logger_component_ptr.h>
#include <opendaq/device_info_ptr.h>
#include <coretypes/string_ptr.h>
#include <vector>
#include <opendaq/mirrored_device_config_ptr.h>
#include <opendaq/streaming_ptr.h>
#include <map>
#include <thread>
#include <boost/asio/executor_work_guard.hpp>
#include <boost/asio/io_context.hpp>
#include <opendaq/module_ptr.h>
#include <tsl/ordered_map.h>

BEGIN_NAMESPACE_OPENDAQ
struct ModuleLibrary;

class ModuleManagerImpl : public ImplementationOfWeak<IModuleManager, IModuleManagerUtils>
{
public:
    explicit ModuleManagerImpl(const BaseObjectPtr& path);
    ~ModuleManagerImpl() override;

    ErrCode INTERFACE_FUNC getModules(IList** availableModules) override;
    ErrCode INTERFACE_FUNC addModule(IModule* module) override;
    ErrCode INTERFACE_FUNC loadModules(IContext* context) override;

    ErrCode INTERFACE_FUNC getAvailableDevices(IList** availableDevices) override;
    ErrCode INTERFACE_FUNC getAvailableDeviceTypes(IDict** deviceTypes) override;
    ErrCode INTERFACE_FUNC createDevice(IDevice** device, IString* connectionString, IComponent* parent, IPropertyObject* config = nullptr) override;
    ErrCode INTERFACE_FUNC getAvailableFunctionBlockTypes(IDict** functionBlockTypes) override;
    ErrCode INTERFACE_FUNC createFunctionBlock(IFunctionBlock** functionBlock, IString* id, IComponent* parent, IPropertyObject* config = nullptr, IString* localId = nullptr) override;
    ErrCode INTERFACE_FUNC createStreaming(IStreaming** streaming, IString* connectionString, IPropertyObject* config = nullptr) override;
    ErrCode INTERFACE_FUNC getAvailableStreamingTypes(IDict** streamingTypes) override;
    ErrCode INTERFACE_FUNC createDefaultAddDeviceConfig(IPropertyObject** defaultConfig) override;
    ErrCode INTERFACE_FUNC createServer(IServer** server, IString* serverTypeId, IDevice* rootDevice, IPropertyObject* serverConfig = nullptr) override;

private:
    
    static void populateDeviceConfigFromConnStrOptions(const PropertyObjectPtr& devConfig,
                                                       const tsl::ordered_map<std::string, ObjectPtr<IBaseObject>>& options);
    static PropertyObjectPtr populateDeviceConfig(const PropertyObjectPtr& config,
                                                  const DeviceTypePtr& deviceType,
                                                  const tsl::ordered_map<std::string, ObjectPtr<IBaseObject>>& connStrOptions);

    std::string getPrefixFromConnectionString(std::string connectionString) const;
    static std::pair<std::string, tsl::ordered_map<std::string, BaseObjectPtr>> splitConnectionStringAndOptions(const std::string& connectionString);

    DeviceInfoPtr getDiscoveredDeviceInfo(const StringPtr& inputConnectionString, bool useSmartConnection) const;
    static StringPtr resolveSmartConnectionString(const StringPtr& inputConnectionString,
                                                  const DeviceInfoPtr& discoveredDeviceInfo,
                                                  const PropertyObjectPtr& config,
                                                  const LoggerComponentPtr& loggerComponent);
    DeviceTypePtr getDeviceTypeFromConnectionString(const StringPtr& connectionString, const ModulePtr& module) const;
    static uint16_t getServerCapabilityPriority(const ServerCapabilityPtr& cap);

    static void replaceSubDeviceOldProtocolIds(const DevicePtr& device);
    static ServerCapabilityPtr replaceOldProtocolIds(const ServerCapabilityPtr& cap);
    static StringPtr convertIfOldIdFB(const StringPtr& id);
    static StringPtr convertIfOldIdProtocol(const StringPtr& id);
    
    static void copyGeneralProperties(const PropertyObjectPtr& general, const PropertyObjectPtr& tartgetObj);
    static bool isDefaultAddDeviceConfig(const PropertyObjectPtr& config);

    static ServerCapabilityPtr mergeDiscoveryAndDeviceCapability(const ServerCapabilityPtr& discoveryCap, const ServerCapabilityPtr& deviceCap);
    void mergeDiscoveryAndDeviceCapabilities(const DevicePtr& device, const DeviceInfoPtr& discoveredDeviceInfo) const;
    void completeServerCapabilities(const DevicePtr& device) const;

    void checkNetworkSettings(ListPtr<IDeviceInfo>& list);
    static void setAddressesReachable(const std::map<std::string, bool>& addr, const std::string& type, ListPtr<IDeviceInfo>& info);
    static PropertyObjectPtr populateGeneralConfig(const PropertyObjectPtr& config);
    static ListPtr<IMirroredDeviceConfig> getAllDevicesRecursively(const MirroredDeviceConfigPtr& device);

    AddressInfoPtr findStreamingAddress(const ListPtr<IAddressInfo>& availableAddresses,
                                              const AddressInfoPtr& deviceConnectionAddress,
                                              const StringPtr& primaryAddressType);
    static AddressInfoPtr getDeviceConnectionAddress(const DevicePtr& device);
    static bool isValidConnectionAddressType(const StringPtr& connectionAddressType);
    void configureStreamings(const MirroredDeviceConfigPtr& topDevice, const PropertyObjectPtr& streamingConfig);
    void attachStreamingsToDevice(const MirroredDeviceConfigPtr& device,
                                  const PropertyObjectPtr& generalConfig,
                                  const PropertyObjectPtr& config,
                                  const AddressInfoPtr& deviceConnectionAddress);
    StreamingPtr onCreateStreaming(const StringPtr& connectionString, const PropertyObjectPtr& config) const;

    static PropertyObjectPtr createGeneralConfig();

    bool modulesLoaded;
    std::vector<std::string> paths;
    std::vector<ModuleLibrary> libraries;
    LoggerPtr logger;
    LoggerComponentPtr loggerComponent;

    std::vector<std::thread> pool;
    boost::asio::io_context ioContext;
    boost::asio::executor_work_guard<boost::asio::io_context::executor_type> work;

    DictPtr<IString, IDeviceInfo> availableDevicesGroup;
    std::unordered_map<std::string, size_t> functionBlockCountMap;
};

END_NAMESPACE_OPENDAQ
