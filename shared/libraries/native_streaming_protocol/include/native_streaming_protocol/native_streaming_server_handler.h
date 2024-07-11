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

using OnSignalSubscribedCallback = std::function<void(const SignalPtr& signal)>;
using OnSignalUnsubscribedCallback = std::function<void(const SignalPtr& signal)>;

using ConfigServerCallbacks = std::pair<ProcessConfigProtocolPacketCb, OnPacketBufferReceivedCallback>;
using SetUpConfigProtocolServerCb = std::function<ConfigServerCallbacks(SendConfigProtocolPacketCb cb)>;

class NativeStreamingServerHandler
{
public:
    explicit NativeStreamingServerHandler(const ContextPtr& context,
                                          std::shared_ptr<boost::asio::io_context> ioContextPtr,
                                          const ListPtr<ISignal>& signalsList,
                                          OnSignalSubscribedCallback signalSubscribedHandler,
                                          OnSignalUnsubscribedCallback signalUnsubscribedHandler,
                                          SetUpConfigProtocolServerCb setUpConfigProtocolServerCb);
    ~NativeStreamingServerHandler() = default;

    void startServer(uint16_t port);
    void stopServer();

    void addSignal(const SignalPtr& signal);
    void removeComponentSignals(const StringPtr& componentId);

    void sendPacket(const SignalPtr& signal, const PacketPtr& packet);

protected:
    void initSessionHandler(SessionPtr session);
    void handleTransportLayerProps(const PropertyObjectPtr& propertyObject, std::shared_ptr<ServerSessionHandler> sessionHandler);
    void setUpTransportLayerPropsCallback(std::shared_ptr<ServerSessionHandler> sessionHandler);
    void setUpConfigProtocolCallbacks(std::shared_ptr<ServerSessionHandler> sessionHandler);
    void setUpStreamingInitCallback(std::shared_ptr<ServerSessionHandler> sessionHandler);
    void releaseSessionHandler(SessionPtr session);
    void handleStreamingInit(std::shared_ptr<ServerSessionHandler> sessionHandler);
    bool handleSignalSubscription(const SignalNumericIdType& signalNumericId,
                                  const std::string& signalStringId,
                                  bool subscribe,
                                  const std::string& clientId);
    bool onAuthenticate(const daq::native_streaming::Authentication& authentication);

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
};

END_NAMESPACE_OPENDAQ_NATIVE_STREAMING_PROTOCOL
