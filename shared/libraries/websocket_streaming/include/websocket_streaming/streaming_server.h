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
    using OnStartSignalsReadCallback = std::function<void(const ListPtr<ISignal>& signal)>;
    using OnStopSignalsReadCallback = std::function<void(const ListPtr<ISignal>& signal)>;
    using OnClientConnectedCallback = std::function<void(const std::string& clientId, const std::string& address)>;
    using OnClientDisconnectedCallback = std::function<void(const std::string& clientId)>;

    StreamingServer(const ContextPtr& context);
    ~StreamingServer();

    void start(uint16_t port = daq::streaming_protocol::WEBSOCKET_LISTENING_PORT,
               uint16_t controlPort = daq::streaming_protocol::HTTP_CONTROL_PORT);
    void stop();

    void onAccept(const OnAcceptCallback& callback);
    void onStartSignalsRead(const OnStartSignalsReadCallback& callback);
    void onStopSignalsRead(const OnStopSignalsReadCallback& callback);
    void onClientConnected(const OnClientConnectedCallback& callback);
    void onClientDisconnected(const OnClientDisconnectedCallback& callback);

    void broadcastPacket(const std::string& signalId, const PacketPtr& packet);

    void addSignals(const ListPtr<ISignal>& signals);
    void removeComponentSignals(const StringPtr& componentId);
    void updateComponentSignals(const DictPtr<IString, ISignal>& signals, const StringPtr& componentId);

protected:
    using SignalMap = std::unordered_map<std::string, OutputSignalBasePtr>;
    using ClientMap = std::unordered_map<std::string, std::pair<daq::streaming_protocol::StreamWriterPtr, SignalMap>>;

    void doRead(const std::string& clientId, const stream::StreamPtr& stream);
    void onReadDone(const std::string& clientId,
                    const stream::StreamPtr& stream,
                    const boost::system::error_code& ec,
                    std::size_t bytesRead);
    void removeClient(const std::string& clientId);

    void addToOutputSignals(const SignalPtr& signal,
                            SignalMap& outputSignals,
                            const streaming_protocol::StreamWriterPtr& writer);
    OutputDomainSignalBasePtr addUpdateOrFindDomainSignal(const SignalPtr& domainSignal,
                                                          SignalMap& outputSignals,
                                                          const streaming_protocol::StreamWriterPtr& writer);

    void publishSignalsToClient(const streaming_protocol::StreamWriterPtr& writer,
                                const ListPtr<ISignal>& signals,
                                SignalMap& outputSignals);
    void onAcceptInternal(const daq::stream::StreamPtr& stream);
    OutputDomainSignalBasePtr createOutputDomainSignal(const SignalPtr& daqDomainSignal,
                                                       const std::string& tableId,
                                                       const streaming_protocol::StreamWriterPtr& writer);
    OutputSignalBasePtr createOutputValueSignal(const SignalPtr& daqSignal,
                                                const OutputDomainSignalBasePtr& outputDomainSignal,
                                                const std::string& tableId,
                                                const streaming_protocol::StreamWriterPtr& writer);
    void handleDataDescriptorChanges(OutputSignalBasePtr& outputSignal,
                                     SignalMap& outputSignals,
                                     const streaming_protocol::StreamWriterPtr& writer,
                                     const EventPacketPtr& packet);

    void updateOutputPlaceholderSignal(OutputSignalBasePtr& outputSignal,
                                       SignalMap& outputSignals,
                                       const streaming_protocol::StreamWriterPtr& writer,
                                       bool subscribed);

    void writeProtocolInfo(const daq::streaming_protocol::StreamWriterPtr& writer);
    void writeSignalsAvailable(const daq::streaming_protocol::StreamWriterPtr& writer,
                               const std::vector<std::string>& signalIds);
    void writeSignalsUnavailable(const daq::streaming_protocol::StreamWriterPtr& writer,
                                 const std::vector<std::string>& signalIds);
    void writeInit(const daq::streaming_protocol::StreamWriterPtr& writer);

    bool isSignalSubscribed(const std::string& signalId) const;
    bool subscribeHandler(const std::string& signalId, OutputSignalBasePtr signal);
    bool unsubscribeHandler(const std::string& signalId, OutputSignalBasePtr signal);
    int onControlCommand(const std::string& streamId,
                         const std::string& command,
                         const daq::streaming_protocol::SignalIds& signalIds,
                         std::string& errorMessage);

    void startReadSignals(const ListPtr<ISignal>& signals);
    void stopReadSignals(const ListPtr<ISignal>& signals);

    uint16_t port;
    boost::asio::io_context ioContext;
    boost::asio::executor_work_guard<boost::asio::io_context::executor_type> work;
    daq::stream::WebsocketServerUniquePtr server;
    std::unique_ptr<daq::streaming_protocol::ControlServer> controlServer;
    std::thread serverThread;
    ClientMap clients;
    OnAcceptCallback onAcceptCallback;
    OnStartSignalsReadCallback onStartSignalsReadCallback;
    OnStopSignalsReadCallback onStopSignalsReadCallback;
    OnClientConnectedCallback clientConnectedHandler;
    OnClientDisconnectedCallback clientDisconnectedHandler;
    LoggerPtr logger;
    LoggerComponentPtr loggerComponent;
    daq::streaming_protocol::LogCallback logCallback;
    bool serverRunning{false};
    std::mutex sync;

private:
    static DataRuleType getSignalRuleType(const SignalPtr& domainSignal);
};

END_NAMESPACE_OPENDAQ_WEBSOCKET_STREAMING
