#include "streaming_benchmark_server.h"
#include "test_base.h"

#include <atomic>
#include <chrono>
#include <memory>
#include <mutex>
#include <thread>
#include <unordered_map>
#include <vector>

// Throughput ceiling of the transport alone: the benchmark server and a client handler in one process,
// no readers and no processing thread.

using namespace daq;
using namespace daq::opendaq_native_streaming_protocol;
using daq::streaming_benchmark::waitFor;
namespace bench = daq::testutils::benchmark;

namespace streaming_benchmark
{

class StreamingBenchmarkTest : public testing::Test
{
public:
    const uint16_t serverPort = 7420;
    const std::string serverAddress = "127.0.0.1";
    const std::chrono::milliseconds connectTimeout = std::chrono::seconds(30);

    void SetUp() override
    {
        settings = bench::Settings::fromEnv();
        server =
            std::make_unique<daq::streaming_benchmark::BenchmarkServer>(settings, serverPort, NullContext(Logger(nullptr, LogLevel::Warn)));
        clientContext = NullContext(Logger(nullptr, LogLevel::Warn));
    }

    void TearDown() override
    {
        server->stopProducer();
        clientHandler.reset();
        server.reset();
    }

    void connectClient()
    {
        const auto& serverSignals = server->serverSignals;

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
            [this, &serverSignals]()
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
        for (const auto& signal : server->serverSignals)
            clientHandler->subscribeSignal(signal.getGlobalId());
        ASSERT_TRUE(waitFor([this]() { return subscribedAcks.load() == server->serverSignals.getCount(); }, connectTimeout))
            << "not all subscriptions were acknowledged";
    }

    bench::Snapshot snapshot() const
    {
        bench::Snapshot s;
        s.receivedPackets = receivedPackets.load(std::memory_order_relaxed);
        s.receivedBytes = receivedBytes.load(std::memory_order_relaxed);
        s.generatedPackets = server->generated();
        return s;
    }

protected:
    bench::Settings settings;
    std::unique_ptr<daq::streaming_benchmark::BenchmarkServer> server;
    ContextPtr clientContext;

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
};

TEST_F(StreamingBenchmarkTest, TransportThroughput)
{
    DAQ_BENCHMARK_OR_SKIP();
    settings.print("transport");

    server->start();
    connectClient();
    if (HasFatalFailure())
        return;
    subscribeAll();
    if (HasFatalFailure())
        return;
    // The client counts every packet on the wire, domain packets included.
    server->startProducer(receivedPackets, true);

    ASSERT_TRUE(waitFor([this]() { return receivedPackets.load() > 0; }, connectTimeout)) << "no data packet reached the client";

    const auto& serverSignals = server->serverSignals;
    std::vector<uint64_t> countsBefore(serverSignals.getCount());
    for (size_t i = 0; i < serverSignals.getCount(); ++i)
        countsBefore[i] = counters[i].load();

    const auto summary = bench::measure(settings, [this]() { return snapshot(); }, "transport");

    server->stopProducer();
    ASSERT_EQ(connectionLost.load(), 0u) << "the connection dropped during the run";
    for (size_t i = 0; i < serverSignals.getCount(); ++i)
        EXPECT_GT(counters[i].load(), countsBefore[i]) << "signal " << serverSignals[i].getGlobalId() << " delivered nothing in the window";
    EXPECT_FALSE(summary.samples.empty());
}

}
