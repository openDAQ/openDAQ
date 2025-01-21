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

#include <native_streaming_protocol/native_streaming_protocol.h>
#include <native_streaming_protocol/client_session_handler.h>

#include <opendaq/context_ptr.h>
#include <opendaq/logger_ptr.h>
#include <opendaq/logger_component_ptr.h>
#include <opendaq/packet_ptr.h>

#include <native_streaming/client.hpp>

#include <packet_streaming/packet_streaming_client.h>
#include <packet_streaming/packet_streaming_server.h>

#include <future>

BEGIN_NAMESPACE_OPENDAQ_NATIVE_STREAMING_PROTOCOL


using OnSignalAvailableCallback = std::function<void(const StringPtr& signalStringId,
                                                     const StringPtr& serializedSignal)>;
using OnSignalUnavailableCallback = std::function<void(const StringPtr& signalStringId)>;
using OnPacketCallback = std::function<void(const StringPtr& signalStringId, const PacketPtr& packet)>;
using OnSignalSubscriptionAckCallback = std::function<void(const StringPtr& signalStringId, bool subscribed)>;
using OnConnectionStatusChangedCallback = std::function<void(const EnumerationPtr& status)>;

class NativeStreamingClientHandler;
using NativeStreamingClientHandlerPtr = std::shared_ptr<NativeStreamingClientHandler>;

class NativeStreamingClientImpl : public std::enable_shared_from_this<NativeStreamingClientImpl>
{
public:
    explicit NativeStreamingClientImpl(const ContextPtr& context,
                                       const PropertyObjectPtr& transportLayerProperties,
                                       const PropertyObjectPtr& authenticationObject,
                                       const std::shared_ptr<boost::asio::io_context>& ioContextPtr);

    ~NativeStreamingClientImpl();

    bool connect(std::string host,
                 std::string port,
                 std::string path = "/");

    void subscribeSignal(const StringPtr& signalStringId);
    void unsubscribeSignal(const StringPtr& signalStringId);

    void sendConfigRequest(const config_protocol::PacketBuffer& packet);
    void sendStreamingRequest();
    void sendStreamingPacket(SignalNumericIdType signalNumericId, const PacketPtr& packet);

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
    void manageTransportLayerProps();
    void initClientSessionHandler(SessionPtr session);
    daq::native_streaming::Authentication initClientAuthenticationObject(const PropertyObjectPtr& authenticationObject);
    void initClient(std::string host,
                    std::string port,
                    std::string path);

    void handleSignal(const SignalNumericIdType& signalNumericId,
                      const StringPtr& signalStringId,
                      const StringPtr& serializedSignal,
                      bool available);

    void checkReconnectionResult(const boost::system::error_code& ec);
    void tryReconnect();
    void connectionStatusChanged(const EnumerationPtr& status);

    enum class ConnectionResult
    {
        Connected = 0,
        ServerUnreachable,
        ServerUnsupported
    };

    void onConnectionFailed(const std::string& errorMessage, const ConnectionResult result);
    void onSessionError(const std::string& errorMessage, SessionPtr session);
    void onPacketBufferReceived(const packet_streaming::PacketBufferPtr& packetBuffer);

    ContextPtr context;
    PropertyObjectPtr transportLayerProperties;
    PropertyObjectPtr authenticationObject;
    std::shared_ptr<boost::asio::io_context> ioContextPtr;
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

    // device to client streaming consumer
    std::shared_ptr<packet_streaming::PacketStreamingClient> packetStreamingClientPtr;

    // client to device streaming streaming producer
    std::shared_ptr<packet_streaming::PacketStreamingServer> packetStreamingServerPtr;

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

// wraps transport IO operations' context & thread and client handler
class NativeStreamingClientHandler
{
public:
    explicit NativeStreamingClientHandler(const ContextPtr& context,
                                          const PropertyObjectPtr& transportLayerProperties,
                                          const PropertyObjectPtr& authenticationObject);

    ~NativeStreamingClientHandler();

    std::shared_ptr<boost::asio::io_context> getIoContext();

    // simply forwards calls to client handler
    bool connect(std::string host, std::string port, std::string path = "/");

    void subscribeSignal(const StringPtr& signalStringId);
    void unsubscribeSignal(const StringPtr& signalStringId);

    void sendConfigRequest(const config_protocol::PacketBuffer& packet);
    void sendStreamingRequest();
    void sendStreamingPacket(SignalNumericIdType signalNumericId, const PacketPtr& packet);

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
    void startTransportOperations();
    void stopTransportOperations();

    std::shared_ptr<boost::asio::io_context> ioContextPtr;
    std::thread ioThread;
    LoggerComponentPtr loggerComponent;
    std::shared_ptr<NativeStreamingClientImpl> clientHandlerPtr;
};

END_NAMESPACE_OPENDAQ_NATIVE_STREAMING_PROTOCOL
