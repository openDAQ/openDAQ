#include <testutils/benchmark_utils.h>
#include "test_helpers/benchmark_listener.h"
#include "test_helpers/device_modules.h"
#include "test_helpers/test_helpers.h"

#include <opendaq/input_port_factory.h>

#include <atomic>
#include <chrono>
#include <memory>
#include <thread>
#include <vector>

// End-to-end throughput through a client instance: a reference device behind the native streaming server,
// a client instance over daq.nd://, and a counting input port per value signal.

using namespace daq;
namespace bench = daq::testutils::benchmark;

namespace native_streaming_benchmark
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

using test_helpers::CountingListener;

class NativeStreamingBenchmarkTest : public testing::Test
{
public:
    const std::chrono::milliseconds connectTimeout = std::chrono::seconds(60);

    void SetUp() override
    {
        settings = bench::Settings::fromEnv();
    }

    void TearDown() override
    {
        for (auto& signal : valueSignals)
        {
            if (IEvent* event = signal.asPtr<IMirroredSignalConfig>(true).getOnSubscribeComplete())
                event->clear();
        }
        for (auto& port : ports)
            port.remove();
        ports.clear();
        listeners.clear();
        if (generatorPort.assigned())
            generatorPort.remove();
        generatorPort.release();
        generatorListener.release();
        client.release();
        server.release();
    }

    static ContextPtr createContext()
    {
        auto logger = Logger(nullptr, LogLevel::Warn);
        return Context(
            Scheduler(logger), logger, TypeManager(), ModuleManager("[[none]]"), AuthenticationProvider(), test_helpers::instanceOptions());
    }

    void startServer()
    {
        server = InstanceCustom(createContext(), "serverLocal");
        addRefDeviceModule(server);
        addNativeServerModule(server);

        auto refDevice = server.addDevice("daqref://device0");
        refDevice.setPropertyValue("NumberOfChannels", static_cast<Int>(settings.signals));
        refDevice.setPropertyValue("AcquisitionLoopTime", static_cast<Int>(settings.loopMs));
        // The reference channel's packet size is sample rate times loop time.
        refDevice.setPropertyValue("GlobalSampleRate",
                                   static_cast<Float>(settings.samplesPerPacket) * settings.packetsPerSecondPerSignal());

        // The device loop does not tick at exactly the requested period, so generated packets are counted
        // on one server channel; every channel is served by the same tick.
        for (const auto& signal : refDevice.getSignals(search::Recursive(search::Visible())))
        {
            if (!signal.getDomainSignal().assigned())
                continue;
            generatorListener = createWithImplementation<IInputPortNotifications, CountingListener>(
                generatedPackets, generatorTotalPackets, generatorTotalBytes, generatorTotalEvents);
            generatorPort = InputPort(server.getContext(), nullptr, "benchgen");
            generatorPort.setNotificationMethod(PacketReadyNotification::SameThread);
            generatorPort.setListener(generatorListener);
            generatorPort.connect(signal);
            break;
        }
        ASSERT_TRUE(generatorPort.assigned()) << "no value signal on the reference device";

        auto serverConfig = server.getAvailableServerTypes().get("OpenDAQNativeStreaming").createDefaultConfig();
        serverConfig.setPropertyValue("StreamingCacheablePayloadSizeMax", static_cast<Int>(settings.cacheablePayloadSizeMax));
        server.addServer("OpenDAQNativeStreaming", serverConfig);
    }

    void connectClient()
    {
        client = InstanceCustom(createContext(), "clientLocal");
        addNativeClientModule(client);

        auto config = client.createDefaultAddDeviceConfig();
        PropertyObjectPtr general = config.getPropertyValue("General");
        general.setPropertyValue("PrioritizedStreamingProtocols", List<IString>("OpenDAQNativeStreaming"));
        clientDevice = client.addDevice("daq.nd://127.0.0.1", config);
        // The streaming source is attached to the mirrored signals after the device is returned.
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
            auto listener = createWithImplementation<IInputPortNotifications, CountingListener>(
                counters[i], receivedPackets, receivedBytes, receivedEvents);
            auto port = InputPort(client.getContext(), nullptr, "bench" + std::to_string(i));
            port.setNotificationMethod(PacketReadyNotification::SameThread);
            port.setListener(listener);
            port.connect(valueSignals[i]);
            // The port holds its listener weakly, so the fixture owns it.
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
        s.generatedPackets = generatedPackets.load(std::memory_order_relaxed) * settings.signals;
        return s;
    }

protected:
    bench::Settings settings;

    InstancePtr server;
    InstancePtr client;
    DevicePtr clientDevice;
    std::vector<SignalPtr> valueSignals;
    std::unique_ptr<std::atomic<uint64_t>[]> counters;
    std::vector<InputPortNotificationsPtr> listeners;
    std::vector<InputPortConfigPtr> ports;

    InputPortConfigPtr generatorPort;
    InputPortNotificationsPtr generatorListener;
    std::atomic<uint64_t> generatedPackets{0};
    std::atomic<uint64_t> generatorTotalPackets{0};
    std::atomic<uint64_t> generatorTotalBytes{0};
    std::atomic<uint64_t> generatorTotalEvents{0};

    std::atomic<size_t> subscribedAcks{0};
    std::atomic<uint64_t> receivedPackets{0};
    std::atomic<uint64_t> receivedBytes{0};
    std::atomic<uint64_t> receivedEvents{0};
};

TEST_F(NativeStreamingBenchmarkTest, DeviceToClientThroughput)
{
    DAQ_BENCHMARK_OR_SKIP();
    settings.print("device");

    startServer();
    if (HasFatalFailure())
        return;
    connectClient();
    if (HasFatalFailure())
        return;
    connectPorts();
    if (HasFatalFailure())
        return;

    if (!waitFor([this]() { return allPortsDelivered(); }, connectTimeout))
    {
        size_t silentSignals = 0;
        for (size_t i = 0; i < valueSignals.size(); ++i)
            silentSignals += counters[i].load() == 0 ? 1 : 0;
        const auto first = valueSignals.front().asPtr<IMirroredSignalConfig>(true);
        FAIL() << silentSignals << " of " << valueSignals.size() << " signals delivered nothing; " << receivedPackets.load()
               << " data packets and " << receivedEvents.load()
               << " event packets received in total; first signal streamed=" << static_cast<int>(first.getStreamed())
               << " active source=" << (first.getActiveStreamingSource().assigned() ? first.getActiveStreamingSource() : "none");
    }

    std::vector<uint64_t> countsBefore(valueSignals.size());
    for (size_t i = 0; i < valueSignals.size(); ++i)
        countsBefore[i] = counters[i].load();

    const auto summary = bench::measure(settings, [this]() { return snapshot(); }, "device");
    std::printf("[bench] device: the wire also carries one implicit domain packet per value packet, so the transport handled ~2x the "
                "received rate\n");
    std::printf("[bench] device: packet size follows the device's actual loop period, so compare bytes/s rather than samples per packet\n");

    for (size_t i = 0; i < valueSignals.size(); ++i)
        EXPECT_GT(counters[i].load(), countsBefore[i]) << "signal " << valueSignals[i].getGlobalId() << " delivered nothing in the window";
    EXPECT_FALSE(summary.samples.empty());
}

}
