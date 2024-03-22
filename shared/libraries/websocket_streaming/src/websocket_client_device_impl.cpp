#include "websocket_streaming/websocket_client_device_impl.h"
#include <opendaq/device_info_factory.h>
#include <opendaq/packet_factory.h>
#include <opendaq/event_packet_params.h>
#include "websocket_streaming/websocket_client_signal_factory.h"
#include "websocket_streaming/websocket_streaming_factory.h"
#include <coreobjects/property_object_protected_ptr.h>

BEGIN_NAMESPACE_OPENDAQ_WEBSOCKET_STREAMING

WebsocketClientDeviceImpl::WebsocketClientDeviceImpl(const ContextPtr& ctx,
                                                     const ComponentPtr& parent,
                                                     const StringPtr& localId,
                                                     const StringPtr& connectionString)
    : Device(ctx, parent, localId)
    , connectionString(connectionString)
{
    if (!this->connectionString.assigned())
        throw ArgumentNullException("connectionString cannot be null");
    createWebsocketStreaming();
    activateStreaming();
}

DeviceInfoPtr WebsocketClientDeviceImpl::onGetInfo()
{
    if (deviceInfo != nullptr)
        return deviceInfo;

    deviceInfo = DeviceInfo(connectionString, "WebsocketClientPseudoDevice");
    deviceInfo.freeze();
    return deviceInfo;
}

void WebsocketClientDeviceImpl::activateStreaming()
{
    auto self = this->borrowPtr<DevicePtr>();
    const auto signals = self.getSignals(search::Any());
    websocketStreaming.addSignals(signals);
    websocketStreaming.setActive(true);

    for (const auto& signal : signals)
    {
        auto mirroredSignalConfigPtr = signal.template asPtr<IMirroredSignalConfig>();
        mirroredSignalConfigPtr.setActiveStreamingSource(websocketStreaming.getConnectionString());
    }
}

/// connects to streaming server and waits till the list of available signals received
void WebsocketClientDeviceImpl::createWebsocketStreaming()
{
    bool useRawTcpConnection = connectionString.toStdString().find("daq.tcp://") == 0;
    auto streamingClient = std::make_shared<StreamingClient>(context, connectionString, useRawTcpConnection);

    auto signalInitCallback = [this](const StringPtr& signalId, const SubscribedSignalInfo& sInfo)
    {
        this->onSignalInit(signalId, sInfo);
    };
    streamingClient->onSignalInit(signalInitCallback);

    auto signalUpdatedCallback = [this](const StringPtr& signalId, const SubscribedSignalInfo& sInfo)
    {
        this->onSignalUpdated(signalId, sInfo);
    };
    streamingClient->onSignalUpdated(signalUpdatedCallback);

    auto domainDescriptorCallback = [this](const StringPtr& signalId, const DataDescriptorPtr& domainDescriptor)
    {
        this->onDomainDescriptor(signalId, domainDescriptor);
    };
    streamingClient->onDomainDescriptor(domainDescriptorCallback);

    auto availableSignalsCallback = [this](const std::vector<std::string>& signalIds)
    {
        this->createDeviceSignals(signalIds);
    };
    streamingClient->onAvailableDeviceSignals(availableSignalsCallback);

    websocketStreaming = WebsocketStreaming(streamingClient, connectionString, context);
}

void WebsocketClientDeviceImpl::onSignalInit(const StringPtr& signalId, const SubscribedSignalInfo& sInfo)
{
    if (!sInfo.dataDescriptor.assigned())
        return;

    if (auto signalIt = deviceSignals.find(signalId); signalIt != deviceSignals.end())
    {
        // sets signal name as it appeared in metadata "name"
        signalIt->second.asPtr<IComponentPrivate>().unlockAllAttributes();
        signalIt->second.setName(sInfo.signalName);
        signalIt->second.asPtr<IComponentPrivate>().lockAllAttributes();

        signalIt->second.asPtr<IMirroredSignalPrivate>()->setMirroredDataDescriptor(sInfo.dataDescriptor);
        updateSignalProperties(signalIt->second, sInfo);
    }
}

void WebsocketClientDeviceImpl::onSignalUpdated(const StringPtr& signalId, const SubscribedSignalInfo& sInfo)
{
    if (!sInfo.dataDescriptor.assigned())
        return;

    if (auto signalIt = deviceSignals.find(signalId); signalIt != deviceSignals.end())
        updateSignalProperties(signalIt->second, sInfo);
}

void WebsocketClientDeviceImpl::onDomainDescriptor(const StringPtr& signalId,
                                                      const DataDescriptorPtr& domainDescriptor)
{
    if (!domainDescriptor.assigned())
        return;

    // Sets domain descriptor for data signal
    if (auto signalIt = deviceSignals.find(signalId); signalIt != deviceSignals.end())
        signalIt->second.asPtr<IWebsocketStreamingSignalPrivate>()->createAndAssignDomainSignal(domainDescriptor);
}

void WebsocketClientDeviceImpl::createDeviceSignals(const std::vector<std::string>& signalIds)
{
    // Adds to device only signals published by server explicitly and in the same order these were published
    for (const auto& signalId : signalIds)
    {
        // TODO Streaming didn't provide META info for Time Signals -
        // - for these the mirrored signals will stay incomplete (without descriptors set)
        auto signal = WebsocketClientSignal(this->context, this->signals, signalId);
        this->addSignal(signal);
        deviceSignals.insert({signalId, signal});
    }
}

void WebsocketClientDeviceImpl::updateSignalProperties(const SignalPtr& signal, const SubscribedSignalInfo& sInfo)
{
    signal.asPtr<IComponentPrivate>().unlockAllAttributes();

    if (sInfo.signalProps.name.has_value())
        signal.setName(sInfo.signalProps.name.value());
    if (sInfo.signalProps.description.has_value())
        signal.setDescription(sInfo.signalProps.description.value());

    signal.asPtr<IComponentPrivate>().lockAllAttributes();
}

END_NAMESPACE_OPENDAQ_WEBSOCKET_STREAMING
