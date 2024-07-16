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
#include <opendaq/device_ptr.h>
#include <opendaq/instance.h>
#include <coreobjects/property_object_impl.h>
#include <opendaq/context_ptr.h>
#include <opendaq/module_manager_ptr.h>
#include <opendaq/function_block_ptr.h>

BEGIN_NAMESPACE_OPENDAQ

class InstanceImpl final : public ImplementationOfWeak<IInstance, ISerializable, IUpdatable>
{
public:
    explicit InstanceImpl(ContextPtr context, const StringPtr& localId);
    explicit InstanceImpl(IInstanceBuilder* instanceBuilder);
    ~InstanceImpl() override;

    // IInstance
    ErrCode INTERFACE_FUNC getContext(IContext** context) override;
    ErrCode INTERFACE_FUNC getModuleManager(IModuleManager** manager) override;

    ErrCode INTERFACE_FUNC getAvailableServerTypes(IDict** serverTypes) override;
    ErrCode INTERFACE_FUNC addServer(IString* serverTypeId, IPropertyObject* serverConfig, IServer** server) override;
    ErrCode INTERFACE_FUNC addStandardServers(IList** standardServers) override;
    ErrCode INTERFACE_FUNC removeServer(IServer* server) override;
    ErrCode INTERFACE_FUNC getServers(IList** instanceServers) override;

    ErrCode INTERFACE_FUNC getRootDevice(IDevice** currentRootDevice) override;
    ErrCode INTERFACE_FUNC setRootDevice(IString* connectionString, IPropertyObject* config = nullptr) override;

    // IDevice
    ErrCode INTERFACE_FUNC getInfo(IDeviceInfo** info) override;
    ErrCode INTERFACE_FUNC getDomain(IDeviceDomain** deviceDomain) override;

    ErrCode INTERFACE_FUNC getInputsOutputsFolder(IFolder** inputsOutputsFolder) override;

    ErrCode INTERFACE_FUNC getCustomComponents(IList** customComponents) override;

    ErrCode INTERFACE_FUNC getSignals(IList** signals, ISearchFilter* searchFilter = nullptr) override;
    ErrCode INTERFACE_FUNC getSignalsRecursive(IList** signals, ISearchFilter* searchFilter = nullptr) override;

    ErrCode INTERFACE_FUNC getAvailableDevices(IList** availableDevices) override;
    ErrCode INTERFACE_FUNC getAvailableDeviceTypes(IDict** deviceTypes) override;
    ErrCode INTERFACE_FUNC addDevice(IDevice** device, IString* connectionString, IPropertyObject* config) override;
    ErrCode INTERFACE_FUNC removeDevice(IDevice* device) override;
    ErrCode INTERFACE_FUNC getDevices(IList** devices, ISearchFilter* searchFilter = nullptr) override;
    ErrCode INTERFACE_FUNC createDefaultAddDeviceConfig(IPropertyObject** defaultConfig) override;

    ErrCode INTERFACE_FUNC getAvailableFunctionBlockTypes(IDict** functionBlockTypes) override;
    ErrCode INTERFACE_FUNC addFunctionBlock(IFunctionBlock** functionBlock, IString* typeId, IPropertyObject* config) override;
    ErrCode INTERFACE_FUNC removeFunctionBlock(IFunctionBlock* functionBlock) override;
    ErrCode INTERFACE_FUNC getFunctionBlocks(IList** functionBlocks, ISearchFilter* searchFilter = nullptr) override;

    ErrCode INTERFACE_FUNC getChannels(IList** channels, ISearchFilter* searchFilter = nullptr) override;
    ErrCode INTERFACE_FUNC getChannelsRecursive(IList** channels, ISearchFilter* searchFilter = nullptr) override;

    ErrCode INTERFACE_FUNC saveConfiguration(IString** configuration) override;
    ErrCode INTERFACE_FUNC loadConfiguration(IString* configuration) override;

    ErrCode INTERFACE_FUNC addStreaming(IStreaming** streaming, IString* connectionString, IPropertyObject* config = nullptr) override;

    // IDeviceDomain
    ErrCode INTERFACE_FUNC getTicksSinceOrigin(uint64_t* ticks) override;

    // IComponent

