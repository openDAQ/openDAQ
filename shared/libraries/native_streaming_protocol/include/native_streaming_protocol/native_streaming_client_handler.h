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

#include <native_streaming_protocol/native_streaming_protocol.h>
#include <native_streaming_protocol/client_session_handler.h>

#include <opendaq/context_ptr.h>
#include <opendaq/logger_ptr.h>
#include <opendaq/logger_component_ptr.h>
#include <opendaq/packet_ptr.h>

#include <native_streaming/client.hpp>

#include <future>

BEGIN_NAMESPACE_OPENDAQ_NATIVE_STREAMING_PROTOCOL

enum class ClientConnectionStatus
{
    Connected = 0,
    Reconnecting,
    Unrecoverable
};

inline ConstCharPtr convertConnectionStatusToString(ClientConnectionStatus status)
{
    switch (status)
    {
        case ClientConnectionStatus::Connected:
            return "Connected";
        case ClientConnectionStatus::Reconnecting:
            return "Reconnecting";
        case ClientConnectionStatus::Unrecoverable:
            return "Unrecoverable";
    }

    return "InvalidConnectionStatus";
}

using OnSignalAvailableCallback = std::function<void(const StringPtr& signalStringId,
                                                     const StringPtr& serializedSignal)>;
using OnSignalUnavailableCallback = std::function<void(const StringPtr& signalStringId)>;
using OnPacketCallback = std::function<void(const StringPtr& signalStringId, const PacketPtr& packet)>;
using OnSignalSubscriptionAckCallback = std::function<void(const StringPtr& signalStringId, bool subscribed)>;
using OnConnectionStatusChangedCallback = std::function<void(ClientConnectionStatus status)>;

class NativeStreamingClientHandler;
using NativeStreamingClientHandlerPtr = std::shared_ptr<NativeStreamingClientHandler>;

class NativeStreamingClientHandler
{
public:
    explicit NativeStreamingClientHandler(const ContextPtr& context,
                                          const PropertyObjectPtr& transportLayerProperties);

    ~NativeStreamingClientHandler();

    bool connect(std::string host,
                 std::string port,
                 std::string path = "/");

    void subscribeSignal(const StringPtr& signalStringId);
    void unsubscribeSignal(const StringPtr& signalStringId);
    EventPacketPtr getDataDescriptorChangedEventPacket(const StringPtr& signalStringId);

    void sendConfigRequest(const config_protocol::PacketBuffer& packet);
    void sendStreamingRequest();

    std::shared_ptr<boost::asio::io_context> getIoContext();

    void resetStreamingHandlers();
    void setStreamingHandlers(const OnSignalAvailableCallback& signalAvailableHandler,
                              const OnSignalUnavailableCallback& signalUnavailableHandler,
                              const OnPacketCallback& packetHandler,
                              const OnSignalSubscriptionAckCallback& signalSubscriptionAckCallback,
                              const OnConnectionStatusChangedCallback& connectionStatusChangedCb,
                              const OnStreamingInitDoneCallback& streamingInitDoneCb);

    void resetConfigHandlers();
    void setConfigHandlers(const ProcessConfigProtocolPacketCb& configPacketHandler,
                           const OnConnectionStatusChangedCallback& connectionStatusChangedCb);

protected:
    void readTransportLayerProps();
    void initClientSessionHandler(SessionPtr session);
    void initClient(std::string host,
                    std::string port,
                    std::string path);

    void handlePacket(const SignalNumericIdType& signalNumericId, const PacketPtr& packet);
    void handleSignal(const SignalNumericIdType& signalNumericId,
                      const StringPtr& signalStringId,
                      const StringPtr& serializedSignal,
                      bool available);

    void checkReconnectionResult(const boost::system::error_code& ec);
    void tryReconnect();
    void connectionStatusChanged(ClientConnectionStatus status);

    void startTransportOperations();
    void stopTransportOperations();

    enum class ConnectionResult
    {
        Connected = 0,
        ServerUnreachable,
        ServerUnsupported
    };

    ContextPtr context;
    PropertyObjectPtr transportLayerProperties;
    std::shared_ptr<boost::asio::io_context> ioContextPtr;
    std::thread ioThread;
    LoggerPtr logger;
    LoggerComponentPtr loggerComponent;

    // streaming callbacks
    OnSignalAvailableCallback signalAvailableHandler;
    OnSignalUnavailableCallback signalUnavailableHandler;
    OnPacketCallback packetHandler;
    OnSignalSubscriptionAckCallback signalSubscriptionAckCallback;
    OnConnectionStatusChangedCallback connectionStatusChangedStreamingCb;
    OnStreamingInitDoneCallback streamingInitDoneCb;

    // config callbacks
    OnConnectionStatusChangedCallback connectionStatusChangedConfigCb;
    ProcessConfigProtocolPacketCb configPacketHandler;

    std::shared_ptr<boost::asio::steady_timer> reconnectionTimer;

    std::shared_ptr<daq::native_streaming::Client> client;
    std::shared_ptr<ClientSessionHandler> sessionHandler;
    std::promise<ConnectionResult> connectedPromise;
    std::future<ConnectionResult> connectedFuture;

    std::unordered_map<SignalNumericIdType, StringPtr> signalIds;
    std::mutex registeredSignalsSync;

    bool connectionMonitoringEnabled{false};
    Int heartbeatPeriod;
    Int connectionInactivityTimeout;
    std::chrono::milliseconds connectionTimeout;
    std::chrono::milliseconds reconnectionPeriod;
};

END_NAMESPACE_OPENDAQ_NATIVE_STREAMING_PROTOCOL
