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

namespace streaming_test_helpers
{

/// This alternative implementation of the Streaming Server is designed for testing purposes.
/// It is intended to bypass the `daq::streaming_protocol::BaseSignal` library interface,
/// preventing the generation of artificial time signals and enabling the sharing of the same
/// domain signal among multiple data signals.
///
/// Limitations and requirements:
/// - It does not implement the transfer of signal data; only the publication of
///   signal meta-information is supported.
/// - To simplify the implementation, it exclusively handles data signals
///   with explicit rule and a data type of float64, along with domain signals
///   having a linear rule and unsigned64 type.
/// - The domain signal must be published as available.
/// - The `tableId` for the signal is set using the StringProperty named "tableId,"
///   and it is expected that all published signals have this property.

class MockStreamingServer;
using MockStreamingServerPtr = std::shared_ptr<MockStreamingServer>;

class MockStreamingServer
{
public:
    using OnAcceptCallback = std::function<daq::ListPtr<daq::ISignal>()>;

    MockStreamingServer(const daq::ContextPtr& context);
    ~MockStreamingServer();

    void start(uint16_t port = daq::streaming_protocol::WEBSOCKET_LISTENING_PORT,
               uint16_t controlPort = daq::streaming_protocol::HTTP_CONTROL_PORT);
    void stop();
    void onAccept(const OnAcceptCallback& callback);

protected:
    using SignalMap = std::unordered_map<std::string, std::pair<daq::SizeT, daq::SignalPtr>>;
    using ClientMap = std::unordered_map<std::string, std::pair<daq::streaming_protocol::StreamWriterPtr, SignalMap>>;

    void onAcceptInternal(const daq::stream::StreamPtr& stream);
    void writeProtocolInfo(const daq::streaming_protocol::StreamWriterPtr& writer);
    void writeSignalsAvailable(const daq::streaming_protocol::StreamWriterPtr& writer,
                               const daq::ListPtr<daq::ISignal>& signals);
    void writeInit(const daq::streaming_protocol::StreamWriterPtr& writer);

    void writeSignalSubscribe(const std::string& signalId,
                              daq::SizeT signalNumber,
                              const daq::streaming_protocol::StreamWriterPtr& writer);
    void writeSignalUnsubscribe(const std::string& signalId,
                                daq::SizeT signalNumber,
                                const daq::streaming_protocol::StreamWriterPtr& writer);

    void writeDataSignalMeta(const std::string& tableId,
                             daq::SizeT signalNumber,
                             const daq::SignalPtr& signal,
                             const daq::streaming_protocol::StreamWriterPtr& writer);
    void writeTimeSignalMeta(const std::string& tableId,
                             daq::SizeT signalNumber,
                             const daq::SignalPtr& signal,
                             const daq::streaming_protocol::StreamWriterPtr& writer);

    int onControlCommand(const std::string& streamId,
                         const std::string& command,
                         const daq::streaming_protocol::SignalIds& signalIds,
                         std::string& errorMessage);
    void subscribeSignals(const daq::streaming_protocol::SignalIds& signalIds,
                          const SignalMap& signalMap,
                          const daq::streaming_protocol::StreamWriterPtr& writer);
    void unsubscribeSignals(const daq::streaming_protocol::SignalIds& signalIds,
                            const SignalMap& signalMap,
                            const daq::streaming_protocol::StreamWriterPtr& writer);

    void writeTime(daq::SizeT signalNumber,
                   uint64_t timeTicks,
                   const daq::streaming_protocol::StreamWriterPtr& writer);

    uint16_t port;
    boost::asio::io_context ioContext;
    boost::asio::executor_work_guard<boost::asio::io_context::executor_type> work;
    daq::stream::WebsocketServerUniquePtr server;
    std::unique_ptr<daq::streaming_protocol::ControlServer> controlServer;
    std::thread serverThread;
    ClientMap clients;
    OnAcceptCallback onAcceptCallback;

    daq::LoggerPtr logger;
    daq::LoggerComponentPtr loggerComponent;
    daq::streaming_protocol::LogCallback logCallback;
};

}
