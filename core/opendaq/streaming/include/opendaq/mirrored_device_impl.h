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

#include <opendaq/mirrored_device_config_ptr.h>
#include <opendaq/device_impl.h>
#include <opendaq/streaming_ptr.h>
#include <opendaq/streaming_private.h>

BEGIN_NAMESPACE_OPENDAQ

template <typename... Interfaces>
class MirroredDeviceBase;

using MirroredDevice = MirroredDeviceBase<>;

template <typename... Interfaces>
class MirroredDeviceBase : public DeviceBase<IMirroredDeviceConfig, Interfaces...>
{
public:
    using Super = DeviceBase<IMirroredDeviceConfig, Interfaces...>;
    using Self = MirroredDeviceBase<Interfaces...>;

    explicit MirroredDeviceBase(const ContextPtr& ctx,
                                const ComponentPtr& parent,
                                const StringPtr& localId,
                                const StringPtr& className = nullptr,
                                const StringPtr& name = nullptr);
    ~MirroredDeviceBase() override;

    // IMirroredDevice
    ErrCode INTERFACE_FUNC getStreamingSources(IList** streamingSources) override;

    // IMirroredDeviceConfig
    ErrCode INTERFACE_FUNC addStreamingSource(IStreaming* streamingSource) override;
    ErrCode INTERFACE_FUNC removeStreamingSource(IString* streamingConnectionString) override;

    // IComponentPrivate
    ErrCode INTERFACE_FUNC setComponentConfig(IPropertyObject* config) override;

protected:
    void removed() override;

    StreamingPtr onAddStreaming(const StringPtr& connectionString, const PropertyObjectPtr& config) override;

    void serializeCustomObjectValues(const SerializerPtr& serializer, bool forUpdate) override;
    void deserializeCustomObjectValues(const SerializedObjectPtr& serializedObject,
                                       const BaseObjectPtr& context,
                                       const FunctionPtr& factoryCallback) override;

    virtual bool isAddedToLocalComponentTree();

private:
    void coreEventCallback(ComponentPtr& sender, CoreEventArgsPtr& eventArgs);
    void componentAdded(const ComponentPtr& sender, const CoreEventArgsPtr& eventArgs);
    void componentUpdated(const ComponentPtr& sender, const CoreEventArgsPtr& eventArgs);

    void enableStreamingForAddedComponent(const ComponentPtr& addedComponent);
    void enableStreamingForUpdatedComponent(const ComponentPtr& updatedComponent);
    void tryAddSignalToStreaming(const SignalPtr& signal, const StreamingPtr& streaming);
    void setSignalActiveStreamingSource(const SignalPtr& signal, const StreamingPtr& streaming);
    void completeStreamingConnections(const ComponentPtr& component);

    static ListPtr<IMirroredDeviceConfig> getAllDevicesRecursively(const MirroredDeviceConfigPtr& device);

    AddressInfoPtr findStreamingAddress(const ListPtr<IAddressInfo>& availableAddresses,
                                        const AddressInfoPtr& deviceConnectionAddress,
                                        const StringPtr& primaryAddressType);
    static AddressInfoPtr getDeviceConnectionAddress(const DevicePtr& device);
    void configureStreamings(const MirroredDeviceConfigPtr& topDevice);
    void attachStreamingsToDevice(const MirroredDeviceConfigPtr& device,
                                  const PropertyObjectPtr& generalConfig,
                                  const PropertyObjectPtr& addDeviceConfig,
                                  const AddressInfoPtr& deviceConnectionAddress);

    std::vector<StreamingPtr> streamingSources;
    bool minHopsStreamingHeuristicEnabled{false};
    DictPtr<IString, IPropertyObject> manuallyAddedStreamings; // connection string and config object
};

