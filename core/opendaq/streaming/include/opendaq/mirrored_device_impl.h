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

private:
    void coreEventCallback(ComponentPtr& sender, CoreEventArgsPtr& eventArgs);
    void componentAdded(const ComponentPtr& sender, const CoreEventArgsPtr& eventArgs);
    void componentUpdated(const ComponentPtr& sender, const CoreEventArgsPtr& eventArgs);

    void enableStreamingForAddedComponent(const ComponentPtr& addedComponent);
    void enableStreamingForUpdatedComponent(const ComponentPtr& updatedComponent);
    void tryAddSignalToStreaming(const SignalPtr& signal, const StreamingPtr& streaming);
    void setSignalActiveStreamingSource(const SignalPtr& signal, const StreamingPtr& streaming);
    void completeStreamingConnections(const ComponentPtr& component);

    std::vector<StreamingPtr> streamingSources;
    bool minHopsStreamingHeuristicEnabled{false};
};

template <typename... Interfaces>
MirroredDeviceBase<Interfaces...>::MirroredDeviceBase(const ContextPtr& ctx,
                                                      const ComponentPtr& parent,
                                                      const StringPtr& localId,
                                                      const StringPtr& className,
                                                      const StringPtr& name)
    : Super(ctx, parent, localId, className, name)
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
    if (!IdsParser::isNestedComponentId(deviceSelfGlobalId, addedComponentGlobalId))
        return;

    DAQLOGF_I(this->loggerComponent, "Added Component: {};", addedComponentGlobalId);

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

END_NAMESPACE_OPENDAQ
