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

// The TLS channel of the native streaming modules: the client selects it with the wss transport option.
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
#include <utility>
#include <vector>

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

/// @brief a server with only the TLS listener, on the given port
InstancePtr createTlsOnlyServerInstance(uint16_t tlsPort)
{
    auto instance = Instance("[[none]]");
    addRefDeviceModule(instance);
    addNativeServerModule(instance);
    instance.addDevice("daqref://device1");

    auto config = secureServerConfig(instance, /*mutualTls*/ false, /*plainPortEnabled*/ false);
    config.setPropertyValue("NativeStreamingTlsPort", tlsPort);
    instance.addServer(ServerTypeId, config);
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

/// @brief checks the ids, order and ports of the transport options of a capability
void expectOptions(const ServerCapabilityPtr& capability, const std::vector<std::pair<std::string, Int>>& expected)
{
    const DictPtr<IString, ITransportOption> options = capability.getAvailableTransportOptions();

    std::vector<std::pair<std::string, Int>> actual;
    for (const auto& [id, option] : options)
        actual.emplace_back(id.toStdString(), TransportOptionPtr(option).getPort());

    ASSERT_EQ(actual, expected) << capability.getProtocolId();
}

/// @brief checks that the device is connected over the given option at the given port
void expectConnectedOver(const DevicePtr& device, const std::string& optionId, Int port)
{
    expectOptions(device.getInfo().getConfigurationConnectionInfo(), {{optionId, port}});
}

/// @brief the connection strings of the streaming sources of a device
std::vector<std::string> streamingSourceConnectionStrings(const DevicePtr& device)
{
    std::vector<std::string> connectionStrings;
    for (const auto& source : device.asPtr<IMirroredDevice>().getStreamingSources())
        connectionStrings.push_back(source.getConnectionString().toStdString());
    return connectionStrings;
}

/// @brief waits until a connection status of the device has the given value
bool waitForConnectionStatus(const DevicePtr& device,
                             const std::string& statusName,
                             const std::string& value,
                             std::chrono::seconds timeout = std::chrono::seconds(15))
{
    const auto statuses = device.getConnectionStatusContainer();
    const auto deadline = std::chrono::steady_clock::now() + timeout;
    while (std::chrono::steady_clock::now() < deadline)
    {
        if (statuses.getStatus(statusName).getValue().toStdString() == value)
            return true;
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    return false;
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

TEST_F(NativeTlsTest, TypesListTlsOption)
{
    auto client = createClientInstance();
    const auto deviceTypes = client.getAvailableDeviceTypes();

    for (const auto& id : {PlainConfigId, PlainStreamingId})
        ASSERT_TRUE(deviceTypes.get(id).getAvailableTransportOptions().hasKey(WssOption)) << id;
}

// The server publishes one capability per protocol, with an option for each listener, plaintext first
TEST_F(NativeTlsTest, ServerPublishesOptionPerListener)
{
    auto server = createServerInstance();
    const auto capabilities = server.getInfo().getServerCapabilities();

    std::set<std::string> ids;
    for (const auto& capability : capabilities)
    {
        ids.insert(capability.getProtocolId().toStdString());
        expectOptions(capability, {{WsOption, PlainPort}, {WssOption, TlsPort}});
        ASSERT_EQ(test_helpers::capabilityPort(capability), PlainPort) << capability.getProtocolId();
    }

    ASSERT_EQ(ids, (std::set<std::string>{PlainStreamingId, PlainConfigId}));
}

TEST_F(NativeTlsTest, ConnectDeviceServerAuthentication)
{
    auto server = createServerInstance();
    auto client = createClientInstance();

    const auto deviceConnectionString = connectionString("daq.nd", TlsPort);
    auto device = client.addDevice(deviceConnectionString, secureAddDeviceConfig(client, deviceConnectionString));

    ASSERT_TRUE(device.assigned());
    ASSERT_GT(device.getSignalsRecursive().getCount(), 0u);

    const auto connectionInfo = device.getInfo().getConfigurationConnectionInfo();
    ASSERT_EQ(connectionInfo.getProtocolId(), PlainConfigId);
    ASSERT_EQ(connectionInfo.getPrefix(), "daq.nd");
    ASSERT_EQ(test_helpers::capabilityPort(connectionInfo), TlsPort);

    // only the option the connection uses
    expectConnectedOver(device, WssOption, TlsPort);
}

// The capabilities of the server keep the port of their default option, even though the connection
// is made over another one
TEST_F(NativeTlsTest, CompletedCapabilitiesNameTheirOwnPort)
{
    auto server = createServerInstance();
    auto client = createClientInstance();

    const auto deviceConnectionString = connectionString("daq.nd", TlsPort);
    auto device = client.addDevice(deviceConnectionString, secureAddDeviceConfig(client, deviceConnectionString));

    for (const auto& id : {PlainConfigId, PlainStreamingId})
    {
        const auto capability = device.getInfo().getServerCapability(id);
        const std::string capabilityConnectionString = capability.getConnectionString().toStdString();
        ASSERT_EQ(test_helpers::capabilityPort(capability), PlainPort) << id;
        ASSERT_NE(capabilityConnectionString.find(":" + std::to_string(PlainPort)), std::string::npos) << id << " " << capabilityConnectionString;
    }
}

// Without a selection the config connects over the first option of the type, at its port
TEST_F(NativeTlsTest, ConnectDeviceOverWsByDefault)
{
    auto server = createServerInstance();
    auto client = createClientInstance();

    const std::string deviceConnectionString = "daq.nd://127.0.0.1/";
    auto device = client.addDevice(deviceConnectionString, addDeviceConfig(client, deviceConnectionString));

    ASSERT_TRUE(device.assigned());
    expectConnectedOver(device, WsOption, PlainPort);
}

// no port in the connection string: the TLS option has to fall back to its own port, not to the
// plaintext one
TEST_F(NativeTlsTest, ConnectDeviceWithoutExplicitPort)
{
    auto server = createServerInstance();
    auto client = createClientInstance();

    const std::string deviceConnectionString = "daq.nd://127.0.0.1/";
    auto device = client.addDevice(deviceConnectionString, secureAddDeviceConfig(client, deviceConnectionString));

    ASSERT_TRUE(device.assigned());
    expectConnectedOver(device, WssOption, TlsPort);
}

TEST_F(NativeTlsTest, ConnectPseudoDeviceWithoutExplicitPort)
{
    auto server = createServerInstance();
    auto client = createClientInstance();

    const std::string deviceConnectionString = "daq.ns://127.0.0.1/";
    auto device = client.addDevice(deviceConnectionString, secureAddDeviceConfig(client, deviceConnectionString));

    ASSERT_TRUE(device.assigned());
    expectConnectedOver(device, WssOption, TlsPort);
}

TEST_F(NativeTlsTest, ConnectPseudoDevice)
{
    auto server = createServerInstance();
    auto client = createClientInstance();

    const auto deviceConnectionString = connectionString("daq.ns", TlsPort);
    auto device = client.addDevice(deviceConnectionString, secureAddDeviceConfig(client, deviceConnectionString));

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
    const auto deviceConnectionString = connectionString("daq.nd", TlsPort);
    auto device = client.addDevice(deviceConnectionString, secureAddDeviceConfig(client, deviceConnectionString));

    ASSERT_TRUE(device.assigned());
    ASSERT_GT(device.getSignalsRecursive().getCount(), 0u);
}

#else

TEST_F(NativeTlsTest, ConnectMutualTls)
{
    auto server = createServerInstance(/*mutualTls*/ true);
    auto client = createClientInstance();

    const auto deviceConnectionString = connectionString("daq.nd", TlsPort);
    auto device = client.addDevice(deviceConnectionString,
                                   secureAddDeviceConfig(client, deviceConnectionString, /*mutualTls*/ true));

    ASSERT_TRUE(device.assigned());
    ASSERT_GT(device.getSignalsRecursive().getCount(), 0u);
}

TEST_F(NativeTlsTest, ConnectWithoutVerification)
{
    auto server = createServerInstance();
    auto client = createClientInstance();

    const auto deviceConnectionString = connectionString("daq.nd", TlsPort);
    auto config = addDeviceConfig(client, deviceConnectionString);
    selectUnverifiedTls(config);
    auto device = client.addDevice(deviceConnectionString, config);

    ASSERT_TRUE(device.assigned());
    ASSERT_GT(device.getSignalsRecursive().getCount(), 0u);
}

TEST_F(NativeTlsTest, RejectedByWrongCertificateAuthority)
{
    auto server = createServerInstance();
    auto client = createClientInstance();

    const auto deviceConnectionString = connectionString("daq.nd", TlsPort);
    ASSERT_ANY_THROW(client.addDevice(deviceConnectionString,
                                      secureAddDeviceConfig(client, deviceConnectionString, /*mutualTls*/ false, OtherCaCert)));
}

TEST_F(NativeTlsTest, RejectedWithoutCertificateAuthority)
{
    auto server = createServerInstance();
    auto client = createClientInstance();

    // verification is on by default and has nothing to verify against
    const auto deviceConnectionString = connectionString("daq.nd", TlsPort);
    auto config = addDeviceConfig(client, deviceConnectionString);
    config.setTransportOption(WssOption);
    ASSERT_ANY_THROW(client.addDevice(deviceConnectionString, config));
}

TEST_F(NativeTlsTest, RejectedByMutualTlsWithoutClientCertificate)
{
    auto server = createServerInstance(/*mutualTls*/ true);
    auto client = createClientInstance();

    const auto deviceConnectionString = connectionString("daq.nd", TlsPort);
    ASSERT_ANY_THROW(client.addDevice(deviceConnectionString,
                                      secureAddDeviceConfig(client, deviceConnectionString, /*mutualTls*/ false)));
}

#endif

TEST_F(NativeTlsTest, PlaintextChannelStillWorksAlongside)
{
    auto server = createServerInstance();

    // separate clients: the same remote device reached twice from one instance collides on local id
    auto secureClient = createClientInstance();
    auto plainClient = createClientInstance();

    const auto secureConnectionString = connectionString("daq.nd", TlsPort);
    auto secureDevice = secureClient.addDevice(secureConnectionString, secureAddDeviceConfig(secureClient, secureConnectionString));
    auto plainDevice = plainClient.addDevice(connectionString("daq.nd", PlainPort));

    ASSERT_TRUE(secureDevice.assigned());
    ASSERT_TRUE(plainDevice.assigned());
    expectConnectedOver(plainDevice, WsOption, PlainPort);
}

// A legacy config never selected a transport option: it connects over ws, as before
TEST_F(NativeTlsTest, LegacyConfigConnectsOverWs)
{
    auto server = createServerInstance();
    auto client = createClientInstance();

    auto device = client.addDevice("daq.nd://127.0.0.1/", client.createDefaultAddDeviceConfig());

    ASSERT_TRUE(device.assigned());
    expectConnectedOver(device, WsOption, PlainPort);
}

TEST_F(NativeTlsTest, PlaintextOptionRefusedOnTlsPort)
{
    auto server = createServerInstance();
    auto client = createClientInstance();

    ASSERT_ANY_THROW(client.addDevice(connectionString("daq.nd", TlsPort)));
}

TEST_F(NativeTlsTest, TlsOptionRefusedOnPlaintextPort)
{
    auto server = createServerInstance();
    auto client = createClientInstance();

    const auto deviceConnectionString = connectionString("daq.nd", PlainPort);
    auto config = secureAddDeviceConfig(client, deviceConnectionString);
    ASSERT_ANY_THROW(client.addDevice(deviceConnectionString, config));
}

TEST_F(NativeTlsTest, TlsOnlyServer)
{
    auto server = createServerInstance(/*mutualTls*/ false, /*plainPortEnabled*/ false);

    std::set<std::string> ids;
    for (const auto& capability : server.getInfo().getServerCapabilities())
    {
        ids.insert(capability.getProtocolId().toStdString());
        expectOptions(capability, {{WssOption, TlsPort}});
        ASSERT_EQ(test_helpers::capabilityPort(capability), TlsPort) << capability.getProtocolId();
    }
    ASSERT_EQ(ids, (std::set<std::string>{PlainStreamingId, PlainConfigId}));

    auto client = createClientInstance();
    const auto deviceConnectionString = connectionString("daq.nd", TlsPort);
    auto device = client.addDevice(deviceConnectionString, secureAddDeviceConfig(client, deviceConnectionString));
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

TEST_F(NativeTlsTest, PortFromConnectionString)
{
    constexpr uint16_t listeningPort = 7500;
    auto server = createTlsOnlyServerInstance(listeningPort);
    auto client = createClientInstance();

    const auto deviceConnectionString = connectionString("daq.nd", listeningPort);
    auto device = client.addDevice(deviceConnectionString, secureAddDeviceConfig(client, deviceConnectionString));

    ASSERT_TRUE(device.assigned());
    expectConnectedOver(device, WssOption, listeningPort);
}

TEST_F(NativeTlsTest, PortSetWithoutPortInConnectionString)
{
    constexpr uint16_t listeningPort = 7500;
    auto server = createTlsOnlyServerInstance(listeningPort);
    auto client = createClientInstance();

    const std::string deviceConnectionString = "daq.nd://127.0.0.1/";
    auto config = secureAddDeviceConfig(client, deviceConnectionString);
    config.setPort(listeningPort);
    auto device = client.addDevice(deviceConnectionString, config);

    ASSERT_TRUE(device.assigned());
    expectConnectedOver(device, WssOption, listeningPort);
}

// The port set wins over another port the connection string names; the module manager logs a warning
TEST_F(NativeTlsTest, PortSetOverridesConnectionString)
{
    constexpr uint16_t listeningPort = 7500;
    auto server = createTlsOnlyServerInstance(listeningPort);
    auto client = createClientInstance();

    // nothing listens on the port named here, so the connection can only be made at the port set
    const auto deviceConnectionString = connectionString("daq.nd", TlsPort);
    auto config = secureAddDeviceConfig(client, deviceConnectionString);
    config.setPort(listeningPort);
    auto device = client.addDevice(deviceConnectionString, config);

    ASSERT_TRUE(device.assigned());
    ASSERT_GT(device.getSignalsRecursive().getCount(), 0u);
    expectConnectedOver(device, WssOption, listeningPort);
}

TEST_F(NativeTlsTest, CapabilitiesRemovedForBothListeners)
{
    auto instance = Instance("[[none]]");
    addNativeServerModule(instance);

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
    {
        expectOptions(capability, {{WssOption, TlsPort}});
        ASSERT_EQ(test_helpers::capabilityPort(capability), TlsPort) << capability.getProtocolId();
    }

    instance.removeServer(server);
    ASSERT_EQ(instance.getInfo().getServerCapabilities().getCount(), 0u);

    ASSERT_NO_THROW(instance.addServer(ServerTypeId, secureServerConfig(instance, false, /*plainPortEnabled*/ false)));
}

TEST_F(NativeTlsTest, SubscribeReadUnsubscribeOverTls)
{
    SKIP_TEST_MAC_CI;
    auto server = createServerInstance();
    auto client = createClientInstance();

    const auto deviceConnectionString = connectionString("daq.ns", TlsPort);
    auto device = client.addDevice(deviceConnectionString, secureAddDeviceConfig(client, deviceConnectionString));

    auto signal = device.getSignalsRecursive()[0].template asPtr<IMirroredSignalConfig>();
    auto domainSignal = signal.getDomainSignal().template asPtr<IMirroredSignalConfig>();
    ASSERT_TRUE(domainSignal.assigned());

    const StringPtr streamingSource = signal.getActiveStreamingSource();
    ASSERT_TRUE(streamingSource.assigned());
    ASSERT_EQ(domainSignal.getActiveStreamingSource(), streamingSource);

    // the data comes off the source on the TLS port, not off a plaintext one to the same server
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

// The streaming the device gets connects over the option its own streaming config selects
TEST_F(NativeTlsTest, StreamingOverTlsWhenItsConfigSelectsTls)
{
    auto server = createServerInstance();
    auto client = createClientInstance();

    const auto deviceConnectionString = connectionString("daq.nd", TlsPort);
    auto config = secureAddDeviceConfig(client, deviceConnectionString);
    selectTls(nativeStreamingConfig(config));
    auto device = client.addDevice(deviceConnectionString, config);

    ASSERT_EQ(streamingSourceConnectionStrings(device), std::vector<std::string>{"daq.ns://127.0.0.1:" + std::to_string(TlsPort)});
}

// The streaming config does not follow the option of the device: without its own selection it connects over ws
TEST_F(NativeTlsTest, StreamingOverWsUnlessItsConfigSelectsTls)
{
    auto server = createServerInstance();
    auto client = createClientInstance();

    const auto deviceConnectionString = connectionString("daq.nd", TlsPort);
    auto device = client.addDevice(deviceConnectionString, secureAddDeviceConfig(client, deviceConnectionString));

    ASSERT_EQ(streamingSourceConnectionStrings(device), std::vector<std::string>{"daq.ns://127.0.0.1:" + std::to_string(PlainPort)});
}

// The streaming config selects ws, which a TLS-only server does not announce: the device gets no streaming,
// and no plaintext connection is tried instead
TEST_F(NativeTlsTest, NoStreamingOverOptionTheServerDoesNotAnnounce)
{
    auto server = createServerInstance(/*mutualTls*/ false, /*plainPortEnabled*/ false);
    auto client = createClientInstance();

    const auto deviceConnectionString = connectionString("daq.nd", TlsPort);
    auto device = client.addDevice(deviceConnectionString, secureAddDeviceConfig(client, deviceConnectionString));

    ASSERT_TRUE(device.assigned());
    ASSERT_TRUE(streamingSourceConnectionStrings(device).empty());
}

TEST_F(NativeTlsTest, AddStreamingOverTls)
{
    auto server = createServerInstance();
    auto client = createClientInstance();

    const auto deviceConnectionString = connectionString("daq.nd", TlsPort);
    auto config = secureAddDeviceConfig(client, deviceConnectionString);
    config.getStreamingOptions().setStreamingType("");
    auto device = client.addDevice(deviceConnectionString, config);
    ASSERT_TRUE(streamingSourceConnectionStrings(device).empty());

    auto streamingConfig = nativeStreamingConfig(addDeviceConfig(client, deviceConnectionString));
    selectTls(streamingConfig);
    const auto streaming = device.addStreaming(connectionString("daq.ns", TlsPort), streamingConfig);

    ASSERT_TRUE(streaming.assigned());
    ASSERT_EQ(streamingSourceConnectionStrings(device), std::vector<std::string>{connectionString("daq.ns", TlsPort)});
}

TEST_F(NativeTlsTest, LoadedDeviceConnectsOverItsOption)
{
    auto server = createServerInstance();
    auto client = createClientInstance();

    const std::string deviceConnectionString = "daq.nd://127.0.0.1/";
    client.addDevice(deviceConnectionString, secureAddDeviceConfig(client, deviceConnectionString));

    auto loaded = createClientInstance();
    loaded.loadConfiguration(client.saveConfiguration());

    const auto devices = loaded.getDevices();
    ASSERT_EQ(devices.getCount(), 1u);
    expectConnectedOver(devices[0], WssOption, TlsPort);
}

// A server with only the TLS listener: the device can get back only over TLS
TEST_F(NativeTlsTest, ReconnectsOverTls)
{
    SKIP_TEST_MAC_CI;
    auto server = createServerInstance(/*mutualTls*/ false, /*plainPortEnabled*/ false);
    auto client = createClientInstance();

    const std::string deviceConnectionString = "daq.nd://127.0.0.1/";
    auto device = client.addDevice(deviceConnectionString, secureAddDeviceConfig(client, deviceConnectionString));

    server.release();
    ASSERT_TRUE(waitForConnectionStatus(device, "ConfigurationStatus", "Reconnecting"));

    server = createServerInstance(/*mutualTls*/ false, /*plainPortEnabled*/ false);
    ASSERT_TRUE(waitForConnectionStatus(device, "ConfigurationStatus", "Connected"));
}

// The discovery tests below are disabled: the client reads only the plaintext mDNS service, and the
// server does not announce its options there yet.
TEST_F(NativeTlsTest, DISABLED_DiscoveringServerWithBothListeners)
{
    const std::string path = "/test/native_tls/discovery/";
    auto server = createDiscoverableServerInstance(path);
    auto client = createClientInstance();

    const auto found = discoverNativeCapabilities(client, path, {PlainConfigId, PlainStreamingId});
    ASSERT_EQ(found.count(PlainConfigId), 1u);
    ASSERT_EQ(found.count(PlainStreamingId), 1u);

    for (const auto& [id, capability] : found)
    {
        // one service, with an option for each listener; its port and connection strings are those of the first one
        expectOptions(capability, {{WsOption, PlainPort}, {WssOption, TlsPort}});
        ASSERT_EQ(test_helpers::capabilityPort(capability), PlainPort) << id;

        const std::string connectionString = capability.getConnectionString().toStdString();
        ASSERT_NE(connectionString.find(":" + std::to_string(PlainPort)), std::string::npos) << id << " " << connectionString;
    }

    ASSERT_EQ(found.at(PlainConfigId).getPrefix(), "daq.nd");
    ASSERT_EQ(found.at(PlainStreamingId).getPrefix(), "daq.ns");
}

// The connection string of a discovered server names the port of its first option, so a client which selects
// another one also sets its port
TEST_F(NativeTlsTest, DISABLED_ConnectToDiscoveredServerOverTls)
{
    const std::string path = "/test/native_tls/discovery_connect/";
    auto server = createDiscoverableServerInstance(path);
    auto client = createClientInstance();

    const auto found = discoverNativeCapabilities(client, path, {PlainConfigId});
    ASSERT_EQ(found.count(PlainConfigId), 1u);

    const auto capability = found.at(PlainConfigId);
    const TransportOptionPtr wss = capability.getAvailableTransportOptions().get(WssOption);

    const std::string deviceConnectionString = capability.getConnectionString().toStdString();
    auto config = secureAddDeviceConfig(client, deviceConnectionString);
    config.setPort(wss.getPort());

    auto device = client.addDevice(deviceConnectionString, config);

    ASSERT_TRUE(device.assigned());
    ASSERT_GT(device.getSignalsRecursive().getCount(), 0u);
}

TEST_F(NativeTlsTest, DISABLED_DiscoveringTlsOnlyServer)
{
    const std::string path = "/test/native_tls/discovery_tls_only/";
    auto server = createDiscoverableServerInstance(path, /*plainPortEnabled*/ false);
    auto client = createClientInstance();

    const auto found = discoverNativeCapabilities(client, path, {PlainConfigId, PlainStreamingId});
    ASSERT_EQ(found.count(PlainConfigId), 1u);
    ASSERT_EQ(found.count(PlainStreamingId), 1u);

    // with no plaintext listener the TLS one is the only option
    for (const auto& [id, capability] : found)
    {
        expectOptions(capability, {{WssOption, TlsPort}});
        ASSERT_EQ(test_helpers::capabilityPort(capability), TlsPort) << id;
    }
}

#endif
