/*
 * Copyright 2022-2024 openDAQ d.o.o.
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

#include <native_streaming_protocol/server_session_handler.h>
#include <native_streaming_protocol/streaming_manager.h>

#include <opendaq/context_ptr.h>
#include <opendaq/logger_ptr.h>
#include <opendaq/logger_component_ptr.h>
#include <opendaq/signal_ptr.h>

#include <native_streaming/server.hpp>

BEGIN_NAMESPACE_OPENDAQ_NATIVE_STREAMING_PROTOCOL

static const SizeT UNLIMITED_CONFIGURATION_CONNECTIONS = 0;

using OnSignalSubscribedCallback = std::function<void(const SignalPtr& signal)>;
using OnSignalUnsubscribedCallback = std::function<void(const SignalPtr& signal)>;

using ConfigServerCallbacks = std::pair<ProcessConfigProtocolPacketCb, OnPacketBufferReceivedCallback>;
using SetUpConfigProtocolServerCb = std::function<ConfigServerCallbacks(SendConfigProtocolPacketCb cb, const UserPtr& user)>;

class NativeStreamingServerHandler : public std::enable_shared_from_this<NativeStreamingServerHandler>
{
public:
    explicit NativeStreamingServerHandler(const ContextPtr& context,
                                          std::shared_ptr<boost::asio::io_context> ioContextPtr,
                                          const ListPtr<ISignal>& signalsList,
                                          OnSignalSubscribedCallback signalSubscribedHandler,
                                          OnSignalUnsubscribedCallback signalUnsubscribedHandler,
                                          SetUpConfigProtocolServerCb setUpConfigProtocolServerCb,
                                          SizeT maxAllowedConfigConnections = UNLIMITED_CONFIGURATION_CONNECTIONS);
    ~NativeStreamingServerHandler() = default;

    void startServer(uint16_t port);
    void stopServer();

    void addSignal(const SignalPtr& signal);
    void removeComponentSignals(const StringPtr& componentId);

    void sendPacket(const SignalPtr& signal, PacketPtr&& packet);

protected:
    void initSessionHandler(SessionPtr session);
    void handleTransportLayerProps(const PropertyObjectPtr& propertyObject, std::shared_ptr<ServerSessionHandler> sessionHandler);
    void setUpTransportLayerPropsCallback(std::shared_ptr<ServerSessionHandler> sessionHandler);
    void setUpConfigProtocolCallbacks(std::shared_ptr<ServerSessionHandler> sessionHandler,
                                      config_protocol::PacketBuffer&& firstPacketBuffer);
    void connectConfigProtocol(std::shared_ptr<ServerSessionHandler> sessionHandler,
                               config_protocol::PacketBuffer&& firstPacketBuffer);
    void setUpStreamingInitCallback(std::shared_ptr<ServerSessionHandler> sessionHandler);
    void releaseSessionHandler(SessionPtr session);
    void handleStreamingInit(std::shared_ptr<ServerSessionHandler> sessionHandler);
    bool handleSignalSubscription(const SignalNumericIdType& signalNumericId,
                                  const SignalPtr& signal,
                                  bool subscribe,
                                  const std::string& clientId);
    bool onAuthenticate(const daq::native_streaming::Authentication& authentication, std::shared_ptr<void>& userContextOut);
    void onSessionError(const std::string &errorMessage, SessionPtr session);

    ContextPtr context;
    std::shared_ptr<boost::asio::io_context> ioContextPtr;
    LoggerPtr logger;
    LoggerComponentPtr loggerComponent;

    std::shared_ptr<daq::native_streaming::Server> server;
    StreamingManager streamingManager;

    using Clients = std::unordered_map<std::string, std::shared_ptr<ServerSessionHandler>>;
    Clients sessionHandlers;

    OnSignalSubscribedCallback signalSubscribedHandler;
    OnSignalUnsubscribedCallback signalUnsubscribedHandler;
    SetUpConfigProtocolServerCb setUpConfigProtocolServerCb;

    std::mutex sync;
    size_t connectedClientIndex;

    SizeT maxAllowedConfigConnections;
    SizeT configConnectionsCount;
};

END_NAMESPACE_OPENDAQ_NATIVE_STREAMING_PROTOCOL
