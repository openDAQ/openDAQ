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
#include <native_streaming_server_module/common.h>
#include <opendaq/device_ptr.h>
#include <opendaq/packet_reader_ptr.h>
#include <opendaq/server.h>
#include <opendaq/server_impl.h>
#include <coretypes/intfs.h>
#include <native_streaming_protocol/native_streaming_server_handler.h>
#include <opendaq/connection_internal.h>
#include <tsl/ordered_map.h>
#include <boost/asio/thread_pool.hpp>
#include <config_protocol/config_protocol_server.h>

BEGIN_NAMESPACE_OPENDAQ_NATIVE_STREAMING_SERVER_MODULE

class NativeStreamingServerImpl : public daq::Server
{
public:
    explicit NativeStreamingServerImpl(const DevicePtr& rootDevice,
                                       const PropertyObjectPtr& config,
                                       const ContextPtr& context);
    ~NativeStreamingServerImpl() override;
    static PropertyObjectPtr createDefaultConfig(const ContextPtr& context);
    static ServerTypePtr createType(const ContextPtr& context);
    static PropertyObjectPtr populateDefaultConfig(const PropertyObjectPtr& config, const ContextPtr& context);

protected:
    using PacketBufferPtr = std::shared_ptr<config_protocol::PacketBuffer>;
    using ConfigServerPtr = std::shared_ptr<config_protocol::ConfigProtocolServer>;
    using PacketStreamingClientPtr = std::shared_ptr<packet_streaming::PacketStreamingClient>;

    PropertyObjectPtr getDiscoveryConfig() override;
    void onStopServer() override;
    StreamingPtr onGetStreaming() override;
    void prepareServerHandler();

    std::shared_ptr<opendaq_native_streaming_protocol::NativeStreamingServerHandler> serverHandler;

    void startReading();
    void stopReading();
    void startReadThread();
    void addReader(SignalPtr signalToRead);
    void removeReader(SignalPtr signalToRead);
    void clearIndices();

    void startTransportOperations();
    void stopTransportOperations();

    void startProcessingOperations();
    void stopProcessingOperations();

    void stopServerInternal();

    void addSignalsOfComponent(ComponentPtr& component);
    void componentAdded(ComponentPtr& sender, CoreEventArgsPtr& eventArgs);
    void componentRemoved(ComponentPtr& sender, CoreEventArgsPtr& eventArgs);
    void componentUpdated(ComponentPtr& updatedComponent);
    void coreEventCallback(ComponentPtr& sender, CoreEventArgsPtr& eventArgs);
    void initWorkerPool();

    static void populateDefaultConfigFromProvider(const ContextPtr& context, const PropertyObjectPtr& config);

    void processClientConfigRequest(const ConfigServerPtr& configServerPtr, const PacketBufferPtr& packetBufferPtr, opendaq_native_streaming_protocol::SendConfigProtocolPacketCb sendConfigPacketCb);
    static void processConfigRequestAndSendReply(const ConfigServerPtr& configServerPtr, const PacketBufferPtr& packetBufferPtr, opendaq_native_streaming_protocol::SendConfigProtocolPacketCb sendConfigPacketCb);
    void dispatchClientConfigRequest(const ConfigServerPtr& configServerPtr, opendaq_native_streaming_protocol::SendConfigProtocolPacketCb sendConfigPacketCb, config_protocol::PacketBuffer&& packetBuffer);
    void dispatchClientToDeviceStreamingPacket(const ConfigServerPtr& configServerPtr, const PacketStreamingClientPtr& packetStreamingClientPtr, const packet_streaming::PacketBufferPtr& packetBufferPtr);

    std::thread readThread;
    std::atomic<bool> readThreadActive;
    std::chrono::milliseconds readThreadSleepTime;
    std::vector<std::tuple<SignalPtr, std::string, InputPortPtr, ObjectPtr<IConnectionInternal>>> signalReaders;
    std::vector<IPacket*> packetBuf;
    tsl::ordered_map<std::string, opendaq_native_streaming_protocol::PacketBufferData> packetIndices;

    std::shared_ptr<boost::asio::io_context> transportIOContextPtr;
    std::thread transportThread;

    std::shared_ptr<boost::asio::io_context> processingIOContextPtr;
    std::thread processingThread;
    boost::asio::io_context::strand processingStrand;

    std::string rootDeviceGlobalId;

    LoggerPtr logger;
    LoggerComponentPtr loggerComponent;

    std::mutex readersSync;
    bool serverStopped;
    size_t maxPacketReadCount;
    std::unordered_map<std::string, SizeT> registeredClientIds;
    std::unordered_map<std::string, SizeT> disconnectedClientIds;
    StreamingPtr streaming;
    std::unique_ptr<boost::asio::thread_pool> workerPool;
};

OPENDAQ_DECLARE_CLASS_FACTORY_WITH_INTERFACE(
    INTERNAL_FACTORY, NativeStreamingServer, daq::IServer,
    DevicePtr, rootDevice,
    PropertyObjectPtr, config,
    const ContextPtr&, context
)

END_NAMESPACE_OPENDAQ_NATIVE_STREAMING_SERVER_MODULE
