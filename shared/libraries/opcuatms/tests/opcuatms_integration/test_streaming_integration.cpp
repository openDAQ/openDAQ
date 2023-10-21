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

#include <opendaq/data_rule_factory.h>
#include <signal_generator/signal_generator.h>
#include <chrono>
#include <future>
#include <thread>
#include <coreobjects/property_object_factory.h>

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
    const std::string OPCUA_URL = "opc.tcp://127.0.0.1/";
    const std::string STREAMING_URL = "daq.wss://127.0.0.1/";

    using ReadCallback = std::function<void(const ListPtr<IPacket>& readPackets)>;

    void SetUp() override
    {
        logger = Logger();
        auto clientLogger = Logger();
        clientContext = Context(Scheduler(clientLogger, 1), clientLogger, nullptr, nullptr);
        instance = createDevice();

        createStreamingCallback = Function([this](const StreamingInfoPtr& /*streamingConfig*/,
                                                  bool /*isRootDevice*/)
                                           {
                                               return createStreaming();
                                           });
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

    PacketReaderPtr createReader(const DevicePtr& device, const std::string& signalName)
    {
        auto signals = device.getSignalsRecursive();

        for (const auto& signal : signals)
        {
            const auto descriptor = signal.getDescriptor();
            if (descriptor.assigned() && descriptor.getName() == signalName)
                return PacketReader(signal);
        }

        throw NotFoundException();
    }

    ListPtr<IPacket> tryReadPackets(const PacketReaderPtr& reader, size_t packetCount, uint64_t timeoutMs = 500)
    {
        auto allPackets = List<IPacket>();
        auto lastPacketReceived = std::chrono::system_clock::now();

        while (allPackets.getCount() < packetCount)
        {
            if (reader.getAvailableCount() == 0)
            {
                auto now = std::chrono::system_clock::now();
                uint64_t diffMs = (uint64_t) std::chrono::duration_cast<std::chrono::milliseconds>(now - lastPacketReceived).count();
                if (diffMs > timeoutMs)
                    break;

                std::this_thread::sleep_for(std::chrono::milliseconds(20));
                continue;
            }

            auto packets = reader.readAll();

            for (const auto& packet : packets)
                allPackets.pushBack(packet);

            lastPacketReceived = std::chrono::system_clock::now();
        }

        return allPackets;
    }

    bool packetsEqual(const ListPtr<IPacket>& listA, const ListPtr<IPacket>& listB, bool compareDescriptors = true)
    {
        if (listA.getCount() != listB.getCount())
            return false;

        for (SizeT i = 0; i < listA.getCount(); i++)
        {
            if (!BaseObjectPtr::Equals(listA.getItemAt(i), listB.getItemAt(i)))
                return false;
        }

        return true;
    }

protected:
    InstancePtr createDevice()
    {

        const auto moduleManager = ModuleManager("[[none]]");
        auto context = Context(Scheduler(logger, 1), logger, TypeManager(), moduleManager);

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

    void setActiveStreamingSource(const DevicePtr& device)
    {
        for (const auto& signal : device.getSignalsRecursive())
        {
            auto mirroredSignalConfigPtr = signal.template asPtr<IMirroredSignalConfig>();
            mirroredSignalConfigPtr.setActiveStreamingSource(STREAMING_URL);
        }
    }

    LoggerPtr logger;
    ContextPtr clientContext;
    InstancePtr instance;
    FunctionPtr createStreamingCallback;
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
    streamingServer.start();

    auto server = TmsServer(instance);
    server.start();

    auto client = TmsClient(clientContext, nullptr, OPCUA_URL, createStreamingCallback);
    auto clientDevice = client.connect();
    setActiveStreamingSource(clientDevice);

    auto clientStepReader = createReader(clientDevice, "ByteStep");

    generatePackets(packetsToRead);

    auto serverReceivedPackets = tryReadPackets(serverStepReader, packetsToRead + 1);
    auto clientReceivedPackets = tryReadPackets(clientStepReader, packetsToRead + 1);

    ASSERT_EQ(serverReceivedPackets.getCount(), packetsToRead + 1);
    ASSERT_EQ(clientReceivedPackets.getCount(), packetsToRead + 1);
    ASSERT_TRUE(packetsEqual(serverReceivedPackets, clientReceivedPackets));
}

// TODO websocket streaming does not recreate half assigned data descriptor changed event packet on client side
// both: value and domain descriptors are always assigned in event packet
// while on server side one descriptor can be assigned only
// client side always generates 2 event packets for each server side event packet:
// one for value descriptor changed and another for domain descriptor changed
TEST_F(StreamingIntegrationTest, DISABLED_ChangingSignal)
{
    const size_t packetsToGenerate = 5;
    const size_t initialEventPackets = 1;
    const size_t packetsPerChange = 2;  // one triggered by data signal and one trigegred by domain signal
    const size_t packetsToRead = initialEventPackets + packetsToGenerate + (packetsToGenerate - 1) * packetsPerChange;

    auto serverStepReader = createReader(instance, "ChangingSignal");

    auto streamingServer = WebsocketStreamingServer(instance);
    streamingServer.setStreamingPort(STREAMING_PORT);
    streamingServer.start();

    auto server = TmsServer(instance);
    server.start();

    auto client = TmsClient(clientContext, nullptr, OPCUA_URL, createStreamingCallback);
    auto clientDevice = client.connect();
    setActiveStreamingSource(clientDevice);

    auto clientStepReader = createReader(clientDevice, "ChangingSignal");

    generatePackets(packetsToGenerate);

    auto serverReceivedPackets = tryReadPackets(serverStepReader, packetsToRead);
    auto clientReceivedPackets = tryReadPackets(clientStepReader, packetsToRead);
    ASSERT_EQ(serverReceivedPackets.getCount(), packetsToRead);
    ASSERT_EQ(clientReceivedPackets.getCount(), packetsToRead);
    // TODO: this fails
    //ASSERT_TRUE(packetsEqual(serverReceivedPackets, clientReceivedPackets));
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
    streamingServer.start();

    auto server = TmsServer(instance);
    server.start();

    auto client = TmsClient(clientContext, nullptr, OPCUA_URL, createStreamingCallback);
    auto clientDevice = client.connect();
    setActiveStreamingSource(clientDevice);

    for (const auto& signal : signals)
        clientReaders.insert({signal, createReader(clientDevice, signal)});

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
        ASSERT_EQ(sentPackets.getCount(), packetsToRead + 1);
        ASSERT_EQ(receivedPackets.getCount(), packetsToRead + 1);
        ASSERT_TRUE(packetsEqual(sentPackets, receivedPackets));
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
    streamingServer.start();

    auto server = TmsServer(instance);
    server.start();

    auto streaming = createStreaming();
    auto createStreamingCb = Function([&](const StreamingInfoPtr& /*streamingConfig*/,
                                          bool /*isRootDevice*/)
                                      {
                                          return streaming;
                                      });
    auto client = TmsClient(clientContext, nullptr, OPCUA_URL, createStreamingCb);
    auto clientDevice = client.connect();
    setActiveStreamingSource(clientDevice);

    streaming.setActive(False);

    auto clientStepReader = createReader(clientDevice, "Sine");

    auto clientReceivedPackets = tryReadPackets(clientStepReader, packetsToRead + 1);
    ASSERT_EQ(clientReceivedPackets.getCount(), 1u); // Single event packet only

    generatePackets(packetsToRead);

    auto serverReceivedPackets = tryReadPackets(serverStepReader, packetsToRead + 1);
    ASSERT_EQ(serverReceivedPackets.getCount(), packetsToRead + 1);

    clientReceivedPackets = tryReadPackets(clientStepReader, packetsToRead + 1);
    ASSERT_EQ(clientReceivedPackets.getCount(), 0u); // no data packets since streaming is inactive

    streaming.setActive(True);

    clientReceivedPackets = tryReadPackets(clientStepReader, packetsToRead + 1);
    ASSERT_EQ(clientReceivedPackets.getCount(), 0u); // still no data packets available
}