template <typename... Interfaces>
MirroredDeviceBase<Interfaces...>::MirroredDeviceBase(const ContextPtr& ctx,
                                                      const ComponentPtr& parent,
                                                      const StringPtr& localId,
                                                      const StringPtr& className,
                                                      const StringPtr& name)
    : Super(ctx, parent, localId, className, name)
    , manuallyAddedStreamings(Dict<IString, IPropertyObject>())
{
    this->context.getOnCoreEvent() += event(this, &MirroredDeviceBase::coreEventCallback);
}

template <typename... Interfaces>
MirroredDeviceBase<Interfaces...>::~MirroredDeviceBase()
{
    this->context.getOnCoreEvent() -= event(this, &MirroredDeviceBase::coreEventCallback);
}

template <typename... Interfaces>
ErrCode MirroredDeviceBase<Interfaces...>::getStreamingSources(IList** streamingSources)
{
    OPENDAQ_PARAM_NOT_NULL(streamingSources);

    auto streamingSourcesPtr = List<IStreaming>();

    auto lock = this->getRecursiveConfigLock();
    for (const auto& streaming : this->streamingSources)
    {
        streamingSourcesPtr.pushBack(streaming);
    }

    *streamingSources = streamingSourcesPtr.detach();
    return OPENDAQ_SUCCESS;
}

template <typename... Interfaces>
ErrCode MirroredDeviceBase<Interfaces...>::addStreamingSource(IStreaming* streamingSource)
{
    OPENDAQ_PARAM_NOT_NULL(streamingSource);

    const auto streamingPtr = StreamingPtr::Borrow(streamingSource);
    const auto connectionString = streamingPtr.getConnectionString();

    auto lock = this->getRecursiveConfigLock();

    auto it = std::find_if(streamingSources.begin(),
                           streamingSources.end(),
                           [&connectionString](const StreamingPtr& item)
                           {
                               return connectionString == item.getConnectionString();
                           });

    if (it != streamingSources.end())
    {
        return DAQ_MAKE_ERROR_INFO(
            OPENDAQ_ERR_DUPLICATEITEM,
            fmt::format(
                R"(Device with global Id "{}" already has streaming source "{}" )",
                this->globalId,
                connectionString
            )
        );
    }

    streamingSources.push_back(streamingPtr);

    auto errCode =
        daqTry(
            [this, streamingPtr] {
                this->connectionStatusContainer.addStreamingConnectionStatus(
                    streamingPtr.getConnectionString(),
                    streamingPtr.getConnectionStatus(),
                    streamingPtr
                );
            }
        );
    if (OPENDAQ_FAILED(errCode))
        return errCode;

    const auto thisPtr = this->template borrowPtr<DevicePtr>();
    errCode = streamingPtr.template asPtr<IStreamingPrivate>()->setOwnerDevice(thisPtr);
    if (OPENDAQ_FAILED(errCode))
        return errCode;

    return OPENDAQ_SUCCESS;
}

template <typename... Interfaces>
ErrCode MirroredDeviceBase<Interfaces...>::removeStreamingSource(IString* streamingConnectionString)
{
    OPENDAQ_PARAM_NOT_NULL(streamingConnectionString);

    auto lock = this->getRecursiveConfigLock();

    const auto streamingConnectionStringPtr = StringPtr::Borrow(streamingConnectionString);

    auto it = std::find_if(streamingSources.begin(),
                           streamingSources.end(),
                           [&streamingConnectionStringPtr](const StreamingPtr& item)
                           {
                               return streamingConnectionStringPtr == item.getConnectionString();
                           });

    if (it == streamingSources.end())
    {
        return DAQ_MAKE_ERROR_INFO(
            OPENDAQ_ERR_NOTFOUND,
            fmt::format(
                R"(Device with global Id "{}" does not have streaming source "{}" )",
                this->globalId,
                streamingConnectionStringPtr
            )
        );
    }

    auto errCode =
        daqTry(
            [this, streamingConnectionStringPtr] {
                this->connectionStatusContainer.removeStreamingConnectionStatus(streamingConnectionStringPtr);
            }
        );
    if (OPENDAQ_FAILED(errCode))
        return errCode;

    errCode = (*it).template asPtr<IStreamingPrivate>()->setOwnerDevice(nullptr);
    if (OPENDAQ_FAILED(errCode))
        return errCode;

    if (manuallyAddedStreamings.hasKey(streamingConnectionStringPtr))
        manuallyAddedStreamings.remove(streamingConnectionStringPtr);

    streamingSources.erase(it);

    return OPENDAQ_SUCCESS;
}

