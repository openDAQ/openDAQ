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
#include <daq_discovery/daq_discovery_client.h>
#include <opendaq/module_authenticator_ptr.h>

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
    ErrCode INTERFACE_FUNC loadModule(IString* path, IModule** module) override;
    ErrCode INTERFACE_FUNC setAuthenticatedOnly(Bool authenticatedOnly) override;
    ErrCode INTERFACE_FUNC setModuleAuthenticator(IModuleAuthenticator* authenticator) override;
    ErrCode INTERFACE_FUNC getVendorKeys(IDict** vendorKeys) override;

    ErrCode INTERFACE_FUNC getAvailableDevices(IList** availableDevices) override;
    ErrCode INTERFACE_FUNC getAvailableDeviceTypes(IDict** deviceTypes) override;
    ErrCode INTERFACE_FUNC createDevice(IDevice** device, IString* connectionString, IComponent* parent, IPropertyObject* config = nullptr) override;
    ErrCode INTERFACE_FUNC getAvailableFunctionBlockTypes(IDict** functionBlockTypes) override;
    ErrCode INTERFACE_FUNC createFunctionBlock(IFunctionBlock** functionBlock, IString* id, IComponent* parent, IPropertyObject* config = nullptr, IString* localId = nullptr) override;
    ErrCode INTERFACE_FUNC createStreaming(IStreaming** streaming, IString* connectionString, IPropertyObject* config = nullptr) override;
    ErrCode INTERFACE_FUNC getAvailableStreamingTypes(IDict** streamingTypes) override;
    ErrCode INTERFACE_FUNC createDefaultAddDeviceConfig(IPropertyObject** defaultConfig) override;
    ErrCode INTERFACE_FUNC createServer(IServer** server, IString* serverTypeId, IDevice* rootDevice, IPropertyObject* serverConfig = nullptr) override;
    ErrCode INTERFACE_FUNC changeIpConfig(IString* iface, IString* manufacturer, IString* serialNumber, IPropertyObject* config) override;
    ErrCode INTERFACE_FUNC requestIpConfig(IString* iface, IString* manufacturer, IString* serialNumber, IPropertyObject** config) override;
    ErrCode INTERFACE_FUNC completeDeviceCapabilities(IDevice* device) override;
    ErrCode INTERFACE_FUNC createDevices(IDict** devices, IDict* connectionArgs, IComponent* parent, IDict* errCodes = nullptr, IDict* errorInfos = nullptr) override;
    ErrCode INTERFACE_FUNC getDiscoveryInfo(IDeviceInfo** deviceInfo, IString* manufacturer, IString* serialNumber) override;

private:
    
    static void PopulateDeviceTypeConfigFromConnStrOptions(PropertyObjectPtr& deviceTypeConfig,
                                                           const tsl::ordered_map<std::string, ObjectPtr<IBaseObject>>& options);
    static PropertyObjectPtr PopulateDeviceTypeConfig(PropertyObjectPtr& addDeviceConfig,
                                                      const PropertyObjectPtr& inputConfig,
                                                      const DeviceTypePtr& deviceType,
                                                      const tsl::ordered_map<std::string, ObjectPtr<IBaseObject>>& connStrOptions);

    std::string getPrefixFromConnectionString(std::string connectionString) const;
    static std::pair<std::string, tsl::ordered_map<std::string, BaseObjectPtr>> SplitConnectionStringAndOptions(const std::string& connectionString);

    DeviceInfoPtr getSmartConnectionDeviceInfo(const StringPtr& inputConnectionString) const;
    DeviceInfoPtr getDiscoveredDeviceInfo(const DeviceInfoPtr& deviceInfo) const;
    StringPtr getConnectionStringWithType(const ServerCapabilityPtr& capability, const StringPtr& primaryAddressType) const;
    StringPtr resolveSmartConnectionString(const StringPtr& inputConnectionString,
                                           const DeviceInfoPtr& discoveredDeviceInfo,
                                           const PropertyObjectPtr& generalConfig,
                                           const LoggerComponentPtr& loggerComponent) const;
    DeviceTypePtr getDeviceTypeFromConnectionString(const StringPtr& connectionString, const ModulePtr& module) const;

    static uint16_t GetServerCapabilityPriority(const ServerCapabilityPtr& cap);

    static void ReplaceSubDeviceOldProtocolIds(const DevicePtr& device);
    static ServerCapabilityPtr ReplaceOldProtocolIds(const ServerCapabilityPtr& cap);
    static StringPtr ConvertIfOldIdFB(const StringPtr& id);
    static StringPtr ConvertIfOldIdProtocol(const StringPtr& id);
    
    static void CopyCommonGeneralPropValues(PropertyObjectPtr& addDeviceConfig);
    static bool IsDefaultAddDeviceConfig(const PropertyObjectPtr& config);

    static ServerCapabilityPtr MergeDiscoveryAndDeviceCapability(const ServerCapabilityPtr& discoveryCap, const ServerCapabilityPtr& deviceCap);
    void mergeDiscoveryAndDeviceCapabilities(const DevicePtr& device, const DeviceInfoPtr& discoveredDeviceInfo) const;
    void completeServerCapabilities(const DevicePtr& device) const;

    void checkNetworkSettings(ListPtr<IDeviceInfo>& list);
    void setAddressesReachable(const std::map<std::string, bool>& addr, ListPtr<IDeviceInfo>& info);

    static PropertyObjectPtr PopulateGeneralConfig(PropertyObjectPtr& addDeviceConfig, const PropertyObjectPtr& inputConfig);

    StreamingPtr onCreateStreaming(const StringPtr& connectionString, const PropertyObjectPtr& config) const;

    static PropertyObjectPtr CreateGeneralConfig();
    static void OverrideConfigProperties(PropertyObjectPtr& targetConfig, const PropertyObjectPtr& sourceConfig);

    DictPtr<IString, IDeviceInfo> discoverDevicesWithIpModification();
    std::pair<StringPtr, DeviceInfoPtr> populateDiscoveredDevice(const discovery::MdnsDiscoveredDevice& discoveredDevice);
    void onCompleteCapabilities(const DevicePtr& device, const DeviceInfoPtr& discoveredDeviceInfo);

    bool authenticatedModulesOnly;
    ModuleAuthenticatorPtr moduleAuthenticator;
    DictPtr<IString, IString> moduleKeys;

    bool modulesLoaded;
    std::vector<std::string> paths;
    std::vector<ModuleLibrary> libraries;
    LoggerPtr logger;
    LoggerComponentPtr loggerComponent;

    std::vector<std::thread> pool;
    boost::asio::io_context ioContext;
    boost::asio::executor_work_guard<boost::asio::io_context::executor_type> work;

    DictPtr<IString, IDeviceInfo> availableDevicesGroup;
    std::mutex availableDevicesSearchSync;
    std::unordered_map<std::string, size_t> functionBlockCountMap;

    DictPtr<IString, IDeviceInfo> availableDevicesWithIpConfig;
    discovery::DiscoveryClient discoveryClient; // for discovering devices which has IP modification feature enabled
    ContextPtr context;

    std::chrono::time_point<std::chrono::steady_clock> lastScanTime;
    std::chrono::milliseconds rescanTimer;
};

END_NAMESPACE_OPENDAQ
