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

#include <gtest/gtest.h>

#include <opendaq/opendaq.h>

#include <native_streaming_protocol/native_streaming_client_handler.h>
#include <native_streaming_protocol/native_streaming_server_handler.h>

#include <memory>
#include <future>

using ClientCountType = size_t;
using WorkGuardType = boost::asio::executor_work_guard<boost::asio::io_context::executor_type>;

class ClientAttributesBase
{
public:
    std::shared_ptr<daq::opendaq_native_streaming_protocol::NativeStreamingClientHandler> clientHandler;
    daq::ContextPtr clientContext;

    std::promise< daq::EnumerationPtr > connectionStatusPromise;
    std::future< daq::EnumerationPtr > connectionStatusFuture;

    void setUp()
    {
        clientContext = NullContext(Logger(nullptr, daq::LogLevel::Trace));

        connectionStatusPromise = std::promise< daq::EnumerationPtr >();
        connectionStatusFuture = connectionStatusPromise.get_future();
    }

    void tearDown()
    {
        clientHandler.reset();
    }

    static daq::PropertyObjectPtr createTransportLayerConfig()
    {
        static size_t clientId = 123456;
        auto config = daq::PropertyObject();

        config.addProperty(daq::BoolProperty("MonitoringEnabled", daq::True));
        config.addProperty(daq::IntProperty("HeartbeatPeriod", 1000));
        config.addProperty(daq::IntProperty("InactivityTimeout", 1500));
        config.addProperty(daq::IntProperty("ConnectionTimeout", 1000));
        config.addProperty(daq::IntProperty("StreamingInitTimeout", 1000));
        config.addProperty(daq::IntProperty("ReconnectionPeriod", 1000));
        config.addProperty(daq::StringProperty("ClientId", std::to_string(clientId++)));

        return config;
    }

    static daq::PropertyObjectPtr createAuthenticationConfig()
    {
        auto config = daq::PropertyObject();

        config.addProperty(daq::StringProperty("Username", ""));
        config.addProperty(daq::StringProperty("Password", ""));

        return config;
    }
};

class ProtocolTestBase : public testing::TestWithParam<std::tuple<ClientCountType, bool>>
{
public:
    const uint16_t NATIVE_STREAMING_SERVER_PORT = 7420;
    const std::string NATIVE_STREAMING_LISTENING_PORT = "7420";
    const std::string SERVER_ADDRESS = "127.0.0.1";
    const std::chrono::milliseconds timeout = std::chrono::milliseconds(500);

    void SetUp() override
    {
        serverContext = NullContext(Logger(nullptr, daq::LogLevel::Trace));
    }

    void TearDown() override
    {
    }

    void startIoOperations()
    {
        ioContextPtrServer = std::make_shared<boost::asio::io_context>();
        workGuardServer = std::make_unique<WorkGuardType>(ioContextPtrServer->get_executor());
        execThreadServer = std::thread([this]() { ioContextPtrServer->run(); });
    }

    void stopIoOperations()
    {
        if (ioContextPtrServer)
            ioContextPtrServer->stop();
        if (execThreadServer.joinable())
            execThreadServer.join();
        workGuardServer.reset();
        ioContextPtrServer.reset();
    }

protected:
    daq::ContextPtr serverContext;
    ClientCountType clientsCount;

    /// async operations handler
    std::shared_ptr<boost::asio::io_context> ioContextPtrServer;
    /// prevents boost::asio::io_context::run() from returning when there is no more async operations pending
    std::unique_ptr<WorkGuardType> workGuardServer;
    /// async operations runner thread
    std::thread execThreadServer;

    std::shared_ptr<daq::opendaq_native_streaming_protocol::NativeStreamingServerHandler> serverHandler;
};
