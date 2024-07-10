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

#include <native_streaming_protocol/native_streaming_protocol_types.h>
#include <native_streaming/session.hpp>

#include <opendaq/context_ptr.h>
#include <coreobjects/property_object_ptr.h>

#include <config_protocol/config_protocol.h>

BEGIN_NAMESPACE_OPENDAQ_NATIVE_STREAMING_PROTOCOL

class BaseSessionHandler
{
public:
    BaseSessionHandler(const ContextPtr& daqContext,
                       SessionPtr session,
                       const std::shared_ptr<boost::asio::io_context>& ioContextPtr,
                       native_streaming::OnSessionErrorCallback errorHandler,
                       ConstCharPtr loggerComponentName);
    virtual ~BaseSessionHandler();

    void startReading();
    const SessionPtr getSession() const;
    void sendConfigurationPacket(const config_protocol::PacketBuffer& packet);

    void setConfigPacketReceivedHandler(const ProcessConfigProtocolPacketCb& configPacketReceivedHandler);
    void startConnectionActivityMonitoring(Int period, Int timeout);

protected:
    virtual daq::native_streaming::ReadTask readHeader(const void* data, size_t size);
    daq::native_streaming::ReadTask readConfigurationPacket(const void *data, size_t size);

    daq::native_streaming::ReadTask createReadHeaderTask();
    daq::native_streaming::ReadTask createReadStopTask();
    daq::native_streaming::ReadTask discardPayload(const void* data, size_t size);

    size_t calculatePayloadSize(const std::vector<daq::native_streaming::WriteTask>& writePayloadTasks);
    daq::native_streaming::WriteTask createWriteHeaderTask(PayloadType payloadType, size_t payloadSize);
    daq::native_streaming::WriteTask createWriteStringTask(const std::string& str);

    template<typename T>
    daq::native_streaming::WriteTask createWriteNumberTask(const T& value)
    {
        auto valueCopy = std::make_shared<T>(value);
        boost::asio::const_buffer valuePayload(valueCopy.get(), sizeof(T));
        daq::native_streaming::WriteHandler valuePayloadHandler = [valueCopy]() {};
        return daq::native_streaming::WriteTask(valuePayload, valuePayloadHandler);
    }

    static void copyData(void* destination, const void* source, size_t bytesToCopy, size_t sourceOffset, size_t sourceSize);
    static std::string getStringFromData(const void *source, size_t stringSize, size_t sourceOffset, size_t sourceSize);

    SessionPtr session;
    ProcessConfigProtocolPacketCb configPacketReceivedHandler;
    native_streaming::OnSessionErrorCallback errorHandler;
    std::shared_ptr<boost::asio::io_context> ioContextPtr;
    std::shared_ptr<boost::asio::steady_timer> connectionInactivityTimer;
    LoggerComponentPtr loggerComponent;
    bool connectionActivityMonitoringStarted{false};
};
END_NAMESPACE_OPENDAQ_NATIVE_STREAMING_PROTOCOL
