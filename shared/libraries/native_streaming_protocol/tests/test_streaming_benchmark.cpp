#include <testutils/benchmark_utils.h>
#include "test_base.h"

#include <opendaq/opendaq.h>

#include <atomic>
#include <chrono>
#include <memory>
#include <mutex>
#include <thread>
#include <unordered_map>
#include <vector>

// Throughput ceiling of the transport alone: a server handler and a client handler in one process,
// no readers and no processing thread, fed by a producer paced by an in-flight window.

using namespace daq;
using namespace daq::opendaq_native_streaming_protocol;
namespace bench = daq::testutils::benchmark;

namespace streaming_benchmark
{

template <typename Predicate>
bool waitFor(Predicate predicate, std::chrono::milliseconds timeout)
{
    const auto deadline = std::chrono::steady_clock::now() + timeout;
    while (!predicate())
    {
        if (std::chrono::steady_clock::now() > deadline)
            return false;
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    return true;
}

class StreamingBenchmarkTest : public testing::Test
{
public:
    const uint16_t serverPort = 7420;
    const std::string serverAddress = "127.0.0.1";
    const std::chrono::milliseconds connectTimeout = std::chrono::seconds(30);

    void SetUp() override
    {
        settings = bench::Settings::fromEnv();
        serverContext = NullContext(Logger(nullptr, LogLevel::Warn));
        clientContext = NullContext(Logger(nullptr, LogLevel::Warn));
    }

    void TearDown() override
    {
        stopProducer();
        clientHandler.reset();
        if (ioContext)
            ioContext->stop();
        if (ioThread.joinable())
            ioThread.join();
        workGuard.reset();
        serverHandler.reset();
        ioContext.reset();
    }

    void createSignals()
    {
        valueDescriptor = DataDescriptorBuilder().setSampleType(SampleType::Float32).build();
        domainDescriptor = DataDescriptorBuilder()
                               .setSampleType(SampleType::Int64)
                               .setRule(LinearDataRule(1, 0))
                               .setTickResolution(Ratio(1, 1000000))
                               .setOrigin("1970-01-01T00:00:00Z")
                               .setUnit(Unit("s", -1, "second", "time"))
                               .build();

        serverSignals = List<ISignal>();
        for (size_t i = 0; i < settings.signals; ++i)
        {
            auto domainSignal = SignalWithDescriptor(serverContext, domainDescriptor, nullptr, "domain" + std::to_string(i));
            auto valueSignal = SignalWithDescriptor(serverContext, valueDescriptor, nullptr, "value" + std::to_string(i));
            valueSignal.setDomainSignal(domainSignal);
            domainSignals.push_back(domainSignal);
            valueSignals.push_back(valueSignal);
            serverSignals.pushBack(domainSignal);
            serverSignals.pushBack(valueSignal);
        }
    }

    void startServer()
    {
        ioContext = std::make_shared<boost::asio::io_context>();
        workGuard = std::make_unique<WorkGuardType>(ioContext->get_executor());
        ioThread = std::thread([this]() { ioContext->run(); });

        auto config = NativeStreamingServerHandler::createDefaultConfig();
        config.setPropertyValue("StreamingCacheablePayloadSizeMax", static_cast<Int>(settings.cacheablePayloadSizeMax));

        // The packet streaming server needs the descriptor event before data; the server module sends it
        // through the reader, here it is sent on subscription.
        OnSignalSubscribedCallback onSubscribed = [this](const SignalPtr& signal)
        {
            const auto domain = signal.getDomainSignal();
            auto eventPacket = domain.assigned() ? DataDescriptorChangedEventPacket(valueDescriptor, domainDescriptor)
                                                 : DataDescriptorChangedEventPacket(domainDescriptor, NullDataDescriptor());
            serverHandler->sendPacket(signal.getGlobalId().toStdString(), std::move(eventPacket));
        };
        OnSignalUnsubscribedCallback onUnsubscribed = [](const SignalPtr&) {};
        SetUpConfigProtocolServerCb setUpConfigProtocolServerCb = [](SendConfigProtocolPacketCb, const UserPtr&, ClientType)
        { return std::make_pair(nullptr, nullptr); };

        serverHandler = std::make_shared<NativeStreamingServerHandler>(
            serverContext,
            ioContext,
            serverSignals,
            onSubscribed,
            onUnsubscribed,
            setUpConfigProtocolServerCb,
            [](const std::string&, const std::string&, bool, ClientType, const std::string&) {},
            [](const std::string&) {},
            config);
        serverHandler->startServer(serverPort);
    }

    void connectClient()
    {
        clientHandler = std::make_shared<NativeStreamingClientHandler>(
            clientContext, ClientAttributesBase::createTransportLayerConfig(), ClientAttributesBase::createAuthenticationConfig());

        OnSignalAvailableCallback onSignalAvailable = [this](const StringPtr& signalStringId, const StringPtr&)
        {
            std::lock_guard<std::mutex> lock(signalsSync);
            availableSignalIds.push_back(signalStringId.toStdString());
        };
        OnSignalUnavailableCallback onSignalUnavailable = [](const StringPtr&) {};
        OnPacketCallback onPacket = [this](const StringPtr& signalStringId, const PacketPtr& packet)
        {
            if (packet.getType() != daq::PacketType::Data)
                return;
            // Same lookup shape as the real client's signal-id map.
            const auto it = signalIndices.find(signalStringId.toStdString());
            if (it == signalIndices.end())
                return;
            counters[it->second].fetch_add(1, std::memory_order_relaxed);
            receivedPackets.fetch_add(1, std::memory_order_relaxed);
            if (it->second % 2 == 1)
                receivedBytes.fetch_add(packet.asPtr<IDataPacket>(true).getRawDataSize(), std::memory_order_relaxed);
        };
        OnSignalSubscriptionAckCallback onSubscriptionAck = [this](const StringPtr&, bool subscribed)
        {
            if (subscribed)
                ++subscribedAcks;
        };
        OnConnectionStatusChangedCallback onConnectionStatus = [this](const EnumerationPtr& status, const StringPtr&)
        {
            if (status.getValue() != "Connected")
                ++connectionLost;
        };
        OnStreamingInitDoneCallback onStreamingInitDone = [this]() { streamingInitDone = true; };

        clientHandler->setStreamingHandlers(
            onSignalAvailable, onSignalUnavailable, onPacket, onSubscriptionAck, onConnectionStatus, onStreamingInitDone);

        ASSERT_TRUE(clientHandler->connect(serverAddress, std::to_string(serverPort)));
        clientHandler->sendStreamingRequest();
        ASSERT_TRUE(waitFor([this]() { return streamingInitDone.load(); }, connectTimeout)) << "streaming init timed out";
        ASSERT_TRUE(waitFor(
            [this]()
            {
                std::lock_guard<std::mutex> lock(signalsSync);
                return availableSignalIds.size() == serverSignals.getCount();
            },
            connectTimeout))
            << "not all signals became available";

        // Counters are indexed domain, value, domain, value, ... in server order; the odd ones carry raw data.
        counters = std::make_unique<std::atomic<uint64_t>[]>(serverSignals.getCount());
        for (size_t i = 0; i < serverSignals.getCount(); ++i)
            signalIndices[serverSignals[i].getGlobalId().toStdString()] = i;
    }

    void subscribeAll()
    {
        for (const auto& signal : serverSignals)
            clientHandler->subscribeSignal(signal.getGlobalId());
        ASSERT_TRUE(waitFor([this]() { return subscribedAcks.load() == serverSignals.getCount(); }, connectTimeout))
            << "not all subscriptions were acknowledged";
    }

    void startProducer()
    {
        producerRunning = true;
        producerThread = std::thread([this]() { produce(); });
    }

    void stopProducer()
    {
        producerRunning = false;
        if (producerThread.joinable())
            producerThread.join();
    }

    // One iteration sends one domain and one value packet per signal in a buffer the server adopts.
    void produce()
    {
        const size_t packetsPerIteration = 2 * settings.signals;
        std::vector<IPacket*> packetBuffer(packetsPerIteration);
        tsl::ordered_map<std::string, PacketBufferData> packetIndices;
        for (size_t i = 0; i < settings.signals; ++i)
        {
            PacketBufferData domainData;
            domainData.index = static_cast<int>(2 * i);
            domainData.count = 1;
            packetIndices.insert({domainSignals[i].getGlobalId().toStdString(), domainData});

            PacketBufferData valueData;
            valueData.index = static_cast<int>(2 * i + 1);
            valueData.count = 1;
            packetIndices.insert({valueSignals[i].getGlobalId().toStdString(), valueData});
        }

        int64_t offset = 0;
        while (producerRunning)
        {
            for (size_t i = 0; i < settings.signals; ++i)
            {
                auto domainPacket = DataPacket(domainDescriptor, settings.samplesPerPacket, offset);
                auto valuePacket = DataPacketWithDomain(domainPacket, valueDescriptor, settings.samplesPerPacket);
                packetBuffer[2 * i] = domainPacket.detach();
                packetBuffer[2 * i + 1] = valuePacket.detach();
            }
            offset += static_cast<int64_t>(settings.samplesPerPacket);

            serverHandler->processStreamingPackets(packetIndices, packetBuffer);
            serverHandler->sendAvailableStreamingPackets();
            generatedPackets.fetch_add(packetsPerIteration, std::memory_order_relaxed);

            while (producerRunning &&
                   generatedPackets.load(std::memory_order_relaxed) - receivedPackets.load(std::memory_order_relaxed) > settings.inFlight)
                std::this_thread::sleep_for(std::chrono::microseconds(200));
        }
    }

    bench::Snapshot snapshot() const
    {
        bench::Snapshot s;
        s.receivedPackets = receivedPackets.load(std::memory_order_relaxed);
        s.receivedBytes = receivedBytes.load(std::memory_order_relaxed);
        s.generatedPackets = generatedPackets.load(std::memory_order_relaxed);
        return s;
    }

protected:
    bench::Settings settings;

    ContextPtr serverContext;
    ContextPtr clientContext;

    DataDescriptorPtr valueDescriptor;
    DataDescriptorPtr domainDescriptor;
    std::vector<SignalConfigPtr> domainSignals;
    std::vector<SignalConfigPtr> valueSignals;
    ListPtr<ISignal> serverSignals;

    std::shared_ptr<boost::asio::io_context> ioContext;
    std::unique_ptr<WorkGuardType> workGuard;
    std::thread ioThread;
    std::shared_ptr<NativeStreamingServerHandler> serverHandler;

    std::shared_ptr<NativeStreamingClientHandler> clientHandler;
    std::mutex signalsSync;
    std::vector<std::string> availableSignalIds;
    std::unordered_map<std::string, size_t> signalIndices;
    std::unique_ptr<std::atomic<uint64_t>[]> counters;
    std::atomic<size_t> subscribedAcks{0};
    std::atomic<size_t> connectionLost{0};
    std::atomic<bool> streamingInitDone{false};

    std::atomic<uint64_t> receivedPackets{0};
    std::atomic<uint64_t> receivedBytes{0};
    std::atomic<uint64_t> generatedPackets{0};

    std::atomic<bool> producerRunning{false};
    std::thread producerThread;
};

TEST_F(StreamingBenchmarkTest, TransportThroughput)
{
    DAQ_BENCHMARK_OR_SKIP();
    settings.print("transport");

    createSignals();
    startServer();
    connectClient();
    if (HasFatalFailure())
        return;
    subscribeAll();
    if (HasFatalFailure())
        return;
    startProducer();

    ASSERT_TRUE(waitFor([this]() { return receivedPackets.load() > 0; }, connectTimeout)) << "no data packet reached the client";

    std::vector<uint64_t> countsBefore(serverSignals.getCount());
    for (size_t i = 0; i < serverSignals.getCount(); ++i)
        countsBefore[i] = counters[i].load();

    const auto summary = bench::measure(settings, [this]() { return snapshot(); }, "transport");

    stopProducer();
    ASSERT_EQ(connectionLost.load(), 0u) << "the connection dropped during the run";
    for (size_t i = 0; i < serverSignals.getCount(); ++i)
        EXPECT_GT(counters[i].load(), countsBefore[i]) << "signal " << serverSignals[i].getGlobalId() << " delivered nothing in the window";
    EXPECT_FALSE(summary.samples.empty());
}

}
