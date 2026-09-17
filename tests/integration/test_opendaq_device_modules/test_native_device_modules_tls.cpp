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

// The secure (daq.nss:// and daq.nds://) channel of the native streaming modules.
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
InstancePtr createDiscoverableServerInstance(const std::string& path, bool plainPortEnabled = true)
{
    auto instance = InstanceBuilder()
                        .setModulePath("[[none]]")
                        .addDiscoveryServer("mdns")
                        .setDefaultRootDeviceLocalId("local")
                        .build();

    addRefDeviceModule(instance);
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

TEST_F(NativeTlsTest, SecureTypesAvailable)
{
    auto client = createClientInstance();
    const auto deviceTypes = client.getAvailableDeviceTypes();

    ASSERT_TRUE(deviceTypes.hasKey(SecureConfigId));
    ASSERT_TRUE(deviceTypes.hasKey(SecureStreamingId));
    ASSERT_EQ(deviceTypes.get(SecureConfigId).getConnectionStringPrefix(), "daq.nds");
    ASSERT_EQ(deviceTypes.get(SecureStreamingId).getConnectionStringPrefix(), "daq.nss");
}

TEST_F(NativeTlsTest, ServerPublishesBothChannels)
{
    auto server = createServerInstance();
    const auto capabilities = server.getInfo().getServerCapabilities();

    std::set<std::string> ids;
    for (const auto& capability : capabilities)
        ids.insert(capability.getProtocolId().toStdString());

    ASSERT_EQ(ids.count(PlainStreamingId), 1u);
    ASSERT_EQ(ids.count(PlainConfigId), 1u);
    ASSERT_EQ(ids.count(SecureStreamingId), 1u);
    ASSERT_EQ(ids.count(SecureConfigId), 1u);

    for (const auto& capability : capabilities)
    {
        const auto id = capability.getProtocolId().toStdString();
        if (id != SecureStreamingId && id != SecureConfigId)
            continue;

        ASSERT_EQ(capability.getProtocolGroupId(), "NativeStreaming");
        ASSERT_EQ(capability.getProtocolSecurityLevel(), 10) << id;
        ASSERT_EQ(capability.getPort(), TlsPort) << id;
    }
}

TEST_F(NativeTlsTest, ConnectDeviceServerAuthentication)
{
    auto server = createServerInstance();
    auto client = createClientInstance();

    auto device = client.addDevice(connectionString("daq.nds", TlsPort),
                                   secureClientConfig(client, SecureConfigId));

    ASSERT_TRUE(device.assigned());
    ASSERT_GT(device.getSignalsRecursive().getCount(), 0u);

    const auto connectionInfo = device.getInfo().getConfigurationConnectionInfo();
    ASSERT_EQ(connectionInfo.getProtocolId(), SecureConfigId);
    ASSERT_EQ(connectionInfo.getPrefix(), "daq.nds");
    ASSERT_EQ(connectionInfo.getPort(), TlsPort);
    ASSERT_EQ(connectionInfo.getProtocolSecurityLevel(), 10);
}

// no port in the connection string: the secure channel has to fall back to its own default, not to
// the plaintext one
TEST_F(NativeTlsTest, ConnectDeviceWithoutExplicitPort)
{
    auto server = createServerInstance();
    auto client = createClientInstance();

    auto device = client.addDevice("daq.nds://127.0.0.1/", secureClientConfig(client, SecureConfigId));

    ASSERT_TRUE(device.assigned());
    ASSERT_EQ(device.getInfo().getConfigurationConnectionInfo().getPort(), TlsPort);
}

TEST_F(NativeTlsTest, ConnectPseudoDeviceWithoutExplicitPort)
{
    auto server = createServerInstance();
    auto client = createClientInstance();

    auto device = client.addDevice("daq.nss://127.0.0.1/", secureClientConfig(client, SecureStreamingId));

    ASSERT_TRUE(device.assigned());
    ASSERT_EQ(device.getInfo().getConfigurationConnectionInfo().getPort(), TlsPort);
}

TEST_F(NativeTlsTest, ConnectPseudoDevice)
{
    auto server = createServerInstance();
    auto client = createClientInstance();

    auto device = client.addDevice(connectionString("daq.nss", TlsPort),
                                   secureClientConfig(client, SecureStreamingId));

    ASSERT_TRUE(device.assigned());
    ASSERT_GT(device.getSignals(search::Recursive(search::Any())).getCount(), 0u);
}

TEST_F(NativeTlsTest, ConnectMutualTls)
{
    auto server = createServerInstance(/*mutualTls*/ true);
    auto client = createClientInstance();

    auto device = client.addDevice(connectionString("daq.nds", TlsPort),
                                   secureClientConfig(client, SecureConfigId, /*mutualTls*/ true));

    ASSERT_TRUE(device.assigned());
    ASSERT_GT(device.getSignalsRecursive().getCount(), 0u);
}

TEST_F(NativeTlsTest, ConnectWithoutVerification)
{
    auto server = createServerInstance();
    auto client = createClientInstance();

    auto device = client.addDevice(connectionString("daq.nds", TlsPort),
                                   unverifiedClientConfig(client, SecureConfigId));

    ASSERT_TRUE(device.assigned());
    ASSERT_GT(device.getSignalsRecursive().getCount(), 0u);
}

TEST_F(NativeTlsTest, RejectedByWrongCertificateAuthority)
{
    auto server = createServerInstance();
    auto client = createClientInstance();

    ASSERT_ANY_THROW(client.addDevice(connectionString("daq.nds", TlsPort),
                                      secureClientConfig(client, SecureConfigId, /*mutualTls*/ false, OtherCaCert)));
}

TEST_F(NativeTlsTest, RejectedWithoutCertificateAuthority)
{
    auto server = createServerInstance();
    auto client = createClientInstance();

    // verification is on by default and has nothing to verify against
    auto config = client.getAvailableDeviceTypes().get(SecureConfigId).createDefaultConfig();
    ASSERT_ANY_THROW(client.addDevice(connectionString("daq.nds", TlsPort), config));
}

TEST_F(NativeTlsTest, RejectedByMutualTlsWithoutClientCertificate)
{
    auto server = createServerInstance(/*mutualTls*/ true);
    auto client = createClientInstance();

    ASSERT_ANY_THROW(client.addDevice(connectionString("daq.nds", TlsPort),
                                      secureClientConfig(client, SecureConfigId, /*mutualTls*/ false)));
}

TEST_F(NativeTlsTest, PlaintextChannelStillWorksAlongside)
{
    auto server = createServerInstance();

    // separate clients: the same remote device reached twice from one instance collides on local id
    auto secureClient = createClientInstance();
    auto plainClient = createClientInstance();

    auto secureDevice = secureClient.addDevice(connectionString("daq.nds", TlsPort),
                                               secureClientConfig(secureClient, SecureConfigId));
    auto plainDevice = plainClient.addDevice(connectionString("daq.nd", PlainPort));

    ASSERT_TRUE(secureDevice.assigned());
    ASSERT_TRUE(plainDevice.assigned());
}

TEST_F(NativeTlsTest, PlaintextPrefixRefusedOnTlsPort)
{
    auto server = createServerInstance();
    auto client = createClientInstance();

    ASSERT_ANY_THROW(client.addDevice(connectionString("daq.nd", TlsPort)));
}

TEST_F(NativeTlsTest, SecurePrefixRefusedOnPlaintextPort)
{
    auto server = createServerInstance();
    auto client = createClientInstance();

    ASSERT_ANY_THROW(client.addDevice(connectionString("daq.nds", PlainPort),
                                      secureClientConfig(client, SecureConfigId)));
}

TEST_F(NativeTlsTest, TlsOnlyServer)
{
    auto server = createServerInstance(/*mutualTls*/ false, /*plainPortEnabled*/ false);

    std::set<std::string> ids;
    for (const auto& capability : server.getInfo().getServerCapabilities())
        ids.insert(capability.getProtocolId().toStdString());

    ASSERT_EQ(ids.count(SecureStreamingId), 1u);
    ASSERT_EQ(ids.count(SecureConfigId), 1u);
    ASSERT_EQ(ids.count(PlainStreamingId), 0u);
    ASSERT_EQ(ids.count(PlainConfigId), 0u);

    auto client = createClientInstance();
    auto device = client.addDevice(connectionString("daq.nds", TlsPort),
                                   secureClientConfig(client, SecureConfigId));
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

    // the configuration carries its channel default, which must not be mistaken for a port the caller
    // chose and must not displace the one in the connection string
    auto clientConfig = secureClientConfig(client, SecureConfigId);
    ASSERT_EQ(clientConfig.getPropertyValue("Port"), TlsPort);

    auto device = client.addDevice(connectionString("daq.nds", listeningPort), clientConfig);

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

    auto clientConfig = secureClientConfig(client, SecureConfigId);
    clientConfig.setPropertyValue("Port", listeningPort);

    // nothing listens on the port named here, so the connection can only be made from the configuration
    auto device = client.addDevice(connectionString("daq.nds", TlsPort), clientConfig);

    ASSERT_TRUE(device.assigned());
    ASSERT_GT(device.getSignalsRecursive().getCount(), 0u);
}

TEST_F(NativeTlsTest, CapabilitiesRemovedPerChannel)
{
    auto instance = Instance("[[none]]");
    addNativeServerModule(instance);

    auto server = instance.addServer(ServerTypeId, secureServerConfig(instance));
    ASSERT_EQ(instance.getInfo().getServerCapabilities().getCount(), 4u);

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
        ASSERT_EQ(capability.getProtocolSecurityLevel(), 10) << capability.getProtocolId();

    instance.removeServer(server);
    ASSERT_EQ(instance.getInfo().getServerCapabilities().getCount(), 0u);

    ASSERT_NO_THROW(instance.addServer(ServerTypeId, secureServerConfig(instance, false, /*plainPortEnabled*/ false)));
}

TEST_F(NativeTlsTest, SubscribeReadUnsubscribeOverTls)
{
    SKIP_TEST_MAC_CI;
    auto server = createServerInstance();
    auto client = createClientInstance();

    auto device = client.addDevice(connectionString("daq.nss", TlsPort),
                                   secureClientConfig(client, SecureStreamingId));

    auto signal = device.getSignalsRecursive()[0].template asPtr<IMirroredSignalConfig>();
    auto domainSignal = signal.getDomainSignal().template asPtr<IMirroredSignalConfig>();
    ASSERT_TRUE(domainSignal.assigned());

    const StringPtr streamingSource = signal.getActiveStreamingSource();
    ASSERT_TRUE(streamingSource.assigned());
    ASSERT_EQ(domainSignal.getActiveStreamingSource(), streamingSource);

    // the data comes off the encrypted source, not off a plaintext one to the same server
    ASSERT_EQ(streamingSource.toStdString().find("daq.nss://"), 0u) << streamingSource;

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

TEST_F(NativeTlsTest, DiscoveringSecureServer)
{
    const std::string path = "/test/native_tls/discovery/";
    auto server = createDiscoverableServerInstance(path);
    auto client = createClientInstance();

    const auto found = discoverNativeCapabilities(client, path, {SecureConfigId, SecureStreamingId});

    ASSERT_EQ(found.count(SecureConfigId), 1u) << "the TLS service was not discovered";
    ASSERT_EQ(found.count(SecureStreamingId), 1u) << "the TLS service was not discovered";

    // the plaintext listener is advertised as a service of its own, alongside
    ASSERT_EQ(found.count(PlainConfigId), 1u);
    ASSERT_EQ(found.count(PlainStreamingId), 1u);

    for (const auto& [id, capability] : found)
    {
        const bool secure = id == SecureConfigId || id == SecureStreamingId;
        const auto port = secure ? TlsPort : PlainPort;

        ASSERT_EQ(capability.getProtocolGroupId(), "NativeStreaming") << id;
        ASSERT_EQ(capability.getProtocolSecurityLevel(), secure ? 10 : 0) << id;

        // each service advertises the port of the channel it belongs to, and both the property and the
        // connection strings built from it have to carry that port
        ASSERT_EQ(capability.getPort(), port) << id;

        const std::string connectionString = capability.getConnectionString().toStdString();
        ASSERT_NE(connectionString.find(":" + std::to_string(port)), std::string::npos)
            << id << " " << connectionString;
    }

    ASSERT_EQ(found.at(SecureConfigId).getPrefix(), "daq.nds");
    ASSERT_EQ(found.at(SecureStreamingId).getPrefix(), "daq.nss");
}

TEST_F(NativeTlsTest, ConnectToDiscoveredSecureServer)
{
    const std::string path = "/test/native_tls/discovery_connect/";
    auto server = createDiscoverableServerInstance(path);
    auto client = createClientInstance();

    const auto found = discoverNativeCapabilities(client, path, {SecureConfigId});
    ASSERT_EQ(found.count(SecureConfigId), 1u) << "the TLS service was not discovered";

    auto device = client.addDevice(found.at(SecureConfigId).getConnectionString(),
                                   secureClientConfig(client, SecureConfigId));

    ASSERT_TRUE(device.assigned());
    ASSERT_GT(device.getSignalsRecursive().getCount(), 0u);
}

TEST_F(NativeTlsTest, DiscoveringTlsOnlyServer)
{
    const std::string path = "/test/native_tls/discovery_tls_only/";
    auto server = createDiscoverableServerInstance(path, /*plainPortEnabled*/ false);
    auto client = createClientInstance();

    const auto found = discoverNativeCapabilities(client, path, {SecureConfigId, SecureStreamingId});

    ASSERT_EQ(found.count(SecureConfigId), 1u) << "the TLS service was not discovered";
    ASSERT_EQ(found.count(SecureStreamingId), 1u) << "the TLS service was not discovered";

    // with no plaintext listener there is nothing to advertise on the plaintext service
    ASSERT_EQ(found.count(PlainConfigId), 0u);
    ASSERT_EQ(found.count(PlainStreamingId), 0u);
}

// The mirrored device is meant to prefer the encrypted streaming source over the plaintext one, but
// that needs OpenDAQNativeStreamingSecure in the prioritized protocol list.
// Until then a client connected over daq.nds:// gets no native streaming source from
// the capabilities at all, since the secure protocol id is not in the list.
TEST_F(NativeTlsTest, DISABLED_PrefersSecureStreamingSource)
{
    auto server = createServerInstance();
    auto client = createClientInstance();

    auto addDeviceConfig = client.createDefaultAddDeviceConfig();
    applySecureClientConfig(addDeviceConfig);

    auto device = client.addDevice(connectionString("daq.nds", TlsPort), addDeviceConfig);
    const auto sources = device.asPtr<IMirroredDevice>().getStreamingSources();

    ASSERT_EQ(sources.getCount(), 1u);
    ASSERT_EQ(sources[0].getProtocolId(), SecureStreamingId);
}

#endif