    ErrCode INTERFACE_FUNC getLocalId(IString** localId) override;
    ErrCode INTERFACE_FUNC getGlobalId(IString** globalId) override;
    ErrCode INTERFACE_FUNC getActive(Bool* active) override;
    ErrCode INTERFACE_FUNC setActive(Bool active) override;
    ErrCode INTERFACE_FUNC getParent(IComponent** parent) override;
    ErrCode INTERFACE_FUNC getName(IString** name) override;
    ErrCode INTERFACE_FUNC setName(IString* name) override;
    ErrCode INTERFACE_FUNC getDescription(IString** description) override;
    ErrCode INTERFACE_FUNC setDescription(IString* description) override;
    ErrCode INTERFACE_FUNC getTags(ITags** tags) override;
    ErrCode INTERFACE_FUNC getVisible(Bool* visible) override;
    ErrCode INTERFACE_FUNC setVisible(Bool visible) override;
    ErrCode INTERFACE_FUNC getLockedAttributes(IList** attributes) override;
    ErrCode INTERFACE_FUNC getOnComponentCoreEvent(IEvent** event) override;
    ErrCode INTERFACE_FUNC getStatusContainer(IComponentStatusContainer** statusContainer) override;
    ErrCode INTERFACE_FUNC findComponent(IString* id, IComponent** outComponent) override;

    // IFolder

    ErrCode INTERFACE_FUNC getItems(IList** items, ISearchFilter* searchFilter) override;
    ErrCode INTERFACE_FUNC getItem(IString* localId, IComponent** item) override;
    ErrCode INTERFACE_FUNC isEmpty(Bool* empty) override;
    ErrCode INTERFACE_FUNC hasItem(IString* localId, Bool* value) override;

    // IPropertyObject
    ErrCode INTERFACE_FUNC getClassName(IString** className) override;

    ErrCode INTERFACE_FUNC setPropertyValue(IString* propertyName, IBaseObject* value) override;
    ErrCode INTERFACE_FUNC getPropertyValue(IString* propertyName, IBaseObject** value) override;

    ErrCode INTERFACE_FUNC getPropertySelectionValue(IString* propertyName, IBaseObject** value) override;
    ErrCode INTERFACE_FUNC clearPropertyValue(IString* propertyName) override;

    ErrCode INTERFACE_FUNC getProperty(IString* propertyName, IProperty** property) override;
    ErrCode INTERFACE_FUNC addProperty(IProperty* property) override;
    ErrCode INTERFACE_FUNC removeProperty(IString* propertyName) override;
    ErrCode INTERFACE_FUNC hasProperty(IString* propertyName, Bool* hasProperty) override;

    ErrCode INTERFACE_FUNC getVisibleProperties(IList** properties) override;
    ErrCode INTERFACE_FUNC getAllProperties(IList** properties) override;

    ErrCode INTERFACE_FUNC setPropertyOrder(IList* orderedPropertyNames) override;

    ErrCode INTERFACE_FUNC getOnPropertyValueWrite(IString* propertyName, IEvent** event) override;
    ErrCode INTERFACE_FUNC getOnPropertyValueRead(IString* propertyName, IEvent** event) override;

    ErrCode INTERFACE_FUNC beginUpdate() override;
    ErrCode INTERFACE_FUNC endUpdate() override;
    ErrCode INTERFACE_FUNC getOnEndUpdate(IEvent** event) override;
    ErrCode INTERFACE_FUNC getPermissionManager(IPermissionManager** permissionManager) override;

    // ISerializable
    ErrCode INTERFACE_FUNC serialize(ISerializer* serializer) override;
    ErrCode INTERFACE_FUNC getSerializeId(ConstCharPtr* id) const override;

    static ConstCharPtr SerializeId();
    static ErrCode Deserialize(ISerializedObject* serialized, IBaseObject* context, IBaseObject** obj);

    // IUpdatable
    ErrCode INTERFACE_FUNC update(ISerializedObject* obj) override;
    ErrCode INTERFACE_FUNC serializeForUpdate(ISerializer* serializer) override;
    ErrCode INTERFACE_FUNC updateEnded() override;

private:
    DevicePtr rootDevice;
    ContextPtr context;
    ModuleManagerPtr moduleManager;
    LoggerComponentPtr loggerComponent;

    std::mutex configSync;
    std::vector<ServerPtr> servers;

    bool rootDeviceSet;

    void stopServers();
    DevicePtr createDevice(const StringPtr& connectionString, const PropertyObjectPtr& config = nullptr);

    template<class F>
    void forEachComponent(const ComponentPtr& component, F&& callback);

    static StringPtr convertIfOldIdProtocol(const StringPtr& id);
};

END_NAMESPACE_OPENDAQ
