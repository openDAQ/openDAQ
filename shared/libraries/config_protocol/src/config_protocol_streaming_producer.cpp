#include <config_protocol/config_protocol_streaming_producer.h>
#include <opendaq/custom_log.h>

namespace daq::config_protocol
{

ConfigProtocolStreamingProducer::ConfigProtocolStreamingProducer(const ContextPtr& daqContext, const SendDaqPacketCallback& sendDaqPacketCallback)
    : sendDaqPacketCb(sendDaqPacketCallback)
    , signalNumericIdCounter(0)
    , stopped(false)
    , daqContext(daqContext)
    , loggerComponent(this->daqContext.getLogger().getOrAddComponent("ClientToServerStreaming"))
{
    readerThread = std::thread(&ConfigProtocolStreamingProducer::readerThreadFunc, this);
}

ConfigProtocolStreamingProducer::~ConfigProtocolStreamingProducer()
{
    stopped = true;
    if (readerThread.get_id() != std::this_thread::get_id())
    {
        if (readerThread.joinable())
        {
            readerThread.join();
            LOG_I("Streaming producer thread joined");
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

uint32_t ConfigProtocolStreamingProducer::registerOrUpdateSignal(const SignalPtr& signal)
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
        assert(signal.assigned());
        return signal;
    }
    return nullptr;
}

void ConfigProtocolStreamingProducer::addConnection(const SignalPtr& signal, const StringPtr& inputPortRemoteGlobalId)
{
    std::scoped_lock lock(sync);

    const auto signalId = signal.getGlobalId();
    LOG_I("Signal \"{}\" connected to \"{}\" input port", signalId, inputPortRemoteGlobalId);

    if (const auto domainSignal = signal.getDomainSignal(); domainSignal.assigned())
        addStreamingTrigger(domainSignal, signalId);

    addStreamingTrigger(signal, inputPortRemoteGlobalId);
}

void ConfigProtocolStreamingProducer::removeConnection(const SignalPtr& signal, const StringPtr& inputPortRemoteGlobalId)
{
    std::scoped_lock lock(sync);
    LOG_I("Signal \"{}\" disconnected from \"{}\" input port", signal.getGlobalId(), inputPortRemoteGlobalId);
    removeStreamingTrigger(signal, inputPortRemoteGlobalId);
}

void ConfigProtocolStreamingProducer::addStreamingTrigger(const SignalPtr& signal, const StringPtr& triggerComponentId)
{
    const auto signalId = signal.getGlobalId();

    if (const auto iter = streamedSignals.find(signalId); iter != streamedSignals.end())
    {
        auto& streamedSignal = iter->second;
        LOG_I("Component \"{}\" triggers streaming for signal \"{}\"", triggerComponentId, signalId);
        if (streamedSignal.triggerComponents.empty())
            startReadSignal(streamedSignal);
        streamedSignal.triggerComponents.insert(triggerComponentId);
    }
    else
    {
        throw ConfigProtocolException(fmt::format("Signal \"{}\" was not registered", signalId));
    }
}

void ConfigProtocolStreamingProducer::removeStreamingTrigger(const SignalPtr& signal, const StringPtr& triggerComponentId)
{
    const auto signalId = signal.getGlobalId();

    if (const auto iter = streamedSignals.find(signalId); iter != streamedSignals.end())
    {
        auto& streamedSignal = iter->second;
        if (const auto triggerIt = streamedSignal.triggerComponents.find(triggerComponentId);
            triggerIt != streamedSignal.triggerComponents.end())
        {
            LOG_I("Remove streaming trigger \"{}\" for signal \"{}\"", triggerComponentId, signalId);
            streamedSignal.triggerComponents.erase(triggerIt);
        }
        if (streamedSignal.triggerComponents.empty())
        {
            if (const auto domainSignal = signal.getDomainSignal(); domainSignal.assigned())
                removeStreamingTrigger(domainSignal, signalId);
            stopReadSignal(streamedSignal);
        }
    }
    else
    {
        throw ConfigProtocolException(fmt::format("Signal \"{}\" was not registered", signalId));
    }
}

void ConfigProtocolStreamingProducer::startReadSignal(StreamedSignal& streamedSignal)
{
    LOG_I("Start read signal \"{}\", numeric Id \"{}\"", streamedSignal.signal.getGlobalId(), streamedSignal.signalNumericId);
    streamedSignal.reader = PacketReader(streamedSignal.signal);
}

void ConfigProtocolStreamingProducer::stopReadSignal(StreamedSignal& streamedSignal)
{
    LOG_I("Stop read signal \"{}\", numeric Id \"{}\"", streamedSignal.signal.getGlobalId(), streamedSignal.signalNumericId);
    streamedSignal.reader.release();
    streamedSignal.signal.release();
}

void ConfigProtocolStreamingProducer::readerThreadFunc()
{
    while (!stopped)
    {
        std::scoped_lock lock(sync);

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

ConfigProtocolStreamingProducer::StreamedSignal::StreamedSignal(const SignalPtr& signal, uint32_t signalNumericId)
    : signal(signal)
    , signalNumericId(signalNumericId)
{
}

}
