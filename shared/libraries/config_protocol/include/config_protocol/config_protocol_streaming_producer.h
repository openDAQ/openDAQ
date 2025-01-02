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
#include <config_protocol/config_protocol.h>
#include <coreobjects/object_keys.h>
#include <opendaq/packet_ptr.h>
#include <opendaq/reader_factory.h>
#include <opendaq/input_port_ptr.h>
#include <thread>

namespace daq::config_protocol
{

using SendDaqPacketCallback = std::function<void(const PacketPtr& /*packet*/, SignalNumericIdType /*signalNumericId*/)>;

class ConfigProtocolStreamingProducer
{
public:
    ConfigProtocolStreamingProducer(const ContextPtr& daqContext, const SendDaqPacketCallback& sendDaqPacketCallback);
    ~ConfigProtocolStreamingProducer();

    SignalNumericIdType registerOrUpdateSignal(const SignalPtr& signal);
    SignalPtr findRegisteredSignal(const StringPtr& signalId);
    void addConnection(const SignalPtr& signal, const StringPtr& inputPortRemoteGlobalId);
    void removeConnection(const SignalPtr& signal, const StringPtr& inputPortRemoteGlobalId, std::vector<SignalNumericIdType>& unusedSignlasIds);

private:
    struct StreamedSignal
    {
        StreamedSignal(const SignalPtr& signal, SignalNumericIdType signalNumericId);

        SignalPtr signal;

        // The numeric id of the signal used to identify it on a consumer side
        SignalNumericIdType signalNumericId;

        // reader used to read signal packets
        PacketReaderPtr reader;

        // Holds the remote global IDs of components that trigger a signal to be streamed.
        // This could either be the ID of a connected input port or the ID of a data signal
        // for which this signal serves as a domain signal.
        std::unordered_set<StringPtr, StringHash, StringEqualTo> triggerComponents;
    };

    void addStreamingTrigger(const SignalPtr& signal, const StringPtr& triggerComponentId);
    void removeStreamingTrigger(const SignalPtr& signal, const StringPtr& triggerComponentId, std::vector<SignalNumericIdType>& unusedSignlasIds);
    void startReadSignal(StreamedSignal& streamedSignal);
    void stopReadSignal(StreamedSignal& streamedSignal);

    void readerThreadFunc();
    bool hasSignalToRead();
    void startReadThread();
    void stopReadThread();

    std::unordered_map<StringPtr, StreamedSignal, StringHash, StringEqualTo> streamedSignals;
    std::thread readerThread;
    SendDaqPacketCallback sendDaqPacketCb;
    SignalNumericIdType signalNumericIdCounter;
    bool readThreadRunning;
    ContextPtr daqContext;
    LoggerComponentPtr loggerComponent;
    std::chrono::milliseconds readThreadSleepTime;
    std::mutex sync;
};

}