template <typename... Interfaces>
void MirroredDeviceBase<Interfaces...>::removed()
{
    for (const auto& streamingSource : streamingSources)
    {
        this->connectionStatusContainer.removeStreamingConnectionStatus(streamingSource.getConnectionString());
        checkErrorInfo(streamingSource.template asPtr<IStreamingPrivate>()->setOwnerDevice(nullptr));
    }

    // disconnects all streaming connections
    streamingSources.clear();

    Super::removed();
}

template <typename... Interfaces>
ErrCode MirroredDeviceBase<Interfaces...>::setComponentConfig(IPropertyObject* config)
{
    if (!this->isAddedToLocalComponentTree())
    {
        return DAQ_MAKE_ERROR_INFO(
            OPENDAQ_ERR_INVALID_OPERATION,
            "Cannot set config for device added to remote component tree"
        );
    }
    ErrCode errCode = Super::setComponentConfig(config);
    if (OPENDAQ_FAILED(errCode))
        return errCode;

    if (this->componentConfig.assigned() && this->componentConfig.hasProperty("General"))
    {
        PropertyObjectPtr generalConfig = this->componentConfig.getPropertyValue("General");
        if (generalConfig.getPropertyValue("StreamingConnectionHeuristic") == 1)
            minHopsStreamingHeuristicEnabled = true;
    }

    return errCode;
}

template <typename... Interfaces>
StreamingPtr MirroredDeviceBase<Interfaces...>::onAddStreaming(const StringPtr& connectionString, const PropertyObjectPtr& config)
{
    auto lock = this->getRecursiveConfigLock();

    auto it = std::find_if(streamingSources.begin(),
                           streamingSources.end(),
                           [&connectionString](const StreamingPtr& item)
                           {
                               return connectionString == item.getConnectionString();
                           });

    if (it != streamingSources.end())
    {
        DAQ_THROW_EXCEPTION(DuplicateItemException,
                            R"(Device with global Id "{}" already has streaming source "{}" )",
                            this->globalId,
                            connectionString
        );
    }

    const ModuleManagerUtilsPtr managerUtils = this->context.getModuleManager().template asPtr<IModuleManagerUtils>();
    auto streamingPtr = managerUtils.createStreaming(connectionString, config);
    streamingSources.push_back(streamingPtr);

    manuallyAddedStreamings.set(connectionString, config);

    this->connectionStatusContainer.addStreamingConnectionStatus(streamingPtr.getConnectionString(),
                                                                 streamingPtr.getConnectionStatus(),
                                                                 streamingPtr);
    const auto thisPtr = this->template borrowPtr<DevicePtr>();
    checkErrorInfo(streamingPtr.template asPtr<IStreamingPrivate>()->setOwnerDevice(thisPtr));

    return streamingPtr;
}

template <typename... Interfaces>
void MirroredDeviceBase<Interfaces...>::coreEventCallback(ComponentPtr& sender, CoreEventArgsPtr& eventArgs)
{
    if (!this->isAddedToLocalComponentTree())
        return;

    switch (static_cast<CoreEventId>(eventArgs.getEventId()))
    {
        case CoreEventId::ComponentAdded:
            componentAdded(sender, eventArgs);
            break;
        case CoreEventId::ComponentUpdateEnd:
            componentUpdated(sender, eventArgs);
            break;
        default:
            break;
    }
}

