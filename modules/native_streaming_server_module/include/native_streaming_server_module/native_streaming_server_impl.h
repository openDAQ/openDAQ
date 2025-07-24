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

#include <config_protocol/config_protocol_server.h>
#include <opendaq/streaming_to_device_server_impl.h>

BEGIN_NAMESPACE_OPENDAQ_NATIVE_STREAMING_SERVER_MODULE

class NativeStreamingServerBaseImpl
{
public:
    explicit NativeStreamingServerBaseImpl(const DevicePtr& rootDevice,
                                           const PropertyObjectPtr& config,
                                           const ContextPtr& context);
    ~NativeStreamingServerBaseImpl();
    static PropertyObjectPtr createDefaultConfig(const ContextPtr& context);
    static ServerTypePtr createType(const ContextPtr& context);
    static PropertyObjectPtr populateDefaultConfig(const PropertyObjectPtr& config, const ContextPtr& context);

protected:
    PropertyObjectPtr onGetDiscoveryConfig();
    void doStopServer();
    void prepareServerHandler();

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

    static void populateDefaultConfigFromProvider(const ContextPtr& context, const PropertyObjectPtr& config);

    // TODO refactoring duplicating code
    virtual std::shared_ptr<config_protocol::ConfigProtocolServer> createConfigProtocolServer(config_protocol::NotificationReadyCallback sendConfigPacketCb,
                                                                                              const UserPtr& user,
                                                                                              ClientType connectionType);
    StringPtr id;
    PropertyObjectPtr config;
    WeakRefPtr<IDevice> rootDeviceRef;
    ContextPtr context;

    std::shared_ptr<opendaq_native_streaming_protocol::NativeStreamingServerHandler> serverHandler;
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
};

class NativeStreamingToDeviceServerImpl final : public daq::StreamingToDeviceServer, public NativeStreamingServerBaseImpl
{
public:
    NativeStreamingToDeviceServerImpl(const DevicePtr& rootDevice,
                                      const PropertyObjectPtr& config,
                                      const ContextPtr& context);

protected:
    PropertyObjectPtr getDiscoveryConfig() override;
    void onStopServer() override;

    std::shared_ptr<config_protocol::ConfigProtocolServer> createConfigProtocolServer(config_protocol::NotificationReadyCallback sendConfigPacketCb,
                                                                                      const UserPtr& user,
                                                                                      ClientType connectionType) override;
};

class NativeStreamingServerBasicImpl final : public daq::StreamingServer, public NativeStreamingServerBaseImpl
{
public:
    NativeStreamingServerBasicImpl(const DevicePtr& rootDevice,
                                    const PropertyObjectPtr& config,
                                    const ContextPtr& context);

protected:
    PropertyObjectPtr getDiscoveryConfig() override;
    void onStopServer() override;

    std::shared_ptr<config_protocol::ConfigProtocolServer> createConfigProtocolServer(config_protocol::NotificationReadyCallback sendConfigPacketCb,
                                                                                      const UserPtr& user,
                                                                                      ClientType connectionType) override;
};

OPENDAQ_DECLARE_CLASS_FACTORY_WITH_INTERFACE(
    INTERNAL_FACTORY, NativeStreamingServerBasic, daq::IStreamingServer,
    DevicePtr, rootDevice,
    PropertyObjectPtr, config,
    const ContextPtr&, context
)

OPENDAQ_DECLARE_CLASS_FACTORY_WITH_INTERFACE(
    INTERNAL_FACTORY, NativeStreamingToDeviceServer, daq::IStreamingToDeviceServer,
    DevicePtr, rootDevice,
    PropertyObjectPtr, config,
    const ContextPtr&, context
)

END_NAMESPACE_OPENDAQ_NATIVE_STREAMING_SERVER_MODULE
