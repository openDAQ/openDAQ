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
                           const LoggerComponentPtr& logger,
                           bool globalIdFromSignal)
    : loggerComponent(logger)
    , port(port)
    , connection(port.getConnection())
    , readMode(mode)
    , domainInfo(logger)
    , sampleRate(-1)
    , commonSampleRate(-1)
    , globalIdFromSignal(globalIdFromSignal)
{
    trContext.domainIn = SampleType::Undefined;
    trContext.domainOut = domainReadType;
    trContext.valueIn = SampleType::Undefined;
    trContext.valueOut = mode == ReadMode::RawValue ? SampleType::Undefined : valueReadType;
}

SignalReader::SignalReader(const SignalReader& old,
                           const InputPortNotificationsPtr& listener,
                           SampleType valueReadType,
                           SampleType domainReadType)
    : loggerComponent(old.loggerComponent)
    , port(old.port)
    , connection(port.getConnection())
    , readMode(old.readMode)
    , domainInfo(loggerComponent)
    , sampleRate(-1)
    , commonSampleRate(-1)
    , unused(old.unused)
    , globalIdFromSignal(old.globalIdFromSignal)
    , trContext{} // In-out types might change
{
    // Preserve transform from the old reader
    trContext.valueTransform = old.trContext.valueTransform;
    trContext.domainTransform = old.trContext.domainTransform;
    // Type-related state gets set as in new
    trContext.domainIn = SampleType::Undefined;
    trContext.domainOut = domainReadType;
    trContext.valueIn = SampleType::Undefined;
    trContext.valueOut = readMode == ReadMode::RawValue ? SampleType::Undefined : valueReadType;

    info = old.info;

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
            try
            {
                handleDescriptorChanged(connection.dequeue());
            }
            catch (const std::exception& e)
            {
                invalid = true;
                LOG_D("Failed to handle descriptor read from port: {}", e.what())
                (void) e;
            }
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
        count += acrossDescriptorChanges ? connection.getSamplesUntilNextGapPacket() : connection.getSamplesUntilNextEventPacket();
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

    if (valueDescriptorChanged)
    {
        if (newValueDescriptor.assigned() && trContext.valueOut == SampleType::Undefined)
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
    
            trContext.valueOut = valueType;
        }

        invalid = !onValueDescriptorUpdate(newValueDescriptor);
    }
    if (domainDescriptorChanged)
    {
        auto validDomain = onDomainDescriptorUpdate(newDomainDescriptor);
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

            try
            {
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
            catch (const std::exception& e)
            {
                LOG_D("Failed to change descriptor: {}", e.what())
                validDomain = false;
                (void) e;
            }
        }

        invalid = invalid || !validDomain;
    }

    LOG_T("[Signal Descriptor Changed: {} | {} | {}]", port.getSignal().getLocalId(), printSync(synced), invalid ? "Invalid" : "Valid")
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

    domainInfo.adjustToCommonEpochResolution(minEpoch, maxResolution);
    synced = SyncStatus::Unsynchronized;
}

std::unique_ptr<DomainValue> SignalReader::readDomainStart()
{
    DataPacketPtr domainPacket = info.dataPacket.getDomainPacket();
    if (!domainPacket.assigned())
    {
        DAQ_THROW_EXCEPTION(InvalidStateException, "Packet must have a domain packet assigned!");
    }

    return TypedReadingUtils::readDomainValue(trContext.domainIn,
                                              trContext.domainOut,
                                              trContext.domainLayout,
                                              domainPacket,
                                              info.prevSampleIndex,
                                              trContext.domainInfo);
}

const DomainInfo& SignalReader::getDomainInfo() const
{
    return trContext.domainInfo;
}

SampleType SignalReader::getValueReadType() const
{
    return trContext.valueOut;
}

void SignalReader::setValueTransformFunction(FunctionPtr transform)
{
    trContext.valueTransform = std::move(transform);
}

void SignalReader::setDomainTransformFunction(FunctionPtr transform)
{
    trContext.domainTransform = std::move(transform);
}

FunctionPtr SignalReader::getValueTransformFunction() const
{
    return trContext.valueTransform;
}