template <typename... Interfaces>
void MirroredDeviceBase<Interfaces...>::componentAdded(const ComponentPtr& sender, const CoreEventArgsPtr& eventArgs)
{
    auto deviceSelf = this->template borrowPtr<DevicePtr>();

    ComponentPtr addedComponent = eventArgs.getParameters().get("Component");

    auto deviceSelfGlobalId = deviceSelf.getGlobalId().toStdString();
    auto addedComponentGlobalId = addedComponent.getGlobalId().toStdString();

    if (!IdsParser::isNestedComponentId(deviceSelfGlobalId, addedComponentGlobalId) &&
        addedComponentGlobalId != deviceSelfGlobalId)
        return;

    DAQLOGF_I(this->loggerComponent, "Added Component: {};", addedComponentGlobalId);

    if (addedComponentGlobalId == deviceSelfGlobalId)
    {
        configureStreamings(deviceSelf);
        return;
    }

    completeStreamingConnections(addedComponent);
    if (auto topAddedDevice = addedComponent.asPtrOrNull<IDevice>(); topAddedDevice.assigned() && minHopsStreamingHeuristicEnabled)
    {
        // TODO enable streaming separately for top added and each of the nested devices - from bottom to top
        //        ListPtr<IMirroredDeviceConfig> addedDevices = topAddedDevice.getDevices(search::Recursive(search::Any()));
        //        addedDevices.pushFront(topAddedDevice);
        //        for (const auto& addedDevice : addedDevices)
        //            enableStreamingForAddedComponent(addedDevices);
        enableStreamingForAddedComponent(addedComponent);
    }
    else
    {
        enableStreamingForAddedComponent(addedComponent);
    }
}

template <typename... Interfaces>
void MirroredDeviceBase<Interfaces...>::componentUpdated(const ComponentPtr& sender, const CoreEventArgsPtr& eventArgs)
{
    auto deviceSelf = this->template borrowPtr<DevicePtr>();

    ComponentPtr updatedComponent = sender;

    auto deviceSelfGlobalId = deviceSelf.getGlobalId().toStdString();
    auto updatedComponentGlobalId = updatedComponent.getGlobalId().toStdString();
    if (deviceSelfGlobalId == updatedComponentGlobalId ||
        IdsParser::isNestedComponentId(deviceSelfGlobalId, updatedComponentGlobalId) ||
        IdsParser::isNestedComponentId(updatedComponentGlobalId, deviceSelfGlobalId))
    {
        DAQLOGF_I(this->loggerComponent, "Updated Component: {};", updatedComponentGlobalId);

        if (deviceSelfGlobalId == updatedComponentGlobalId ||
            IdsParser::isNestedComponentId(updatedComponentGlobalId, deviceSelfGlobalId))
        {
            completeStreamingConnections(deviceSelf);
            enableStreamingForUpdatedComponent(deviceSelf);
        }
        else
        {
            completeStreamingConnections(updatedComponent);
            enableStreamingForUpdatedComponent(updatedComponent);
        }
    }
}

template <typename... Interfaces>
void MirroredDeviceBase<Interfaces...>::completeStreamingConnections(const ComponentPtr& component)
{
    auto deviceSelf = this->template borrowPtr<DevicePtr>();
    if (!minHopsStreamingHeuristicEnabled)
        return;

    const auto deviceHasStreamingCaps = [](const DevicePtr& dev)
    {
        for (const auto& cap : dev.getInfo().getServerCapabilities())
        {
            if ((cap.getProtocolType() == ProtocolType::Streaming || cap.getProtocolType() == ProtocolType::ConfigurationAndStreaming) &&
                cap.getConnectionString().assigned() &&
                cap.getConnectionString() != "")
                return true;
        }
        return false;
    };

    if (auto topDevice = component.asPtrOrNull<IMirroredDeviceConfig>(); topDevice.assigned())
    {
        ListPtr<IMirroredDeviceConfig> devices = topDevice.getDevices(search::Recursive(search::Any()));
        if (topDevice != deviceSelf)
            devices.pushFront(topDevice);

        for (const auto& device : devices)
        {
            // Try establish streaming connections for devices that have the required capabilities but no sources yet
            // assuming these are newly added devices
            if (device.getStreamingSources().getCount() == 0 && deviceHasStreamingCaps(device))
            {
               // TODO try to establish new streaming connections using server caps
            }
        }
    }
}

