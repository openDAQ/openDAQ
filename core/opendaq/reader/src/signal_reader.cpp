#include <opendaq/event_packet_ids.h>
#include <opendaq/signal_reader.h>
#include <opendaq/event_packet_params.h>
#include <opendaq/reader_errors.h>
#include <opendaq/custom_log.h>
#include <opendaq/packet_factory.h>

BEGIN_NAMESPACE_OPENDAQ

SignalReader::SignalReader(const InputPortConfigPtr& port,
                           SampleType valueReadType,
                           SampleType domainReadType,
                           ReadMode mode,
                           const LoggerComponentPtr& logger)
    : loggerComponent(logger)
    , valueReader(createReaderForType(mode == ReadMode::RawValue ? SampleType::Undefined : valueReadType, nullptr))
    , domainReader(createReaderForType(domainReadType, nullptr))
    , port(port)
    , connection(port.getConnection())
    , readMode(mode)
    , domainInfo(logger)
    , sampleRate(-1)
{
}

SignalReader::SignalReader(const SignalReader& old,
                           const InputPortNotificationsPtr& listener,
                           SampleType valueReadType,
                           SampleType domainReadType)
    : loggerComponent(old.loggerComponent)
    , valueReader(createReaderForType(old.readMode == ReadMode::RawValue ? SampleType::Undefined : valueReadType,
                                      old.valueReader->getTransformFunction()))
    , domainReader(createReaderForType(domainReadType, old.domainReader->getTransformFunction()))
    , port(old.port)
    , connection(port.getConnection())
    , changeCallback(old.changeCallback)
    , readMode(old.readMode)
    , domainInfo(loggerComponent)
    , sampleRate(-1)
{
    info = old.info;

    port.setListener(listener);
    readDescriptorFromPort();
}

SignalReader::SignalReader(const SignalInfo& old,
                           const InputPortNotificationsPtr& listener,
                           SampleType valueReadType,
                           SampleType domainReadType)
    : loggerComponent(old.loggerComponent)
    , valueReader(createReaderForType(old.readMode == ReadMode::RawValue ? SampleType::Undefined : valueReadType, old.valueTransformFunction))
    , domainReader(createReaderForType(domainReadType, old.domainTransformFunction))
    , port(old.port)
    , connection(port.getConnection())
    , changeCallback(old.changeCallback)
    , readMode(old.readMode)
    , domainInfo(loggerComponent)
    , sampleRate(-1)
{
    port.setListener(listener);
    readDescriptorFromPort();
}

void SignalReader::readDescriptorFromPort()
{
    PacketPtr packet = connection.peek();
    if (packet.assigned() && packet.getType() == PacketType::Event)
    {
        auto eventPacket = packet.asPtr<IEventPacket>(true);
        if (eventPacket.getEventId() == event_packet_id::DATA_DESCRIPTOR_CHANGED)
        {
            handleDescriptorChanged(connection.dequeue());
            return;
        }
    }

    const auto signal = port.getSignal();
    if (!signal.assigned())
    {
        throw InvalidStateException("Input port must already have a signal assigned");
    }

    const auto descriptor = signal.getDescriptor();
    if (!descriptor.assigned())
    {
        throw InvalidStateException("Input port connected signal must have a descriptor assigned.");
    }

    DataDescriptorPtr domainDescriptor;
    auto domainSignal = signal.getDomainSignal();
    if (domainSignal.assigned())
    {
        domainDescriptor = domainSignal.getDescriptor();
    }

    handleDescriptorChanged(DataDescriptorChangedEventPacket(descriptor, domainDescriptor));
}

SizeT SignalReader::getAvailable(bool acrossDescriptorChanges = false) const
{
    SizeT count = 0;
    if (info.dataPacket.assigned())
    {
        count = info.dataPacket.getSampleCount() - info.prevSampleIndex;
    }

    count += acrossDescriptorChanges
        ? connection.getAvailableSamples()
        : connection.getSamplesUntilNextDescriptor();
    return count;
}

[[maybe_unused]]
static std::string printSync(SyncStatus synced)
{
    switch (synced)
    {
        case SyncStatus::Unsynchronized:
            return "Unsynchronized";
        case SyncStatus::Synchronizing:
            return "Synchronizing";
        case SyncStatus::Synchronized:
            return "Synchronized";
    }

    return "<Unknown>";
}

