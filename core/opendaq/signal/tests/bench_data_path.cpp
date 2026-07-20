/*
 * Data path micro-benchmark: Signal::sendPacket -> Connection::enqueue/dequeue.
 *
 * Standalone (EXCLUDE_FROM_ALL) benchmark over the public API only, so it builds
 * unchanged before and after data-path optimizations. Run without arguments to
 * execute all scenarios; pass scenario names to run a subset.
 *
 * Output: one line per metric, integer nanoseconds, comma-separated:
 *   <scenario>,<metric>,<value>
 */

#include <opendaq/context_factory.h>
#include <opendaq/data_descriptor_factory.h>
#include <opendaq/input_port_factory.h>
#include <opendaq/packet_factory.h>
#include <opendaq/signal_factory.h>
#include <opendaq/signal_private_ptr.h>

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <string>
#include <thread>
#include <vector>

using namespace daq;

namespace
{

using Clock = std::chrono::steady_clock;

struct Fixture
{
    ContextPtr context;
    SignalConfigPtr signal;
    DataDescriptorPtr descriptor;
    std::vector<InputPortConfigPtr> ports;
    std::vector<ConnectionPtr> connections;
    PacketPtr packet;

    explicit Fixture(size_t connectionCount, size_t samplesPerPacket = 64)
    {
        context = NullContext();
        descriptor = DataDescriptorBuilder().setSampleType(SampleType::Float64).build();
        signal = Signal(context, nullptr, "sig");
        signal.setDescriptor(descriptor);

        for (size_t i = 0; i < connectionCount; ++i)
        {
            auto port = InputPort(context, nullptr, "ip" + std::to_string(i));
            port.connect(signal);
            ports.push_back(port);
            connections.push_back(port.getConnection());
        }

        // drop the initial DATA_DESCRIPTOR_CHANGED event packets so scenarios start clean
        for (auto& connection : connections)
            connection.dequeueAll();

        packet = DataPacket(descriptor, samplesPerPacket);
    }

