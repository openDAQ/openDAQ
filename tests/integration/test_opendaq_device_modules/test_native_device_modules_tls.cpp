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

// The TLS transport of the native streaming modules. Plain and TLS are transports of the same types and
// capabilities; a client selects TLS in the "AddDeviceConfig" or "AddStreamingConfig" of its config.
#if NATIVE_STREAMING_ENABLE_TLS

#include "test_helpers/device_modules.h"
#include "test_helpers/native_tls.h"
#include "test_helpers/test_helpers.h"

#include <algorithm>
#include <chrono>
#include <map>
#include <set>
#include <string>
#include <thread>

using namespace daq;
using namespace daq::test_helpers::native_tls;

using NativeTlsTest = testing::Test;

namespace
{

InstancePtr createServerInstance(bool mutualTls = false, bool plainPortEnabled = true)
{
    auto instance = Instance("[[none]]");
    addRefDeviceModule(instance);
    addNativeServerModule(instance);
    instance.addDevice("daqref://device1");

    instance.addServer(ServerTypeId, secureServerConfig(instance, mutualTls, plainPortEnabled));
    return instance;
}

InstancePtr createClientInstance()
{
    auto instance = Instance("[[none]]");
    addNativeClientModule(instance);
    return instance;
}

/// @brief a server which advertises itself over mDNS, under a path unique to the calling test
/// @param serialNumber when set, the root device carries it, so that the server is found as "daq://openDAQ_<serial>"
InstancePtr createDiscoverableServerInstance(const std::string& path, bool plainPortEnabled = true, const std::string& serialNumber = "")
{
    auto instance = InstanceBuilder()
                        .setModulePath("[[none]]")
                        .addDiscoveryServer("mdns")
                        .setDefaultRootDeviceLocalId("local")
                        .build();

    addRefDeviceModule(instance);
    if (!serialNumber.empty())
    {
        auto rootConfig = PropertyObject();
        rootConfig.addProperty(StringProperty("SerialNumber", serialNumber));
        instance.setRootDevice("daqref://device0", rootConfig);
    }
    instance.addDevice("daqref://device1");
    addNativeServerModule(instance);

    auto config = secureServerConfig(instance, /*mutualTls*/ false, plainPortEnabled);
    config.setPropertyValue("Path", path);
    instance.addServer(ServerTypeId, config).enableDiscovery();

    return instance;
}

/// @brief the native capabilities discovered under the given path, by protocol id
/// @param required stops the search as soon as these are all in; a missing one only costs the timeout,
/// so that the test can report which of them never showed up
std::map<std::string, ServerCapabilityPtr> discoverNativeCapabilities(const InstancePtr& client,
                                                                      const std::string& path,
                                                                      const std::set<std::string>& required,
                                                                      std::chrono::seconds timeout = std::chrono::seconds(20))
{
    std::map<std::string, ServerCapabilityPtr> found;
    const auto deadline = std::chrono::steady_clock::now() + timeout;

    while (true)
    {
        found.clear();
        for (const auto& deviceInfo : client.getAvailableDevices())
        {
            for (const auto& capability : deviceInfo.getServerCapabilities())
            {
                if (!test_helpers::isSufix(capability.getConnectionString(), path))
                    continue;
                found[capability.getProtocolId().toStdString()] = capability;
            }
        }

        const bool complete =
            std::all_of(required.begin(), required.end(), [&found](const std::string& id) { return found.count(id) == 1; });

        if (complete || std::chrono::steady_clock::now() >= deadline)
            return found;

        std::this_thread::sleep_for(std::chrono::milliseconds(200));
    }
}

}

TEST_F(NativeTlsTest, TypesListTlsTransport)
{
    auto client = createClientInstance();
    const auto deviceTypes = client.getAvailableDeviceTypes();

    for (const auto& id : {ConfigId, StreamingId})
    {
        const auto transports = deviceTypes.get(id).getAvailableTransports();
        ASSERT_EQ(transports.getKeyList(), List<IString>("plain", "tls")) << id;
        ASSERT_EQ(transports.get("plain"), PlainPort) << id;
        ASSERT_EQ(transports.get("tls"), TlsPort) << id;
    }

    ASSERT_EQ(deviceTypes.get(ConfigId).getConnectionStringPrefix(), "daq.nd");
    ASSERT_EQ(deviceTypes.get(StreamingId).getConnectionStringPrefix(), "daq.ns");
}

