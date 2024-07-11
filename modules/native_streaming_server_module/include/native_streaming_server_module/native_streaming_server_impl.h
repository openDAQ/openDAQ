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
#include <native_streaming_server_module/common.h>
#include <opendaq/device_ptr.h>
#include <opendaq/packet_reader_ptr.h>
#include <opendaq/server.h>
#include <opendaq/server_impl.h>
#include <coretypes/intfs.h>

#include <native_streaming_protocol/native_streaming_server_handler.h>

BEGIN_NAMESPACE_OPENDAQ_NATIVE_STREAMING_SERVER_MODULE

class NativeStreamingServerImpl : public daq::Server
{
public:
    explicit NativeStreamingServerImpl(daq::DevicePtr rootDevice, PropertyObjectPtr config, const ContextPtr& context);
    ~NativeStreamingServerImpl() override;
    static PropertyObjectPtr createDefaultConfig(const ContextPtr& context);
    static ServerTypePtr createType(const ContextPtr& context);
    static PropertyObjectPtr populateDefaultConfig(const PropertyObjectPtr& config, const ContextPtr& context);

protected:
    PropertyObjectPtr getDiscoveryConfig() override;
    void onStopServer() override;
    void prepareServerHandler();

    std::shared_ptr<opendaq_native_streaming_protocol::NativeStreamingServerHandler> serverHandler;

    void startReading();
    void stopReading();
    void startReadThread();
    void createReaders();
    void addReader(SignalPtr signalToRead);
    void removeReader(SignalPtr signalToRead);

    void startTransportOperations();
    void stopTransportOperations();

    void startProcessingOperations();
    void stopProcessingOperations();

    void addSignalsOfComponent(ComponentPtr& component);
    void componentAdded(ComponentPtr& sender, CoreEventArgsPtr& eventArgs);
    void componentRemoved(ComponentPtr& sender, CoreEventArgsPtr& eventArgs);
    void componentUpdated(ComponentPtr& updatedComponent);
    void coreEventCallback(ComponentPtr& sender, CoreEventArgsPtr& eventArgs);

    static void populateDefaultConfigFromProvider(const ContextPtr& context, const PropertyObjectPtr& config);

    std::thread readThread;
    bool readThreadActive;
    std::chrono::milliseconds readThreadSleepTime;
    std::vector<std::pair<SignalPtr, PacketReaderPtr>> signalReaders;

    std::shared_ptr<boost::asio::io_context> transportIOContextPtr;
    std::thread transportThread;

    boost::asio::io_context processingIOContext;
    std::thread processingThread;
    boost::asio::io_context::strand processingStrand;

    LoggerPtr logger;
    LoggerComponentPtr loggerComponent;

    std::mutex readersSync;
};

OPENDAQ_DECLARE_CLASS_FACTORY_WITH_INTERFACE(
    INTERNAL_FACTORY, NativeStreamingServer, daq::IServer,
    DevicePtr, rootDevice,
    PropertyObjectPtr, config,
    const ContextPtr&, context
)

END_NAMESPACE_OPENDAQ_NATIVE_STREAMING_SERVER_MODULE
