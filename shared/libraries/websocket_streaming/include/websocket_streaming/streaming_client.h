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
#include <opendaq/device_ptr.h>
#include <string>
#include <thread>
#include "websocket_streaming/websocket_streaming.h"
#include "websocket_streaming/input_signal.h"
#include "websocket_streaming/signal_info.h"
#include "stream/WebsocketClientStream.hpp"
#include "streaming_protocol/ProtocolHandler.hpp"
#include "streaming_protocol/SubscribedSignal.hpp"
#include "opendaq/signal_factory.h"
#include "streaming_protocol/Logging.hpp"
#include <opendaq/context_ptr.h>
#include <opendaq/logger_ptr.h>
#include <opendaq/logger_component_ptr.h>
#include <opendaq/event_packet_ptr.h>

#include <condition_variable>
#include <mutex>
#include <future>

BEGIN_NAMESPACE_OPENDAQ_WEBSOCKET_STREAMING

class StreamingClient;
using StreamingClientPtr = std::shared_ptr<StreamingClient>;

class StreamingClient
{
public:
    using OnPacketCallback = std::function<void(const StringPtr& signalId, const PacketPtr& packet)>;
    using OnSignalCallback = std::function<void(const StringPtr& signalId, const SubscribedSignalInfo& signalInfo)>;
    using OnFindSignalCallback = std::function<SignalConfigPtr(const StringPtr& signalId)>;
    using OnDomainDescriptorCallback =
        std::function<void(const StringPtr& signalId, const DataDescriptorPtr& domainDescriptor)>;
    using OnAvailableSignalsCallback = std::function<void(const std::vector<std::string>& signalIds)>;
    using OnSubsciptionAckCallback = std::function<void(const std::string& signalId, bool subscribed)>;

    StreamingClient(const ContextPtr& context, const std::string& connectionString, bool useRawTcpConnection = false);
    StreamingClient(const ContextPtr& context, const std::string& host, uint16_t port, const std::string& target, bool useRawTcpConnection = false);
    ~StreamingClient();

    bool connect();
    void disconnect();
    void onPacket(const OnPacketCallback& callack);
    void onSignalInit(const OnSignalCallback& callback);
    void onSignalUpdated(const OnSignalCallback& callback);
    void onDomainDescriptor(const OnDomainDescriptorCallback& callback);
    void onAvailableStreamingSignals(const OnAvailableSignalsCallback& callback);
    void onAvailableDeviceSignals(const OnAvailableSignalsCallback& callback);
    void onFindSignal(const OnFindSignalCallback& callback);
    void onSubscriptionAck(const OnSubsciptionAckCallback& callback);
    std::string getHost();
    uint16_t getPort();
    std::string getTarget();
    bool isConnected();
    void setConnectTimeout(std::chrono::milliseconds timeout);
    EventPacketPtr getDataDescriptorChangedEventPacket(const StringPtr& signalStringId);
    void subscribeSignals(const std::vector<std::string>& signalIds);
    void unsubscribeSignals(const std::vector<std::string>& signalIds);

protected:
    void parseConnectionString(const std::string& url);
    void onSignalMeta(const daq::streaming_protocol::SubscribedSignal& subscribedSignal,
                      const std::string& method,
                      const nlohmann::json& params);
    void onProtocolMeta(daq::streaming_protocol::ProtocolHandler& protocolHandler, const std::string& method, const nlohmann::json& params);
    void onMessage(const daq::streaming_protocol::SubscribedSignal& subscribedSignal, uint64_t timeStamp, const uint8_t* data, size_t size);
    void setDataSignal(const daq::streaming_protocol::SubscribedSignal& subscribedSignal);
    void setTimeSignal(const daq::streaming_protocol::SubscribedSignal& subscribedSignal);
    void publishSignalChanges(const std::string& signalId, const InputSignalPtr& signal);
    void onSignal(const daq::streaming_protocol::SubscribedSignal& subscribedSignal, const nlohmann::json& params);
    void setSignalInitSatisfied(const std::string& signalId);
    void setDomainDescriptor(const std::string& signalId,
                             const InputSignalPtr& inputSignal,
                             const DataDescriptorPtr& domainDescriptor);
    std::pair<std::string, InputSignalPtr> findSignalByTableId(const std::string& tableId);

    LoggerPtr logger;
    LoggerComponentPtr loggerComponent;
    daq::streaming_protocol::LogCallback logCallback;

    std::string host;
    uint16_t port;
    std::string target;
    bool connected = false;
    boost::asio::io_context ioContext;
    daq::streaming_protocol::SignalContainer signalContainer;
    daq::streaming_protocol::ProtocolHanlderPtr protocolHandler;
    std::unordered_map<std::string, InputSignalPtr> signals;
    OnPacketCallback onPacketCallback = [](const StringPtr&, const PacketPtr&) {};
    OnSignalCallback onSignalInitCallback = [](const StringPtr&, const SubscribedSignalInfo&) {};
    OnDomainDescriptorCallback onDomainDescriptorCallback = [](const StringPtr&, const DataDescriptorPtr&) {};
    OnAvailableSignalsCallback onAvailableStreamingSignalsCb = [](const std::vector<std::string>& signalIds) {};
    OnAvailableSignalsCallback onAvailableDeviceSignalsCb = [](const std::vector<std::string>& signalIds) {};
    OnFindSignalCallback onFindSignalCallback = [](const StringPtr& signalId) { return nullptr; };
    OnSignalCallback onSignalUpdatedCallback = [](const StringPtr& signalId, const SubscribedSignalInfo&) {};
    OnSubsciptionAckCallback onSubscriptionAckCallback = [](const StringPtr& signalId, bool subscribed) {};
    std::thread clientThread;
    std::mutex clientMutex;
    std::condition_variable conditionVariable;
    std::chrono::milliseconds connectTimeout{1000};

    // signal meta-information (signal description, tableId, related signals, etc.)
    // is published only for subscribed signals.
    // as workaround we temporarily subscribe all signals to receive signal meta-info
    // at initialization stage.
    // To manage this the 'signalInitializedStatus' is used, it is map of 4-element tuples, where:
    // 1-st is std::promise
    // 2-nd is std::future
    // 3-rd: boolean flag indicating that initial subscription completion ack is filtered-out
    // 4-th: boolean flag indicating that initial unsubscription completion ack is filtered-out
    std::unordered_map<std::string, std::tuple<std::promise<void>, std::future<void>, bool, bool>> signalInitializedStatus;

    std::unordered_map<std::string, DataDescriptorPtr> cachedDomainDescriptors;
    bool useRawTcpConnection;
};

END_NAMESPACE_OPENDAQ_WEBSOCKET_STREAMING