TEST_F(NativeTlsTest, ServerPublishesOneCapabilityPerProtocol)
{
    auto server = createServerInstance();
    const auto capabilities = server.getInfo().getServerCapabilities();

    std::set<std::string> ids;
    for (const auto& capability : capabilities)
        ids.insert(capability.getProtocolId().toStdString());
    ASSERT_EQ(ids, (std::set<std::string>{StreamingId, ConfigId}));

    for (const auto& capability : capabilities)
    {
        const auto id = capability.getProtocolId().toStdString();
        const auto transports = capability.getAvailableTransports();

        // plain comes first, so the port older clients read is the plaintext one
        ASSERT_EQ(transports.getKeyList(), List<IString>("plain", "tls")) << id;
        ASSERT_EQ(transports.get("plain"), PlainPort) << id;
        ASSERT_EQ(transports.get("tls"), TlsPort) << id;
        ASSERT_EQ(capability.getPort(), PlainPort) << id;
    }
}

TEST_F(NativeTlsTest, ConnectDeviceServerAuthentication)
{
    auto server = createServerInstance();
    auto client = createClientInstance();

    auto device = client.addDevice(connectionString("daq.nd", TlsPort),
                                   secureClientConfig(client, ConfigId));

    ASSERT_TRUE(device.assigned());
    ASSERT_GT(device.getSignalsRecursive().getCount(), 0u);

    // lists only the transport the device is connected over
    const auto connectionInfo = device.getInfo().getConfigurationConnectionInfo();
    ASSERT_EQ(connectionInfo.getProtocolId(), ConfigId);
    ASSERT_EQ(connectionInfo.getPrefix(), "daq.nd");
    ASSERT_EQ(connectionInfo.getAvailableTransports().getKeyList(), List<IString>("tls"));
    ASSERT_EQ(connectionInfo.getPort(), TlsPort);
}

// no port in the connection string: the TLS transport has to fall back to its own default, not to
// the plaintext one
TEST_F(NativeTlsTest, ConnectDeviceWithoutExplicitPort)
{
    auto server = createServerInstance();
    auto client = createClientInstance();

    auto device = client.addDevice("daq.nd://127.0.0.1/", secureClientConfig(client, ConfigId));

    ASSERT_TRUE(device.assigned());
    ASSERT_EQ(device.getInfo().getConfigurationConnectionInfo().getPort(), TlsPort);
}

TEST_F(NativeTlsTest, ConnectPseudoDeviceWithoutExplicitPort)
{
    auto server = createServerInstance();
    auto client = createClientInstance();

    auto device = client.addDevice("daq.ns://127.0.0.1/", secureClientConfig(client, StreamingId));

    ASSERT_TRUE(device.assigned());
    ASSERT_EQ(device.getInfo().getConfigurationConnectionInfo().getPort(), TlsPort);
}

TEST_F(NativeTlsTest, ConnectPseudoDevice)
{
    auto server = createServerInstance();
    auto client = createClientInstance();

    auto device = client.addDevice(connectionString("daq.ns", TlsPort),
                                   secureClientConfig(client, StreamingId));

    ASSERT_TRUE(device.assigned());
    ASSERT_GT(device.getSignals(search::Recursive(search::Any())).getCount(), 0u);
}

#ifdef NATIVE_STREAMING_TLS_WITHOUT_SIGN

// the client accepts whatever certificate the server presents
TEST_F(NativeTlsTest, AcceptsServerOfAnyAuthority)
{
    auto server = Instance("[[none]]");
    addRefDeviceModule(server);
    addNativeServerModule(server);
    server.addDevice("daqref://device1");

    // signed by an authority the client has never been told about
    auto serverConfig = secureServerConfig(server);
    serverConfig.setPropertyValue("CertificateFilePath", OtherCaCert);
    serverConfig.setPropertyValue("KeyFilePath", OtherCaKey);
    server.addServer(ServerTypeId, serverConfig);

    auto client = createClientInstance();
    auto device = client.addDevice(connectionString("daq.nd", TlsPort),
                                   secureClientConfig(client, ConfigId));

    ASSERT_TRUE(device.assigned());
    ASSERT_GT(device.getSignalsRecursive().getCount(), 0u);
}

