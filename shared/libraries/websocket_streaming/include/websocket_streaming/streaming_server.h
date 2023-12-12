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
#include "websocket_streaming/websocket_streaming.h"
#include "stream/WebsocketServer.hpp"
#include "websocket_streaming/output_signal.h"
#include "streaming_protocol/StreamWriter.h"
#include "streaming_protocol/ControlServer.hpp"
#include "streaming_protocol/Logging.hpp"
#include <thread>
#include <boost/asio/io_context.hpp>
#include <functional>

#include <opendaq/data_packet_ptr.h>
#include <opendaq/event_packet_ptr.h>

#include <opendaq/context_ptr.h>
#include <opendaq/logger_ptr.h>
#include <opendaq/logger_component_ptr.h>


BEGIN_NAMESPACE_OPENDAQ_WEBSOCKET_STREAMING

class StreamingServer;
using StreamingServerPtr = std::shared_ptr<StreamingServer>;

class StreamingServer
{
public:
    using OnAcceptCallback = std::function<ListPtr<ISignal>(const daq::streaming_protocol::StreamWriterPtr& writer)>;
    using OnSubscribeCallback = std::function<void(const daq::SignalPtr& signal)>;
    using OnUnsubscribeCallback = std::function<void(const daq::SignalPtr& signal)>;

    StreamingServer(const ContextPtr& context);
    ~StreamingServer();

    void start(uint16_t port = daq::streaming_protocol::WEBSOCKET_LISTENING_PORT,
               uint16_t controlPort = daq::streaming_protocol::HTTP_CONTROL_PORT);
    void stop();
    void onAccept(const OnAcceptCallback& callback);
    void onSubscribe(const OnSubscribeCallback& callback);
    void onUnsubscribe(const OnUnsubscribeCallback& callback);
    void unicastPacket(const daq::streaming_protocol::StreamWriterPtr& client, const std::string& signalId, const PacketPtr& packet);
    void broadcastPacket(const std::string& signalId, const PacketPtr &packet);
    void sendPacketToSubscribers(const std::string& signalId, const PacketPtr& packet);

protected:
    using SignalMap = std::unordered_map<std::string, OutputSignalPtr>;
    using ClientMap = std::unordered_map<daq::streaming_protocol::StreamWriterPtr, SignalMap>;

    void onAcceptInternal(const daq::stream::StreamPtr& stream);
    void writeProtocolInfo(const daq::streaming_protocol::StreamWriterPtr& writer);
    void writeSignalsAvailable(const daq::streaming_protocol::StreamWriterPtr& writer, const ListPtr<ISignal>& signals);
    void writeInit(const daq::streaming_protocol::StreamWriterPtr& writer);
    bool isSignalSubscribed(const std::string& signalId) const;
    void subscribeHandler(const std::string& signalId, OutputSignalPtr signal);
    void unsubscribeHandler(const std::string& signalId, OutputSignalPtr signal);
    int onControlCommand(const std::string& streamId,
                         const std::string& command,
                         const daq::streaming_protocol::SignalIds& signalIds,
                         std::string& errorMessage);

    uint16_t port;
    boost::asio::io_context ioContext;
    boost::asio::executor_work_guard<boost::asio::io_context::executor_type> work;
    daq::stream::WebsocketServerUniquePtr server;
    std::unique_ptr<daq::streaming_protocol::ControlServer> controlServer;
    std::thread serverThread;
    ClientMap clients;
    OnAcceptCallback onAcceptCallback;
    OnSubscribeCallback onSubscribeCallback;
    OnUnsubscribeCallback onUnsubscribeCallback;
    LoggerPtr logger;
    LoggerComponentPtr loggerComponent;
    daq::streaming_protocol::LogCallback logCallback;
};

END_NAMESPACE_OPENDAQ_WEBSOCKET_STREAMING