void SignalReader::handleDescriptorChanged(const EventPacketPtr& eventPacket)
{
    if (!eventPacket.assigned())
        return;

    auto params = eventPacket.getParameters();
    DataDescriptorPtr newValueDescriptor = params[event_packet_param::DATA_DESCRIPTOR];
    DataDescriptorPtr newDomainDescriptor = params[event_packet_param::DOMAIN_DATA_DESCRIPTOR];

    if (newValueDescriptor.assigned() && valueReader->getReadType() == SampleType::Undefined)
    {
        valueReader = createReaderForType(newValueDescriptor.getSampleType(), valueReader->getTransformFunction());
    }
    
    invalid = !valueReader->handleDescriptorChanged(newValueDescriptor);
    auto validDomain = domainReader->handleDescriptorChanged(newDomainDescriptor);
    if (validDomain && newDomainDescriptor.assigned())
    {
        auto newResolution = newDomainDescriptor.getTickResolution();
        if (domainInfo.resolution != newResolution)
        {
            domainInfo.resolution = newResolution;
            synced = SyncStatus::Unsynchronized;
        }

        std::string origin = newDomainDescriptor.getOrigin();
        auto newOrigin = reader::parseEpoch(origin);
        if (domainInfo.epoch != newOrigin)
        {
            domainInfo.epoch = newOrigin;
            synced = SyncStatus::Unsynchronized;
        }

        auto newSampleRate = reader::getSampleRate(newDomainDescriptor);
        if (sampleRate == -1)
        {
            sampleRate = newSampleRate;
        }
        else if (sampleRate != newSampleRate)
        {
            validDomain = false;
        }
    }

    invalid = invalid || !validDomain;

    // If both value and domain are still convertible
    // check with the user if new state is valid for them
    if (!invalid && changeCallback.assigned())
    {
        bool descriptorOk = false;
        ErrCode errCode = wrapHandlerReturn(changeCallback, descriptorOk, newValueDescriptor, newDomainDescriptor);
        invalid = !descriptorOk || OPENDAQ_FAILED(errCode);

        if (OPENDAQ_FAILED(errCode))
            daqClearErrorInfo();
    }

    LOG_T("[Signal Descriptor Changed: {} | {} | {}]",
        port.getSignal().getLocalId(),
        printSync(synced),
        invalid ? "Invalid" : "Valid"
    )
}

void SignalReader::prepare(void* outValues, SizeT count, std::chrono::milliseconds timeoutTime)
{
    info.prepare(outValues, count, timeoutTime);
}

void SignalReader::prepareWithDomain(void* outValues, void* domain, SizeT count, std::chrono::milliseconds timeoutTime)
{
    info.prepareWithDomain(outValues, domain, count, timeoutTime);
}

void SignalReader::setStartInfo(std::chrono::system_clock::time_point minEpoch, const RatioPtr& maxResolution)
{
    LOG_T("---")

    domainInfo.setEpochOffset(minEpoch, maxResolution);
    domainInfo.setMaxResolution(maxResolution);

    synced = SyncStatus::Unsynchronized;
}

std::unique_ptr<Comparable> SignalReader::readStartDomain()
{
    readUntilNextDataPacket();

    DataPacketPtr domainPacket = info.dataPacket.getDomainPacket();
    if (!domainPacket.assigned())
    {
        throw InvalidStateException("Packet must have a domain packet assigned!");
    }

    return domainReader->readStart(domainPacket.getData(), info.prevSampleIndex, domainInfo);
}

void SignalReader::readUntilNextDataPacket()
{
    if (info.dataPacket.assigned())
    {
        return;
    }

    PacketPtr packet = connection.dequeue();
    while (packet.assigned() && packet.getType() != PacketType::Data)
    {
        bool firstData{false};
        ErrCode errCode = handlePacket(packet, firstData);
        checkErrorInfo(errCode);

        packet = connection.dequeue();
    }        
    
    info.dataPacket = packet;
}

bool SignalReader::sync(const Comparable& commonStart)
{
    if (synced == SyncStatus::Synchronized)
        return true;

    readUntilNextDataPacket();

    SizeT startPackets = info.prevSampleIndex;
    Int droppedPackets = 0;

    while (info.dataPacket.assigned())
    {
        auto domainPacket = info.dataPacket.getDomainPacket();

        info.prevSampleIndex = domainReader->getOffsetTo(
            domainInfo,
            commonStart,
            domainPacket.getData(),
            domainPacket.getSampleCount()
        );

        if (info.prevSampleIndex == static_cast<SizeT>(-1))
        {
            droppedPackets += static_cast<Int>(domainPacket.getSampleCount() - startPackets);

            info.dataPacket = nullptr;
            readUntilNextDataPacket();

            startPackets = 0;
        }
        else
        {
            droppedPackets += static_cast<Int>(info.prevSampleIndex - startPackets);
            break;
        }
    }

    synced = info.prevSampleIndex != static_cast<SizeT>(-1)
        ? SyncStatus::Synchronized
        : SyncStatus::Synchronizing;


    LOG_T("[Syncing: {} | {}, dropped {} samples]", port.getSignal().getLocalId(), printSync(synced), droppedPackets);
    return synced == SyncStatus::Synchronized;
}