// nothing but the transport has to be configured on the client for a secure connection
TEST_F(NativeTlsTest, ConnectsWithOnlyTransportSelected)
{
    auto server = createServerInstance();
    auto client = createClientInstance();

    auto config = client.getAvailableDeviceTypes().get(ConfigId).createDefaultConfig();
    selectTls(config);
    auto device = client.addDevice(connectionString("daq.nd", TlsPort), config);

    ASSERT_TRUE(device.assigned());
    ASSERT_GT(device.getSignalsRecursive().getCount(), 0u);
}

#else

TEST_F(NativeTlsTest, ConnectMutualTls)
{
    auto server = createServerInstance(/*mutualTls*/ true);
    auto client = createClientInstance();

    auto device = client.addDevice(connectionString("daq.nd", TlsPort),
                                   secureClientConfig(client, ConfigId, /*mutualTls*/ true));

    ASSERT_TRUE(device.assigned());
    ASSERT_GT(device.getSignalsRecursive().getCount(), 0u);
}

TEST_F(NativeTlsTest, ConnectWithoutVerification)
{
    auto server = createServerInstance();
    auto client = createClientInstance();

    auto device = client.addDevice(connectionString("daq.nd", TlsPort),
                                   unverifiedClientConfig(client, ConfigId));

    ASSERT_TRUE(device.assigned());
    ASSERT_GT(device.getSignalsRecursive().getCount(), 0u);
}

TEST_F(NativeTlsTest, RejectedByWrongCertificateAuthority)
{
    auto server = createServerInstance();
    auto client = createClientInstance();

    ASSERT_ANY_THROW(client.addDevice(connectionString("daq.nd", TlsPort),
                                      secureClientConfig(client, ConfigId, /*mutualTls*/ false, OtherCaCert)));
}

TEST_F(NativeTlsTest, RejectedWithoutCertificateAuthority)
{
    auto server = createServerInstance();
    auto client = createClientInstance();

    // verification is on by default and has nothing to verify against
    auto config = client.getAvailableDeviceTypes().get(ConfigId).createDefaultConfig();
    selectTls(config);
    ASSERT_ANY_THROW(client.addDevice(connectionString("daq.nd", TlsPort), config));
}

TEST_F(NativeTlsTest, RejectedByMutualTlsWithoutClientCertificate)
{
    auto server = createServerInstance(/*mutualTls*/ true);
    auto client = createClientInstance();

    ASSERT_ANY_THROW(client.addDevice(connectionString("daq.nd", TlsPort),
                                      secureClientConfig(client, ConfigId, /*mutualTls*/ false)));
}

#endif

TEST_F(NativeTlsTest, PlaintextChannelStillWorksAlongside)
{
    auto server = createServerInstance();

    // separate clients: the same remote device reached twice from one instance collides on local id
    auto secureClient = createClientInstance();
    auto plainClient = createClientInstance();

    auto secureDevice = secureClient.addDevice(connectionString("daq.nd", TlsPort),
                                               secureClientConfig(secureClient, ConfigId));
    auto plainDevice = plainClient.addDevice(connectionString("daq.nd", PlainPort));

    ASSERT_TRUE(secureDevice.assigned());
    ASSERT_TRUE(plainDevice.assigned());
}

TEST_F(NativeTlsTest, PlainTransportRefusedOnTlsPort)
{
    auto server = createServerInstance();
    auto client = createClientInstance();

    ASSERT_ANY_THROW(client.addDevice(connectionString("daq.nd", TlsPort)));
}

TEST_F(NativeTlsTest, TlsTransportRefusedOnPlaintextPort)
{
    auto server = createServerInstance();
    auto client = createClientInstance();

    ASSERT_ANY_THROW(client.addDevice(connectionString("daq.nd", PlainPort),
                                      secureClientConfig(client, ConfigId)));
}

