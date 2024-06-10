#include <gtest/gtest.h>
#include <opcuatms_client/tms_client.h>
#include <opcuatms_server/tms_server.h>
#include <opendaq/instance_factory.h>
#include <opendaq/logger_factory.h>
#include <opendaq/mock/mock_device_module.h>
#include <opendaq/mock/mock_fb_module.h>
#include <opendaq/data_descriptor_factory.h>
#include <opendaq/signal_factory.h>
#include <testutils/memcheck_listener.h>
#include "websocket_streaming/websocket_streaming_server.h"
#include "stream/WebsocketClientStream.hpp"
#include <spdlog/spdlog.h>
#include <websocket_streaming/websocket_streaming_factory.h>
#include <opendaq/search_filter_factory.h>
#include <opendaq/data_rule_factory.h>
#include <signal_generator/signal_generator.h>
#include <chrono>
#include <future>
#include <thread>
#include <coreobjects/property_object_factory.h>
#include <opendaq/mirrored_device_config_ptr.h>
#include <coreobjects/authentication_provider_factory.h>

using namespace daq;
using namespace daq::opcua;
using namespace std::chrono_literals;
using namespace daq::stream;
using namespace daq::websocket_streaming;
using namespace daq::streaming_protocol;

class StreamingIntegrationTest : public testing::Test
{
public:
    const uint16_t STREAMING_PORT = 7414;
    const uint16_t STREAMING_CONTROL_PORT = 7438;
    const std::string OPCUA_URL = "opc.tcp://127.0.0.1/";
    const std::string STREAMING_URL = "daq.lt://127.0.0.1/";

    using ReadCallback = std::function<void(const ListPtr<IPacket>& readPackets)>;

    void SetUp() override
    {
        logger = Logger();
        loggerComponent = logger.getOrAddComponent("StreamingIntegrationTest");
        auto clientLogger = Logger();
        clientContext = Context(Scheduler(clientLogger, 1), clientLogger, TypeManager(), nullptr, nullptr);
        instance = createDevice();
    }

    void TearDown() override
    {
        std::this_thread::sleep_for(10ms);
    }

    InstancePtr getInstance()
    {
        return instance;
    }

    void generatePackets(size_t packetCount)
    {
        auto devices = instance.getDevices();

        for (const auto& device : devices)
        {
            auto name = device.getInfo().getName();
            if (name == "MockPhysicalDevice")
                device.setPropertyValue("GeneratePackets", packetCount);
        }
    }

    SignalPtr getSignal(const DevicePtr& device, const std::string& signalName)
    {
        auto signals = device.getSignalsRecursive();

        for (const auto& signal : signals)
        {
            const auto descriptor = signal.getDescriptor();
            if (descriptor.assigned() && descriptor.getName() == signalName)
            {
                return signal;
            }
        }

        throw NotFoundException();
    }

    PacketReaderPtr createReader(const DevicePtr& device, const std::string& signalName)
    {
        auto signals = device.getSignals(search::Recursive(search::Visible()));

        for (const auto& signal : signals)
        {
            const auto descriptor = signal.getDescriptor();
            if (descriptor.assigned() && descriptor.getName() == signalName)
                return PacketReader(signal);
        }

        throw NotFoundException();
    }

    ListPtr<IPacket> tryReadPackets(const PacketReaderPtr& reader,
                                    size_t packetCount,
                                    std::chrono::seconds timeout = std::chrono::seconds(60))
    {
        auto allPackets = List<IPacket>();
        auto startPoint = std::chrono::system_clock::now();

        while (allPackets.getCount() < packetCount)
        {
            if (reader.getAvailableCount() == 0)
            {
                auto now = std::chrono::system_clock::now();
                auto timeElapsed = now - startPoint;
                if (timeElapsed > timeout)
                {
                    LOG_E("Timeout expired: packets count expected {}, packets count ready {}",
                          packetCount, allPackets.getCount());
                    break;
                }

                std::this_thread::sleep_for(std::chrono::milliseconds(20));
                continue;
            }

            auto packets = reader.readAll();

            for (const auto& packet : packets)
                allPackets.pushBack(packet);
        }

        return allPackets;
    }