ErrCode SignalReader::handlePacket(const PacketPtr& packet, bool& firstData)
{
    ErrCode errCode = OPENDAQ_SUCCESS;
    switch (packet.getType())
    {
        case PacketType::Data:
        {
            info.dataPacket = packet;

            errCode = readPacketData();
            firstData = true;
            break;
        }
        case PacketType::Event:
        {
            // Handle events

            auto eventPacket = packet.asPtrOrNull<IEventPacket>(true);
            if (eventPacket.getEventId() == event_packet_id::DATA_DESCRIPTOR_CHANGED)
            {
                try
                {
                    handleDescriptorChanged(eventPacket);
                }
                catch (...)
                {
                    errCode = OPENDAQ_ERR_GENERALERROR;
                }

                if (OPENDAQ_FAILED(errCode))
                {
                    invalid = true;

                    return makeErrorInfo(
                        OPENDAQ_ERR_INVALID_DATA,
                        "Exception occurred while processing a signal descriptor change",
                        nullptr
                    );
                }

                if (invalid)
                {
                    return makeErrorInfo(
                        OPENDAQ_ERR_INVALID_DATA,
                        "Signal no longer compatible with the reader or other signals",
                        nullptr
                    );
                }
            }
            break;
        }
        case PacketType::None:
            break;
    }

    return errCode;
}

ErrCode SignalReader::readPackets()
{
    bool firstData = false;
    ErrCode errCode = OPENDAQ_SUCCESS;

    ReadInfo::Duration remainingTime = info.timeout;
    while (info.remainingToRead > 0 && remainingTime.count() >= 0)
    {
        PacketPtr packet = info.dataPacket;
        if (!packet.assigned())
        {
            packet = connection.dequeue();
        }

        if (packet.assigned())
            errCode = handlePacket(packet, firstData);

        if (info.timeout.count() != 0)
            remainingTime = info.timeout - info.durationFromStart();
    }

    return errCode;
}

void* SignalReader::getValuePacketData(const DataPacketPtr& packet) const
{
    switch (readMode)
    {
        case ReadMode::RawValue:
        case ReadMode::Unscaled:
            return packet.getRawData();
        case ReadMode::Scaled:
            return packet.getData();
    }

    throw InvalidOperationException("Unknown Reader read-mode of {}", static_cast<std::underlying_type_t<ReadMode>>(readMode));
}

ErrCode SignalReader::readPacketData()
{
    auto remainingSampleCount = info.dataPacket.getSampleCount() - info.prevSampleIndex;
    SizeT toRead = std::min(info.remainingToRead, remainingSampleCount);

    ErrCode errCode = valueReader->readData(getValuePacketData(info.dataPacket), info.prevSampleIndex, &info.values, toRead);
    if (OPENDAQ_FAILED(errCode))
    {
        return errCode;
    }

    if (info.domainValues != nullptr)
    {
        auto dataPacket = info.dataPacket;
        if (!dataPacket.getDomainPacket().assigned())
        {
            return makeErrorInfo(
                OPENDAQ_ERR_INVALIDSTATE,
                "Packets must have an associated domain packets to read domain data.",
                nullptr
            );
        }

        LOG_T("[Reading: {} ", port.getSignal().getLocalId());

        auto domainPacket = dataPacket.getDomainPacket();
        errCode = domainReader->readData(domainPacket.getData(), info.prevSampleIndex, &info.domainValues, toRead);
        if (errCode == OPENDAQ_ERR_INVALIDSTATE)
        {
            if (!trySetDomainSampleType(domainPacket))
            {
                return errCode;
            }
            errCode = domainReader->readData(domainPacket.getData(), info.prevSampleIndex, &info.domainValues, toRead);
        }

        LOG_T("]");

        if (OPENDAQ_FAILED(errCode))
        {
            return errCode;
        }
    }

    if (toRead < remainingSampleCount)
    {
        info.prevSampleIndex += toRead;
    }
    else
    {
        info.reset();
    }

    info.remainingToRead -= toRead;
    return OPENDAQ_SUCCESS;
}

bool SignalReader::trySetDomainSampleType(const daq::DataPacketPtr& domainPacket) const
{
    ObjectPtr<IErrorInfo> errInfo;
    daqGetErrorInfo(&errInfo);
    daqClearErrorInfo();

    auto dataDescriptor = domainPacket.getDataDescriptor();
    if (!domainReader->handleDescriptorChanged(dataDescriptor))
    {
        daqSetErrorInfo(errInfo);
        return false;
    }
    return true;
}

END_NAMESPACE_OPENDAQ
