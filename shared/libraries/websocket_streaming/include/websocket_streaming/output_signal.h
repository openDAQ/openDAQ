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
#include <opendaq/signal_config_ptr.h>
#include "streaming_protocol/BaseSynchronousSignal.hpp"
#include <opendaq/data_descriptor_ptr.h>
#include <opendaq/packet_factory.h>
#include "streaming_protocol/Logging.hpp"

BEGIN_NAMESPACE_OPENDAQ_WEBSOCKET_STREAMING

class OutputSignal;
using OutputSignalPtr = std::shared_ptr<OutputSignal>;

class OutputSignal
{
public:
    using SignalStreamPtr = std::shared_ptr<daq::streaming_protocol::BaseSynchronousSignal>;

    OutputSignal(const daq::stream::StreamPtr& stream, const SignalPtr& signal,
                 daq::streaming_protocol::LogCallback logCb);
    OutputSignal(const daq::streaming_protocol::StreamWriterPtr& writer, const SignalPtr& signal,
                 daq::streaming_protocol::LogCallback logCb);

    virtual void write(const PacketPtr& packet);
    virtual void write(const void* data, size_t sampleCount);
    SignalPtr getCoreSignal();
    void setSubscribed(bool subscribed);
    bool isSubscribed();

protected:
    DataDescriptorPtr getValueDescriptor();
    DataDescriptorPtr getDomainDescriptor();
    uint64_t getRuleDelta();
    uint64_t getTickResolution();
    void createSignalStream();
    void createStreamedSignal();
    void subscribeToCoreEvent();
    void writeEventPacket(const EventPacketPtr& packet);
    void writeDataPacket(const DataPacketPtr& packet);
    void writeDescriptorChangedPacket(const EventPacketPtr& packet);
    void processCoreEvent(ComponentPtr& component, CoreEventArgsPtr& args);

    SignalPtr signal;
    SignalConfigPtr streamedSignal;
    daq::streaming_protocol::StreamWriterPtr writer;
    SignalStreamPtr stream;
    size_t sampleSize;
    bool subscribed;
    bool writeStartDomainValue{false};
    std::mutex subscribedSync;
    daq::streaming_protocol::LogCallback logCallback;
};

END_NAMESPACE_OPENDAQ_WEBSOCKET_STREAMING
