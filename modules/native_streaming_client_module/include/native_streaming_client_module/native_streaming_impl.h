/*
 * Copyright 2022-2024 Blueberry d.o.o.
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
#include <opendaq/streaming_impl.h>
#include <native_streaming_client_module/common.h>

#include <native_streaming_protocol/native_streaming_client_handler.h>

BEGIN_NAMESPACE_OPENDAQ_NATIVE_STREAMING_CLIENT_MODULE

static const char* NativeStreamingPrefix = "daq.ns://";
static const char* NativeStreamingID = "daq.ns";

class NativeStreamingImpl : public Streaming
{
public:
    explicit NativeStreamingImpl(const StringPtr& connectionString,
        const ContextPtr& context,
        opendaq_native_streaming_protocol::NativeStreamingClientHandlerPtr transportClientHandler,
        std::shared_ptr<boost::asio::io_context> processingIOContextPtr,
        Int streamingInitTimeout,
        const ProcedurePtr& onDeviceSignalAvailableCallback,
        const ProcedurePtr& onDeviceSignalUnavailableCallback,
        opendaq_native_streaming_protocol::OnConnectionStatusChangedCallback onDeviceConnectionStatusChangedCb);

    ~NativeStreamingImpl();

protected:
    void onSetActive(bool active) override;
    StringPtr onGetSignalStreamingId(const StringPtr& signalRemoteId) override;
    void onAddSignal(const MirroredSignalConfigPtr& signal) override;
    void onRemoveSignal(const MirroredSignalConfigPtr& signal) override;
    void onSubscribeSignal(const StringPtr& signalRemoteId, const StringPtr& domainSignalRemoteId) override;
    void onUnsubscribeSignal(const StringPtr& signalRemoteId, const StringPtr& domainSignalRemoteId) override;
    EventPacketPtr onCreateDataDescriptorChangedEventPacket(const StringPtr& signalRemoteId) override;

    void checkAndSubscribe(const StringPtr& signalRemoteId);
    void checkAndUnsubscribe(const StringPtr& signalRemoteId);

    void signalAvailableHandler(const StringPtr& signalStringId,
                                const StringPtr& serializedSignal);
    void addToAvailableSignals(const StringPtr& signalStringId);
    void addToAvailableSignalsOnReconnection(const StringPtr& signalStringId);

    void subscribeAckHandler(const StringPtr& signalStringId, bool subscribed);

    void signalUnavailableHandler(const StringPtr& signalStringId);
    void removeFromAvailableSignals(const StringPtr& signalStringId);

    void removeFromAddedSignals(const StringPtr& signalStringId);

    void connectionStatusChangedHandler(opendaq_native_streaming_protocol::ClientConnectionStatus status);

    void prepareClientHandler();

    void onPacket(const StringPtr& signalStringId, const PacketPtr& packet);
    void handleEventPacket(const MirroredSignalConfigPtr& signal, const EventPacketPtr& eventPacket);

    void startTransportOperations();
    void stopTransportOperations();

    opendaq_native_streaming_protocol::NativeStreamingClientHandlerPtr transportClientHandler;

    // pseudo device callbacks
    ProcedurePtr onDeviceSignalAvailableCallback;
    ProcedurePtr onDeviceSignalUnavailableCallback;
    opendaq_native_streaming_protocol::OnConnectionStatusChangedCallback onDeviceConnectionStatusChangedCb;

    std::map<StringPtr, SizeT> availableSignals;
    std::map<StringPtr, SizeT> availableSignalsReconnection;
    opendaq_native_streaming_protocol::ClientConnectionStatus connectionStatus;

    std::shared_ptr<boost::asio::io_context> processingIOContextPtr;
    boost::asio::io_context::strand processingStrand;

    std::promise<void> protocolInitPromise;
    std::future<void> protocolInitFuture;
    LoggerComponentPtr loggerComponent;

    std::chrono::milliseconds streamingInitTimeout;
    std::shared_ptr<boost::asio::io_context> timerContextPtr;
    std::shared_ptr<boost::asio::steady_timer> protocolInitTimer;

    std::mutex availableSignalsSync;
};

END_NAMESPACE_OPENDAQ_NATIVE_STREAMING_CLIENT_MODULE
