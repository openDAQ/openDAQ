/*
 * Copyright 2022-2023 Blueberry d.o.o.
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

enum class ClientReconnectionStatus
{
    Connected = 0,
    Reconnecting,
    Restored,
    Unrecoverable
};

using OnSignalAvailableCallback = std::function<void(const StringPtr& signalStringId,
                                                     const StringPtr& serializedSignal)>;
using OnSignalUnavailableCallback = std::function<void(const StringPtr& signalStringId)>;
using OnPacketCallback = std::function<void(const StringPtr& signalStringId, const PacketPtr& packet)>;
using OnSignalSubscriptionAckCallback = std::function<void(const StringPtr& signalStringId, bool subscribed)>;
using OnReconnectionStatusChangedCallback = std::function<void(ClientReconnectionStatus status)>;

class NativeStreamingClientHandler;
using NativeStreamingClientHandlerPtr = std::shared_ptr<NativeStreamingClientHandler>;

class NativeStreamingClientHandler
{
public:
    explicit NativeStreamingClientHandler(const ContextPtr& context, const PropertyObjectPtr& transportLayerProperties);

    ~NativeStreamingClientHandler();

    bool connect(std::string host,
                 std::string port,
                 std::string path = "/");

    void subscribeSignal(const StringPtr& signalStringId);
    void unsubscribeSignal(const StringPtr& signalStringId);
    EventPacketPtr getDataDescriptorChangedEventPacket(const StringPtr& signalStringId);

    void sendConfigRequest(const config_protocol::PacketBuffer& packet);

    void setIoContext(const std::shared_ptr<boost::asio::io_context>& ioContextPtr);
    void setSignalAvailableHandler(const OnSignalAvailableCallback& signalAvailableHandler);
    void setSignalUnavailableHandler(const OnSignalUnavailableCallback& signalUnavailableHandler);
    void setPacketHandler(const OnPacketCallback& packetHandler);
    void setSignalSubscriptionAckCallback(const OnSignalSubscriptionAckCallback& signalSubscriptionAckCallback);
    void setReconnectionStatusChangedCb(const OnReconnectionStatusChangedCallback& reconnectionStatusChangedCb);
    void setConfigPacketHandler(const ConfigProtocolPacketCb& configPacketHandler);

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

    void checkReconnectionStatus(const boost::system::error_code& ec);
    void checkProtocolInitializationStatus(const boost::system::error_code& ec);
    void tryReconnect();
    bool isProtocolInitialized(std::chrono::milliseconds timeout = std::chrono::milliseconds(0));

    enum class ConnectionResult
    {
        Connected = 0,
        ServerUnreachable,
        ServerUnsupported
    };

    ContextPtr context;
    PropertyObjectPtr transportLayerProperties;
    std::shared_ptr<boost::asio::io_context> ioContextPtr;
    LoggerPtr logger;
    LoggerComponentPtr loggerComponent;
    OnSignalAvailableCallback signalAvailableHandler;
    OnSignalUnavailableCallback signalUnavailableHandler;
    OnPacketCallback packetHandler;
    OnSignalSubscriptionAckCallback signalSubscriptionAckCallback;
    OnReconnectionStatusChangedCallback reconnectionStatusChangedCb;
    ConfigProtocolPacketCb configPacketHandler = [](const config_protocol::PacketBuffer& packet) {};

    std::shared_ptr<boost::asio::steady_timer> reconnectionTimer;
    std::shared_ptr<boost::asio::steady_timer> protocolInitTimer;

    std::shared_ptr<daq::native_streaming::Client> client;
    std::shared_ptr<ClientSessionHandler> sessionHandler;
    std::promise<ConnectionResult> connectedPromise;
    std::future<ConnectionResult> connectedFuture;
    std::promise<void> protocolInitPromise;
    std::future<void> protocolInitFuture;

    std::unordered_map<SignalNumericIdType, StringPtr> signalIds;
    std::mutex sync;

    bool heartbeatEnabled{false};
    Int heartbeatPeriod;
    Int heartbeatTimeout;
    std::chrono::milliseconds connectionTimeout;
    std::chrono::milliseconds streamingInitTimeout;
    std::chrono::milliseconds reconnectionPeriod;
};

END_NAMESPACE_OPENDAQ_NATIVE_STREAMING_PROTOCOL
