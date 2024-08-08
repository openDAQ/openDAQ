#include "websocket_streaming/websocket_client_device_impl.h"
#include <opendaq/device_info_factory.h>
#include <opendaq/packet_factory.h>
#include <opendaq/event_packet_params.h>
#include "websocket_streaming/websocket_client_signal_factory.h"
#include "websocket_streaming/websocket_streaming_factory.h"
#include <coreobjects/property_object_protected_ptr.h>
#include <opendaq/component_ptr.h>

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
    this->name = "WebsocketClientPseudoDevice";
    createWebsocketStreaming();
    activateStreaming();
}

void WebsocketClientDeviceImpl::removed()
{
    websocketStreaming.release();
    Device::removed();
}

DeviceInfoPtr WebsocketClientDeviceImpl::onGetInfo()
{
    auto deviceInfo = DeviceInfo(connectionString, "WebsocketClientPseudoDevice");
    deviceInfo.freeze();
    return deviceInfo;
}

void WebsocketClientDeviceImpl::activateStreaming()
{
    websocketStreaming.setActive(true);
    for (const auto& [_, signal] : deviceSignals)
    {
        websocketStreaming.addSignals({signal});
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
    streamingClient->onDeviceAvailableSignalInit(signalInitCallback);

    auto signalUpdatedCallback = [this](const StringPtr& signalId, const SubscribedSignalInfo& sInfo)
    {
        this->onSignalUpdated(signalId, sInfo);
    };
    streamingClient->onDeviceSignalUpdated(signalUpdatedCallback);

    auto domainSignalInitCallback = [this](const StringPtr& dataSignalId,const StringPtr& domainSignalId)
    {
        this->onDomainSignalInit(dataSignalId, domainSignalId);
    };
    streamingClient->onDeviceDomainSingalInit(domainSignalInitCallback);

    auto availableSignalsCallback = [this](const std::vector<std::string>& signalIds)
    {
        this->registerAvailableSignals(signalIds);
    };
    streamingClient->onDeviceAvailableSignals(availableSignalsCallback);

    auto unavailableSignalsCallback = [this](const std::vector<std::string>& signalIds)
    {
        this->removeSignals(signalIds);
    };
    streamingClient->onDeviceUnavailableSignals(unavailableSignalsCallback);

    auto hiddenSignalCallback = [this](const StringPtr& signalId, const SubscribedSignalInfo& sInfo)
    {
        this->registerHiddenSignal(signalId, sInfo);
    };
    streamingClient->onDeviceHiddenSignal(hiddenSignalCallback);

    auto signalsInitDoneCallback = [this]()
    {
        this->addInitializedSignals();
    };
    streamingClient->onDeviceSignalsInitDone(signalsInitDoneCallback);

    websocketStreaming = WebsocketStreaming(streamingClient, connectionString, context);
}

void WebsocketClientDeviceImpl::onSignalInit(const StringPtr& signalId, const SubscribedSignalInfo& sInfo)
{
    if (!sInfo.dataDescriptor.assigned())
        return;

    if (auto signalIt = deviceSignals.find(signalId); signalIt != deviceSignals.end())
    {
        // sets signal name as it appeared in metadata "Name"
        signalIt->second.asPtr<IComponentPrivate>().unlockAllAttributes();
        signalIt->second.setName(sInfo.signalName);
        signalIt->second.asPtr<IComponentPrivate>().lockAllAttributes();

        signalIt->second.asPtr<IMirroredSignalPrivate>().setMirroredDataDescriptor(sInfo.dataDescriptor);
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

void WebsocketClientDeviceImpl::onDomainSignalInit(const StringPtr& signalId, const StringPtr& domainSignalId)
{
    // Sets domain signal for data signal
    if (auto dataSignalIt = deviceSignals.find(signalId); dataSignalIt != deviceSignals.end())
    {
        auto domainSignalIt = deviceSignals.find(domainSignalId);
        if (domainSignalIt != deviceSignals.end())
        {
            // domain signal is found in device
            auto domainSignal = domainSignalIt->second;
            auto dataSignal = dataSignalIt->second;
            dataSignal.asPtr<IMirroredSignalPrivate>().setMirroredDomainSignal(domainSignal);
        }
    }
}

void WebsocketClientDeviceImpl::registerAvailableSignals(const std::vector<std::string>& signalIds)
{
    // Adds to device only signals published by server explicitly and in the same order these were published
    for (const auto& signalId : signalIds)
    {
        // TODO Streaming do not support some types of signals and do not provide META info -
        // - for those the mirrored signals will stay incomplete (without descriptors set)
        auto signal = WebsocketClientSignal(this->context, this->signals, signalId);
        deviceSignals.insert({signalId, signal});
        orderedSignalIds.push_back(signalId);
    }
}

void WebsocketClientDeviceImpl::removeSignals(const std::vector<std::string>& signalIds)
{
    for (const auto& removedSignalId : signalIds)
    {
        auto signalIter = deviceSignals.find(removedSignalId);
        if (signalIter == deviceSignals.end())
        {
            LOG_E("Signal with id {} is not found in LT streaming device", removedSignalId);
            continue;
        }

        auto signalToRemove = signalIter->second;

        // recreate signal -> domainSignal relations in the same way as on server
        for (const auto& [_, dataSignal] : deviceSignals)
        {
            auto domainSignal = dataSignal.getDomainSignal();
            if (domainSignal.assigned() && domainSignal.asPtr<IMirroredSignalConfig>().getRemoteId() == removedSignalId)
            {
                dataSignal.asPtr<IMirroredSignalPrivate>().setMirroredDomainSignal(nullptr);
            }
        }

        this->removeSignal(signalToRemove);
        deviceSignals.erase(signalIter);
        orderedSignalIds.erase(std::remove(orderedSignalIds.begin(), orderedSignalIds.end(), removedSignalId), orderedSignalIds.end());
    }
}

void WebsocketClientDeviceImpl::registerHiddenSignal(const StringPtr& signalId, const SubscribedSignalInfo& sInfo)
{
    // Adds 'hidden' signal which were not published by server explicitly as available
    auto signal = WebsocketClientSignal(this->context, this->signals, signalId);
    deviceSignals.insert({signalId, signal});
    onSignalInit(signalId, sInfo);
    orderedSignalIds.push_back(signalId.toStdString());
}

void WebsocketClientDeviceImpl::addInitializedSignals()
{
    for (const auto& signalId : orderedSignalIds)
    {
        if (auto signalIt = deviceSignals.find(String(signalId)); signalIt != deviceSignals.end())
        {
            auto signal = signalIt->second;

            if (!this->signals.findComponent(signal.getLocalId()).assigned())
            {
                if (websocketStreaming.assigned())
                {
                    websocketStreaming.addSignals({signal});
                    auto mirroredSignalConfigPtr = signal.template asPtr<IMirroredSignalConfig>();
                    mirroredSignalConfigPtr.setActiveStreamingSource(websocketStreaming.getConnectionString());
                }

                this->addSignal(signal);
            }
        }
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
