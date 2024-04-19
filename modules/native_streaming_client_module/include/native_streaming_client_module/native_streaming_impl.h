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
static const char* NativeStreamingID = "opendaq_native_streaming";

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
    void onAddSignal(const MirroredSignalConfigPtr& signal) override;
    void onRemoveSignal(const MirroredSignalConfigPtr& signal) override;
    void onSubscribeSignal(const StringPtr& signalStreamingId) override;
    void onUnsubscribeSignal(const StringPtr& signalStreamingId) override;

    void signalAvailableHandler(const StringPtr& signalStringId, const StringPtr& serializedSignal);
    void signalUnavailableHandler(const StringPtr& signalStringId);

    void connectionStatusChangedHandler(opendaq_native_streaming_protocol::ClientConnectionStatus status);

    void prepareClientHandler();

    void startTransportOperations();
    void stopTransportOperations();

    opendaq_native_streaming_protocol::NativeStreamingClientHandlerPtr transportClientHandler;

    // pseudo device callbacks
    ProcedurePtr onDeviceSignalAvailableCallback;
    ProcedurePtr onDeviceSignalUnavailableCallback;
    opendaq_native_streaming_protocol::OnConnectionStatusChangedCallback onDeviceConnectionStatusChangedCb;

    opendaq_native_streaming_protocol::ClientConnectionStatus connectionStatus;

    std::shared_ptr<boost::asio::io_context> processingIOContextPtr;
    boost::asio::io_context::strand processingStrand;

    std::promise<void> protocolInitPromise;
    std::future<void> protocolInitFuture;

    std::chrono::milliseconds streamingInitTimeout;
    std::shared_ptr<boost::asio::io_context> timerContextPtr;
    std::shared_ptr<boost::asio::steady_timer> protocolInitTimer;
};

END_NAMESPACE_OPENDAQ_NATIVE_STREAMING_CLIENT_MODULE