    bool packetsEqual(const ListPtr<IPacket>& listA, const ListPtr<IPacket>& listB, bool compareDescriptors = true)
    {
        bool result = true;
        if (listA.getCount() != listB.getCount())
        {
            LOG_E("Compared packets count differs: A {}, B {}", listA.getCount(), listB.getCount());
            result = false;
        }

        auto count = std::min(listA.getCount(), listB.getCount());

        for (SizeT i = 0; i < count; i++)
        {
            if (!compareDescriptors &&
                listA.getItemAt(i).getType() == PacketType::Event &&
                listB.getItemAt(i).getType() == PacketType::Event)
                continue;
            if (!BaseObjectPtr::Equals(listA.getItemAt(i), listB.getItemAt(i)))
            {
                LOG_E("Packets at index {} differs: A - \"{}\", B - \"{}\"",
                      i, listA.getItemAt(i).toString(), listB.getItemAt(i).toString());
                result = false;
            }
        }

        return result;
    }

protected:
    InstancePtr createDevice()
    {

        const auto moduleManager = ModuleManager("[[none]]");
        auto context = Context(Scheduler(logger, 1), logger, TypeManager(), moduleManager, AuthenticationProvider());

        const ModulePtr deviceModule(MockDeviceModule_Create(context));
        moduleManager.addModule(deviceModule);


        auto instance = InstanceCustom(context, "localInstance");

        instance.addDevice("mock_phys_device");

        return instance;
    }

    StreamingPtr createStreaming()
    {
        return WebsocketStreaming(STREAMING_URL, clientContext);
    }

    void setStreamingSource(const DevicePtr& device)
    {
        streaming = createStreaming();
        streaming.setActive(true);

        auto mirroredDeviceConfig = device.template asPtr<IMirroredDeviceConfig>();
        mirroredDeviceConfig.addStreamingSource(streaming);
        auto signals = device.getSignals(search::Recursive(search::Visible()));
        streaming.addSignals(signals);
        for (const auto& signal : signals)
        {
            auto mirroredSignalConfigPtr = signal.template asPtr<IMirroredSignalConfig>();
            mirroredSignalConfigPtr.setActiveStreamingSource(streaming.getConnectionString());
        }
    }

    LoggerPtr logger;
    LoggerComponentPtr loggerComponent;
    ContextPtr clientContext;
    InstancePtr instance;
    StreamingPtr streaming;
};

TEST_F(StreamingIntegrationTest, Connect)
{
    std::string host = "127.0.0.1";
    std::string target = "/";
    uint16_t port = 2000;

    // start server

    boost::asio::io_context serverContext;
    auto acceptFunc = [this](StreamPtr stream) {};
    auto server = std::make_shared<daq::stream::WebsocketServer>(serverContext, acceptFunc, port);

    auto serverThread = std::thread([&server, &serverContext]() {
        server->start();
        serverContext.run();
    });

    // start client

    auto signalMetaCallback = [this](const SubscribedSignal& subscribedSignal, const std::string& method, const nlohmann::json& params) {};
    auto protocolMetaCallback = [this](ProtocolHandler& protocolHandler, const std::string& method, const nlohmann::json& params) {};
    auto messageCallback = [this](const SubscribedSignal& subscribedSignal, uint64_t timeStamp, const uint8_t* data, size_t size) {};

    auto loggerComponent = logger.addComponent("StreamingClient");
    auto logCallback = [loggerComponent](spdlog::source_loc location, spdlog::level::level_enum level, const char* msg) {
        loggerComponent.logMessage(SourceLocation{location.filename, location.line, location.funcname}, msg, static_cast<LogLevel>(level));
    };

    daq::streaming_protocol::SignalContainer signalContainer(logCallback);
    boost::asio::io_context clientContext;

    signalContainer.setDataAsRawCb(messageCallback);
    signalContainer.setSignalMetaCb(signalMetaCallback);

    auto protocolHandler = std::make_shared<ProtocolHandler>(clientContext, signalContainer, protocolMetaCallback, logCallback);
    auto clientStream = std::make_unique<WebsocketClientStream>(clientContext, host, std::to_string(port), target);
    protocolHandler->startWithSyncInit(std::move(clientStream));
    auto clientThread = std::thread([&clientContext]() { clientContext.run(); });

    // wait a bit

    std::this_thread::sleep_for(std::chrono::milliseconds(1000));

    // stop client

    clientContext.stop();
    clientThread.join();

    // stop server

    server->stop();
    serverThread.join();
}