FunctionPtr SignalReader::getDomainTransformFunction() const
{
    return trContext.domainTransform;
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
        const auto valueDescriptorParam = valueDescriptorChanged ? descriptorToEventPacketParam(dataDescriptor) : nullptr;
        const auto domainDescriptorParam = domainDescriptorChanged ? descriptorToEventPacketParam(domainDescriptor) : nullptr;
        packetToReturn = DataDescriptorChangedEventPacket(valueDescriptorParam, domainDescriptorParam);
    }

    if (packetToReturn.assigned())
    {
        synced = SyncStatus::Unsynchronized;
        bool firstData{false};
        const ErrCode errCode = handlePacket(packetToReturn, firstData);
        if (OPENDAQ_FAILED(errCode))
            daqClearErrorInfo();
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

bool SignalReader::sync(const DomainValue* commonStart, std::chrono::system_clock::rep* firstSampleAbsoluteTimestamp)
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

    SizeT startSamples = info.prevSampleIndex;
    [[maybe_unused]] Int droppedSamples = 0;

    while (info.dataPacket.assigned())
    {
        auto domainPacket = info.dataPacket.getDomainPacket();
        // Check if commonStart can be reached within the current packet
        info.prevSampleIndex = TypedReadingUtils::findDomainValue(trContext.domainIn,
                                                                  trContext.domainOut,
                                                                  trContext.domainLayout,
                                                                  domainPacket,
                                                                  commonStart,
                                                                  &cachedFirstTimestamp);

        if (info.prevSampleIndex == static_cast<SizeT>(-1))
        {
            // commonStart is outside the packet.
            // Drop the entire packet (startSamples have already been used).
            droppedSamples += static_cast<Int>(domainPacket.getSampleCount() - startSamples);

            info.dataPacket = nullptr;

            // Dequeue a Data packet into info.dataPacket
            if (isFirstPacketEvent())
                // Encountered an Event packet before arriving at common start
                return false;

            startSamples = 0;
        }
        else
        {
            droppedSamples += static_cast<Int>(info.prevSampleIndex - startSamples);
            if (firstSampleAbsoluteTimestamp)
                *firstSampleAbsoluteTimestamp = cachedFirstTimestamp;
            break;
        }
    }

    synced = info.prevSampleIndex != static_cast<SizeT>(-1) ? SyncStatus::Synchronized : SyncStatus::Synchronizing;

    LOG_T("[Syncing: {} | {}, dropped {} samples]", port.getSignal().getLocalId(), printSync(synced), droppedSamples);
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
                    errCode = DAQ_MAKE_ERROR_INFO(OPENDAQ_ERR_GENERALERROR, "Failed to handle descriptor change");
                }

                if (OPENDAQ_FAILED(errCode))
                {
                    invalid = true;

                    return DAQ_EXTEND_ERROR_INFO(
                        errCode, OPENDAQ_ERR_INVALID_DATA, "Exception occurred while processing a signal descriptor change");
                }

                if (invalid)
                {
                    return DAQ_MAKE_ERROR_INFO(OPENDAQ_ERR_INVALID_DATA, "Signal no longer compatible with the reader or other signals");
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
    if (unused)
        return OPENDAQ_SUCCESS;

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
        {
            errCode = handlePacket(packet, firstData);
            OPENDAQ_RETURN_IF_FAILED(errCode);
        }
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

    DAQ_THROW_EXCEPTION(
        InvalidOperationException, "Unknown Reader read-mode of {}", static_cast<std::underlying_type_t<ReadMode>>(readMode));
}

bool SignalReader::isSynced() const
{
    return synced == SyncStatus::Synchronized;
}

StringPtr SignalReader::getComponentGlobalId() const
{
    if (globalIdFromSignal)
        return port.getSignal().getGlobalId();
    return port.getGlobalId();
}

bool SignalReader::isConnected() const
{
    return port.getConnection().assigned();
}

