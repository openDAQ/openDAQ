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
#include <websocket_streaming/websocket_streaming.h>
#include <opendaq/signal_ptr.h>
#include <streaming_protocol/SynchronousSignal.hpp>
#include <opendaq/sample_type.h>
#include <opendaq/signal_factory.h>

BEGIN_NAMESPACE_OPENDAQ_WEBSOCKET_STREAMING

class InputSignal;
using InputSignalPtr = std::shared_ptr<InputSignal>;

class InputSignal
{
public:
    InputSignal();

    PacketPtr asPacket(uint64_t packetOffset, const uint8_t* data, size_t size);
    PacketPtr createDecriptorChangedPacket();
    void setDataDescriptor(const daq::streaming_protocol::SubscribedSignal& dataSignal);
    void setDomainDescriptor(const daq::streaming_protocol::SubscribedSignal& timeSignal);
    bool hasDescriptors();
    DataDescriptorPtr getSignalDescriptor();
    DataDescriptorPtr getDomainSignalDescriptor();

protected:
    DataDescriptorPtr currentDataDescriptor;
    DataDescriptorPtr currentDomainDataDescriptor;

    std::string name;
    std::string description;
};

END_NAMESPACE_OPENDAQ_WEBSOCKET_STREAMING
