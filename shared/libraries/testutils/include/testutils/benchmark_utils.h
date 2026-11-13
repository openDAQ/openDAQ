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

#pragma once

#include <gtest/gtest.h>

#include <algorithm>
#include <chrono>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <functional>
#include <string>
#include <thread>
#include <vector>

// Shared scaffolding for the streaming throughput benchmarks: opt-in gtest cases that report rates
// instead of asserting them.

namespace daq::testutils::benchmark
{

inline const char* envValue(const char* name)
{
    const char* value = std::getenv(name);
    return (value != nullptr && *value != '\0') ? value : nullptr;
}

inline size_t envSize(const char* name, size_t defaultValue)
{
    const char* value = envValue(name);
    return value ? static_cast<size_t>(std::strtoull(value, nullptr, 10)) : defaultValue;
}

inline std::string envString(const char* name, const std::string& defaultValue = "")
{
    const char* value = envValue(name);
    return value ? std::string(value) : defaultValue;
}

inline bool enabled()
{
    return envString("OPENDAQ_BENCHMARK") == "1";
}

// Pins the process to the CPUs in the mask; a zero mask leaves the affinity alone.
bool applyAffinity(uint64_t mask);

// OPENDAQ_BENCH_AFFINITY is a hexadecimal CPU mask; on a hybrid CPU pin to the performance cores for
// repeatable rates.
inline uint64_t affinityFromEnv()
{
    const char* value = envValue("OPENDAQ_BENCH_AFFINITY");
    return value ? std::strtoull(value, nullptr, 16) : 0;
}

// Load and timing knobs, overridable from the environment; the defaults approximate a device sending
// ~128-byte Float32 packets at 100 packets/s per signal.
struct Settings
{
    size_t signals = 250;
    size_t samplesPerPacket = 32;
    size_t loopMs = 10;
    size_t warmUpSeconds = 3;
    size_t seconds = 10;
    size_t cacheablePayloadSizeMax = 16384;
    size_t inFlight = 10000;
    std::string csvPath;

    static Settings fromEnv()
    {
        Settings s;
        s.signals = envSize("OPENDAQ_BENCH_SIGNALS", s.signals);
        s.samplesPerPacket = envSize("OPENDAQ_BENCH_SAMPLES", s.samplesPerPacket);
        s.loopMs = envSize("OPENDAQ_BENCH_LOOP_MS", s.loopMs);
        s.warmUpSeconds = envSize("OPENDAQ_BENCH_WARMUP", s.warmUpSeconds);
        s.seconds = envSize("OPENDAQ_BENCH_SECONDS", s.seconds);
        s.cacheablePayloadSizeMax = envSize("OPENDAQ_BENCH_CACHEABLE", s.cacheablePayloadSizeMax);
        s.inFlight = envSize("OPENDAQ_BENCH_INFLIGHT", s.inFlight);
        s.csvPath = envString("OPENDAQ_BENCH_CSV");
        return s;
    }

    // Packets per second one signal produces at the device loop rate.
    double packetsPerSecondPerSignal() const
    {
        return 1000.0 / static_cast<double>(loopMs);
    }

