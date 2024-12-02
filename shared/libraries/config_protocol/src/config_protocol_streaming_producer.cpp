#include <config_protocol/config_protocol_streaming_producer.h>
#include <opendaq/custom_log.h>

namespace daq::config_protocol
{

ConfigProtocolStreamingProducer::ConfigProtocolStreamingProducer(const ContextPtr& daqContext, const SendDaqPacketCallback& sendDaqPacketCallback)
    : sendDaqPacketCb(sendDaqPacketCallback)
    , signalNumericIdCounter(0)
    , readThreadRunning(false)
    , daqContext(daqContext)
    , loggerComponent(this->daqContext.getLogger().getOrAddComponent("ClientToServerStreamingProducer"))
    , readThreadSleepTime(std::chrono::milliseconds(20))
{
}

ConfigProtocolStreamingProducer::~ConfigProtocolStreamingProducer()
{
    if (readThreadRunning)
        stopReadThread();
}

SignalNumericIdType ConfigProtocolStreamingProducer::registerOrUpdateSignal(const SignalPtr& signal)
{
    std::scoped_lock lock(sync);

    const auto signalId = signal.getGlobalId();

    if (const auto iter = streamedSignals.find(signalId); iter != streamedSignals.end())
    {
        auto& streamedSignal = iter->second;
        if (!streamedSignal.signal.assigned())
            streamedSignal.signal = signal;
        return streamedSignal.signalNumericId;
    }
    else
    {
        const auto signalNumericId = ++signalNumericIdCounter;
        streamedSignals.insert({signalId, StreamedSignal(signal, signalNumericId)});
        return signalNumericId;
    }
}

SignalPtr ConfigProtocolStreamingProducer::findRegisteredSignal(const StringPtr& signalId)
{
    std::scoped_lock lock(sync);

    if (const auto iter = streamedSignals.find(signalId); iter != streamedSignals.end())
    {
        auto signal = iter->second.signal;
        return signal;
    }
    return nullptr;
}

void ConfigProtocolStreamingProducer::addConnection(const SignalPtr& signal, const StringPtr& inputPortRemoteGlobalId)
{
    std::scoped_lock lock(sync);

    const auto signalId = signal.getGlobalId();
    LOG_D("Signal \"{}\" connected to \"{}\" input port", signalId, inputPortRemoteGlobalId);

    if (!readThreadRunning)
        startReadThread();

    if (const auto domainSignal = signal.getDomainSignal(); domainSignal.assigned())
        addStreamingTrigger(domainSignal, signalId);

    addStreamingTrigger(signal, inputPortRemoteGlobalId);
}

void ConfigProtocolStreamingProducer::removeConnection(const SignalPtr& signal, const StringPtr& inputPortRemoteGlobalId, std::vector<SignalNumericIdType>& unusedSignlasIds)
{
    std::scoped_lock lock(sync);
    LOG_D("Signal \"{}\" disconnected from \"{}\" input port", signal.getGlobalId(), inputPortRemoteGlobalId);
    removeStreamingTrigger(signal, inputPortRemoteGlobalId, unusedSignlasIds);

    if (!hasSignalToRead() && readThreadRunning)
        stopReadThread();
}

void ConfigProtocolStreamingProducer::addStreamingTrigger(const SignalPtr& signal, const StringPtr& triggerComponentId)
{
    const auto signalId = signal.getGlobalId();

    if (const auto iter = streamedSignals.find(signalId); iter != streamedSignals.end())
    {
        auto& streamedSignal = iter->second;
        LOG_D("Component \"{}\" triggers streaming for signal \"{}\"", triggerComponentId, signalId);
        if (streamedSignal.triggerComponents.empty())
            startReadSignal(streamedSignal);
        streamedSignal.triggerComponents.insert(triggerComponentId);
    }
    else
    {
        throw ConfigProtocolException(fmt::format("Signal \"{}\" was not registered", signalId));
    }
}

void ConfigProtocolStreamingProducer::removeStreamingTrigger(const SignalPtr& signal, const StringPtr& triggerComponentId, std::vector<SignalNumericIdType>& unusedSignlasIds)
{
    const auto signalId = signal.getGlobalId();

    if (const auto iter = streamedSignals.find(signalId); iter != streamedSignals.end())
    {
        auto& streamedSignal = iter->second;
        if (const auto triggerIt = streamedSignal.triggerComponents.find(triggerComponentId);
            triggerIt != streamedSignal.triggerComponents.end())
        {
            LOG_D("Remove streaming trigger \"{}\" for signal \"{}\"", triggerComponentId, signalId);
            streamedSignal.triggerComponents.erase(triggerIt);
        }
        if (streamedSignal.triggerComponents.empty())
        {
            if (const auto domainSignal = signal.getDomainSignal(); domainSignal.assigned())
                removeStreamingTrigger(domainSignal, signalId, unusedSignlasIds);
            stopReadSignal(streamedSignal);
            unusedSignlasIds.push_back(streamedSignal.signalNumericId);
        }
    }
    else
    {
        throw ConfigProtocolException(fmt::format("Signal \"{}\" was not registered", signalId));
    }
}

void ConfigProtocolStreamingProducer::startReadSignal(StreamedSignal& streamedSignal)
{
    LOG_D("Start read signal \"{}\", numeric Id \"{}\"", streamedSignal.signal.getGlobalId(), streamedSignal.signalNumericId);
    streamedSignal.reader = PacketReader(streamedSignal.signal);
}

void ConfigProtocolStreamingProducer::stopReadSignal(StreamedSignal& streamedSignal)
{
    LOG_D("Stop read signal \"{}\", numeric Id \"{}\"", streamedSignal.signal.getGlobalId(), streamedSignal.signalNumericId);
    streamedSignal.reader.release();
    streamedSignal.signal.release();
}

void ConfigProtocolStreamingProducer::readerThreadFunc()
{
    LOG_D("Streaming producer thread started");
    while (readThreadRunning)
    {
        {
            std::unique_lock<std::mutex> lock(sync, std::try_to_lock);
            if (lock.owns_lock())
            {
                for (const auto& [_, streamedSignal] : streamedSignals)
                {
                    if (const auto& reader = streamedSignal.reader; reader.assigned())
                    {
                        PacketPtr packet = reader.read();
                        while (packet.assigned())
                        {
                            if (sendDaqPacketCb)
                                sendDaqPacketCb(packet, streamedSignal.signalNumericId);
                            packet = reader.read();
                        }
                    }
                }
            }
        }

        std::this_thread::sleep_for(readThreadSleepTime);
    }
    LOG_D("Streaming producer thread stopped");
}

bool ConfigProtocolStreamingProducer::hasSignalToRead()
{
    for (const auto& [_, streamedSignal] : streamedSignals)
        if (const auto& reader = streamedSignal.reader; reader.assigned())
             return true;
    return false;
}

void ConfigProtocolStreamingProducer::startReadThread()
{
    assert(!readThreadRunning);
    readThreadRunning = true;
    readerThread = std::thread(&ConfigProtocolStreamingProducer::readerThreadFunc, this);
}

void ConfigProtocolStreamingProducer::stopReadThread()
{
    assert(readThreadRunning);
    readThreadRunning = false;
    if (readerThread.get_id() != std::this_thread::get_id())
    {
        if (readerThread.joinable())
        {
             readerThread.join();
             LOG_D("Streaming producer thread joined");
        }
        else
        {
             LOG_W("Streaming producer thread is not joinable");
        }
    }
    else
    {
        LOG_C("Streaming producer thread cannot join itself");
    }
}

ConfigProtocolStreamingProducer::StreamedSignal::StreamedSignal(const SignalPtr& signal, SignalNumericIdType signalNumericId)
    : signal(signal)
    , signalNumericId(signalNumericId)
{
}

}