TEST_F(NativeTlsTest, TlsOnlyServer)
{
    auto server = createServerInstance(/*mutualTls*/ false, /*plainPortEnabled*/ false);

    std::set<std::string> ids;
    for (const auto& capability : server.getInfo().getServerCapabilities())
    {
        ids.insert(capability.getProtocolId().toStdString());

        ASSERT_EQ(capability.getAvailableTransports().getKeyList(), List<IString>("tls")) << capability.getProtocolId();
        ASSERT_EQ(capability.getPort(), TlsPort) << capability.getProtocolId();
    }
    ASSERT_EQ(ids, (std::set<std::string>{StreamingId, ConfigId}));

    auto client = createClientInstance();
    auto device = client.addDevice(connectionString("daq.nd", TlsPort),
                                   secureClientConfig(client, ConfigId));
    ASSERT_TRUE(device.assigned());

    ASSERT_ANY_THROW(client.addDevice(connectionString("daq.nd", PlainPort)));
}

TEST_F(NativeTlsTest, ServerRefusesTlsWithoutSecrets)
{
    auto instance = Instance("[[none]]");
    addNativeServerModule(instance);

    auto config = instance.getAvailableServerTypes().get(ServerTypeId).createDefaultConfig();
    config.setPropertyValue("EnableTlsPort", True);

    ASSERT_THROW(instance.addServer(ServerTypeId, config), InvalidParameterException);
}

#ifndef NATIVE_STREAMING_TLS_WITHOUT_SIGN

TEST_F(NativeTlsTest, ServerRefusesMutualTlsWithoutCertificateAuthority)
{
    auto instance = Instance("[[none]]");
    addNativeServerModule(instance);

    auto config = instance.getAvailableServerTypes().get(ServerTypeId).createDefaultConfig();
    config.setPropertyValue("EnableTlsPort", True);
    config.setPropertyValue("CertificateFilePath", ServerCert);
    config.setPropertyValue("KeyFilePath", ServerKey);
    config.setPropertyValue("EnableMutualTls", True);

    ASSERT_THROW(instance.addServer(ServerTypeId, config), InvalidParameterException);
}

#endif

TEST_F(NativeTlsTest, ServerRefusesConfigurationWithoutAnyListener)
{
    auto instance = Instance("[[none]]");
    addNativeServerModule(instance);

    auto config = instance.getAvailableServerTypes().get(ServerTypeId).createDefaultConfig();
    config.setPropertyValue("EnablePort", False);

    ASSERT_THROW(instance.addServer(ServerTypeId, config), InvalidParameterException);
}

TEST_F(NativeTlsTest, PortFromConnectionStringUsedWhenConfigurationIsDefault)
{
    constexpr uint16_t listeningPort = 7500;

    auto serverInstance = Instance("[[none]]");
    addRefDeviceModule(serverInstance);
    addNativeServerModule(serverInstance);
    serverInstance.addDevice("daqref://device1");

    auto serverConfig = secureServerConfig(serverInstance, /*mutualTls*/ false, /*plainPortEnabled*/ false);
    serverConfig.setPropertyValue("NativeStreamingTlsPort", listeningPort);
    serverInstance.addServer(ServerTypeId, serverConfig);

    auto client = createClientInstance();

    // the configuration carries the port of the default transport, which must not be mistaken for a port
    // the caller chose and must not displace the one in the connection string
    auto clientConfig = secureClientConfig(client, ConfigId);
    ASSERT_EQ(clientConfig.getPropertyValue("Port"), PlainPort);

    auto device = client.addDevice(connectionString("daq.nd", listeningPort), clientConfig);

    ASSERT_TRUE(device.assigned());
    ASSERT_EQ(device.getInfo().getConfigurationConnectionInfo().getPort(), listeningPort);
}

TEST_F(NativeTlsTest, PortFromConfigurationOverridesConnectionString)
{
    constexpr uint16_t listeningPort = 7500;

    auto serverInstance = Instance("[[none]]");
    addRefDeviceModule(serverInstance);
    addNativeServerModule(serverInstance);
    serverInstance.addDevice("daqref://device1");

    auto serverConfig = secureServerConfig(serverInstance, /*mutualTls*/ false, /*plainPortEnabled*/ false);
    serverConfig.setPropertyValue("NativeStreamingTlsPort", listeningPort);
    serverInstance.addServer(ServerTypeId, serverConfig);

    auto client = createClientInstance();

    auto clientConfig = secureClientConfig(client, ConfigId);
    clientConfig.setPropertyValue("Port", listeningPort);

    // nothing listens on the port named here, so the connection can only be made from the configuration
    auto device = client.addDevice(connectionString("daq.nd", TlsPort), clientConfig);

    ASSERT_TRUE(device.assigned());
    ASSERT_GT(device.getSignalsRecursive().getCount(), 0u);

    // the connection info reports the port the device is connected at
    ASSERT_EQ(device.getInfo().getConfigurationConnectionInfo().getAvailableTransports().get("tls"), listeningPort);
}

