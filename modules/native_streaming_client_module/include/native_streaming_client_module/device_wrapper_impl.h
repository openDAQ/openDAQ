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

#include <native_streaming_client_module/common.h>

#include <opendaq/device_ptr.h>
#include <coretypes/impl.h>
#include <opendaq/context_ptr.h>

// All required Interfaces are included from this header
#include <config_protocol/config_client_device_impl.h>

BEGIN_NAMESPACE_OPENDAQ_NATIVE_STREAMING_CLIENT_MODULE

/// redirects all interface calls to wrapped root device obtained from config protocol

class DeviceWrapperImpl : public ImplementationOfWeak<IDevice,
                                                      IDeviceDomain,
                                                      ISerializable,
                                                      IOwnable,
                                                      IFreezable,
                                                      IUpdatable,
                                                      IPropertyObjectProtected,
                                                      IPropertyObjectInternal,
                                                      IRemovable,
                                                      IComponentPrivate,
                                                      IDeserializeComponent,
                                                      IDevicePrivate,
                                                      IConfigClientObject>
{
public:
    explicit DeviceWrapperImpl();
    ~DeviceWrapperImpl() override;

    // IDevice
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
    ErrCode INTERFACE_FUNC getAvailableFunctionBlockTypes(IDict** functionBlockTypes) override;
    ErrCode INTERFACE_FUNC addFunctionBlock(IFunctionBlock** functionBlock, IString* typeId, IPropertyObject* config) override;
    ErrCode INTERFACE_FUNC removeFunctionBlock(IFunctionBlock* functionBlock) override;
    ErrCode INTERFACE_FUNC getFunctionBlocks(IList** functionBlocks, ISearchFilter* searchFilter = nullptr) override;
    ErrCode INTERFACE_FUNC getChannels(IList** channels, ISearchFilter* searchFilter = nullptr) override;
    ErrCode INTERFACE_FUNC getChannelsRecursive(IList** channels, ISearchFilter* searchFilter = nullptr) override;
    ErrCode INTERFACE_FUNC saveConfiguration(IString** configuration) override;
    ErrCode INTERFACE_FUNC loadConfiguration(IString* configuration) override;

    // IDeviceDomain
    ErrCode INTERFACE_FUNC getTickResolution(IRatio** resolution) override;
    ErrCode INTERFACE_FUNC getTicksSinceOrigin(uint64_t* ticks) override;
    ErrCode INTERFACE_FUNC getOrigin(IString** origin) override;
    ErrCode INTERFACE_FUNC getUnit(IUnit** unit) override;

    // IComponent
    ErrCode INTERFACE_FUNC getContext(IContext** context) override;
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

    // ISerializable
    ErrCode INTERFACE_FUNC serialize(ISerializer* serializer) override;
    ErrCode INTERFACE_FUNC getSerializeId(ConstCharPtr* id) const override;

    // IOwnable
    ErrCode INTERFACE_FUNC setOwner(IPropertyObject* owner) override;

    // IFreezable
    ErrCode INTERFACE_FUNC freeze() override;
    ErrCode INTERFACE_FUNC isFrozen(Bool* isFrozen) const override;

    // IUpdatable
    ErrCode INTERFACE_FUNC update(ISerializedObject* update) override;
    ErrCode INTERFACE_FUNC serializeForUpdate(ISerializer* serializer) override;

    // IPropertyObjectProtected
    ErrCode INTERFACE_FUNC setProtectedPropertyValue(IString* propertyName, IBaseObject* value) override;
    ErrCode INTERFACE_FUNC clearProtectedPropertyValue(IString* propertyName) override;

    // IPropertyObjectInternal
    ErrCode INTERFACE_FUNC checkForReferences(IProperty* property, Bool* isReferenced) override;
    ErrCode INTERFACE_FUNC enableCoreEventTrigger() override;
    ErrCode INTERFACE_FUNC disableCoreEventTrigger() override;
    ErrCode INTERFACE_FUNC getCoreEventTrigger(IProcedure** trigger) override;
    ErrCode INTERFACE_FUNC setCoreEventTrigger(IProcedure* trigger) override;
    ErrCode INTERFACE_FUNC clone(IPropertyObject** cloned) override;
    ErrCode INTERFACE_FUNC setPath(IString* path) override;

    // IRemovable
    ErrCode INTERFACE_FUNC remove() override;
    ErrCode INTERFACE_FUNC isRemoved(Bool* removed) override;

    // IComponentPrivate
    ErrCode INTERFACE_FUNC lockAttributes(IList* attributes) override;
    ErrCode INTERFACE_FUNC lockAllAttributes() override;
    ErrCode INTERFACE_FUNC unlockAttributes(IList* attributes) override;
    ErrCode INTERFACE_FUNC unlockAllAttributes() override;
    ErrCode INTERFACE_FUNC triggerComponentCoreEvent(ICoreEventArgs* args) override;

    // IDeserializeComponent
    ErrCode INTERFACE_FUNC deserializeValues(ISerializedObject* serializedObject, IBaseObject* context, IFunction* callbackFactory) override;
    ErrCode INTERFACE_FUNC complete() override;
    ErrCode INTERFACE_FUNC getDeserializedParameter(IString* parameter, IBaseObject** value) override;

    // IDevicePrivate
    ErrCode INTERFACE_FUNC addStreamingOption(IStreamingInfo* info) override;
    ErrCode INTERFACE_FUNC removeStreamingOption(IString* protocolId) override;
    ErrCode INTERFACE_FUNC getStreamingOptions(IList** streamingOptions) override;

    // IConfigClientObject
    ErrCode INTERFACE_FUNC getRemoteGlobalId(IString** remoteGlobalId) override;

protected:
    DevicePtr wrappedDevice;
};

END_NAMESPACE_OPENDAQ_NATIVE_STREAMING_CLIENT_MODULE