    void print(const char* testName) const
    {
        std::printf("[bench] %s: signals=%zu samples=%zu loop=%zums cacheable=%zu inflight=%zu warmup=%zus window=%zus affinity=0x%llx\n",
                    testName,
                    signals,
                    samplesPerPacket,
                    loopMs,
                    cacheablePayloadSizeMax,
                    inFlight,
                    warmUpSeconds,
                    seconds,
                    static_cast<unsigned long long>(affinityFromEnv()));
        std::fflush(stdout);
    }
};

// Cumulative counters read once per second; the sampler works on the differences.
struct Snapshot
{
    uint64_t receivedPackets = 0;
    uint64_t receivedBytes = 0;
    uint64_t generatedPackets = 0;
};

struct Sample
{
    size_t second = 0;
    double receivedPacketsPerSecond = 0;
    double receivedBytesPerSecond = 0;
    double generatedPacketsPerSecond = 0;
    double ratio = 0;
};

struct Summary
{
    std::vector<Sample> samples;
    double meanReceivedPacketsPerSecond = 0;
    double minReceivedPacketsPerSecond = 0;
    double maxReceivedPacketsPerSecond = 0;
    double meanReceivedBytesPerSecond = 0;
    double meanRatio = 0;
    double minRatio = 0;
};

// Sleeps through the warm-up, then samples the counters once a second, printing one line per second
// and a summary; the ratio is received over generated packets in that second.
inline Summary measure(const Settings& settings, const std::function<Snapshot()>& snapshot, const char* testName)
{
    using Clock = std::chrono::steady_clock;

    std::this_thread::sleep_for(std::chrono::seconds(settings.warmUpSeconds));

    Summary summary;
    auto previous = snapshot();
    auto previousTime = Clock::now();
    auto tick = previousTime;

    for (size_t second = 1; second <= settings.seconds; ++second)
    {
        tick += std::chrono::seconds(1);
        std::this_thread::sleep_until(tick);

        const auto current = snapshot();
        const auto now = Clock::now();
        const double elapsed = std::chrono::duration<double>(now - previousTime).count();

        Sample sample;
        sample.second = second;
        sample.receivedPacketsPerSecond = static_cast<double>(current.receivedPackets - previous.receivedPackets) / elapsed;
        sample.receivedBytesPerSecond = static_cast<double>(current.receivedBytes - previous.receivedBytes) / elapsed;
        sample.generatedPacketsPerSecond = static_cast<double>(current.generatedPackets - previous.generatedPackets) / elapsed;
        sample.ratio = sample.generatedPacketsPerSecond > 0 ? sample.receivedPacketsPerSecond / sample.generatedPacketsPerSecond : 0;
        summary.samples.push_back(sample);

        std::printf("[bench] %s t=%2zu  recv %9.0f pkt/s %7.2f MB/s  gen %9.0f pkt/s  ratio %.3f\n",
                    testName,
                    second,
                    sample.receivedPacketsPerSecond,
                    sample.receivedBytesPerSecond / 1e6,
                    sample.generatedPacketsPerSecond,
                    sample.ratio);
        std::fflush(stdout);

        previous = current;
        previousTime = now;
    }

    if (!summary.samples.empty())
    {
        summary.minReceivedPacketsPerSecond = summary.samples.front().receivedPacketsPerSecond;
        summary.maxReceivedPacketsPerSecond = summary.samples.front().receivedPacketsPerSecond;
        summary.minRatio = summary.samples.front().ratio;
        for (const auto& sample : summary.samples)
        {
            summary.meanReceivedPacketsPerSecond += sample.receivedPacketsPerSecond;
            summary.meanReceivedBytesPerSecond += sample.receivedBytesPerSecond;
            summary.meanRatio += sample.ratio;
            summary.minReceivedPacketsPerSecond = std::min(summary.minReceivedPacketsPerSecond, sample.receivedPacketsPerSecond);
            summary.maxReceivedPacketsPerSecond = std::max(summary.maxReceivedPacketsPerSecond, sample.receivedPacketsPerSecond);
            summary.minRatio = std::min(summary.minRatio, sample.ratio);
        }
        const auto count = static_cast<double>(summary.samples.size());
        summary.meanReceivedPacketsPerSecond /= count;
        summary.meanReceivedBytesPerSecond /= count;
        summary.meanRatio /= count;
    }

    std::printf("[bench] %s summary: recv pkt/s mean %.0f min %.0f max %.0f; %.2f MB/s; ratio mean %.3f min %.3f\n",
                testName,
                summary.meanReceivedPacketsPerSecond,
                summary.minReceivedPacketsPerSecond,
                summary.maxReceivedPacketsPerSecond,
                summary.meanReceivedBytesPerSecond / 1e6,
                summary.meanRatio,
                summary.minRatio);
    std::fflush(stdout);

    if (!settings.csvPath.empty())
    {
        // Append so a before/after series from several runs lands in one file.
        std::ofstream csv(settings.csvPath, std::ios::app);
        for (const auto& sample : summary.samples)
        {
            csv << testName << ',' << settings.signals << ',' << settings.samplesPerPacket << ',' << sample.second << ','
                << sample.receivedPacketsPerSecond << ',' << sample.receivedBytesPerSecond << ',' << sample.generatedPacketsPerSecond << ','
                << sample.ratio << '\n';
        }
    }

    return summary;
}

}

#define DAQ_BENCHMARK_OR_SKIP()                                                                      \
    do                                                                                               \
    {                                                                                                \
        if (!daq::testutils::benchmark::enabled())                                                   \
            GTEST_SKIP() << "Benchmark; set OPENDAQ_BENCHMARK=1 to run it";                          \
        if (!daq::testutils::benchmark::applyAffinity(daq::testutils::benchmark::affinityFromEnv())) \
            FAIL() << "Failed to apply OPENDAQ_BENCH_AFFINITY";                                      \
    } while (false)