TEST_F(NativeTlsTest, CapabilitiesRemoved)
{
    auto instance = Instance("[[none]]");
    addNativeServerModule(instance);

    // both channels share one capability per protocol
    auto server = instance.addServer(ServerTypeId, secureServerConfig(instance));
    ASSERT_EQ(instance.getInfo().getServerCapabilities().getCount(), 2u);

    instance.removeServer(server);
    ASSERT_EQ(instance.getInfo().getServerCapabilities().getCount(), 0u);

    // both listeners are free again, which a leaked capability would also prevent
    ASSERT_NO_THROW(instance.addServer(ServerTypeId, secureServerConfig(instance)));
}

TEST_F(NativeTlsTest, CapabilitiesRemovedForTlsOnlyServer)
{
    auto instance = Instance("[[none]]");
    addNativeServerModule(instance);

    auto server = instance.addServer(ServerTypeId, secureServerConfig(instance, /*mutualTls*/ false, /*plainPortEnabled*/ false));

    const auto capabilities = instance.getInfo().getServerCapabilities();
    ASSERT_EQ(capabilities.getCount(), 2u);
    for (const auto& capability : capabilities)
        ASSERT_EQ(capability.getAvailableTransports().getKeyList(), List<IString>("tls")) << capability.getProtocolId();

    instance.removeServer(server);
    ASSERT_EQ(instance.getInfo().getServerCapabilities().getCount(), 0u);

    ASSERT_NO_THROW(instance.addServer(ServerTypeId, secureServerConfig(instance, false, /*plainPortEnabled*/ false)));
}

TEST_F(NativeTlsTest, SubscribeReadUnsubscribeOverTls)
{
    SKIP_TEST_MAC_CI;
    auto server = createServerInstance();
    auto client = createClientInstance();

    auto device = client.addDevice(connectionString("daq.ns", TlsPort),
                                   secureClientConfig(client, StreamingId));

    auto signal = device.getSignalsRecursive()[0].template asPtr<IMirroredSignalConfig>();
    auto domainSignal = signal.getDomainSignal().template asPtr<IMirroredSignalConfig>();
    ASSERT_TRUE(domainSignal.assigned());

    const StringPtr streamingSource = signal.getActiveStreamingSource();
    ASSERT_TRUE(streamingSource.assigned());
    ASSERT_EQ(domainSignal.getActiveStreamingSource(), streamingSource);

    // the data comes off the encrypted source, not off a plaintext one to the same server
    ASSERT_NE(streamingSource.toStdString().find(":" + std::to_string(TlsPort)), std::string::npos) << streamingSource;

    test_helpers::SignalAckListener acks(signal);
    test_helpers::SignalAckListener domainAcks(domainSignal);

    StreamReaderPtr reader = daq::StreamReader<double, uint64_t>(signal, ReadTimeoutType::Any);

    ASSERT_TRUE(acks.waitForSubscribeAck());
    ASSERT_EQ(acks.subscribeAckStreaming(), streamingSource);
    ASSERT_TRUE(domainAcks.waitForSubscribeAck());
    ASSERT_EQ(domainAcks.subscribeAckStreaming(), streamingSource);

    // the first read only picks up the descriptor changes
    {
        daq::SizeT count = 0;
        reader.read(nullptr, &count, 1000);
    }

    double samples[100];
    for (int i = 0; i < 5; ++i)
    {
        daq::SizeT count = 100;
        reader.read(samples, &count, 5000);
        EXPECT_GT(count, 0u) << "iteration " << i;
    }

    reader.release();

    ASSERT_TRUE(acks.waitForUnsubscribeAck());
    ASSERT_EQ(acks.unsubscribeAckStreaming(), streamingSource);
    ASSERT_TRUE(domainAcks.waitForUnsubscribeAck());
    ASSERT_EQ(domainAcks.unsubscribeAckStreaming(), streamingSource);
}

