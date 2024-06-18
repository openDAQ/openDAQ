#include <opendaq/event_packet_ids.h>
#include <opendaq/signal_reader.h>
#include <opendaq/event_packet_params.h>
#include <opendaq/reader_errors.h>
#include <opendaq/custom_log.h>
#include <opendaq/packet_factory.h>
#include <opendaq/reader_factory.h>

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
    , commonSampleRate(-1)
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
    , readMode(old.readMode)
    , domainInfo(loggerComponent)
    , sampleRate(-1)
    , commonSampleRate(-1)
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
    , readMode(old.readMode)
    , domainInfo(loggerComponent)
    , sampleRate(-1)
    , commonSampleRate(-1)
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

    if (connection.assigned())
    {
        count += acrossDescriptorChanges
            ? connection.getAvailableSamples()
            : connection.getSamplesUntilNextDescriptor();
    }
    return count * sampleRateDivider;
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

void SignalReader::setCommonSampleRate(const std::int64_t commonSampleRate)
{
    this->commonSampleRate = commonSampleRate;

    if (sampleRate > 0)
    {
        sampleRateDivider = static_cast<int32_t>(commonSampleRate / sampleRate);

        if ((sampleRateDivider == 0) || (commonSampleRate % sampleRateDivider != 0))
            invalid = true;
    }
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
        SampleType valueType;
        auto postScaling = newValueDescriptor.getPostScaling();
        if (!postScaling.assigned() || readMode == ReadMode::Scaled)
        {
            valueType = newValueDescriptor.getSampleType();
        }
        else
        {
            valueType = postScaling.getInputSampleType();
        }

        valueReader = createReaderForType(valueType, valueReader->getTransformFunction());
    }
    
    invalid = !valueReader->handleDescriptorChanged(newValueDescriptor, readMode);
    auto validDomain = domainReader->handleDescriptorChanged(newDomainDescriptor, readMode);
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

        const auto domainRule = newDomainDescriptor.getRule();
        if (domainRule.getType() == DataRuleType::Linear)
        {
            const auto domainRuleParams = domainRule.getParameters();
            packetDelta = domainRuleParams.get("delta");
        } 
    }

    invalid = invalid || !validDomain;

    LOG_T("[Signal Descriptor Changed: {} | {} | {}]",
        port.getSignal().getLocalId(),
        printSync(synced),
        invalid ? "Invalid" : "Valid"
    )
}

void SignalReader::prepare(void* outValues, SizeT count)
{
    info.prepare(outValues, sampleRateDivider == 0 ? 0 : (count / sampleRateDivider), std::chrono::milliseconds(0));
}

void SignalReader::prepareWithDomain(void* outValues, void* domain, SizeT count)
{
    info.prepareWithDomain(outValues, domain, sampleRateDivider == 0 ? 0 : (count / sampleRateDivider), std::chrono::milliseconds(0));
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
    DataPacketPtr domainPacket = info.dataPacket.getDomainPacket();
    if (!domainPacket.assigned())
    {
        throw InvalidStateException("Packet must have a domain packet assigned!");
    }

    return domainReader->readStart(domainPacket.getData(), info.prevSampleIndex, domainInfo);
}

bool SignalReader::isFirstPacketEvent()
{
    if (info.dataPacket.assigned())
    {
        return false;
    }

    if (!connection.assigned())
    {
        return false;
    }

    auto packet = connection.peek();
    while (packet.assigned())
    {
        if (packet.getType() == PacketType::Data)
        {
            connection.dequeue();
            info.dataPacket = packet;
            info.prevSampleIndex = 0;
            return false;
        }
        else if (packet.getType() == PacketType::Event)
        {
            return true;
        }
        connection.dequeue();
        packet = connection.peek();
    }
    return false;
}

EventPacketPtr SignalReader::readUntilNextDataPacket()
{
    if (!isFirstPacketEvent())
        return nullptr;

    EventPacketPtr packetToReturn;
    DataDescriptorPtr dataDescriptor;
    DataDescriptorPtr domainDescriptor;

    PacketPtr packet = connection.dequeue();
    while (packet.assigned())
    {
        if (packet.getType() == PacketType::Data)
            break;
        
        if (packet.getType() == PacketType::Event)
        {
            auto eventPacket = packet.asPtr<IEventPacket>(true);
            auto packetId = eventPacket.getEventId();
            if (packetId == event_packet_id::DATA_DESCRIPTOR_CHANGED)
            {
                auto params = eventPacket.getParameters();
                DataDescriptorPtr newValueDescriptor = params[event_packet_param::DATA_DESCRIPTOR];
                DataDescriptorPtr newDomainDescriptor = params[event_packet_param::DOMAIN_DATA_DESCRIPTOR];

                if (newValueDescriptor.assigned())
                {
                    dataDescriptor = newValueDescriptor;
                }
                if (newDomainDescriptor.assigned())
                {
                    domainDescriptor = newDomainDescriptor;
                }
            }
            else if (packetId == event_packet_id::IMPLICIT_DOMAIN_GAP_DETECTED)
            {
                if (!dataDescriptor.assigned() && !domainDescriptor.assigned())
                {
                    packetToReturn = packet;
                }
                break;
            }
        }

        packet = connection.dequeue();
    }

    if (packet.assigned() && packet.getType() == PacketType::Data)
    {
        info.dataPacket = packet;
        info.prevSampleIndex = 0;
    }

    if (!packetToReturn.assigned() && (dataDescriptor.assigned() || domainDescriptor.assigned()))
        packetToReturn = DataDescriptorChangedEventPacket(dataDescriptor, domainDescriptor);

    if (packetToReturn.assigned())
    {
        bool firstData {false};
        handlePacket(packetToReturn, firstData);
    }

    return packetToReturn;
}

bool SignalReader::sync(const Comparable& commonStart)
{
    if (synced == SyncStatus::Synchronized)
        return true;

    if (isFirstPacketEvent())
       return false;

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

            if (isFirstPacketEvent())
                return false;

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

    while (info.remainingToRead != 0)
    {
        PacketPtr packet = info.dataPacket;
        if (!packet.assigned())
        {
            packet = connection.dequeue();
        }

        if (packet.assigned())
            errCode = handlePacket(packet, firstData);
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

    if (info.values != nullptr)
    {
        ErrCode errCode = valueReader->readData(getValuePacketData(info.dataPacket), info.prevSampleIndex, &info.values, toRead);
        if (OPENDAQ_FAILED(errCode))
        {
            return errCode;
        }
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
        ErrCode errCode = domainReader->readData(domainPacket.getData(), info.prevSampleIndex, &info.domainValues, toRead);
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
    if (!domainReader->handleDescriptorChanged(dataDescriptor, readMode))
    {
        daqSetErrorInfo(errInfo);
        return false;
    }
    return true;
}

END_NAMESPACE_OPENDAQ
