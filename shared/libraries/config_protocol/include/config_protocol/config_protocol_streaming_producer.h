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
#include <config_protocol/config_protocol.h>
#include <coreobjects/object_keys.h>
#include <opendaq/packet_ptr.h>
#include <opendaq/reader_factory.h>
#include <opendaq/input_port_ptr.h>
#include <thread>

namespace daq::config_protocol
{

using SendDaqPacketCallback = std::function<void(const PacketPtr&, uint32_t signalNumericId)>;

class ConfigProtocolStreamingProducer
{
public:
    ConfigProtocolStreamingProducer(const ContextPtr& daqContext, const SendDaqPacketCallback& sendDaqPacketCallback);
    ~ConfigProtocolStreamingProducer();

    uint32_t registerOrUpdateSignal(const SignalPtr& signal);
    SignalPtr findRegisteredSignal(const StringPtr& signalId);
    void addConnection(const SignalPtr& signal, const StringPtr& inputPortRemoteGlobalId);
    void removeConnection(const SignalPtr& signal, const StringPtr& inputPortRemoteGlobalId);

private:
    struct StreamedSignal
    {
        StreamedSignal(const SignalPtr& signal, uint32_t signalNumericId);

        SignalPtr signal;

        // The numeric id of the signal used to identify it on a consumer side
        uint32_t signalNumericId;

        // reader used to read signal packets
        PacketReaderPtr reader;

        // Holds the remote global IDs of components that trigger a signal to be streamed.
        // This could either be the ID of a connected input port or the ID of a data signal
        // for which this signal serves as a domain signal.
        std::unordered_set<StringPtr> triggerComponents;
    };

    void addStreamingTrigger(const SignalPtr& signal, const StringPtr& triggerComponentId);
    void removeStreamingTrigger(const SignalPtr& signal, const StringPtr& triggerComponentId);
    void startReadSignal(StreamedSignal& streamedSignal);
    void stopReadSignal(StreamedSignal& streamedSignal);
    void readerThreadFunc();

    std::unordered_map<StringPtr, StreamedSignal, StringHash, StringEqualTo> streamedSignals;
    std::thread readerThread;
    SendDaqPacketCallback sendDaqPacketCb;
    uint32_t signalNumericIdCounter;
    bool stopped;
    ContextPtr daqContext;
    LoggerComponentPtr loggerComponent;
    std::mutex sync;
};

}