// One service advertises both channels: the capability lists every transport, with the port of each
TEST_F(NativeTlsTest, DiscoveringServerWithBothChannels)
{
    const std::string path = "/test/native_tls/discovery/";
    auto server = createDiscoverableServerInstance(path);
    auto client = createClientInstance();

    const auto found = discoverNativeCapabilities(client, path, {ConfigId, StreamingId});

    ASSERT_EQ(found.count(ConfigId), 1u);
    ASSERT_EQ(found.count(StreamingId), 1u);

    for (const auto& [id, capability] : found)
    {
        const auto transports = capability.getAvailableTransports();
        ASSERT_EQ(transports.getKeyList(), List<IString>("plain", "tls")) << id;
        ASSERT_EQ(transports.get("plain"), PlainPort) << id;
        ASSERT_EQ(transports.get("tls"), TlsPort) << id;

        // the default transport decides the port and the connection strings
        ASSERT_EQ(capability.getPort(), PlainPort) << id;

        const std::string connectionString = capability.getConnectionString().toStdString();
        ASSERT_NE(connectionString.find(":" + std::to_string(PlainPort)), std::string::npos)
            << id << " " << connectionString;
    }

    ASSERT_EQ(found.at(ConfigId).getPrefix(), "daq.nd");
    ASSERT_EQ(found.at(StreamingId).getPrefix(), "daq.ns");
}

TEST_F(NativeTlsTest, ConnectToDiscoveredServerOverTls)
{
    const std::string path = "/test/native_tls/discovery_connect/";
    auto server = createDiscoverableServerInstance(path, /*plainPortEnabled*/ false);
    auto client = createClientInstance();

    const auto found = discoverNativeCapabilities(client, path, {ConfigId});
    ASSERT_EQ(found.count(ConfigId), 1u) << "the TLS service was not discovered";

    // the connection string carries the port, the config selects the transport
    auto device = client.addDevice(found.at(ConfigId).getConnectionString(), secureClientConfig(client, ConfigId));

    ASSERT_TRUE(device.assigned());
    ASSERT_GT(device.getSignalsRecursive().getCount(), 0u);
}

// a connection string taken from discovery carries the port of the default transport; the selected one's is used
TEST_F(NativeTlsTest, DiscoveredConnectionStringFollowsSelectedTransport)
{
    const std::string path = "/test/native_tls/discovered_selected_transport/";
    auto server = createDiscoverableServerInstance(path);
    auto client = createClientInstance();

    const auto found = discoverNativeCapabilities(client, path, {ConfigId});
    ASSERT_EQ(found.count(ConfigId), 1u);

    const StringPtr connectionString = found.at(ConfigId).getConnectionString();
    ASSERT_NE(connectionString.toStdString().find(":" + std::to_string(PlainPort)), std::string::npos) << connectionString;

    auto device = client.addDevice(connectionString, secureClientConfig(client, ConfigId));

    const auto connectionInfo = device.getInfo().getConfigurationConnectionInfo();
    ASSERT_EQ(connectionInfo.getPort(), TlsPort);
    ASSERT_EQ(connectionInfo.getAvailableTransports().getKeyList(), List<IString>("tls"));
}

TEST_F(NativeTlsTest, SmartConnectionFollowsSelectedTransport)
{
    const std::string path = "/test/native_tls/smart_selected_transport/";
    const std::string serialNumber = "NativeTlsSmartConnection";
    auto server = createDiscoverableServerInstance(path, /*plainPortEnabled*/ true, serialNumber);
    auto client = createClientInstance();

    const auto found = discoverNativeCapabilities(client, path, {ConfigId});
    ASSERT_EQ(found.count(ConfigId), 1u);

    auto device = client.addDevice("daq://openDAQ_" + serialNumber, secureClientConfig(client, ConfigId));

    const auto connectionInfo = device.getInfo().getConfigurationConnectionInfo();
    ASSERT_EQ(connectionInfo.getPort(), TlsPort);
    ASSERT_EQ(connectionInfo.getAvailableTransports().getKeyList(), List<IString>("tls"));
}

