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
#include <map>
#include <unordered_set>
#include <opendaq/ids_parser.h>
#include <opendaq/custom_log.h>
#include <opendaq/search_filter_factory.h>
#include <opendaq/mirrored_signal_config_ptr.h>
#include <opendaq/module_manager_utils_ptr.h>

BEGIN_NAMESPACE_OPENDAQ

class StreamingSourceManager;
using StreamingSourceManagerPtr = std::shared_ptr<StreamingSourceManager>;

class StreamingSourceManager
{
public:
    explicit StreamingSourceManager(const ContextPtr& context,
                                    const DevicePtr& ownerDevice,
                                    const PropertyObjectPtr& deviceConfig);

    ~StreamingSourceManager();

private:
    void coreEventCallback(ComponentPtr& sender, CoreEventArgsPtr& eventArgs);

    void componentAdded(const ComponentPtr& sender, const CoreEventArgsPtr& eventArgs);
    void componentUpdated(const ComponentPtr& sender, const CoreEventArgsPtr& eventArgs);

    void enableStreamingForAddedComponent(const ComponentPtr& addedComponent);
    void enableStreamingForUpdatedComponent(const ComponentPtr& updatedComponent);

    static ListPtr<IMirroredDeviceConfig> getAllDevicesRecursively(const MirroredDeviceConfigPtr& device);

    AddressInfoPtr findMatchingAddress(const ListPtr<IAddressInfo>& availableAddresses,
                                       const AddressInfoPtr& deviceConnectionAddress);
    static AddressInfoPtr getDeviceConnectionAddress(const DevicePtr& device);
    void completeStreamingConnections(const MirroredDeviceConfigPtr& topDevice);
    void attachStreamingsToDevice(const MirroredDeviceConfigPtr& device);

    ContextPtr context;
    WeakRefPtr<IDevice> ownerDeviceRef;
    PropertyObjectPtr deviceConfig;
    LoggerComponentPtr loggerComponent;

    bool minHopsStreamingHeuristicEnabled{false};
    bool automaticallyConnectStreamings{false};
    std::unordered_set<std::string> allowedProtocolsOnly;
    std::map<StringPtr, SizeT> prioritizedProtocolsMap; // protocol Id as a key, protocol priority as a value
    StringPtr primaryAddressType;
};

inline StreamingSourceManager::StreamingSourceManager(const ContextPtr& context, const DevicePtr& ownerDevice, const PropertyObjectPtr& deviceConfig)
    : context(context)
    , ownerDeviceRef(ownerDevice)
    , deviceConfig(deviceConfig)
    , loggerComponent(context.getLogger().getOrAddComponent(fmt::format("StreamingSourceManager({})", ownerDevice.getGlobalId())))
{
    PropertyObjectPtr generalConfig = this->deviceConfig.getPropertyValue("General");
    minHopsStreamingHeuristicEnabled = generalConfig.getPropertyValue("StreamingConnectionHeuristic") == 1;
    automaticallyConnectStreamings = generalConfig.getPropertyValue("AutomaticallyConnectStreaming");
    primaryAddressType = generalConfig.getPropertyValue("PrimaryAddressType");

    ListPtr<IString> allowedStreamingProtocols = generalConfig.getPropertyValue("AllowedStreamingProtocols");
    for (SizeT index = 0; index < allowedStreamingProtocols.getCount(); ++index)
        allowedProtocolsOnly.insert(allowedStreamingProtocols[index].toStdString());

    ListPtr<IString> prioritizedStreamingProtocols = generalConfig.getPropertyValue("PrioritizedStreamingProtocols");
    for (SizeT index = 0; index < prioritizedStreamingProtocols.getCount(); ++index)
        prioritizedProtocolsMap.insert({prioritizedStreamingProtocols[index], index});

    this->context.getOnCoreEvent() += event(this, &StreamingSourceManager::coreEventCallback);
}

inline StreamingSourceManager::~StreamingSourceManager()
{
    this->context.getOnCoreEvent() -= event(this, &StreamingSourceManager::coreEventCallback);
}

