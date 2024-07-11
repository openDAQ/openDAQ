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
#include "websocket_streaming/websocket_streaming.h"
#include <opendaq/signal_config_ptr.h>
#include <opendaq/data_descriptor_ptr.h>
#include <opendaq/packet_factory.h>
#include "streaming_protocol/Logging.hpp"
#include "websocket_streaming/signal_info.h"

#include <variant>

BEGIN_NAMESPACE_OPENDAQ_WEBSOCKET_STREAMING

class OutputSignalBase;
using OutputSignalBasePtr = std::shared_ptr<OutputSignalBase>;

class OutputDomainSignalBase;
using OutputDomainSignaBaselPtr = std::shared_ptr<OutputDomainSignalBase>;

class OutputSignalBase
{
public:
    OutputSignalBase(const SignalPtr& signal,
                     const DataDescriptorPtr& domainDescriptor,
                     daq::streaming_protocol::BaseSignalPtr stream,
                     daq::streaming_protocol::LogCallback logCb);
    virtual ~OutputSignalBase();

    virtual void writeDaqPacket(const PacketPtr& packet) = 0;
    virtual void setSubscribed(bool subscribed) = 0;
    virtual bool isDataSignal() = 0;

    SignalPtr getDaqSignal();
    bool isSubscribed();

    void submitTimeConfigChange(const DataDescriptorPtr& domainDescriptor);
    bool isTimeConfigChanged(const DataDescriptorPtr& domainDescriptor);

protected:
    virtual void toStreamedSignal(const SignalPtr& signal, const SignalProps& sigProps) = 0;

    void submitSignalChanges();

    static SignalProps getSignalProps(const SignalPtr& signal);
    void writeDescriptorChangedEvent(const DataDescriptorPtr& descriptor);

    SignalPtr daqSignal;
    SignalConfigPtr streamedDaqSignal; // used as dummy signal for encoding event packets

    daq::streaming_protocol::LogCallback logCallback;

    bool subscribed{false};
    bool doSetStartTime{false};
    std::mutex subscribedSync;
    daq::streaming_protocol::BaseSignalPtr stream;

private:
    void processAttributeChangedCoreEvent(ComponentPtr& component, CoreEventArgsPtr& args);
    void subscribeToCoreEvent();
    void unsubscribeFromCoreEvent();
    void createStreamedSignal();

    DataDescriptorPtr domainDescriptor;
};

class OutputValueSignalBase : public OutputSignalBase
{
public:
    OutputValueSignalBase(daq::streaming_protocol::BaseValueSignalPtr valueStream,
                          const SignalPtr& signal,
                          OutputDomainSignaBaselPtr outputDomainSignal,
                          daq::streaming_protocol::LogCallback logCb);

    void writeDaqPacket(const PacketPtr& packet) override;
    virtual void setSubscribed(bool subscribed) override;
    bool isDataSignal() override;

protected:
    virtual void writeDataPacket(const DataPacketPtr& packet) = 0;
    void toStreamedSignal(const SignalPtr& signal, const SignalProps& sigProps) override;

    OutputDomainSignaBaselPtr outputDomainSignal;

private:
    void writeEventPacket(const EventPacketPtr& packet);
    void writeDescriptorChangedPacket(const EventPacketPtr& packet);

    daq::streaming_protocol::BaseValueSignalPtr valueStream;
};

class OutputDomainSignalBase : public OutputSignalBase
{
public:
    friend class OutputValueSignalBase;
    friend class OutputConstValueSignal;

    OutputDomainSignalBase(daq::streaming_protocol::BaseDomainSignalPtr domainStream,
                       const SignalPtr& signal,
                       daq::streaming_protocol::LogCallback logCb);

    void writeDaqPacket(const PacketPtr& packet) override;
    void setSubscribed(bool subscribed) override;
    bool isDataSignal() override;

    uint64_t calcStartTimeOffset(uint64_t dataPacketTimeStamp);

private:
    void subscribeByDataSignal();
    void unsubscribeByDataSignal();

    size_t subscribedByDataSignalCount{0};
    daq::streaming_protocol::BaseDomainSignalPtr domainStream;
};

class OutputSyncValueSignal : public OutputValueSignalBase
{
public:
    OutputSyncValueSignal(const daq::streaming_protocol::StreamWriterPtr& writer,
                          const SignalPtr& signal,
                          OutputDomainSignaBaselPtr outputDomainSignal,
                          const std::string& tableId,
                          daq::streaming_protocol::LogCallback logCb);

protected:
    void writeDataPacket(const DataPacketPtr& packet) override;

private:
    static daq::streaming_protocol::BaseSynchronousSignalPtr createSignalStream(
        const daq::streaming_protocol::StreamWriterPtr& writer,
        const SignalPtr& signal,
        const std::string& tableId,
        daq::streaming_protocol::LogCallback logCb);

    daq::streaming_protocol::BaseSynchronousSignalPtr syncStream;
};

class OutputConstValueSignal : public OutputValueSignalBase
{
public:
    using ConstantValueType =
        std::variant<int8_t, int16_t , int32_t, int64_t, uint8_t, uint16_t , uint32_t, uint64_t, float, double>;

    OutputConstValueSignal(const daq::streaming_protocol::StreamWriterPtr& writer,
                           const SignalPtr& signal,
                           OutputDomainSignaBaselPtr outputDomainSignal,
                           const std::string& tableId,
                           daq::streaming_protocol::LogCallback logCb);

    void setSubscribed(bool subscribed) override;

protected:
    void writeDataPacket(const DataPacketPtr& packet) override;

private:
    static daq::streaming_protocol::BaseConstantSignalPtr createSignalStream(
        const daq::streaming_protocol::StreamWriterPtr& writer,
        const SignalPtr& signal,
        const std::string& tableId,
        daq::streaming_protocol::LogCallback logCb);

    template <typename DataType>
    static std::vector<std::pair<DataType, uint64_t>>
    extractConstValuesFromDataPacket(const DataPacketPtr& packet);

    template <typename DataType>
    void writeData(const DataPacketPtr& packet, uint64_t firstValueIndex);

    daq::streaming_protocol::BaseConstantSignalPtr constStream;
    std::optional<ConstantValueType> lastConstValue;
};

class OutputLinearDomainSignal : public OutputDomainSignalBase
{
public:
    OutputLinearDomainSignal(const daq::streaming_protocol::StreamWriterPtr& writer,
                             const SignalPtr& signal,
                             const std::string& tableId,
                             daq::streaming_protocol::LogCallback logCb);

protected:
    void toStreamedSignal(const SignalPtr& signal, const SignalProps& sigProps) override;

private:
    static daq::streaming_protocol::LinearTimeSignalPtr createSignalStream(
        const daq::streaming_protocol::StreamWriterPtr& writer,
        const SignalPtr& signal,
        const std::string& tableId,
        daq::streaming_protocol::LogCallback logCb);

    daq::streaming_protocol::LinearTimeSignalPtr linearStream;
};

END_NAMESPACE_OPENDAQ_WEBSOCKET_STREAMING