// an explicit Port in the configuration wins over the port the server announces for the selected transport
TEST_F(NativeTlsTest, PortFromConfigurationOverridesDiscoveredTransportPort)
{
    constexpr uint16_t listeningPort = 7500;
    const std::string path = "/test/native_tls/discovered_explicit_port/";

    // announces the TLS transport at its default port
    auto discoverableServer = createDiscoverableServerInstance(path);

    // the only server listening at the port named in the configuration
    auto serverInstance = Instance("[[none]]");
    addRefDeviceModule(serverInstance);
    addNativeServerModule(serverInstance);
    serverInstance.addDevice("daqref://device1");

    auto serverConfig = secureServerConfig(serverInstance, /*mutualTls*/ false, /*plainPortEnabled*/ false);
    serverConfig.setPropertyValue("NativeStreamingTlsPort", listeningPort);
    serverConfig.setPropertyValue("Path", path);
    serverInstance.addServer(ServerTypeId, serverConfig);

    auto client = createClientInstance();

    const auto found = discoverNativeCapabilities(client, path, {ConfigId});
    ASSERT_EQ(found.count(ConfigId), 1u);
    ASSERT_EQ(found.at(ConfigId).getAvailableTransports().get("tls"), TlsPort);

    auto clientConfig = secureClientConfig(client, ConfigId);
    clientConfig.setPropertyValue("Port", listeningPort);

    auto device = client.addDevice(found.at(ConfigId).getConnectionString(), clientConfig);

    ASSERT_EQ(device.getInfo().getConfigurationConnectionInfo().getAvailableTransports().get("tls"), listeningPort);
}

TEST_F(NativeTlsTest, DiscoveringTlsOnlyServer)
{
    const std::string path = "/test/native_tls/discovery_tls_only/";
    auto server = createDiscoverableServerInstance(path, /*plainPortEnabled*/ false);
    auto client = createClientInstance();

    const auto found = discoverNativeCapabilities(client, path, {ConfigId, StreamingId});

    ASSERT_EQ(found.count(ConfigId), 1u) << "the TLS service was not discovered";
    ASSERT_EQ(found.count(StreamingId), 1u) << "the TLS service was not discovered";

    // with no plaintext listener only the TLS transport is advertised
    for (const auto& [id, capability] : found)
    {
        ASSERT_EQ(capability.getAvailableTransports().getKeyList(), List<IString>("tls")) << id;
        ASSERT_EQ(capability.getPort(), TlsPort) << id;
    }
}

// the streaming attached to the device connects at the port the server announces for the selected transport
TEST_F(NativeTlsTest, StreamsOverSelectedTransport)
{
    auto server = createServerInstance();
    auto client = createClientInstance();

    auto addDeviceConfig = client.createDefaultAddDeviceConfig();
    applySecureClientConfig(addDeviceConfig);

    auto device = client.addDevice(connectionString("daq.nd", TlsPort), addDeviceConfig);
    const auto sources = device.asPtr<IMirroredDevice>().getStreamingSources();

    ASSERT_EQ(sources.getCount(), 1u);
    ASSERT_EQ(sources[0].getProtocolId(), StreamingId);
    ASSERT_NE(sources[0].getConnectionString().toStdString().find(":" + std::to_string(TlsPort)), std::string::npos);
}

// the streaming transport is selected separately: a device configured over TLS streams over the default one
TEST_F(NativeTlsTest, StreamsOverDefaultTransportWhenOnlyDeviceSelectsTls)
{
    auto server = createServerInstance();
    auto client = createClientInstance();

    auto addDeviceConfig = client.createDefaultAddDeviceConfig();
    applySecureClientConfig(addDeviceConfig);
    AddComponentConfigPtr streamingAddConfig = addDeviceConfig.getPropertyValue(std::string("Streaming.") + StreamingId + ".AddStreamingConfig");
    streamingAddConfig.setTransport(CONST_NATIVE_TRANSPORT_PLAIN);

    auto device = client.addDevice(connectionString("daq.nd", TlsPort), addDeviceConfig);
    const auto sources = device.asPtr<IMirroredDevice>().getStreamingSources();

    ASSERT_EQ(sources.getCount(), 1u);
    ASSERT_NE(sources[0].getConnectionString().toStdString().find(":" + std::to_string(PlainPort)), std::string::npos);
}

