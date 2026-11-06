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

#include <native_streaming_protocol/native_streaming_server_handler.h>
#include <testutils/benchmark_utils.h>

#include <opendaq/opendaq.h>

#include <atomic>
#include <chrono>
#include <memory>
#include <string>
#include <thread>
#include <vector>

// A standalone native streaming server with a saturating producer, shared by the transport and the
// client benchmark. The producer hands packets to the server handler the way the server module does
// from its reader ports, bounded by an in-flight window.

namespace daq::streaming_benchmark
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

class BenchmarkServer
{
public:
    using Settings = testutils::benchmark::Settings;
    using WorkGuardType = boost::asio::executor_work_guard<boost::asio::io_context::executor_type>;

    BenchmarkServer(const Settings& settings, uint16_t port, const ContextPtr& context)
        : settings(settings)
        , port(port)
        , context(context)
    {
    }

    ~BenchmarkServer()
    {
        stop();
    }

    void start()
    {
        createSignals();
        startServer();
    }

    // `received` is the consumer's own packet count, every packet on the wire or value packets only;
    // generated packets are counted the same way.
    void startProducer(const std::atomic<uint64_t>& received, bool receivedIncludesDomainPackets)
    {
        receivedPackets = &received;
        generatedPerIteration = receivedIncludesDomainPackets ? 2 * settings.signals : settings.signals;
        producerRunning = true;
        producerThread = std::thread([this]() { produce(); });
    }

    void stopProducer()
    {
        producerRunning = false;
        if (producerThread.joinable())
            producerThread.join();
    }

    void stop()
    {
        stopProducer();
        if (ioContext)
            ioContext->stop();
        if (ioThread.joinable())
            ioThread.join();
        workGuard.reset();
        serverHandler.reset();
        ioContext.reset();
    }

    uint64_t generated() const
    {
        return generatedPackets.load(std::memory_order_relaxed);
    }

    const Settings settings;
    const uint16_t port;
    ContextPtr context;

    DataDescriptorPtr valueDescriptor;
    DataDescriptorPtr domainDescriptor;
    std::vector<SignalConfigPtr> domainSignals;
    std::vector<SignalConfigPtr> valueSignals;
    ListPtr<ISignal> serverSignals;

    std::shared_ptr<opendaq_native_streaming_protocol::NativeStreamingServerHandler> serverHandler;

private:
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
            auto domainSignal = SignalWithDescriptor(context, domainDescriptor, nullptr, "domain" + std::to_string(i));
            auto valueSignal = SignalWithDescriptor(context, valueDescriptor, nullptr, "value" + std::to_string(i));
            valueSignal.setDomainSignal(domainSignal);
            domainSignals.push_back(domainSignal);
            valueSignals.push_back(valueSignal);
            serverSignals.pushBack(domainSignal);
            serverSignals.pushBack(valueSignal);
        }
    }

    void startServer()
    {
        using namespace opendaq_native_streaming_protocol;

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
            context,
            ioContext,
            serverSignals,
            onSubscribed,
            onUnsubscribed,
            setUpConfigProtocolServerCb,
            [](const std::string&, const std::string&, bool, ClientType, const std::string&) {},
            [](const std::string&) {},
            config);
        serverHandler->startServer(port);
    }

    // One iteration sends one domain and one value packet per signal in a buffer the server adopts.
    void produce()
    {
        using opendaq_native_streaming_protocol::PacketBufferData;

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
            generatedPackets.fetch_add(generatedPerIteration, std::memory_order_relaxed);

            while (producerRunning &&
                   generatedPackets.load(std::memory_order_relaxed) - receivedPackets->load(std::memory_order_relaxed) > settings.inFlight)
            {
                std::this_thread::sleep_for(std::chrono::microseconds(200));
            }
        }
    }

    std::shared_ptr<boost::asio::io_context> ioContext;
    std::unique_ptr<WorkGuardType> workGuard;
    std::thread ioThread;

    const std::atomic<uint64_t>* receivedPackets = nullptr;
    size_t generatedPerIteration = 0;
    std::atomic<uint64_t> generatedPackets{0};
    std::atomic<bool> producerRunning{false};
    std::thread producerThread;
};

}