template <typename... Interfaces>
void MirroredDeviceBase<Interfaces...>::enableStreamingForAddedComponent(const ComponentPtr& addedComponent)
{
    auto deviceSelf = this->template borrowPtr<DevicePtr>();

    auto isAncestorComponent = Function(
        [addedComponentId = addedComponent.getGlobalId()](const ComponentPtr& comp)
        {
            return IdsParser::isNestedComponentId(comp.getGlobalId(), addedComponentId);
        }
    );
    auto ancestorDevices = List<IDevice>();
    if (minHopsStreamingHeuristicEnabled) // skip nested devices' streamings if MinHops disabled
        ancestorDevices = deviceSelf.getDevices(search::Recursive(search::Custom(isAncestorComponent)));
    ancestorDevices.pushFront(deviceSelf);

    // collect all relevant streaming sources for the component by retrieving sources from all its ancestor devices
    auto allStreamingSources = List<IStreaming>();
    StreamingPtr activeStreamingSource;

    for (const auto& ancestorDevice : ancestorDevices)
    {
        if (auto mirroredDevice = ancestorDevice.asPtrOrNull<IMirroredDevice>(); mirroredDevice.assigned())
        {
            auto streamingSources = mirroredDevice.getStreamingSources();
            for (const auto& streaming : streamingSources)
               allStreamingSources.pushBack(streaming);

            // streaming sources were created and ordered by priority on the device connection, cache the highest-priority
            // source from the deepest in-tree ancestor or top device if no MinHops enabled to be active for signals of new component
            if (!streamingSources.empty())
               activeStreamingSource = streamingSources[0];
        }
    }

    if (!activeStreamingSource.assigned() || allStreamingSources.empty())
        return;

    auto setupStreamingForSignal = [this, allStreamingSources, activeStreamingSource](const SignalPtr& signal)
    {
        for (const auto& streaming : allStreamingSources)
            tryAddSignalToStreaming(signal, streaming);
        setSignalActiveStreamingSource(signal, activeStreamingSource);
    };

    // setup streaming sources for all signals of the new component
    if (addedComponent.supportsInterface<ISignal>())
    {
        setupStreamingForSignal(addedComponent.asPtr<ISignal>());
    }
    else if (addedComponent.supportsInterface<IFolder>())
    {
        ListPtr<ISignal> nestedSignals = addedComponent.asPtr<IFolder>().getItems(search::Recursive(search::InterfaceId(ISignal::Id)));
        for (const auto& nestedSignal : nestedSignals)
            setupStreamingForSignal(nestedSignal);
    }
}

template <typename... Interfaces>
void MirroredDeviceBase<Interfaces...>::enableStreamingForUpdatedComponent(const ComponentPtr& updatedComponent)
{
    auto deviceSelf = this->template borrowPtr<DevicePtr>();

    // assign streaming sources for all signals which do not have any, assuming these are newly added signals
    if (updatedComponent.supportsInterface<IMirroredSignalConfig>())
    {
        if (updatedComponent.asPtr<IMirroredSignalConfig>().getStreamingSources().getCount() == 0)
            enableStreamingForAddedComponent(updatedComponent);
    }
    else if (updatedComponent.supportsInterface<IFolder>())
    {
        ListPtr<IMirroredSignalConfig> nestedSignals =
            updatedComponent.asPtr<IFolder>().getItems(search::Recursive(search::InterfaceId(ISignal::Id)));
        for (const auto& nestedSignal : nestedSignals)
            if (nestedSignal.getStreamingSources().getCount() == 0)
               enableStreamingForAddedComponent(nestedSignal);
    }
}