// no streaming is attached over a transport the server does not offer
TEST_F(NativeTlsTest, NoStreamingWhenServerLacksSelectedTransport)
{
    auto server = createServerInstance(/*mutualTls*/ false, /*plainPortEnabled*/ false);
    auto client = createClientInstance();

    // the streaming is left at the default plain transport, which this server does not serve
    auto addDeviceConfig = client.createDefaultAddDeviceConfig();
    applySecureClientConfig(addDeviceConfig);
    AddComponentConfigPtr streamingAddConfig = addDeviceConfig.getPropertyValue(std::string("Streaming.") + StreamingId + ".AddStreamingConfig");
    streamingAddConfig.setTransport(CONST_NATIVE_TRANSPORT_PLAIN);

    auto device = client.addDevice(connectionString("daq.nd", TlsPort), addDeviceConfig);

    ASSERT_TRUE(device.assigned());
    ASSERT_EQ(device.asPtr<IMirroredDevice>().getStreamingSources().getCount(), 0u);
}

// the "Connected" connection statuses name the transport of their channel, while the device status keeps its message
TEST_F(NativeTlsTest, ConnectedStatusesNameTransport)
{
    auto server = createServerInstance();
    auto client = createClientInstance();

    auto addDeviceConfig = client.createDefaultAddDeviceConfig();
    applySecureClientConfig(addDeviceConfig);

    auto device = client.addDevice(connectionString("daq.nd", TlsPort), addDeviceConfig);
    const auto connectionStatusContainer = device.getConnectionStatusContainer();

    ASSERT_EQ(connectionStatusContainer.getStatus("ConfigurationStatus"), "Connected");
    ASSERT_EQ(connectionStatusContainer.getStatusMessage("ConfigurationStatus"), "Transport: tls");
    ASSERT_EQ(connectionStatusContainer.getStatus("StreamingStatus_OpenDAQNativeStreaming_1"), "Connected");
    ASSERT_EQ(connectionStatusContainer.getStatusMessage("StreamingStatus_OpenDAQNativeStreaming_1"), "Transport: tls");
    ASSERT_EQ(device.getStatusContainer().getStatusMessage("ConnectionStatus"), "");
}

TEST_F(NativeTlsTest, ConnectedStatusesNameStreamingTransportSelectedSeparately)
{
    auto server = createServerInstance();
    auto client = createClientInstance();

    auto addDeviceConfig = client.createDefaultAddDeviceConfig();
    applySecureClientConfig(addDeviceConfig);
    AddComponentConfigPtr streamingAddConfig = addDeviceConfig.getPropertyValue(std::string("Streaming.") + StreamingId + ".AddStreamingConfig");
    streamingAddConfig.setTransport(CONST_NATIVE_TRANSPORT_PLAIN);

    auto device = client.addDevice(connectionString("daq.nd", TlsPort), addDeviceConfig);
    const auto connectionStatusContainer = device.getConnectionStatusContainer();

    ASSERT_EQ(connectionStatusContainer.getStatusMessage("ConfigurationStatus"), "Transport: tls");
    ASSERT_EQ(connectionStatusContainer.getStatusMessage("StreamingStatus_OpenDAQNativeStreaming_1"), "Transport: plain");
}

TEST_F(NativeTlsTest, PseudoDeviceConnectedStatusNamesTransport)
{
    auto server = createServerInstance();
    auto client = createClientInstance();

    auto device = client.addDevice(connectionString("daq.ns", TlsPort),
                                   secureClientConfig(client, StreamingId));

    ASSERT_EQ(device.getConnectionStatusContainer().getStatus("StreamingStatus_OpenDAQNativeStreaming_1"), "Connected");
    ASSERT_EQ(device.getConnectionStatusContainer().getStatusMessage("StreamingStatus_OpenDAQNativeStreaming_1"), "Transport: tls");
    ASSERT_EQ(device.getStatusContainer().getStatusMessage("ConnectionStatus"), "");
}

#endif
