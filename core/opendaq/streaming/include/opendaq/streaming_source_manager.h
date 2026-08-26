/*
 * Copyright 2022-2026 openDAQ d.o.o.
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
#include <optional>
#include <unordered_set>
#include <opendaq/ids_parser.h>
#include <opendaq/custom_log.h>
#include <opendaq/search_filter_factory.h>
#include <opendaq/mirrored_signal_config_ptr.h>
#include <opendaq/module_manager_utils_ptr.h>

#include <opendaq/mirrored_input_port_config_ptr.h>

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

    ListPtr<IAddressInfo> getSuitableAddressesInfo(const ListPtr<IAddressInfo>& availableAddressesInfo,
                                                   const AddressInfoPtr& deviceConnectionAddress);
    static AddressInfoPtr getDeviceConnectionAddress(const DevicePtr& device);
    void completeStreamingConnections(const MirroredDeviceConfigPtr& topDevice);
    void attachStreamingsToDevice(const MirroredDeviceConfigPtr& device);
    std::unordered_map<std::string, ListPtr<IAddressInfo>> buildDiscoveredStreamingAddrsByProtocol(const MirroredDeviceConfigPtr& device);

    ContextPtr context;
    WeakRefPtr<IDevice> ownerDeviceRef;
    PropertyObjectPtr deviceConfig;
    LoggerComponentPtr loggerComponent;

    bool minHopsStreamingHeuristicEnabled{false};
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

    auto isAncestorOrAddedComponent = Function(
        [addedComponentId = addedComponent.getGlobalId()](const BaseObjectPtr& obj)
        {
            const auto comp = obj.asPtr<IComponent>();
            return IdsParser::isNestedComponentId(comp.getGlobalId(), addedComponentId) || comp.getGlobalId() == addedComponentId;
        }
    );
    auto ancestorDevices = List<IDevice>();
    if (minHopsStreamingHeuristicEnabled) // skip nested devices' streamings if MinHops disabled
        ancestorDevices = ownerDevice.getDevices(search::Recursive(search::Custom(isAncestorOrAddedComponent)));
    ancestorDevices.pushFront(ownerDevice);

    // collect all relevant streaming sources for the added component by retrieving sources from itself (if it is device)
    // and all its ancestor devices, saving by priority in order from bottom to the top;
    // i.e. the sources of added component (or its closest ancestor device) will be the first ones in the saved vector
    std::vector<StreamingPtr> allStreamingSources;
    for (const auto& ancestorDevice : ancestorDevices)
    {
        if (auto mirroredDevice = ancestorDevice.asPtrOrNull<IMirroredDevice>(); mirroredDevice.assigned())
        {
            auto streamingSources = mirroredDevice.getStreamingSources().toVector();
            // prepend to the result vector
            allStreamingSources.insert(allStreamingSources.begin(), streamingSources.begin(), streamingSources.end());
        }
    }

    if (allStreamingSources.empty())
        return;

    auto addSignalToStreamings = [this, &allStreamingSources](const SignalPtr& signal)
    {
        if (!signal.getPublic())
            return;
        for (const auto& streaming : allStreamingSources)
        {
            ErrCode errCode = daqTry([&]()
                                     {
                                         // does not guarantee that signal will be added, as some signals,
                                         // e.g. private ones (by default), may be silently ignored
                                         streaming.addSignals({signal});
                                     });
            if (OPENDAQ_SUCCEEDED(errCode))
            {
                LOG_D("Signal \"{}\" added to streaming \"{}\"", signal.getGlobalId(), streaming.getConnectionString());
            }
            else if (errCode == OPENDAQ_ERR_DUPLICATEITEM)
            {
                daqClearErrorInfo();
            }
            else
            {
                checkErrorInfo(errCode);
            }
        }
    };

    auto activateStreamingForSignal = [this, &allStreamingSources](const SignalPtr& signal)
    {
        if (!signal.getPublic())
            return;
        auto mirroredSignalConfigPtr = signal.template asPtr<IMirroredSignalConfig>();
        if (!mirroredSignalConfigPtr.getActiveStreamingSource().assigned())
        {
            // streaming sources were created (by completeStreamingConnections) and ordered by priority above,
            // set the highest-priority source as active for signal, if relevant
            auto signalStreamingSources = mirroredSignalConfigPtr.getStreamingSources();
            for (const auto& streaming : allStreamingSources)
            {
                auto connectionString = streaming.getConnectionString();
                auto it = std::find(signalStreamingSources.begin(), signalStreamingSources.end(), connectionString);
                if (it != signalStreamingSources.end())
                {
                    mirroredSignalConfigPtr.setActiveStreamingSource(connectionString);
                    LOG_D("Set active streaming source \"{}\" for signal \"{}\"", connectionString, signal.getGlobalId());
                    break;
                }
            }
        }
    };

    // setup streaming sources for all signals of the new component
    ListPtr<ISignal> addedSignals = List<ISignal>();
    if (auto addedSignal = addedComponent.asPtrOrNull<ISignal>(); addedSignal.assigned())
        addedSignals.pushBack(addedSignal);
    else if (auto addedFolder = addedComponent.asPtrOrNull<IFolder>(); addedFolder.assigned())
        addedSignals = addedFolder.getItems(search::Recursive(search::InterfaceId(ISignal::Id)));

    // add all signals before activating any: activation may subscribe a listened signal along with its domain signal, which must already be added
    for (const auto& signal : addedSignals)
        addSignalToStreamings(signal);
    for (const auto& signal : addedSignals)
        activateStreamingForSignal(signal);

    auto setupStreamingForInputPort = [this, &allStreamingSources](const InputPortPtr& inputPort)
    {
        for (const auto& streaming : allStreamingSources)
        {
            if (!streaming.getClientToDeviceStreamingEnabled())
                continue;
            ErrCode errCode = daqTry([&]()
                                     {
                                         streaming.addInputPorts({inputPort});
                                     });
            if (OPENDAQ_SUCCEEDED(errCode))
            {
                LOG_I("InputPort \"{}\" added to streaming \"{}\"", inputPort.getGlobalId(), streaming.getConnectionString());
            }
            else if (errCode != OPENDAQ_ERR_DUPLICATEITEM)
            {
                checkErrorInfo(errCode);
            }
            else
            {
                daqClearErrorInfo();
            }
        }
        auto mirroredInputPortConfigPtr = inputPort.template asPtr<IMirroredInputPortConfig>();
        if (!mirroredInputPortConfigPtr.getActiveStreamingSource().assigned())
        {
            // streaming sources were created (by completeStreamingConnections) and ordered by priority above,
            // set the highest-priority source as active for inputPort, if relevant
            auto inputPortStreamingSources = mirroredInputPortConfigPtr.getStreamingSources();
            for (const auto& streaming : allStreamingSources)
            {
                if (!streaming.getClientToDeviceStreamingEnabled())
                    continue;
                auto connectionString = streaming.getConnectionString();
                auto it = std::find(inputPortStreamingSources.begin(), inputPortStreamingSources.end(), connectionString);
                if (it != inputPortStreamingSources.end())
                {
                    mirroredInputPortConfigPtr.setActiveStreamingSource(connectionString);
                    LOG_I("Set active streaming source \"{}\" for inputPort \"{}\"", connectionString, inputPort.getGlobalId());
                    break;
                }
            }
        }
    };

    // setup streaming sources for all input ports of the new component
    if (auto addedInputPort = addedComponent.asPtrOrNull<IInputPort>(); addedInputPort.assigned())
    {
        setupStreamingForInputPort(addedInputPort);
    }
    else if (auto addedFolder = addedComponent.asPtrOrNull<IFolder>(); addedFolder.assigned())
    {
        ListPtr<IInputPort> nestedInputPorts = addedFolder.getItems(search::Recursive(search::InterfaceId(IInputPort::Id)));
        for (const auto& nestedInputPort : nestedInputPorts)
            setupStreamingForInputPort(nestedInputPort);
    }
}

inline void StreamingSourceManager::enableStreamingForUpdatedComponent(const ComponentPtr& updatedComponent)
{
    // setup streaming sources for all nested signals which do not have any, assuming these are newly added signals
    if (auto updatedSignal = updatedComponent.asPtrOrNull<IMirroredSignalConfig>(); updatedSignal.assigned())
    {
        if (updatedSignal.getStreamingSources().getCount() == 0)
            enableStreamingForAddedComponent(updatedSignal);
    }
    else if (auto updatedFolder = updatedComponent.asPtrOrNull<IFolder>(); updatedFolder.assigned())
    {
        auto isNewlyAddedDomainSignal = Function(
            [](const BaseObjectPtr& obj)
            {
                auto signal = obj.asPtrOrNull<IMirroredSignalConfig>();
                return signal.assigned()
                       && signal.getStreamingSources().getCount() == 0
                       && !signal.getDomainSignal().assigned();
            }
        );
        auto isNewlyAddedValueSignal = Function(
            [](const BaseObjectPtr& obj)
            {
                auto signal = obj.asPtrOrNull<IMirroredSignalConfig>();
                return signal.assigned()
                       && signal.getStreamingSources().getCount() == 0
                       && signal.getDomainSignal().assigned();
            }
        );

        ListPtr<IMirroredSignalConfig> nestedAddedDomainSignals =
            updatedFolder.getItems(search::Recursive(search::Custom(isNewlyAddedDomainSignal)));
        ListPtr<IMirroredSignalConfig> nestedAddedValueSignals =
            updatedFolder.getItems(search::Recursive(search::Custom(isNewlyAddedValueSignal)));

        // setup streaming sources for domain signals first
        for (const auto& nestedDomainSignal : nestedAddedDomainSignals)
            enableStreamingForAddedComponent(nestedDomainSignal);
        for (const auto& nestedValueSignal : nestedAddedValueSignals)
            enableStreamingForAddedComponent(nestedValueSignal);
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

inline ListPtr<IAddressInfo> StreamingSourceManager::getSuitableAddressesInfo(const ListPtr<IAddressInfo>& availableAddressesInfo,
                                                                              const AddressInfoPtr& deviceConnectionAddress)
{
    
    auto result = List<IAddressInfo>();

    auto const addToResult = [&result, &deviceConnectionAddress](AddressInfoPtr addressInfo)
    {
        if (deviceConnectionAddress.assigned() && addressInfo.getAddress() == deviceConnectionAddress.getAddress())
            result.pushFront(addressInfo);
        else
            result.pushBack(addressInfo);
    };

    if (primaryAddressType == "IPv4" || primaryAddressType == "IPv6")
    {
        for (const auto& addressInfo : availableAddressesInfo)
        {
            if (addressInfo.getType() == primaryAddressType)
                addToResult(addressInfo);
        }

        if (!result.empty())
            return result;

        LOG_W("Server streaming capability does not provide any addresses of primary {} type", primaryAddressType);
    }

    for (const auto& addressInfo : availableAddressesInfo)
    {
        addToResult(addressInfo);
    }

    return result;
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
    // Get the full address information used for device configuration connection
    const auto deviceConnectionAddress = getDeviceConnectionAddress(device);

    const ModuleManagerUtilsPtr managerUtils = this->context.getModuleManager().template asPtr<IModuleManagerUtils>();

    // Build a map of discovered addresses by protocol ID for quick lookup
    const auto discoveredAddrsByProtocol = buildDiscoveredStreamingAddrsByProtocol(device);

    // Will be sorted later.
    std::vector<std::pair<SizeT, StreamingPtr>> prioritizedStreaming;

    // Streaming sources selected within this call are attached to the device only afterwards,
    // so the pending ones have to be checked along with the already attached ones
    const auto anyStreamingMatches = [&prioritizedStreaming](const auto& attachedSources, const auto& predicate) -> bool
    {
        return std::any_of(attachedSources.begin(), attachedSources.end(), predicate) ||
               std::any_of(prioritizedStreaming.begin(),
                           prioritizedStreaming.end(),
                           [&predicate](const auto& item) { return predicate(item.second); });
    };

    const auto capabilityPriority = [this](const ServerCapabilityPtr& cap) -> std::optional<SizeT>
    {
        if (cap.getProtocolType() != ProtocolType::Streaming)
            return std::nullopt;

        const StringPtr protocolId = cap.getProtocolId();
        if (!allowedProtocolsOnly.empty() && !allowedProtocolsOnly.count(protocolId.toStdString()))
            return std::nullopt;

        const auto protocolIt = prioritizedProtocolsMap.find(protocolId);
        if (protocolIt == prioritizedProtocolsMap.end())
            return std::nullopt;

        return protocolIt->second;
    };

    const auto capabilityGroupId = [](const ServerCapabilityPtr& cap) -> StringPtr
    {
        const auto groupId = cap.getProtocolGroupId();
        return groupId.assigned() && groupId.getLength() > 0 ? groupId : nullptr;
    };

    std::unordered_map<std::string, SizeT> bestPriorityPerGroup;
    for (const auto& cap : device.getInfo().getServerCapabilities())
    {
        const auto groupId = capabilityGroupId(cap);
        if (!groupId.assigned())
            continue;

        const auto priority = capabilityPriority(cap);
        if (!priority.has_value())
            continue;

        const auto [it, inserted] = bestPriorityPerGroup.try_emplace(groupId.toStdString(), *priority);
        if (!inserted)
            it->second = std::min(it->second, *priority);
    }

    // connect via all allowed streaming capabilities which are not connected yet
    for (const auto& cap : device.getInfo().getServerCapabilities())
    {
        if (cap.getProtocolType() != ProtocolType::Streaming)
            continue;

        const auto protoGroupId = capabilityGroupId(cap);

        LOG_D("Device {} has streaming capability: name [{}] group id [{}] id [{}] string [{}] prefix [{}]",
              device.getGlobalId(),
              cap.getProtocolName(),
              protoGroupId.assigned() ? protoGroupId : "<none>",
              cap.getProtocolId(),
              cap.getConnectionString(),
              cap.getPrefix());

        const StringPtr protocolId = cap.getProtocolId();
        const auto priority = capabilityPriority(cap);
        if (!priority.has_value())
            continue;

        const auto addedStreamingSources = device.getStreamingSources();
        if (protoGroupId.assigned())
        {
            // only the most preferred protocol of the group is connected
            if (*priority != bestPriorityPerGroup.at(protoGroupId.toStdString()))
            {
                LOG_D("Device {} has a more preferred protocol in group [{}] than [{}], skipping it",
                      device.getGlobalId(),
                      protoGroupId,
                      protocolId);
                continue;
            }

            // the group may already have been connected by a previous call
            const auto sameGroup = [&protoGroupId](const StreamingPtr& item)
            {
                return item.getProtocolGroupId() == protoGroupId;
            };

            if (anyStreamingMatches(addedStreamingSources, sameGroup))
            {
                LOG_D("Device {} already has streaming source with protocol group id [{}], skipping adding another one",
                      device.getGlobalId(),
                      protoGroupId);
                continue;
            }
        }

        // Prioritize discovery addresses if available for this protocol
        ListPtr<IAddressInfo> addressesToUse =
            (discoveredAddrsByProtocol.count(protocolId) > 0) ? discoveredAddrsByProtocol.at(protocolId) : cap.getAddressInfo();

        // get the addresses of primary type giving the priority to known config connection address
        const auto suitableAddressesInfo = getSuitableAddressesInfo(addressesToUse, deviceConnectionAddress);

        auto streamingConnectionStrings = List<IString>();
        for (const auto& addressInfo : suitableAddressesInfo)
            streamingConnectionStrings.pushBack(addressInfo.getConnectionString());
        if (streamingConnectionStrings.empty())
            streamingConnectionStrings.pushBack(cap.getConnectionString());

        // try all suitable addresses until the first successful streaming connection
        for (const auto& connectionString : streamingConnectionStrings)
        {
            if (!connectionString.assigned())
                continue;

            const auto sameConnectionString = [&connectionString](const StreamingPtr& item)
            {
                return connectionString == item.getConnectionString();
            };

            if (anyStreamingMatches(addedStreamingSources, sameConnectionString))
                break;

            StreamingPtr streaming;

            auto errCode = daqTry(
                [&]()
                {
                    streaming = managerUtils.createStreaming(connectionString, deviceConfig);
                    return OPENDAQ_SUCCESS;
                });
            if (OPENDAQ_FAILED(errCode))
                daqClearErrorInfo();
            if (!streaming.assigned())
                continue;

            prioritizedStreaming.emplace_back(*priority, streaming);
            break;
        }
    }

    std::stable_sort(prioritizedStreaming.begin(),
                     prioritizedStreaming.end(),
                     [](const auto& item0, const auto& item1) { return item0.first < item1.first; });

    // add streaming sources ordered by protocol priority
    for (const auto& [_, streaming] : prioritizedStreaming)
    {
        streaming.setActive(true);
        device.addStreamingSource(streaming);
        LOG_I("Device {} added new streaming connection {}", device.getGlobalId(), streaming.getConnectionString());
    }
}

inline std::unordered_map<std::string, ListPtr<IAddressInfo>>
StreamingSourceManager::buildDiscoveredStreamingAddrsByProtocol(const MirroredDeviceConfigPtr& device)
{
    std::unordered_map<std::string, ListPtr<IAddressInfo>> output;

    const ModuleManagerUtilsPtr managerUtils = this->context.getModuleManager().template asPtr<IModuleManagerUtils>();
    const auto deviceInfo = device.getInfo();
    const StringPtr deviceManufacturer = deviceInfo.getManufacturer();
    const StringPtr deviceSerialNumber = deviceInfo.getSerialNumber();

    if (deviceManufacturer.assigned() && deviceManufacturer.getLength() > 0 &&
        deviceSerialNumber.assigned() && deviceSerialNumber.getLength() > 0)
    {
        // Try to get discovery info for this device to prioritize real/discovered addresses
        DeviceInfoPtr discoveryInfo;
        const auto errCode = managerUtils->getDiscoveryInfo(&discoveryInfo, deviceManufacturer, deviceSerialNumber);
        if (OPENDAQ_FAILED(errCode))
            daqClearErrorInfo();

        if (discoveryInfo.assigned())
        {
            for (const auto& discoveryCap : discoveryInfo.getServerCapabilities())
            {
                if (discoveryCap.getProtocolType() != ProtocolType::Streaming)
                    continue;

                output[discoveryCap.getProtocolId().toStdString()] = discoveryCap.getAddressInfo();
            }
        }
    }
    return output;
}

END_NAMESPACE_OPENDAQ
