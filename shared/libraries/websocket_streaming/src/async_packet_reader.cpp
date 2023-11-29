#include "websocket_streaming/async_packet_reader.h"
#include <opendaq/instance_factory.h>
#include <opendaq/custom_log.h>
#include <opendaq/search_filter_factory.h>

BEGIN_NAMESPACE_OPENDAQ_WEBSOCKET_STREAMING

AsyncPacketReader::AsyncPacketReader(const DevicePtr& device, const ContextPtr& context)
    : device(device)
    , context(context)
    , logger(context.getLogger())
    , loggerComponent(logger.getOrAddComponent("WebsocketStreamingPacketReader"))
{
    setLoopFrequency(50);
    onPacketCallback = [](const SignalPtr& signal, const ListPtr<IPacket>& packets) {};
}

AsyncPacketReader::~AsyncPacketReader()
{
    stop();
}

void AsyncPacketReader::start()
{
    readThreadStarted = true;
    this->readThread = std::thread([this]()
    {
        this->startReadThread();
        LOG_I("Reading thread finished");
    });
}

void AsyncPacketReader::stop()
{
    readThreadStarted = false;
    if (readThread.joinable())
    {
        readThread.join();
        LOG_I("Reading thread joined");
    }

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
    while (readThreadStarted)
    {
        {
            std::scoped_lock lock(readersSync);
            for (const auto& [signal, reader] : signalReaders)
            {
                if (reader.getAvailableCount() == 0)
                    continue;

                const auto& packets = reader.readAll();
                onPacketCallback(signal, packets);
            }
        }

        std::this_thread::sleep_for(sleepTime);
    }
}

void AsyncPacketReader::createReaders()
{
    signalReaders.clear();
    auto signals = device.getSignals(search::Recursive(search::Any()));

    for (const auto& signal : signals)
    {
        addReader(signal);
    }
}

void AsyncPacketReader::startReadSignal(const SignalPtr& signal)
{
    std::scoped_lock lock(readersSync);
    addReader(signal);
}

void AsyncPacketReader::stopReadSignal(const SignalPtr& signal)
{
    std::scoped_lock lock(readersSync);
    removeReader(signal);
}

void AsyncPacketReader::addReader(SignalPtr signalToRead)
{
    auto it = std::find_if(signalReaders.begin(),
                           signalReaders.end(),
                           [&signalToRead](const std::pair<SignalPtr, PacketReaderPtr>& element)
                           {
                               return element.first == signalToRead;
                           });
    if (it != signalReaders.end())
        return;

    LOG_I("Add reader for signal {}", signalToRead.getGlobalId());
    auto reader = PacketReader(signalToRead);
    signalReaders.push_back(std::pair<SignalPtr, PacketReaderPtr>({signalToRead, reader}));
}

void AsyncPacketReader::removeReader(SignalPtr signalToRead)
{
    auto it = std::find_if(signalReaders.begin(),
                           signalReaders.end(),
                           [&signalToRead](const std::pair<SignalPtr, PacketReaderPtr>& element)
                           {
                               return element.first == signalToRead;
                           });
    if (it == signalReaders.end())
        return;

    LOG_I("Remove reader for signal {}", signalToRead.getGlobalId());
    signalReaders.erase(it);
}

END_NAMESPACE_OPENDAQ_WEBSOCKET_STREAMING

