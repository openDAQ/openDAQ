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
    const auto signals = self.getSignals();
    websocketStreaming.addSignals(signals);
    websocketStreaming.setActive(true);

    for (const auto& signal : signals)
    {
        auto signalConfigPtr = signal.asPtr<ISignalConfig>();
        signalConfigPtr.setActiveStreamingSource(websocketStreaming.getConnectionString());
    }
}

void WebsocketClientDeviceImpl::createWebsocketStreaming()
{
    auto streamingClient = std::make_shared<StreamingClient>(context, connectionString);

    auto newSignalCallback = [this](const StringPtr& signalId, const SubscribedSignalInfo& sInfo)
    {
        this->onNewSignal(signalId, sInfo);
    };
    streamingClient->onNewSignal(newSignalCallback);

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
        this->initializeDeviceSignals(signalIds);
    };
    streamingClient->onAvailableDeviceSignals(availableSignalsCallback);

    websocketStreaming = WebsocketStreaming(streamingClient, connectionString, context);
}

void WebsocketClientDeviceImpl::onNewSignal(const StringPtr& signalId, const SubscribedSignalInfo& sInfo)
{
    if (!sInfo.dataDescriptor.assigned())
        return;

    registerSignalAttributes(signalId, sInfo);
}

void WebsocketClientDeviceImpl::onSignalUpdated(const StringPtr& signalId, const SubscribedSignalInfo& sInfo)
{
    if (!sInfo.dataDescriptor.assigned())
        return;

    if (auto signalIt = deviceSignals.find(signalId); signalIt != deviceSignals.end())
        updateSignal(signalIt->second, sInfo);
}

void WebsocketClientDeviceImpl::onDomainDescriptor(const StringPtr& signalId,
                                                      const DataDescriptorPtr& domainDescriptor)
{
    if (!domainDescriptor.assigned())
        return;

    // Sets domain descriptor for data signal attributes
    if (auto iter = deviceSignalsAttributes.find(signalId); iter != deviceSignalsAttributes.end())
    {
        auto& signalAttributes = iter->second;
        signalAttributes.first = domainDescriptor;
    }
}

void WebsocketClientDeviceImpl::initializeDeviceSignals(const std::vector<std::string>& signalIds)
{
    // Adds to device only signals published by server explicitly and in the same order these were published
    for (const auto& signalId : signalIds)
    {

        if (auto iter = deviceSignalsAttributes.find(signalId); iter == deviceSignalsAttributes.end())
        {
            // TODO Streaming didn't provide META info for Time Signals -
            // - for these signals device provides incomplete mirrored signals (without descriptor)
            auto signal = WebsocketClientSignal(this->context, this->signals, nullptr, nullptr, signalId);
            this->addSignal(signal);
            deviceSignals.insert({signalId, signal});
        }
        else
        {
            // Streaming provided META info for the Signal - device has complete signal (with descriptors)
            auto [domainDescriptor, sInfo] = iter->second;

            auto signal = WebsocketClientSignal(this->context, this->signals, sInfo.dataDescriptor, domainDescriptor, signalId);
            this->addSignal(signal);
            deviceSignals.insert({signalId, signal});
            updateSignal(signal, sInfo);
        }
    }
}

void WebsocketClientDeviceImpl::registerSignalAttributes(const StringPtr& signalId, const SubscribedSignalInfo& sInfo)
{
    deviceSignalsAttributes.insert({signalId, {nullptr, sInfo}});
}

void WebsocketClientDeviceImpl::updateSignal(const SignalPtr& signal, const SubscribedSignalInfo& sInfo)
{
    auto protectedObject = signal.asPtr<IPropertyObjectProtected>();
    if (sInfo.signalProps.name.has_value())
        protectedObject.setProtectedPropertyValue("Name", sInfo.signalProps.name.value());
    if (sInfo.signalProps.description.has_value())
        protectedObject.setProtectedPropertyValue("Description", sInfo.signalProps.description.value());
}

END_NAMESPACE_OPENDAQ_WEBSOCKET_STREAMING