ErrCode SignalReader::readPacketData()
{
    auto remainingSampleCount = info.dataPacket.getSampleCount() - info.prevSampleIndex;
    SizeT toRead = std::min(info.remainingToRead, remainingSampleCount);

    if (info.values != nullptr)
    {
        ErrCode errCode = TypedReadingUtils::readData(trContext.valueIn,
                                              trContext.valueOut,
                                              false,
                                              trContext.valueLayout,
                                              getValuePacketData(info.dataPacket),
                                              info.prevSampleIndex,
                                              &info.values,
                                              toRead,
                                              trContext.valueTransform);
        OPENDAQ_RETURN_IF_FAILED(errCode);
    }

    if (info.domainValues != nullptr)
    {
        auto dataPacket = info.dataPacket;
        if (!dataPacket.getDomainPacket().assigned())
        {
            return DAQ_MAKE_ERROR_INFO(OPENDAQ_ERR_INVALIDSTATE, "Packets must have an associated domain packets to read domain data.");
        }

        LOG_T("[Reading: {} ", port.getSignal().getLocalId());

        auto domainPacket = dataPacket.getDomainPacket();
        ErrCode errCode = TypedReadingUtils::readData(trContext.domainIn,
                                                      trContext.domainOut,
                                                      true,
                                                      trContext.domainLayout,
                                                      domainPacket.getData(),
                                                      info.prevSampleIndex,
                                                      &info.domainValues,
                                                      toRead,
                                                      trContext.domainTransform);
        
        if (errCode == OPENDAQ_ERR_INVALIDSTATE)
        {
            if (!trySetDomainSampleType(domainPacket))
                return DAQ_EXTEND_ERROR_INFO(errCode, "Failed to set domain sample type for packet");
            daqClearErrorInfo();
            errCode = TypedReadingUtils::readData(trContext.domainIn,
                                                  trContext.domainOut,
                                                  true,
                                                  trContext.domainLayout,
                                                  domainPacket.getData(),
                                                  info.prevSampleIndex,
                                                  &info.domainValues,
                                                  toRead,
                                                  trContext.domainTransform);
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

bool SignalReader::trySetDomainSampleType(const daq::DataPacketPtr& domainPacket)
{
    ObjectPtr<IErrorInfo> errorInfo;
    daqGetErrorInfo(&errorInfo);
    daqClearErrorInfo();

    auto dataDescriptor = domainPacket.getDataDescriptor();
    if (onDomainDescriptorUpdate(dataDescriptor))
        return true;

    daqSetErrorInfo(errorInfo);
    return false;
}

bool SignalReader::onValueDescriptorUpdate(const DataDescriptorPtr& valueDescriptor)
{
    if (!valueDescriptor.assigned())
        return false;

    const auto postScaling = valueDescriptor.getPostScaling();
    if (!postScaling.assigned() || readMode == ReadMode::Scaled)
    {
        trContext.valueIn = valueDescriptor.getSampleType();
    }
    else
    {
        trContext.valueIn = postScaling.getInputSampleType();
    }

    trContext.valueLayout.rawSampleSize = valueDescriptor.getRawSampleSize();
    auto dimensions = valueDescriptor.getDimensions();
    if (dimensions.assigned() && dimensions.getCount() == 1)
    {
        trContext.valueLayout.valuesPerSample = dimensions[0].getSize();
    }

    trContext.valueLayout.descriptor = valueDescriptor;

    return TypedReadingUtils::isSampleTypeConvertible(trContext.valueIn, trContext.valueOut, false);
}

bool SignalReader::onDomainDescriptorUpdate(const DataDescriptorPtr& domainDescriptor)
{
    if (!domainDescriptor.assigned())
        return false;

    const auto postScaling = domainDescriptor.getPostScaling();
    if (!postScaling.assigned() || readMode == ReadMode::Scaled)
    {
        trContext.domainIn = domainDescriptor.getSampleType();
    }
    else
    {
        trContext.domainIn = postScaling.getInputSampleType();
    }

    trContext.domainLayout.rawSampleSize = domainDescriptor.getRawSampleSize();
    auto dimensions = domainDescriptor.getDimensions();
    if (dimensions.assigned() && dimensions.getCount() == 1)
    {
        trContext.domainLayout.valuesPerSample = dimensions[0].getSize();
    }

    trContext.domainLayout.descriptor = domainDescriptor;

    trContext.domainInfo = DomainInfo::fromDescriptor(domainDescriptor);

    return TypedReadingUtils::isSampleTypeConvertible(trContext.domainIn, trContext.domainOut, true);
}


END_NAMESPACE_OPENDAQ