TEST_F(StreamingIntegrationTest, ByteStep)
{
    const size_t packetsToRead = 10;

    auto serverStepReader = createReader(instance, "ByteStep");

    auto streamingServer = WebsocketStreamingServer(instance);
    streamingServer.setStreamingPort(STREAMING_PORT);
    streamingServer.setControlPort(STREAMING_CONTROL_PORT);
    streamingServer.start();

    auto server = TmsServer(instance);
    server.start();

    auto client = TmsClient(clientContext, nullptr, OPCUA_URL);
    auto clientDevice = client.connect();
    setStreamingSource(clientDevice);

    auto mirroredSignalPtr = getSignal(clientDevice, "ByteStep").template asPtr<IMirroredSignalConfig>();
    std::promise<StringPtr> subscribeCompletePromise;
    std::future<StringPtr> subscribeCompleteFuture = subscribeCompletePromise.get_future();
    mirroredSignalPtr.getOnSubscribeComplete() +=
        [&subscribeCompletePromise](MirroredSignalConfigPtr& sender, SubscriptionEventArgsPtr& args)
    {
        subscribeCompletePromise.set_value(args.getStreamingConnectionString());
    };

    auto clientStepReader = createReader(clientDevice, "ByteStep");

    ASSERT_EQ(subscribeCompleteFuture.wait_for(std::chrono::seconds(1)), std::future_status::ready);
    ASSERT_EQ(subscribeCompleteFuture.get(), mirroredSignalPtr.getActiveStreamingSource());

    generatePackets(packetsToRead);

    auto serverReceivedPackets = tryReadPackets(serverStepReader, packetsToRead + 1);
    auto clientReceivedPackets = tryReadPackets(clientStepReader, packetsToRead + 1);

    EXPECT_EQ(serverReceivedPackets.getCount(), packetsToRead + 1);
    EXPECT_EQ(clientReceivedPackets.getCount(), packetsToRead + 1);
    EXPECT_TRUE(packetsEqual(serverReceivedPackets, clientReceivedPackets));
}

TEST_F(StreamingIntegrationTest, ChangingSignal)
{
    const size_t packetsToGenerate = 5;
    const size_t initialEventPackets = 1;
    const size_t packetsPerChange = 2;  // one triggered by data signal and one trigegred by domain signal
    const size_t packetsToRead = initialEventPackets + packetsToGenerate + (packetsToGenerate - 1) * packetsPerChange;

    auto serverStepReader = createReader(instance, "ChangingSignal");

    auto streamingServer = WebsocketStreamingServer(instance);
    streamingServer.setStreamingPort(STREAMING_PORT);
    streamingServer.setControlPort(STREAMING_CONTROL_PORT);
    streamingServer.start();

    auto server = TmsServer(instance);
    server.start();

    auto client = TmsClient(clientContext, nullptr, OPCUA_URL);
    auto clientDevice = client.connect();
    setStreamingSource(clientDevice);

    auto mirroredSignalPtr = getSignal(clientDevice, "ChangingSignal").template asPtr<IMirroredSignalConfig>();
    std::promise<StringPtr> subscribeCompletePromise;
    std::future<StringPtr> subscribeCompleteFuture = subscribeCompletePromise.get_future();
    mirroredSignalPtr.getOnSubscribeComplete() +=
        [&subscribeCompletePromise](MirroredSignalConfigPtr& sender, SubscriptionEventArgsPtr& args)
    {
        subscribeCompletePromise.set_value(args.getStreamingConnectionString());
    };

    auto clientStepReader = createReader(clientDevice, "ChangingSignal");

    ASSERT_EQ(subscribeCompleteFuture.wait_for(std::chrono::seconds(1)), std::future_status::ready);
    ASSERT_EQ(subscribeCompleteFuture.get(), mirroredSignalPtr.getActiveStreamingSource());

    generatePackets(packetsToGenerate);

    auto serverReceivedPackets = tryReadPackets(serverStepReader, packetsToRead);
    auto clientReceivedPackets = tryReadPackets(clientStepReader, packetsToRead);
    EXPECT_EQ(serverReceivedPackets.getCount(), packetsToRead);
    EXPECT_EQ(clientReceivedPackets.getCount(), packetsToRead);
    // TODO websocket streaming does not recreate half assigned data descriptor changed event packet on client side
    // both: value and domain descriptors are always assigned in event packet
    // while on server side one descriptor can be assigned only
    EXPECT_TRUE(packetsEqual(serverReceivedPackets, clientReceivedPackets, false));
}

