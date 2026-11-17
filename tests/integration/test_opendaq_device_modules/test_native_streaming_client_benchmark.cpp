#include <streaming_benchmark_server.h>
#include "test_helpers/benchmark_listener.h"
#include "test_helpers/device_modules.h"
#include "test_helpers/test_helpers.h"

#include <opendaq/input_port_factory.h>

#include <atomic>
#include <chrono>
#include <memory>
#include <thread>
#include <vector>

// Throughput of the whole client path under a saturating producer: the benchmark server feeds a client
// instance over daq.ns://, with a counting input port per value signal.

using namespace daq;
using daq::streaming_benchmark::waitFor;
namespace bench = daq::testutils::benchmark;

namespace native_streaming_client_benchmark
{

class NativeStreamingClientBenchmarkTest : public testing::Test
{
public:
    const uint16_t serverPort = 7420;
    const std::chrono::milliseconds connectTimeout = std::chrono::seconds(60);

    void SetUp() override
    {
        settings = bench::Settings::fromEnv();
        server =
            std::make_unique<daq::streaming_benchmark::BenchmarkServer>(settings, serverPort, NullContext(Logger(nullptr, LogLevel::Warn)));
    }

    void TearDown() override
    {
        server->stopProducer();
        for (auto& signal : valueSignals)
        {
            if (IEvent* event = signal.asPtr<IMirroredSignalConfig>(true).getOnSubscribeComplete())
                event->clear();
        }
        for (auto& port : ports)
            port.remove();
        ports.clear();
        listeners.clear();
        client.release();
        server.reset();
    }

    static ContextPtr createContext()
    {
        auto logger = Logger(nullptr, LogLevel::Warn);
        return Context(
            Scheduler(logger), logger, TypeManager(), ModuleManager("[[none]]"), AuthenticationProvider(), test_helpers::instanceOptions());
    }

    void connectClient()
    {
        client = InstanceCustom(createContext(), "clientLocal");
        addNativeClientModule(client);

        clientDevice = client.addDevice("daq.ns://127.0.0.1:" + std::to_string(serverPort));
        ASSERT_TRUE(test_helpers::waitForStreamingSources(clientDevice, 1, connectTimeout)) << "streaming source not attached";

        for (const auto& signal : clientDevice.getSignals(search::Recursive(search::Visible())))
        {
            if (signal.getDomainSignal().assigned())
                valueSignals.push_back(signal);
        }
        ASSERT_EQ(valueSignals.size(), settings.signals) << "unexpected number of value signals on the mirrored device";

        for (auto& signal : valueSignals)
        {
            signal.asPtr<IMirroredSignalConfig>(true).getOnSubscribeComplete() +=
                [this](MirroredSignalConfigPtr&, SubscriptionEventArgsPtr&) { ++subscribedAcks; };
        }
    }

    // Connecting a port is what subscribes the mirrored signal; the acks mark the start of the warm-up.
    void connectPorts()
    {
        counters = std::make_unique<std::atomic<uint64_t>[]>(valueSignals.size());
        for (size_t i = 0; i < valueSignals.size(); ++i)
        {
            auto listener = createWithImplementation<IInputPortNotifications, test_helpers::CountingListener>(
                counters[i], receivedPackets, receivedBytes, receivedEvents);
            auto port = InputPort(client.getContext(), nullptr, "bench" + std::to_string(i));
            port.setNotificationMethod(PacketReadyNotification::SameThread);
            port.setListener(listener);
            port.connect(valueSignals[i]);
            listeners.push_back(listener);
            ports.push_back(port);
        }
        ASSERT_TRUE(waitFor([this]() { return subscribedAcks.load() == valueSignals.size(); }, connectTimeout))
            << "only " << subscribedAcks.load() << " of " << valueSignals.size() << " subscriptions were acknowledged";
    }

    bool allPortsDelivered() const
    {
        for (size_t i = 0; i < valueSignals.size(); ++i)
        {
            if (counters[i].load() == 0)
                return false;
        }
        return true;
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

    InstancePtr client;
    DevicePtr clientDevice;
    std::vector<SignalPtr> valueSignals;
    std::unique_ptr<std::atomic<uint64_t>[]> counters;
    std::vector<InputPortNotificationsPtr> listeners;
    std::vector<InputPortConfigPtr> ports;

    std::atomic<size_t> subscribedAcks{0};
    std::atomic<uint64_t> receivedPackets{0};
    std::atomic<uint64_t> receivedBytes{0};
    std::atomic<uint64_t> receivedEvents{0};
};

TEST_F(NativeStreamingClientBenchmarkTest, ClientThroughput)
{
    DAQ_BENCHMARK_OR_SKIP();
    settings.print("client");

    server->start();
    connectClient();
    if (HasFatalFailure())
        return;
    connectPorts();
    if (HasFatalFailure())
        return;
    // The ports count value packets only; the domain packets travel with them.
    server->startProducer(receivedPackets, false);

    if (!waitFor([this]() { return allPortsDelivered(); }, connectTimeout))
    {
        size_t silentSignals = 0;
        for (size_t i = 0; i < valueSignals.size(); ++i)
            silentSignals += counters[i].load() == 0 ? 1 : 0;
        FAIL() << silentSignals << " of " << valueSignals.size() << " signals delivered nothing; " << receivedPackets.load()
               << " data packets and " << receivedEvents.load() << " event packets received in total";
    }

    std::vector<uint64_t> countsBefore(valueSignals.size());
    for (size_t i = 0; i < valueSignals.size(); ++i)
        countsBefore[i] = counters[i].load();

    const auto summary = bench::measure(settings, [this]() { return snapshot(); }, "client");
    std::printf("[bench] client: the wire also carries one implicit domain packet per value packet, so the transport handled ~2x the "
                "received rate\n");

    server->stopProducer();
    for (size_t i = 0; i < valueSignals.size(); ++i)
        EXPECT_GT(counters[i].load(), countsBefore[i]) << "signal " << valueSignals[i].getGlobalId() << " delivered nothing in the window";
    EXPECT_FALSE(summary.samples.empty());
}

}