template <typename... Interfaces>
void MirroredDeviceBase<Interfaces...>::tryAddSignalToStreaming(const SignalPtr& signal, const StreamingPtr& streaming)
{
    if (!signal.getPublic())
        return;

    ErrCode errCode =
        daqTry([&]()
        {
           streaming.addSignals({signal});
        });
    if (OPENDAQ_SUCCEEDED(errCode))
    {
        DAQLOGF_I(this->loggerComponent, "Signal {} added to streaming {};", signal.getGlobalId(), streaming.getConnectionString());
    }
    else if (errCode != OPENDAQ_ERR_DUPLICATEITEM)
    {
        checkErrorInfo(errCode);
    }
}

template <typename... Interfaces>
void MirroredDeviceBase<Interfaces...>::setSignalActiveStreamingSource(const SignalPtr& signal, const StreamingPtr& streaming)
{
    if (!signal.getPublic())
        return;

    auto mirroredSignalConfigPtr = signal.template asPtr<IMirroredSignalConfig>();
    mirroredSignalConfigPtr.setActiveStreamingSource(streaming.getConnectionString());
}

template <typename... Interfaces>
void MirroredDeviceBase<Interfaces...>::serializeCustomObjectValues(const SerializerPtr& serializer, bool forUpdate)
{
    Super::serializeCustomObjectValues(serializer, forUpdate);

    if (forUpdate)
    {
        if (manuallyAddedStreamings.getCount() > 0)
        {
            serializer.key("ManuallyAddedStreamingConnections");
            manuallyAddedStreamings.serialize(serializer);
        }
    }
}

template <typename... Interfaces>
void MirroredDeviceBase<Interfaces...>::deserializeCustomObjectValues(const SerializedObjectPtr& serializedObject,
                                                                      const BaseObjectPtr& context,
                                                                      const FunctionPtr& factoryCallback)
{
    Super::deserializeCustomObjectValues(serializedObject, context, factoryCallback);

    if (serializedObject.hasKey("ManuallyAddedStreamingConnections"))
    {
        DictPtr<IString, IPropertyObject> streamingConnectionsParams =
            serializedObject.readObject("ManuallyAddedStreamingConnections", context, factoryCallback);

        for (const auto& [connectionString, config] : streamingConnectionsParams)
        {
            try
            {
               onAddStreaming(connectionString, config);
            }
            catch (const DuplicateItemException& e)
            {
                DAQLOGF_D(this->loggerComponent, "Streaming connection {} already exists ({})", connectionString, e.what());
            }
            catch (const DaqException& e)
            {
                DAQLOGF_E(this->loggerComponent, "Failed to connect streaming {}: {}", connectionString, e.what());
            }
        }
    }
}

template <typename... Interfaces>
bool MirroredDeviceBase<Interfaces...>::isAddedToLocalComponentTree()
{
    return false;
}

template <typename... Interfaces>
void MirroredDeviceBase<Interfaces...>::configureStreamings(const MirroredDeviceConfigPtr& topDevice)
{
    // Get the address used for device connection
    const auto deviceConnectionAddress = getDeviceConnectionAddress(topDevice);

    auto addDeviceConfig = topDevice.asPtr<IComponentPrivate>().getComponentConfig();
    assert(addDeviceConfig.assigned());

    PropertyObjectPtr generalConfig = addDeviceConfig.getPropertyValue("General");

    const StringPtr streamingHeuristic = generalConfig.getPropertySelectionValue("StreamingConnectionHeuristic");
    const bool automaticallyConnectStreaming = generalConfig.getPropertyValue("AutomaticallyConnectStreaming");
    if (!automaticallyConnectStreaming)
        return;

    if (streamingHeuristic == "MinConnections")
    {
        attachStreamingsToDevice(topDevice, generalConfig, addDeviceConfig, deviceConnectionAddress);
    }
    else if (streamingHeuristic == "MinHops")
    {
        // The order of handling nested devices is important since we need to establish streaming connections
        // for the leaf devices first. The custom function is used to get the list of sub-devices
        // recursively, because using the recursive search filter does not guarantee the required order
        const auto allDevicesInTree = getAllDevicesRecursively(topDevice);
        for (const auto& device : allDevicesInTree)
        {
            attachStreamingsToDevice(device, generalConfig, addDeviceConfig, deviceConnectionAddress);
        }
    }
}