    void drainAll()
    {
        for (auto& connection : connections)
            connection.dequeueAll();
    }
};

int64_t nsPerOp(Clock::time_point start, Clock::time_point end, int64_t ops)
{
    const auto ns = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count();
    return ops > 0 ? ns / ops : 0;
}

void report(const std::string& scenario, const std::string& metric, int64_t value)
{
    std::cout << scenario << "," << metric << "," << value << std::endl;
}

// sendN: send throughput with `connections` fan-out, draining every 1024 packets
void benchSend(const std::string& name, size_t connectionCount, bool keepLastValue)
{
    Fixture f(connectionCount);
    if (!keepLastValue)
        f.signal.asPtr<ISignalPrivate>(true).enableKeepLastValue(false);

    constexpr int64_t iterations = 2'000'000;
    constexpr int64_t drainEvery = 1024;

    // warmup
    for (int i = 0; i < 10'000; ++i)
        f.signal.sendPacket(f.packet);
    f.drainAll();

    const auto start = Clock::now();
    for (int64_t i = 1; i <= iterations; ++i)
    {
        f.signal.sendPacket(f.packet);
        if (connectionCount > 0 && (i % drainEvery) == 0)
            f.drainAll();
    }
    const auto end = Clock::now();
    f.drainAll();

    report(name, "ns_per_send", nsPerOp(start, end, iterations));
}

// dequeue: single-packet dequeue throughput
void benchDequeue()
{
    Fixture f(1);
    constexpr int64_t iterations = 1'000'000;
    constexpr int64_t chunk = 4096;

    int64_t totalNs = 0;
    int64_t done = 0;
    while (done < iterations)
    {
        for (int64_t i = 0; i < chunk; ++i)
            f.signal.sendPacket(f.packet);

        const auto start = Clock::now();
        for (int64_t i = 0; i < chunk; ++i)
        {
            PacketPtr p = f.connections[0].dequeue();
            if (!p.assigned())
                std::abort();
        }
        const auto end = Clock::now();
        totalNs += std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count();
        done += chunk;
    }

    report("dequeue", "ns_per_dequeue", totalNs / done);
}

// pipe: producer thread sends, consumer thread dequeues, end-to-end throughput
void benchPipe()
{
    Fixture f(1);
    constexpr int64_t iterations = 2'000'000;

    std::atomic<bool> consumerReady{false};
    std::atomic<int64_t> consumed{0};

    std::thread consumer(
        [&]
        {
            consumerReady.store(true);
            auto& connection = f.connections[0];
            int64_t got = 0;
            while (got < iterations)
            {
                PacketPtr p = connection.dequeue();
                if (p.assigned())
                    ++got;
            }
            consumed.store(got);
        });

    while (!consumerReady.load())
        std::this_thread::yield();

    const auto start = Clock::now();
    for (int64_t i = 0; i < iterations; ++i)
        f.signal.sendPacket(f.packet);
    consumer.join();
    const auto end = Clock::now();

    report("pipe", "ns_per_packet", nsPerOp(start, end, iterations));
}

// contend_config: producer sends while another thread hammers a config-lock getter
void benchConfigContention()
{
    Fixture f(1);
    constexpr int64_t iterations = 1'000'000;
    constexpr int64_t drainEvery = 1024;

    std::atomic<bool> stop{false};
    std::thread config(
        [&]
        {
            while (!stop.load(std::memory_order_relaxed))
            {
                Bool isPublic;
                f.signal->getPublic(&isPublic);
            }
        });

    for (int i = 0; i < 10'000; ++i)
        f.signal.sendPacket(f.packet);
    f.drainAll();

    const auto start = Clock::now();
    for (int64_t i = 1; i <= iterations; ++i)
    {
        f.signal.sendPacket(f.packet);
        if ((i % drainEvery) == 0)
            f.drainAll();
    }
    const auto end = Clock::now();

    stop.store(true);
    config.join();

    report("contend_config", "ns_per_send", nsPerOp(start, end, iterations));
}

// scan_stall: consumer thread runs O(n) queue scans while the producer sends;
// measures per-send latency distribution (the scans hold the connection lock today)
void benchScanStall()
{
    Fixture f(1);

    // fill the queue with data packets, then put one event packet at the end so
    // getSamplesUntilNextEventPacket has to walk all data packets
    constexpr int64_t prefill = 8192;
    for (int64_t i = 0; i < prefill; ++i)
        f.signal.sendPacket(f.packet);
    f.signal.setDescriptor(DataDescriptorBuilder().setSampleType(SampleType::Float64).build());

    std::atomic<bool> stop{false};
    std::atomic<bool> scannerReady{false};
    std::thread scanner(
        [&]
        {
            scannerReady.store(true);
            auto& connection = f.connections[0];
            while (!stop.load(std::memory_order_relaxed))
            {
                SizeT samples;
                connection->getSamplesUntilNextEventPacket(&samples);
            }
        });

    while (!scannerReady.load())
        std::this_thread::yield();

    constexpr int64_t iterations = 200'000;
    std::vector<int64_t> laps;
    laps.reserve(iterations);

    for (int64_t i = 0; i < iterations; ++i)
    {
        const auto start = Clock::now();
        f.signal.sendPacket(f.packet);
        const auto end = Clock::now();
        laps.push_back(std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count());
    }

    stop.store(true);
    scanner.join();

    std::sort(laps.begin(), laps.end());
    int64_t sum = 0;
    for (const auto lap : laps)
        sum += lap;

    report("scan_stall", "ns_per_send_mean", sum / static_cast<int64_t>(laps.size()));
    report("scan_stall", "ns_per_send_p99", laps[static_cast<size_t>(laps.size() * 0.99)]);
    report("scan_stall", "ns_per_send_max", laps.back());
}

bool wants(int argc, char** argv, const char* name)
{
    if (argc <= 1)
        return true;
    for (int i = 1; i < argc; ++i)
        if (std::strcmp(argv[i], name) == 0)
            return true;
    return false;
}

}  // namespace

int main(int argc, char** argv)
{
    if (wants(argc, argv, "send0"))
        benchSend("send0", 0, true);
    if (wants(argc, argv, "send1"))
        benchSend("send1", 1, true);
    if (wants(argc, argv, "send4"))
        benchSend("send4", 4, true);
    if (wants(argc, argv, "send1_nolast"))
        benchSend("send1_nolast", 1, false);
    if (wants(argc, argv, "dequeue"))
        benchDequeue();
    if (wants(argc, argv, "pipe"))
        benchPipe();
    if (wants(argc, argv, "contend_config"))
        benchConfigContention();
    if (wants(argc, argv, "scan_stall"))
        benchScanStall();

    return 0;
}
