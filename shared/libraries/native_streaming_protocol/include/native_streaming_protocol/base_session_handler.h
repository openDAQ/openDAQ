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

#include <native_streaming_protocol/native_streaming_protocol_types.h>
#include <native_streaming/session.hpp>

#include <opendaq/context_ptr.h>
#include <coreobjects/property_object_ptr.h>

#include <config_protocol/config_protocol.h>
#include <packet_streaming/packet_streaming.h>

BEGIN_NAMESPACE_OPENDAQ_NATIVE_STREAMING_PROTOCOL

static const SizeT UNLIMITED_PACKET_SEND_TIME = 0;

class BaseSessionHandler: public std::enable_shared_from_this<BaseSessionHandler>
{
public:
    BaseSessionHandler(const ContextPtr& daqContext,
                       SessionPtr session,
                       const std::shared_ptr<boost::asio::io_context>& ioContextPtr,
                       native_streaming::OnSessionErrorCallback errorHandler,
                       OnSignalCallback signalReceivedHandler,
                       OnSubscriptionAckCallback subscriptionAckHandler,
                       OnFindSignalCallback findSignalHandler,
                       OnSignalSubscriptionCallback signalSubscriptionHandler,
                       ConstCharPtr loggerComponentName,
                       SizeT streamingPacketSendTimeout = UNLIMITED_PACKET_SEND_TIME);
    virtual ~BaseSessionHandler();

    void startReading();
    const SessionPtr getSession() const;
    void sendConfigurationPacket(const config_protocol::PacketBuffer& packet);
    void sendPacketBuffer(packet_streaming::PacketBufferPtr&& packetBuffer);

    void schedulePacketBufferWriteTasks(std::vector<daq::native_streaming::WriteTask>&& tasks,
                                        std::optional<std::chrono::steady_clock::time_point>&& timeStamp);
    static void createAndPushPacketBufferTasks(packet_streaming::PacketBufferPtr&& packetBuffer,
                                               std::vector<daq::native_streaming::WriteTask>& tasks);
    static void copyHeadersToBuffer(const packet_streaming::PacketBufferPtr& packetBuffer, char* bufferDestPtr);

    void setConfigPacketReceivedHandler(const ProcessConfigProtocolPacketCb& configPacketReceivedHandler);
    void setPacketBufferReceivedHandler(const OnPacketBufferReceivedCallback& packetBufferReceivedHandler);

    void startConnectionActivityMonitoring(Int period, Int timeout);

    void sendSignalAvailable(const SignalNumericIdType& signalNumericId, const SignalPtr& signal);
    void sendSignalUnavailable(const SignalNumericIdType& signalNumericId, const SignalPtr& signal);
    void sendSubscribingDone(const SignalNumericIdType signalNumericId);
    void sendUnsubscribingDone(const SignalNumericIdType signalNumericId);
    void sendSignalSubscribe(const SignalNumericIdType& signalNumericId, const std::string& signalStringId);
    void sendSignalUnsubscribe(const SignalNumericIdType& signalNumericId, const std::string& signalStringId);

protected:
    virtual daq::native_streaming::ReadTask readHeader(const void* data, size_t size);
    daq::native_streaming::ReadTask readConfigurationPacket(const void *data, size_t size);
    daq::native_streaming::ReadTask readPacketBuffer(const void* data, size_t size);

    daq::native_streaming::ReadTask createReadHeaderTask();
    daq::native_streaming::ReadTask createReadStopTask();
    daq::native_streaming::ReadTask discardPayload(const void* data, size_t size);

    static size_t calculatePayloadSize(const std::vector<daq::native_streaming::WriteTask>& writePayloadTasks);
    static daq::native_streaming::WriteTask createWriteHeaderTask(PayloadType payloadType, size_t payloadSize);
    static daq::native_streaming::WriteTask createWriteStringTask(const std::string& str);

    template<typename T>
    daq::native_streaming::WriteTask createWriteNumberTask(const T& value)
    {
        auto valueCopy = std::make_shared<T>(value);
        boost::asio::const_buffer valuePayload(valueCopy.get(), sizeof(T));
        daq::native_streaming::WriteHandler valuePayloadHandler = [valueCopy]() {};
        return daq::native_streaming::WriteTask(valuePayload, valuePayloadHandler);
    }

    static void copyData(void* destination, const void* source, size_t bytesToCopy, size_t sourceOffset, size_t sourceSize);
    static std::string getStringFromData(const void* source, size_t stringSize, size_t sourceOffset, size_t sourceSize);

    daq::native_streaming::ReadTask readSignalAvailable(const void* data, size_t size);
    daq::native_streaming::ReadTask readSignalUnavailable(const void* data, size_t size);
    daq::native_streaming::ReadTask readSignalSubscribedAck(const void* data, size_t size);
    daq::native_streaming::ReadTask readSignalUnsubscribedAck(const void* data, size_t size);
    daq::native_streaming::ReadTask readSignalSubscribe(const void* data, size_t size);
    daq::native_streaming::ReadTask readSignalUnsubscribe(const void* data, size_t size);

    virtual bool hasUserAccessToSignal(const SignalPtr& signal);
    virtual std::string getClientId();

    SessionPtr session;
    ProcessConfigProtocolPacketCb configPacketReceivedHandler;
    OnPacketBufferReceivedCallback packetBufferReceivedHandler;
    native_streaming::OnSessionErrorCallback errorHandler;
    std::shared_ptr<boost::asio::io_context> ioContextPtr;
    std::shared_ptr<boost::asio::steady_timer> connectionInactivityTimer;
    LoggerComponentPtr loggerComponent;
    bool connectionActivityMonitoringStarted{false};
    std::chrono::milliseconds streamingPacketSendTimeout;

    OnSignalCallback signalReceivedHandler;
    OnSubscriptionAckCallback subscriptionAckHandler;
    OnFindSignalCallback findSignalHandler;
    OnSignalSubscriptionCallback signalSubscriptionHandler;
};
END_NAMESPACE_OPENDAQ_NATIVE_STREAMING_PROTOCOL