template <typename... Interfaces>
void MirroredDeviceBase<Interfaces...>::attachStreamingsToDevice(const MirroredDeviceConfigPtr& device,
                                                                 const PropertyObjectPtr& generalConfig,
                                                                 const PropertyObjectPtr& addDeviceConfig,
                                                                 const AddressInfoPtr& deviceConnectionAddress)
{
    auto deviceInfo = device.getInfo();
    auto signals = device.getSignals(search::Recursive(search::Any()));

    const ListPtr<IString> prioritizedStreamingProtocols = generalConfig.getPropertyValue("PrioritizedStreamingProtocols");
    const ListPtr<IString> allowedStreamingProtocols = generalConfig.getPropertyValue("AllowedStreamingProtocols");
    const StringPtr primaryAddessType = generalConfig.getPropertyValue("PrimaryAddressType");

    // protocol Id as a key, protocol priority as a value
    std::unordered_set<std::string> allowedProtocolsSet;
    for (SizeT index = 0; index < allowedStreamingProtocols.getCount(); ++index)
    {
        allowedProtocolsSet.insert(allowedStreamingProtocols[index]);
    }

    // protocol Id as a key, protocol priority as a value
    std::map<StringPtr, SizeT> prioritizedProtocolsMap;
    for (SizeT index = 0; index < prioritizedStreamingProtocols.getCount(); ++index)
    {
        prioritizedProtocolsMap.insert({prioritizedStreamingProtocols[index], index});
    }

    // protocol priority as a key, streaming source as a value
    std::map<SizeT, StreamingPtr> prioritizedStreamingSourcesMap;

    // connect via all allowed streaming protocols
    for (const auto& cap : device.getInfo().getServerCapabilities())
    {
        DAQLOGF_D(this->loggerComponent, "Device {} has capability: name [{}] id [{}] string [{}] prefix [{}]",
                  device.getGlobalId(),
                  cap.getProtocolName(),
                  cap.getProtocolId(),
                  cap.getConnectionString(),
                  cap.getPrefix());

        const StringPtr protocolId = cap.getPropertyValue("protocolId");
        if (cap.getProtocolType() != ProtocolType::Streaming)
            continue;

        if (!allowedProtocolsSet.empty() && !allowedProtocolsSet.count(protocolId))
            continue;

        const auto protocolIt = prioritizedProtocolsMap.find(protocolId);
        if (protocolIt == prioritizedProtocolsMap.end())
            continue;
        const SizeT protocolPriority = protocolIt->second;

        StreamingPtr streaming;
        const auto streamingAddress = findStreamingAddress(cap.getAddressInfo(), deviceConnectionAddress, primaryAddessType);
        StringPtr connectionString = streamingAddress.assigned() ? streamingAddress.getConnectionString() : cap.getConnectionString();

        if (!connectionString.assigned())
            continue;

        wrapHandlerReturn(this, &MirroredDeviceBase::onAddStreaming, streaming, connectionString, addDeviceConfig);
        if (!streaming.assigned())
            continue;

        prioritizedStreamingSourcesMap.insert_or_assign(protocolPriority, streaming);
    }

    // add streaming sources ordered by protocol priority
    for (const auto& [_, streaming] : prioritizedStreamingSourcesMap)
    {
        device.addStreamingSource(streaming);
        DAQLOGF_I(this->loggerComponent, "Device {} added streaming connection {}", device.getGlobalId(), streaming.getConnectionString());

        streaming.addSignals(signals);
        streaming.setActive(true);
    }

    // set the streaming source with the highest priority as active for device signals
    if (!prioritizedStreamingSourcesMap.empty())
    {
        for (const auto& signal : signals)
        {
            if (!signal.getPublic())
                continue;

            MirroredSignalConfigPtr mirroredSignalConfigPtr = signal.template asPtr<IMirroredSignalConfig>();
            if (!mirroredSignalConfigPtr.getActiveStreamingSource().assigned())
            {
                auto signalStreamingSources = mirroredSignalConfigPtr.getStreamingSources();
                for (const auto& [_, streaming] : prioritizedStreamingSourcesMap)
                {
                    auto connectionString = streaming.getConnectionString();

                    auto it = std::find(signalStreamingSources.begin(), signalStreamingSources.end(), connectionString);
                    if (it != signalStreamingSources.end())
                    {
                        mirroredSignalConfigPtr.setActiveStreamingSource(connectionString);
                        break;
                    }
                }
            }
        }
    }
}

