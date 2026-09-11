/*
 * Copyright 2022-2026 openDAQ d.o.o.
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

#if NATIVE_STREAMING_ENABLE_TLS

#include "test_base.h"

#include <native_streaming_protocol/native_streaming_client_handler.h>
#include <native_streaming_protocol/native_streaming_constants.h>
#include <native_streaming_protocol/native_streaming_server_handler.h>

#include <config_protocol/config_protocol.h>

using namespace daq;
using namespace daq::opendaq_native_streaming_protocol;
using namespace daq::config_protocol;

namespace
{

constexpr const char* CaCert = "native_secrets/ca.crt";
constexpr const char* OtherCaCert = "native_secrets/other-ca.crt";
constexpr const char* ServerCert = "native_secrets/server.crt";
constexpr const char* ServerKey = "native_secrets/server.key";
constexpr const char* ClientCert = "native_secrets/client.crt";
constexpr const char* ClientKey = "native_secrets/client.key";
constexpr const char* OtherCaKey = "native_secrets/other-ca.key";

PropertyObjectPtr createTlsConfig(const char* caCert, bool mutualTls)
{
    auto config = PropertyObject();
    config.addProperty(BoolProperty(PROPERTY_VERIFY_SERVER_CERT_CLIENT, caCert != nullptr));
    config.addProperty(StringProperty(PROPERTY_CA_CERT_FILE_PATH_CLIENT, caCert != nullptr ? caCert : ""));
    config.addProperty(BoolProperty(PROPERTY_ENABLE_MTLS_CLIENT, mutualTls));
    config.addProperty(StringProperty(PROPERTY_CERT_FILE_PATH_CLIENT, mutualTls ? ClientCert : ""));
    config.addProperty(StringProperty(PROPERTY_KEY_FILE_PATH_CLIENT, mutualTls ? ClientKey : ""));
    return config;
}

}

class TlsTransportTest : public testing::Test
{
public:
    const uint16_t TLS_PORT = DEFAULT_TLS_PORT;
    const std::string TLS_LISTENING_PORT = std::to_string(DEFAULT_TLS_PORT);
    const std::string SERVER_ADDRESS = "127.0.0.1";
    const std::chrono::milliseconds timeout = std::chrono::milliseconds(5000);

    void SetUp() override
    {
        serverContext = NullContext(Logger(nullptr, LogLevel::Warn));
        clientContext = NullContext(Logger(nullptr, LogLevel::Warn));

        connectionStatusPromise = std::promise<EnumerationPtr>();
        connectionStatusFuture = connectionStatusPromise.get_future();
    }

    void TearDown() override
    {
        clientHandler.reset();
        stopServer();
    }

    void startServer(bool mutualTls = false, const char* certFile = ServerCert, const char* keyFile = ServerKey)
    {
        ioContextPtrServer = std::make_shared<boost::asio::io_context>();
        workGuardServer = std::make_unique<WorkGuardType>(ioContextPtrServer->get_executor());
        execThreadServer = std::thread([this]() { ioContextPtrServer->run(); });

        serverHandler = std::make_shared<NativeStreamingServerHandler>(
            serverContext,
            ioContextPtrServer,
            List<ISignal>(),
            [](const SignalPtr&) {},
            [](const SignalPtr&) {},
            [this](SendConfigProtocolPacketCb sendPacketCb, const UserPtr&, ClientType)
            {
                serverSendPacketCb = sendPacketCb;
                configProtocolTriggeredPromise.set_value();
                return std::make_pair(ProcessConfigProtocolPacketCb([](PacketBuffer&&) {}), nullptr);
            },
            [](const std::string&, const std::string&, bool, ClientType, const std::string&) {},
            [](const std::string&) {});

        serverHandler->startTlsServer(TLS_PORT, certFile, keyFile, mutualTls ? CaCert : std::string());
    }

    void stopServer()
    {
        if (ioContextPtrServer)
            ioContextPtrServer->stop();
        if (execThreadServer.joinable())
            execThreadServer.join();
        workGuardServer.reset();
        ioContextPtrServer.reset();
        serverHandler.reset();
    }

    void createClient(const char* caCert = CaCert, bool mutualTls = false)
    {
        configProtocolTriggeredPromise = std::promise<void>();
        configProtocolTriggeredFuture = configProtocolTriggeredPromise.get_future();

        clientHandler = std::make_shared<NativeStreamingClientHandler>(clientContext,
                                                                      ClientAttributesBase::createTransportLayerConfig(),
                                                                      ClientAttributesBase::createAuthenticationConfig(),
                                                                      createTlsConfig(caCert, mutualTls));

        clientHandler->setConfigHandlers([](PacketBuffer&&) {},
                                         [this](const EnumerationPtr& status, const StringPtr&)
                                         {
                                             connectionStatusPromise.set_value(status);
                                         });
    }

protected:
    ContextPtr serverContext;
    ContextPtr clientContext;

    std::shared_ptr<boost::asio::io_context> ioContextPtrServer;
    std::unique_ptr<WorkGuardType> workGuardServer;
    std::thread execThreadServer;

    std::shared_ptr<NativeStreamingServerHandler> serverHandler;
    std::shared_ptr<NativeStreamingClientHandler> clientHandler;

    SendConfigProtocolPacketCb serverSendPacketCb;
    std::promise<void> configProtocolTriggeredPromise;
    std::future<void> configProtocolTriggeredFuture;

    std::promise<EnumerationPtr> connectionStatusPromise;
    std::future<EnumerationPtr> connectionStatusFuture;
};

TEST_F(TlsTransportTest, Connect)
{
    startServer();
    createClient();

    ASSERT_TRUE(clientHandler->connect(SERVER_ADDRESS, TLS_LISTENING_PORT));
}

TEST_F(TlsTransportTest, ConnectMutualTls)
{
    startServer(/*mutualTls*/ true);
    createClient(CaCert, /*mutualTls*/ true);

    ASSERT_TRUE(clientHandler->connect(SERVER_ADDRESS, TLS_LISTENING_PORT));
}