TEST_F(StreamingIntegrationTest, AllSignalsAsync)
{
    const size_t packetsToRead = 50;

    std::vector<std::string> signals = {"ByteStep", "IntStep", "Sine"};
    std::unordered_map<std::string, PacketReaderPtr> serverReaders;
    std::unordered_map<std::string, PacketReaderPtr> clientReaders;
    std::unordered_map<std::string, ListPtr<IPacket>> serverPackets;
    std::unordered_map<std::string, ListPtr<IPacket>> clientPackets;

    for (const auto& signal : signals)
        serverReaders.insert({signal, createReader(instance, signal)});

    auto streamingServer = WebsocketStreamingServer(instance);
    streamingServer.setStreamingPort(STREAMING_PORT);
    streamingServer.setControlPort(STREAMING_CONTROL_PORT);
    streamingServer.start();

    auto server = TmsServer(instance);
    server.start();

    auto client = TmsClient(clientContext, nullptr, OPCUA_URL);
    auto clientDevice = client.connect();
    setStreamingSource(clientDevice);

    for (const auto& signal : signals)
    {
        auto mirroredSignalPtr = getSignal(clientDevice, signal).template asPtr<IMirroredSignalConfig>();
        std::promise<StringPtr> subscribeCompletePromise;
        std::future<StringPtr> subscribeCompleteFuture = subscribeCompletePromise.get_future();
        mirroredSignalPtr.getOnSubscribeComplete() +=
            [&subscribeCompletePromise](MirroredSignalConfigPtr& sender, SubscriptionEventArgsPtr& args)
        {
            subscribeCompletePromise.set_value(args.getStreamingConnectionString());
        };

        clientReaders.insert({signal, createReader(clientDevice, signal)});

        ASSERT_EQ(subscribeCompleteFuture.wait_for(std::chrono::seconds(1)), std::future_status::ready);
        ASSERT_EQ(subscribeCompleteFuture.get(), mirroredSignalPtr.getActiveStreamingSource());
    }

    generatePackets(packetsToRead);

    std::vector<std::future<ListPtr<IPacket>>> serverFetures;
    std::vector<std::future<ListPtr<IPacket>>> clientFetures;

    for (const auto& signal : signals)
    {
        auto readFunc = [this](const PacketReaderPtr& reader, size_t packetCount) { return tryReadPackets(reader, packetCount); };
        serverFetures.push_back(std::async(readFunc, serverReaders[signal], packetsToRead + 1));
        auto clientReadFunc = [this](const PacketReaderPtr& reader, size_t packetCount)
        {
            return tryReadPackets(reader, packetCount);
        };
        clientFetures.push_back(std::async(clientReadFunc, clientReaders[signal], packetsToRead + 1));
    }

    for (size_t i = 0; i < serverFetures.size(); i++)
    {
        auto sentPackets = serverFetures[i].get();
        auto receivedPackets = clientFetures[i].get();
        EXPECT_EQ(sentPackets.getCount(), packetsToRead + 1);
        EXPECT_EQ(receivedPackets.getCount(), packetsToRead + 1);
        EXPECT_TRUE(packetsEqual(sentPackets, receivedPackets));
    }
}

TEST_F(StreamingIntegrationTest, DISABLED_StartStopBug)
{
    using namespace daq::websocket_streaming;
    DevicePtr device = createDevice();

    for (size_t i = 0; i < 40; i++)
    {
        TmsServer tmsServer(device, device.getContext());
        tmsServer.start();
    }
}

TEST_F(StreamingIntegrationTest, StreamingDeactivate)
{
    const size_t packetsToRead = 10;

    auto serverStepReader = createReader(instance, "Sine");

    auto streamingServer = WebsocketStreamingServer(instance);
    streamingServer.setStreamingPort(STREAMING_PORT);
    streamingServer.setControlPort(STREAMING_CONTROL_PORT);
    streamingServer.start();

    auto server = TmsServer(instance);
    server.start();

    auto client = TmsClient(clientContext, nullptr, OPCUA_URL);
    auto clientDevice = client.connect();
    setStreamingSource(clientDevice);

    streaming.setActive(False);

    auto clientStepReader = createReader(clientDevice, "Sine");

    auto clientReceivedPackets = tryReadPackets(clientStepReader, 1);
    ASSERT_EQ(clientReceivedPackets.getCount(), 1u); // Single event packet only

    generatePackets(packetsToRead);

    auto serverReceivedPackets = tryReadPackets(serverStepReader, packetsToRead + 1);
    ASSERT_EQ(serverReceivedPackets.getCount(), packetsToRead + 1);

    clientReceivedPackets = tryReadPackets(clientStepReader, packetsToRead, std::chrono::seconds(5));
    ASSERT_EQ(clientReceivedPackets.getCount(), 0u); // no data packets since streaming is inactive

    streaming.setActive(True);

    clientReceivedPackets = tryReadPackets(clientStepReader, packetsToRead, std::chrono::seconds(5));
    ASSERT_EQ(clientReceivedPackets.getCount(), 0u); // still no data packets available
}
