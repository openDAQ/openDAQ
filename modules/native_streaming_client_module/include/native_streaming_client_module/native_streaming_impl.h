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
#include <opendaq/streaming_impl.h>
#include <native_streaming_client_module/common.h>

#include <native_streaming_protocol/native_streaming_client_handler.h>

BEGIN_NAMESPACE_OPENDAQ_NATIVE_STREAMING_CLIENT_MODULE

class NativeStreamingImpl : public Streaming
{
public:
    explicit NativeStreamingImpl(const StringPtr& connectionString,
                                 const StringPtr& host,
                                 const StringPtr& port,
                                 const StringPtr& path,
                                 const ContextPtr& context,
                                 const ProcedurePtr& onDeviceSignalAvailableCallback,
                                 const ProcedurePtr& onDeviceSignalUnavailableCallback);

    ~NativeStreamingImpl();

protected:
    void onSetActive(bool active) override;
    StringPtr onAddSignal(const SignalRemotePtr& signal) override;
    void onRemoveSignal(const SignalRemotePtr& signal) override;

    void signalAvailableHandler(const StringPtr& signalStringId,
                                const StringPtr& domainSignalStringId,
                                const DataDescriptorPtr& signalDescriptor,
                                const StringPtr& name,
                                const StringPtr& description);
    void addToAvailableSignals(const StringPtr& signalStringId);

    void signalUnavailableHandler(const StringPtr& signalStringId);
    void removeFromAvailableSignals(const StringPtr& signalStringId);

    void prepareClientHandler();

    void onPacket(const StringPtr& signalStringId, const PacketPtr& packet);
    void handleEventPacket(const StringPtr& signalId, const EventPacketPtr &eventPacket);
    void handleDataPacket(const StringPtr& signalId, const PacketPtr& dataPacket);
    void handleCachedEventPackets(const StringPtr& signalStreamingId, const SignalRemotePtr& signal);
    StringPtr getSignalStreamingId(const SignalRemotePtr& signal);

    void startAsyncOperations();
    void stopAsyncOperations();

    std::shared_ptr<opendaq_native_streaming_protocol::NativeStreamingClientHandler> clientHandler;
    ProcedurePtr onDeviceSignalAvailableCallback;
    ProcedurePtr onDeviceSignalUnavailableCallback;
    std::vector<std::string> availableSignalIds;
    std::map<StringPtr, std::vector<EventPacketPtr>> cachedEventPackets;

    std::shared_ptr<boost::asio::io_context> ioContextPtr;
    boost::asio::executor_work_guard<boost::asio::io_context::executor_type> workGuard;
    std::thread ioThread;

    LoggerPtr logger;
    LoggerComponentPtr loggerComponent;

    std::mutex availableSignalsSync;
};

END_NAMESPACE_OPENDAQ_NATIVE_STREAMING_CLIENT_MODULE