TEST_F(TlsTransportTest, ConnectWithoutVerification)
{
    startServer();
    createClient(/*caCert*/ nullptr);

    ASSERT_TRUE(clientHandler->connect(SERVER_ADDRESS, TLS_LISTENING_PORT));
}

TEST_F(TlsTransportTest, RejectedByWrongCertificateAuthority)
{
    startServer();
    createClient(OtherCaCert);

    ASSERT_FALSE(clientHandler->connect(SERVER_ADDRESS, TLS_LISTENING_PORT));
}

TEST_F(TlsTransportTest, RejectedByMutualTlsWithoutClientCertificate)
{
    startServer(/*mutualTls*/ true);
    createClient(CaCert, /*mutualTls*/ false);

    ASSERT_FALSE(clientHandler->connect(SERVER_ADDRESS, TLS_LISTENING_PORT));
}

TEST_F(TlsTransportTest, ConfigProtocolOverTls)
{
    startServer();
    createClient();
    ASSERT_TRUE(clientHandler->connect(SERVER_ADDRESS, TLS_LISTENING_PORT));

    clientHandler->sendConfigRequest(PacketBuffer::createGetProtocolInfoRequest(0));
    ASSERT_EQ(configProtocolTriggeredFuture.wait_for(timeout), std::future_status::ready);
}

TEST_F(TlsTransportTest, Reconnection)
{
    startServer();
    createClient();
    ASSERT_TRUE(clientHandler->connect(SERVER_ADDRESS, TLS_LISTENING_PORT));

    stopServer();
    ASSERT_EQ(connectionStatusFuture.wait_for(std::chrono::seconds(5)), std::future_status::ready);
    ASSERT_EQ(connectionStatusFuture.get(), "Reconnecting");

    connectionStatusPromise = std::promise<EnumerationPtr>();
    connectionStatusFuture = connectionStatusPromise.get_future();
    configProtocolTriggeredPromise = std::promise<void>();
    configProtocolTriggeredFuture = configProtocolTriggeredPromise.get_future();

    startServer();

    ASSERT_EQ(connectionStatusFuture.wait_for(std::chrono::seconds(20)), std::future_status::ready);
    ASSERT_EQ(connectionStatusFuture.get(), "Connected");

    clientHandler->sendConfigRequest(PacketBuffer::createGetProtocolInfoRequest(0));
    ASSERT_EQ(configProtocolTriggeredFuture.wait_for(timeout), std::future_status::ready);
}

// the mirror image of Reconnection: a failure of the secure handshake is not something another
// attempt can fix, so the client has to give up rather than retry forever. The server comes back
// with credentials the client's authority never signed, which is what a swapped or misconfigured
// server looks like from here.
TEST_F(TlsTransportTest, ReconnectionRejectedBySecurity)
{
    startServer();
    createClient();
    ASSERT_TRUE(clientHandler->connect(SERVER_ADDRESS, TLS_LISTENING_PORT));

    stopServer();
    ASSERT_EQ(connectionStatusFuture.wait_for(std::chrono::seconds(20)), std::future_status::ready);
    ASSERT_EQ(connectionStatusFuture.get(), "Reconnecting");

    connectionStatusPromise = std::promise<EnumerationPtr>();
    connectionStatusFuture = connectionStatusPromise.get_future();

    startServer(/*mutualTls*/ false, OtherCaCert, OtherCaKey);

    ASSERT_EQ(connectionStatusFuture.wait_for(std::chrono::seconds(20)), std::future_status::ready);
    ASSERT_EQ(connectionStatusFuture.get(), "Unrecoverable");
}

#endif