template <typename... Interfaces>
AddressInfoPtr MirroredDeviceBase<Interfaces...>::findStreamingAddress(const ListPtr<IAddressInfo>& availableAddresses,
                                                                       const AddressInfoPtr& deviceConnectionAddress,
                                                                       const StringPtr& primaryAddressType)
{
    if (primaryAddressType == "IPv4" || primaryAddressType == "IPv6")
    {
        // Attempt to reuse the address of device connection if it meets type constraints
        if (deviceConnectionAddress.assigned() && deviceConnectionAddress.getType() == primaryAddressType)
        {
            for (const auto& addressInfo : availableAddresses)
                if (addressInfo.getAddress() == deviceConnectionAddress.getAddress())
                    return addressInfo;
        }

        // If the device connection address is unavailable for streaming, search for any address matching type constraints
        for (const auto& addressInfo : availableAddresses)
        {
            if (addressInfo.getType() == primaryAddressType)
                return addressInfo;
        }
        DAQLOGF_W(this->loggerComponent, "Server streaming capability does not provide any addresses of primary {} type", primaryAddressType);
    }

    // Attempt to reuse the address of device connection
    for (const auto& addressInfo : availableAddresses)
    {
        if (addressInfo.getAddress() == deviceConnectionAddress.getAddress())
            return addressInfo;
    }

    return nullptr;
}

template <typename... Interfaces>
AddressInfoPtr MirroredDeviceBase<Interfaces...>::getDeviceConnectionAddress(const DevicePtr& device)
{
    const auto info = device.getInfo();
    const auto configConnection = info.getConfigurationConnectionInfo();
    const auto deviceInfoConnectionString = info.getConnectionString();

    if (!configConnection.assigned())
        return nullptr;

    const auto deviceConnectionString =
        (deviceInfoConnectionString.assigned() && deviceInfoConnectionString.getLength())
            ? deviceInfoConnectionString
            : configConnection.getConnectionString();

    for (const auto& addressInfo : configConnection.getAddressInfo())
    {
        if (deviceConnectionString == addressInfo.getConnectionString())
            return addressInfo;
    }

    return nullptr;
}

template <typename... Interfaces>
ListPtr<IMirroredDeviceConfig> MirroredDeviceBase<Interfaces...>::getAllDevicesRecursively(const MirroredDeviceConfigPtr& device)
{
    auto result = List<IMirroredDeviceConfig>();

    const auto childDevices = device.getDevices();
    for (const auto& childDevice : childDevices)
    {
        auto subDevices = getAllDevicesRecursively(childDevice);
        for (const auto& subDevice : subDevices)
        {
            result.pushBack(subDevice);
        }
    }

    result.pushBack(device);

    return result;
}

END_NAMESPACE_OPENDAQ
