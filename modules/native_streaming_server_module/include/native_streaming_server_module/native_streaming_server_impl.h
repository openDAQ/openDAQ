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
    static PropertyObjectPtr createDefaultConfig();
    static ServerTypePtr createType();

protected:
    void onStopServer() override;
    void prepareServerHandler();

    std::shared_ptr<opendaq_native_streaming_protocol::NativeStreamingServerHandler> serverHandler;

    void startReading();
    void stopReading();
    void startReadThread();
    void createReaders();
    void updateReaders();
    void addReader(SignalPtr signalToRead);
    void removeReader(SignalPtr signalToRead);

    void startAsyncOperations();
    void stopAsyncOperations();

    std::thread readThread;
    bool readThreadActive;
    std::chrono::milliseconds readThreadSleepTime;
    std::vector<std::pair<SignalPtr, PacketReaderPtr>> signalReaders;

    std::queue<SignalPtr> signalsToStartRead;
    std::queue<SignalPtr> signalsToStopRead;

    std::shared_ptr<boost::asio::io_context> ioContextPtr;
    boost::asio::executor_work_guard<boost::asio::io_context::executor_type> workGuard;
    std::thread ioThread;

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
