#include <opendaq/custom_log.h>
#include <opendaq/data_descriptor_factory.h>
#include <opendaq/event_packet_ids.h>
#include <opendaq/event_packet_utils.h>
#include <opendaq/packet_factory.h>
#include <opendaq/reader_errors.h>
#include <opendaq/reader_factory.h>
#include <opendaq/signal_reader.h>

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
    if (connection.assigned())
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
    if (connection.assigned())
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
            ? connection.getSamplesUntilNextGapPacket()
            : connection.getSamplesUntilNextEventPacket();
    }
    return count * sampleRateDivider;
}

[[maybe_unused]]
static std::string printSync(SyncStatus synced)
{
    switch (synced)
    {
        case SyncStatus::SynchronizationFailed:
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

    auto [valueDescriptorChanged, domainDescriptorChanged, newValueDescriptor, newDomainDescriptor] =
        parseDataDescriptorEventPacket(eventPacket);

    if (valueDescriptorChanged && newValueDescriptor.assigned() && valueReader->getReadType() == SampleType::Undefined)
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
    
    if (valueDescriptorChanged)
    {
        invalid = !valueReader->handleDescriptorChanged(newValueDescriptor, readMode);
    }
    if (domainDescriptorChanged)
    {
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

            packetDelta = 0;
            const auto domainRule = newDomainDescriptor.getRule();
            if (domainRule.getType() == DataRuleType::Linear)
            {
                const auto domainRuleParams = domainRule.getParameters();
                packetDelta = domainRuleParams.get("delta");
            }
        }

        invalid = invalid || !validDomain;
    }

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
        DAQ_THROW_EXCEPTION(InvalidStateException, "Packet must have a domain packet assigned!");
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
            info.dataPacket = connection.dequeue();
            info.prevSampleIndex = 0;
            return false;
        }

        if (packet.getType() == PacketType::Event)
        {
            return true;
        }

        // For undefined packet types
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
    bool valueDescriptorChanged = false;
    bool domainDescriptorChanged = false;

    PacketPtr packet;
    while (true)
    {
        packet = connection.peek();
        if (!packet.assigned())
        {
            break;
        }

        if (packet.getType() == PacketType::Data)
        {
            connection.dequeue();
            break;
        }

        if (packet.getType() == PacketType::Event)
        {
            auto eventPacket = packet.asPtr<IEventPacket>(true);
            auto packetId = eventPacket.getEventId();
            if (packetId == event_packet_id::DATA_DESCRIPTOR_CHANGED)
            {
                const auto [valueDescChanged, domainDescChanged, newValueDescriptor, newDomainDescriptor] =
                    parseDataDescriptorEventPacket(eventPacket);

                valueDescriptorChanged |= static_cast<bool>(valueDescChanged);
                domainDescriptorChanged |= static_cast<bool>(domainDescChanged);

                if (valueDescChanged)
                    dataDescriptor = newValueDescriptor;
                if (domainDescChanged)
                    domainDescriptor = newDomainDescriptor;
            }
            else if (packetId == event_packet_id::IMPLICIT_DOMAIN_GAP_DETECTED)
            {
                if (!dataDescriptor.assigned() && !domainDescriptor.assigned())
                {
                    connection.dequeue();
                    packetToReturn = packet;
                }
                break;
            }
        }
        connection.dequeue();
    }

    if (packet.assigned() && packet.getType() == PacketType::Data)
    {
        info.dataPacket = packet;
        info.prevSampleIndex = 0;
    }

    if (!packetToReturn.assigned() && (valueDescriptorChanged || domainDescriptorChanged))
    {
        const auto valueDescriptorParam = valueDescriptorChanged
                                              ? descriptorToEventPacketParam(dataDescriptor)
                                              : nullptr;
        const auto domainDescriptorParam = domainDescriptorChanged
                                               ? descriptorToEventPacketParam(domainDescriptor)
                                               : nullptr;
        packetToReturn = DataDescriptorChangedEventPacket(valueDescriptorParam, domainDescriptorParam);
    }

    if (packetToReturn.assigned())
    {
        synced = SyncStatus::Unsynchronized;
        bool firstData {false};
        handlePacket(packetToReturn, firstData);
    }

    return packetToReturn;
}

bool SignalReader::skipUntilLastEventPacket()
{
    info.reset();

    if (!connection.assigned())
        return false;

    bool hasEventPacket = false;
    while (connection.hasEventPacket())
    {
        auto packet = connection.peek();
        if (packet.getType() == PacketType::Event)
        {
            auto eventPacket = packet.asPtr<IEventPacket>(true);
            if (eventPacket.getEventId() == event_packet_id::IMPLICIT_DOMAIN_GAP_DETECTED)
            {
                connection.dequeue();
            }
            else
            {
                hasEventPacket = true;
                break;
            }
        }
        else
        {
            connection.dequeue();
        }
    }

    if (!hasEventPacket)
        connection.dequeueAll();

    return hasEventPacket;
}

bool SignalReader::sync(const Comparable& commonStart, std::chrono::system_clock::rep* firstSampleAbsoluteTimestamp)
{
    if (synced == SyncStatus::Synchronized)
    {
        if (firstSampleAbsoluteTimestamp)
            *firstSampleAbsoluteTimestamp = cachedFirstTimestamp;

        return true;
    }

    if (isFirstPacketEvent())
    {
        return false;
    }

    SizeT startPackets = info.prevSampleIndex;
    Int droppedPackets = 0;

    while (info.dataPacket.assigned())
    {
        auto domainPacket = info.dataPacket.getDomainPacket();
        info.prevSampleIndex = domainReader->getOffsetTo(
            domainInfo,
            commonStart,
            domainPacket.getData(),
            domainPacket.getSampleCount(),
            &cachedFirstTimestamp
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
            if (firstSampleAbsoluteTimestamp)
                *firstSampleAbsoluteTimestamp = cachedFirstTimestamp;
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

                    return DAQ_MAKE_ERROR_INFO(
                        OPENDAQ_ERR_INVALID_DATA,
                        "Exception occurred while processing a signal descriptor change"
                    );
                }

                if (invalid)
                {
                    return DAQ_MAKE_ERROR_INFO(
                        OPENDAQ_ERR_INVALID_DATA,
                        "Signal no longer compatible with the reader or other signals"
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

    DAQ_THROW_EXCEPTION(InvalidOperationException, "Unknown Reader read-mode of {}", static_cast<std::underlying_type_t<ReadMode>>(readMode));
}

bool SignalReader::isSynced() const
{
    return synced == SyncStatus::Synchronized;
}

ErrCode SignalReader::readPacketData()
{
    auto remainingSampleCount = info.dataPacket.getSampleCount() - info.prevSampleIndex;
    SizeT toRead = std::min(info.remainingToRead, remainingSampleCount);

    if (info.values != nullptr)
    {
        ErrCode errCode = valueReader->readData(getValuePacketData(info.dataPacket), info.prevSampleIndex, &info.values, toRead);
        OPENDAQ_RETURN_IF_FAILED(errCode);
    }

    if (info.domainValues != nullptr)
    {
        auto dataPacket = info.dataPacket;
        if (!dataPacket.getDomainPacket().assigned())
        {
            return DAQ_MAKE_ERROR_INFO(
                OPENDAQ_ERR_INVALIDSTATE,
                "Packets must have an associated domain packets to read domain data."
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
            daqClearErrorInfo();
            errCode = domainReader->readData(domainPacket.getData(), info.prevSampleIndex, &info.domainValues, toRead);
        }

        LOG_T("]");

        OPENDAQ_RETURN_IF_FAILED(errCode);
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