inline void StreamingSourceManager::coreEventCallback(ComponentPtr& sender, CoreEventArgsPtr& eventArgs)
{
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

inline void StreamingSourceManager::componentAdded(const ComponentPtr& sender, const CoreEventArgsPtr& eventArgs)
{
    auto ownerDevice = ownerDeviceRef.assigned() ? ownerDeviceRef.getRef() : nullptr;
    if (!ownerDevice.assigned())
        return;
    ComponentPtr addedComponent = eventArgs.getParameters().get("Component");

    auto ownerDeviceGlobalId = ownerDevice.getGlobalId().toStdString();
    auto addedComponentGlobalId = addedComponent.getGlobalId().toStdString();

    if (!IdsParser::isNestedComponentId(ownerDeviceGlobalId, addedComponentGlobalId) &&
        addedComponentGlobalId != ownerDeviceGlobalId)
        return;

    LOG_I("Added Component: {}", addedComponentGlobalId);

    if (auto topAddedDevice = addedComponent.asPtrOrNull<IMirroredDeviceConfig>(); topAddedDevice.assigned())
    {
        if (addedComponentGlobalId == ownerDeviceGlobalId || minHopsStreamingHeuristicEnabled)
            completeStreamingConnections(topAddedDevice);

        if (minHopsStreamingHeuristicEnabled)
        {
            // enable streaming separately for top added and each of the nested devices - from bottom to top
            const auto allDevicesInTree = getAllDevicesRecursively(topAddedDevice);
            for (const auto& device : allDevicesInTree)
                enableStreamingForAddedComponent(device);
        }
        else
        {
            enableStreamingForAddedComponent(topAddedDevice);
        }
    }
    else
    {
        enableStreamingForAddedComponent(addedComponent);
    }
}

inline void StreamingSourceManager::componentUpdated(const ComponentPtr& sender, const CoreEventArgsPtr& eventArgs)
{
    auto ownerDevice = ownerDeviceRef.assigned() ? ownerDeviceRef.getRef() : nullptr;
    if (!ownerDevice.assigned())
        return;
    ComponentPtr updatedComponent = sender;

    auto ownerDeviceGlobalId = ownerDevice.getGlobalId().toStdString();
    auto updatedComponentGlobalId = updatedComponent.getGlobalId().toStdString();

    if (ownerDeviceGlobalId == updatedComponentGlobalId ||
        IdsParser::isNestedComponentId(ownerDeviceGlobalId, updatedComponentGlobalId) ||
        IdsParser::isNestedComponentId(updatedComponentGlobalId, ownerDeviceGlobalId))
    {
        LOG_I("Updated Component: {};", updatedComponentGlobalId);

        if (ownerDeviceGlobalId == updatedComponentGlobalId ||
            IdsParser::isNestedComponentId(updatedComponentGlobalId, ownerDeviceGlobalId))
        {
            context.getModuleManager().asPtr<IModuleManagerUtils>().completeDeviceCapabilities(ownerDevice);
            completeStreamingConnections(ownerDevice);
            enableStreamingForUpdatedComponent(ownerDevice);
        }
        else
        {
            if (auto updatedDevice = updatedComponent.asPtrOrNull<IMirroredSignalConfig>();
                updatedDevice.assigned() && minHopsStreamingHeuristicEnabled)
                completeStreamingConnections(updatedDevice);
            enableStreamingForUpdatedComponent(updatedComponent);
        }
    }
}

inline void StreamingSourceManager::enableStreamingForAddedComponent(const ComponentPtr& addedComponent)
{
    auto ownerDevice = ownerDeviceRef.assigned() ? ownerDeviceRef.getRef() : nullptr;
    if (!ownerDevice.assigned())
        return;

    auto isAncestorComponent = Function(
        [addedComponentId = addedComponent.getGlobalId()](const ComponentPtr& comp)
        {
            return IdsParser::isNestedComponentId(comp.getGlobalId(), addedComponentId) || comp.getGlobalId() == addedComponentId;
        }
        );
    auto ancestorDevices = List<IDevice>();
    if (minHopsStreamingHeuristicEnabled) // skip nested devices' streamings if MinHops disabled
        ancestorDevices = ownerDevice.getDevices(search::Recursive(search::Custom(isAncestorComponent)));
    ancestorDevices.pushFront(ownerDevice);

    // collect all relevant streaming sources for the component by retrieving sources from itself and all its ancestor devices
    // from top to the bottom
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
        if (!signal.getPublic())
            return;
        for (const auto& streaming : allStreamingSources)
        {
            ErrCode errCode = daqTry([&]()
                                     {
                                         streaming.addSignals({signal});
                                     });
            if (OPENDAQ_SUCCEEDED(errCode))
            {
                LOG_D("Signal \"{}\" added to streaming \"{}\"", signal.getGlobalId(), streaming.getConnectionString());
            }
            else if (errCode != OPENDAQ_ERR_DUPLICATEITEM)
            {
                checkErrorInfo(errCode);
            }
        }
        auto mirroredSignalConfigPtr = signal.template asPtr<IMirroredSignalConfig>();
        if (!mirroredSignalConfigPtr.getActiveStreamingSource().assigned())
        {
            LOG_D("Set active streaming source \"{}\" for signal \"{}\"", activeStreamingSource.getConnectionString(), signal.getGlobalId());
            mirroredSignalConfigPtr.setActiveStreamingSource(activeStreamingSource.getConnectionString());
        }
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

inline void StreamingSourceManager::enableStreamingForUpdatedComponent(const ComponentPtr& updatedComponent)
{
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

inline ListPtr<IMirroredDeviceConfig> StreamingSourceManager::getAllDevicesRecursively(const MirroredDeviceConfigPtr& device)
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

inline AddressInfoPtr StreamingSourceManager::findMatchingAddress(const ListPtr<IAddressInfo>& availableAddresses, const AddressInfoPtr& deviceConnectionAddress)
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
        LOG_W("Server streaming capability does not provide any addresses of primary {} type", primaryAddressType);
    }

    // Attempt to reuse the address of device connection
    for (const auto& addressInfo : availableAddresses)
    {
        if (deviceConnectionAddress.assigned() && addressInfo.getAddress() == deviceConnectionAddress.getAddress())
            return addressInfo;
    }

    return nullptr;
}

inline AddressInfoPtr StreamingSourceManager::getDeviceConnectionAddress(const DevicePtr& device)
{
    const auto configurationConnectionInfo = device.getInfo().getConfigurationConnectionInfo();
    const auto deviceInfoConnectionString = device.getInfo().getConnectionString();

    if (!configurationConnectionInfo.assigned())
        return nullptr;

    const auto deviceConnectionString =
        (deviceInfoConnectionString.assigned() && deviceInfoConnectionString.getLength())
            ? deviceInfoConnectionString
            : configurationConnectionInfo.getConnectionString();

    for (const auto& addressInfo : configurationConnectionInfo.getAddressInfo())
    {
        if (deviceConnectionString == addressInfo.getConnectionString())
            return addressInfo;
    }

    return nullptr;
}

inline void StreamingSourceManager::completeStreamingConnections(const MirroredDeviceConfigPtr& topDevice)
{
    if (!automaticallyConnectStreamings)
        return;

    if (minHopsStreamingHeuristicEnabled)
    {
        // The order of handling nested devices is important since we need to establish streaming connections
        // for the leaf devices first. The custom function is used to get the list of sub-devices
        // recursively, because using the recursive search filter does not guarantee the required order
        const auto allDevicesInTree = getAllDevicesRecursively(topDevice);
        for (const auto& device : allDevicesInTree)
        {
            attachStreamingsToDevice(device);
        }
    }
    else
    {
        attachStreamingsToDevice(topDevice);
    }
}

inline void StreamingSourceManager::attachStreamingsToDevice(const MirroredDeviceConfigPtr& device)
{
    // Get the address used for device connection
    const auto deviceConnectionAddress = getDeviceConnectionAddress(device);

    // protocol priority as a key, streaming source as a value
    std::map<SizeT, StreamingPtr> prioritizedStreamingSourcesMap;
    const ModuleManagerUtilsPtr managerUtils = this->context.getModuleManager().template asPtr<IModuleManagerUtils>();

    // connect via all allowed streaming capabilities which are not connected yet
    for (const auto& cap : device.getInfo().getServerCapabilities())
    {
        const StringPtr protocolId = cap.getPropertyValue("protocolId");
        if (cap.getProtocolType() != ProtocolType::Streaming)
            continue;

        LOG_D("Device {} has streaming capability: name [{}] id [{}] string [{}] prefix [{}]",
              device.getGlobalId(),
              cap.getProtocolName(),
              cap.getProtocolId(),
              cap.getConnectionString(),
              cap.getPrefix());

        if (!allowedProtocolsOnly.empty() && !allowedProtocolsOnly.count(protocolId.toStdString()))
            continue;

        const auto protocolIt = prioritizedProtocolsMap.find(protocolId);
        if (protocolIt == prioritizedProtocolsMap.end())
            continue;

        StreamingPtr streaming;
        const auto streamingAddress = findMatchingAddress(cap.getAddressInfo(), deviceConnectionAddress);
        StringPtr connectionString = streamingAddress.assigned() ? streamingAddress.getConnectionString() : cap.getConnectionString();

        if (!connectionString.assigned())
            continue;

        const auto addedStreamingSources = device.getStreamingSources();
        auto it = std::find_if(addedStreamingSources.begin(),
                               addedStreamingSources.end(),
                               [&connectionString](const StreamingPtr& item)
                               {
                                   return connectionString == item.getConnectionString();
                               });
        if (it != addedStreamingSources.end())
            continue;

        auto errCode = daqTry([&]()
                              {
                                  streaming = managerUtils.createStreaming(connectionString, deviceConfig);
                                  return OPENDAQ_SUCCESS;
                              });
        if (OPENDAQ_FAILED(errCode) || !streaming.assigned())
            continue;

        const SizeT protocolPriority = protocolIt->second;
        prioritizedStreamingSourcesMap.insert_or_assign(protocolPriority, streaming);
    }

    // add streaming sources ordered by protocol priority
    for (const auto& [_, streaming] : prioritizedStreamingSourcesMap)
    {
        streaming.setActive(true);
        device.addStreamingSource(streaming);
        LOG_I("Device {} added new streaming connection {}", device.getGlobalId(), streaming.getConnectionString());
    }
}

END_NAMESPACE_OPENDAQ
