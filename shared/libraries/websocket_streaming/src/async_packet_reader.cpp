#include "websocket_streaming/async_packet_reader.h"
#include <opendaq/instance_factory.h>

BEGIN_NAMESPACE_OPENDAQ_WEBSOCKET_STREAMING

AsyncPacketReader::AsyncPacketReader()
{
    setLoopFrequency(50);
    onPacketCallback = [](const SignalPtr& signal, const ListPtr<IPacket>& packets) {};
}

AsyncPacketReader::~AsyncPacketReader()
{
    stopReading();
}

void AsyncPacketReader::startReading(const DevicePtr& device, const ContextPtr& context)
{
    this->device = device;
    this->context = context;

    readThreadStarted = true;
    this->readThread = std::thread([this]() { this->startReadThread(); });
}

void AsyncPacketReader::stopReading()
{
    readThreadStarted = false;
    if (readThread.joinable())
        readThread.join();

    signalReaders.clear();
}

void AsyncPacketReader::onPacket(const OnPacketCallback& callback)
{
    onPacketCallback = callback;
}

void AsyncPacketReader::setLoopFrequency(uint32_t freqency)
{
    uint64_t sleepMs = 1000.0 / freqency;
    this->sleepTime = std::chrono::milliseconds(sleepMs);
}

void AsyncPacketReader::startReadThread()
{
    createReaders();
    
    while (readThreadStarted)
    {
        for (const auto& [signal, reader] : signalReaders)
        {
            if (reader.getAvailableCount() == 0)
                continue;

            const auto& packets = reader.readAll();
            onPacketCallback(signal, packets);
        }

        std::this_thread::sleep_for(sleepTime);
    }
}

void AsyncPacketReader::createReaders()
{
    signalReaders.clear();
    auto signals = device.getSignalsRecursive();

    for (const auto& signal : signals)
    {
        auto reader = PacketReader(signal);
        signalReaders.push_back(std::pair<SignalPtr, PacketReaderPtr>({signal, reader}));
    }
}

END_NAMESPACE_OPENDAQ_WEBSOCKET_STREAMING

