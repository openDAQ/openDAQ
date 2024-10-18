#include <opendaq/stream_reader_impl.h>

#include <coreobjects/ownable_ptr.h>
#include <coreobjects/property_object_factory.h>
#include <opendaq/data_descriptor_ptr.h>
#include <opendaq/event_packet_ids.h>
#include <opendaq/event_packet_params.h>
#include <opendaq/packet_factory.h>
#include <opendaq/reader_errors.h>
#include <opendaq/data_descriptor_factory.h>

#include <coretypes/function.h>
#include <coretypes/validation.h>

using namespace std::chrono;
using namespace std::chrono_literals;

BEGIN_NAMESPACE_OPENDAQ

StreamReaderImpl::StreamReaderImpl(const SignalPtr& signal,
                                   SampleType valueReadType,
                                   SampleType domainReadType,
                                   ReadMode mode,
                                   ReadTimeoutType timeoutType)
    : readMode(mode)
    , timeoutType(timeoutType)
{
    if (!signal.assigned())
        throw ArgumentNullException("Signal must not be null");

    valueReader = createReaderForType(valueReadType, nullptr);
    domainReader = createReaderForType(domainReadType, nullptr);

    this->internalAddRef();
    connectSignal(signal);
}

StreamReaderImpl::StreamReaderImpl(IInputPortConfig* port,
                                   SampleType valueReadType,
                                   SampleType domainReadType,
                                   ReadMode mode,
                                   ReadTimeoutType timeoutType)
    : readMode(mode)
    , timeoutType(timeoutType)
{
    if (!port)
        throw ArgumentNullException("Input port must not be null.");

    valueReader = createReaderForType(valueReadType, nullptr);
    domainReader = createReaderForType(domainReadType, nullptr);

    this->internalAddRef();

    connectInputPort(port);
}

StreamReaderImpl::StreamReaderImpl(const ReaderConfigPtr& readerConfig,
                                   SampleType valueReadType,
                                   SampleType domainReadType,
                                   ReadMode mode)
    : readMode(mode)
{
    if (!readerConfig.assigned())
        throw ArgumentNullException("Existing reader must not be null");

    readerConfig.markAsInvalid();

    timeoutType = readerConfig.getReadTimeoutType();
    inputPort = readerConfig.getInputPorts()[0];

    valueReader = createReaderForType(valueReadType, readerConfig.getValueTransformFunction());
    domainReader = createReaderForType(domainReadType, readerConfig.getDomainTransformFunction());

    connection = inputPort.getConnection();

    this->internalAddRef();
    readDescriptorFromPort();
}

StreamReaderImpl::StreamReaderImpl(StreamReaderImpl* old,
                                   SampleType valueReadType,
                                   SampleType domainReadType)
    : readMode(old->readMode)
    , timeoutType(ReadTimeoutType::All)
    , skipEvents(old->skipEvents)
{
    std::scoped_lock lock(old->mutex);
    dataDescriptor = old->dataDescriptor;
    domainDescriptor = old->domainDescriptor;
    old->invalid = true;

    info = old->info;
    timeoutType = old->timeoutType;

    valueReader = createReaderForType(valueReadType, old->valueReader->getTransformFunction());
    domainReader = createReaderForType(domainReadType, old->domainReader->getTransformFunction());

    old->portBinder = PropertyObject();
    inputPort = old->inputPort;
    inputPort.asPtr<IOwnable>().setOwner(portBinder);

    connection = inputPort.getConnection();
    readCallback = old->readCallback;

    this->internalAddRef();
    inputPort.setListener(this->template thisPtr<InputPortNotificationsPtr>());
    handleDescriptorChanged(createInitDataDescriptorChangedEventPacket());
}

StreamReaderImpl::StreamReaderImpl(const StreamReaderBuilderPtr& builder)
{
    if (!builder.assigned())
        throw ArgumentNullException("Builder must not be null");

    if ((builder.getValueReadType() == SampleType::Undefined || builder.getDomainReadType() == SampleType::Undefined) &&
        builder.getSkipEvents())
    {
        throw InvalidParameterException("Reader cannot skip events when sample type is undefined");
    }

    readMode = builder.getReadMode();
    timeoutType = builder.getReadTimeoutType();
    skipEvents = builder.getSkipEvents();

    valueReader = createReaderForType(builder.getValueReadType(), nullptr);
    domainReader = createReaderForType(builder.getDomainReadType(), nullptr);

    this->internalAddRef();

    if (auto port = builder.getInputPort(); port.assigned())
    {
        if (port.getConnection().assigned())
            throw InvalidParameterException("Signal has to be connected to port after reader is created");

        connectInputPort(port);
    }
    else if (auto signal = builder.getSignal(); signal.assigned())
    {
        connectSignal(builder.getSignal());
    }
    else 
    {
        throw ArgumentNullException("Signal or port must be set");
    }
}

StreamReaderImpl::~StreamReaderImpl()
{
    if (inputPort.assigned() && !portBinder.assigned())
        inputPort.remove();
}

void StreamReaderImpl::readDescriptorFromPort()
{
    auto config = inputPort.asPtrOrNull<IInputPortConfig>();
    if (config.assigned())
    {
        config.setListener(this->thisPtr<InputPortNotificationsPtr>());
    }

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

    handleDescriptorChanged(createInitDataDescriptorChangedEventPacket());
}

void StreamReaderImpl::connectSignal(const SignalPtr& signal)
{
    inputPort = InputPort(signal.getContext(), nullptr, "readsig", true);
    inputPort.setListener(this->thisPtr<InputPortNotificationsPtr>());
    inputPort.setNotificationMethod(PacketReadyNotification::SameThread);

    inputPort.connect(signal);
}

void StreamReaderImpl::connectInputPort(const InputPortConfigPtr& port)
{
    inputPort = port;
    if (inputPort.getConnection().assigned())
        throw InvalidParameterException("Signal has to be connected to port after reader is created");

    portBinder = PropertyObject();
    inputPort.asPtr<IOwnable>().setOwner(portBinder);

    inputPort.setListener(this->thisPtr<InputPortNotificationsPtr>());
    inputPort.setNotificationMethod(PacketReadyNotification::Scheduler);
}

ErrCode StreamReaderImpl::acceptsSignal(IInputPort* port, ISignal* signal, Bool* accept)
{
    OPENDAQ_PARAM_NOT_NULL(port);
    OPENDAQ_PARAM_NOT_NULL(signal);
    OPENDAQ_PARAM_NOT_NULL(accept);

    *accept = True;
    return OPENDAQ_SUCCESS;
}

ErrCode StreamReaderImpl::connected(IInputPort* port)
{
    OPENDAQ_PARAM_NOT_NULL(port);

    std::scoped_lock lock(notify.mutex);
    port->getConnection(&connection);
    return OPENDAQ_SUCCESS;
}

ErrCode StreamReaderImpl::disconnected(IInputPort* port)
{
    OPENDAQ_PARAM_NOT_NULL(port);

    std::scoped_lock lock(notify.mutex);
    connection = nullptr;
    return OPENDAQ_SUCCESS;
}

ErrCode StreamReaderImpl::packetReceived(IInputPort* port)
{
    OPENDAQ_PARAM_NOT_NULL(port);
    ProcedurePtr callback;
    {
        std::scoped_lock lock(notify.mutex);
        callback = readCallback;
    }
    notify.condition.notify_one();
    if (callback.assigned())
        return wrapHandler(callback);
    return OPENDAQ_SUCCESS;
}

ErrCode StreamReaderImpl::getValueReadType(SampleType* sampleType)
{
    OPENDAQ_PARAM_NOT_NULL(sampleType);

    std::unique_lock lock(mutex);

    *sampleType = valueReader->getReadType();
    return OPENDAQ_SUCCESS;
}

ErrCode StreamReaderImpl::getDomainReadType(SampleType* sampleType)
{
    OPENDAQ_PARAM_NOT_NULL(sampleType);

    std::unique_lock lock(mutex);

    *sampleType = domainReader->getReadType();
    return OPENDAQ_SUCCESS;
}

ErrCode StreamReaderImpl::getAvailableCount(SizeT* count)
{
    OPENDAQ_PARAM_NOT_NULL(count);

    std::scoped_lock lock(this->mutex);

    return wrapHandler([count, this]
    {
        *count = 0;
        if (info.dataPacket.assigned())
        {
            *count += info.dataPacket.getSampleCount() - info.prevSampleIndex;
        }
        if (connection.assigned())
        {
            *count += skipEvents 
                ? connection.getSamplesUntilNextGapPacket()
                : connection.getSamplesUntilNextEventPacket();
        }
    });
}

ErrCode StreamReaderImpl::getEmpty(Bool* empty)
{
    OPENDAQ_PARAM_NOT_NULL(empty);

    std::scoped_lock lock(mutex);

    *empty = false;
    if (connection.assigned())
    {
        if (!skipEvents && connection.hasEventPacket())
        {
            return OPENDAQ_SUCCESS;
        }

        if (skipEvents && connection.hasGapPacket())
        {
            return OPENDAQ_SUCCESS;
        }
    }

    if (info.dataPacket.assigned())
    {
        if (info.dataPacket.getSampleCount() > info.prevSampleIndex)
        {
            return OPENDAQ_SUCCESS;
        }
    }
    if (connection.assigned())
    {
        if (connection.getAvailableSamples() > 0)
        {
            return OPENDAQ_SUCCESS;
        }
    }
    *empty = true;
    return OPENDAQ_SUCCESS;
}

ErrCode StreamReaderImpl::getInputPorts(IList** ports)
{
    OPENDAQ_PARAM_NOT_NULL(ports);

    *ports = List<IInputPortConfig>(inputPort).detach();
    return OPENDAQ_SUCCESS;
}

ErrCode StreamReaderImpl::getReadTimeoutType(ReadTimeoutType* timeout)
{
    OPENDAQ_PARAM_NOT_NULL(timeout);

    *timeout = timeoutType;
    return OPENDAQ_SUCCESS;
}

ErrCode StreamReaderImpl::markAsInvalid()
{
    std::unique_lock lock(mutex);
    invalid = true;

    return OPENDAQ_SUCCESS;
}

void StreamReaderImpl::inferReaderReadType(const DataDescriptorPtr& newDescriptor, std::unique_ptr<Reader>& reader) const
{
    reader = createReaderForType(newDescriptor.getSampleType(), reader->getTransformFunction());
}

EventPacketPtr StreamReaderImpl::createInitDataDescriptorChangedEventPacket()
{
    return DataDescriptorChangedEventPacket(dataDescriptor.assigned() ? dataDescriptor : NullDataDescriptor(),
                                            domainDescriptor.assigned() ? domainDescriptor : NullDataDescriptor());
}

void StreamReaderImpl::handleDescriptorChanged(const EventPacketPtr& eventPacket)
{
    if (!eventPacket.assigned())
        return;

    auto params = eventPacket.getParameters();
    DataDescriptorPtr valueDescriptorParam = params[event_packet_param::DATA_DESCRIPTOR];
    DataDescriptorPtr domainDescriptorParam = params[event_packet_param::DOMAIN_DATA_DESCRIPTOR];
    bool valueDescriptorChanged = valueDescriptorParam.assigned();
    bool domainDescriptorChanged = domainDescriptorParam.assigned();
    DataDescriptorPtr newValueDescriptor = valueDescriptorParam != NullDataDescriptor() ? valueDescriptorParam : nullptr;
    DataDescriptorPtr newDomainDescriptor = domainDescriptorParam != NullDataDescriptor() ? domainDescriptorParam : nullptr;

    // Check if value is still convertible
    if (valueDescriptorChanged && newValueDescriptor.assigned())
    {
        dataDescriptor = newValueDescriptor;
        if (valueReader->isUndefined())
        {
            inferReaderReadType(newValueDescriptor, valueReader);
        }

        auto valid = valueReader->handleDescriptorChanged(newValueDescriptor, readMode);
        if (!invalid)
        {
            invalid = !valid;
        }
    }

    // Check if domain is still convertible
    if (domainDescriptorChanged && newDomainDescriptor.assigned())
    {
        if (domainReader->isUndefined())
        {
            inferReaderReadType(newDomainDescriptor, domainReader);
        }

        auto valid = domainReader->handleDescriptorChanged(newDomainDescriptor, readMode);
        if (!invalid)
        {
            invalid = !valid;
        }
    }
}

bool StreamReaderImpl::trySetDomainSampleType(const daq::DataPacketPtr& domainPacket)
{
    ObjectPtr<IErrorInfo> errInfo;
    daqGetErrorInfo(&errInfo);
    daqClearErrorInfo();

    auto dataDescriptor = domainPacket.getDataDescriptor();
    if (domainReader->isUndefined())
    {
        inferReaderReadType(dataDescriptor, domainReader);
    }

    if (!domainReader->handleDescriptorChanged(dataDescriptor, readMode))
    {
        daqSetErrorInfo(errInfo);
        return false;
    }
    return true;
}

void* StreamReaderImpl::getValuePacketData(const DataPacketPtr& packet) const
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

ErrCode StreamReaderImpl::readPacketData()
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
            return makeErrorInfo(OPENDAQ_ERR_INVALIDSTATE, "Packets must have an associated domain packets to read domain data.");
        }

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

ReaderStatusPtr StreamReaderImpl::readPackets()
{
    std::unique_lock notifyLock(notify.mutex);
    if (info.timeout.count() > 0)
    {
        SizeT samplesToRead = info.remainingToRead;
        if (info.dataPacket.assigned())
        {
            samplesToRead -= info.dataPacket.getSampleCount() - info.prevSampleIndex;
        }

        auto condition = [this, samplesToRead]
        {
            if (!connection.assigned())
            {
                return false;
            }

            if (skipEvents)
            {
                if (connection.hasGapPacket())
                {
                    return true;
                }
            }
            else
            {
                if (connection.hasEventPacket())
                {
                    return true;
                }
            }

            auto samples = connection.getAvailableSamples();
            if (timeoutType == ReadTimeoutType::Any)
            {
                return samples != 0;
            }
            return samples >= samplesToRead;
        };
        notify.condition.wait_for(notifyLock, info.timeout, condition);
    }

    NumberPtr offset;
    while (true)
    {
        PacketPtr packet = info.dataPacket;
        if (!packet.assigned() && connection.assigned())
        {
            packet = connection.dequeue();
        }

        if (!packet.assigned())
        {
            break;
        }

        if (packet.getType() == PacketType::Event)
        {
            auto eventPacket = packet.asPtr<IEventPacket>(true);
            if (eventPacket.getEventId() == event_packet_id::DATA_DESCRIPTOR_CHANGED)
            {
                ErrCode errCode = wrapHandler(this, &StreamReaderImpl::handleDescriptorChanged, eventPacket);
                if (OPENDAQ_FAILED(errCode))
                {
                    invalid = true;
                }
            } 
            if (!skipEvents || invalid || eventPacket.getEventId() == event_packet_id::IMPLICIT_DOMAIN_GAP_DETECTED)
            {
                return ReaderStatus(eventPacket, !invalid, offset);
            }
        }

        if (packet.getType() == PacketType::Data)
        {
            info.dataPacket = packet;
            if (info.remainingToRead == 0)
            {
                break;
            }
            if (!offset.assigned())
            {
                const auto domainPacket = info.dataPacket.getDomainPacket();
                if (domainPacket.assigned() && domainPacket.getOffset().assigned())
                {
                    Int delta = 0;
                    const auto domainRule = domainPacket.getDataDescriptor().getRule();
                    if (domainRule.getType() == DataRuleType::Linear)
                    {
                        const auto domainRuleParams = domainRule.getParameters();
                        delta = domainRuleParams.get("delta");
                    }
                    offset = domainPacket.getOffset().getIntValue() + (info.prevSampleIndex * delta);
                }
                if (!offset.assigned())
                {
                    offset = 0;
                }
            }
            readPacketData();
        }
    }

    return ReaderStatus(nullptr, !invalid, offset);
}

ErrCode StreamReaderImpl::read(void* samples, SizeT* count, SizeT timeoutMs, IReaderStatus** status)
{
    OPENDAQ_PARAM_NOT_NULL(count);
    if (*count)
    {
        OPENDAQ_PARAM_NOT_NULL(samples);
    }

    std::scoped_lock lock(mutex);

    if (invalid)
    {
        if (status)
            *status = ReaderStatus(nullptr, !invalid).detach();
        *count = 0;
        return OPENDAQ_IGNORED;
    }

    info.prepare(samples, *count, milliseconds(timeoutMs));

    auto statusPtr = readPackets();

    if (status)
        *status = statusPtr.detach();

    *count = *count - info.remainingToRead;
    return OPENDAQ_SUCCESS;
}

ErrCode StreamReaderImpl::readWithDomain(void* samples,
                                         void* domain,
                                         SizeT* count,
                                         SizeT timeoutMs,
                                         IReaderStatus** status)
{
    OPENDAQ_PARAM_NOT_NULL(count);
    if (*count != 0)
    {
        OPENDAQ_PARAM_NOT_NULL(samples);
        OPENDAQ_PARAM_NOT_NULL(domain);
    }

    std::scoped_lock lock(mutex);

    if (invalid)
    {
        if (status)
            *status = ReaderStatus(nullptr, !invalid).detach();
        *count = 0;
        return OPENDAQ_IGNORED;
    }
    info.prepareWithDomain(samples, domain, *count, milliseconds(timeoutMs));

    auto statusPtr = readPackets();

    *count = *count - info.remainingToRead;

    if (status)
        *status = statusPtr.detach();
    return OPENDAQ_SUCCESS;
}

ErrCode StreamReaderImpl::skipSamples(SizeT* count, IReaderStatus** status)
{
    OPENDAQ_PARAM_NOT_NULL(count);

    std::scoped_lock lock(mutex);

    if (invalid)
    {
        if (status)
            *status = ReaderStatus(nullptr, !invalid).detach();

        *count = 0;
        return OPENDAQ_IGNORED;
    }

    info.prepare(nullptr, *count, milliseconds(0));

    auto statusPtr = readPackets();

    *count = *count - info.remainingToRead;

    if (status)
        *status = statusPtr.detach();
    return OPENDAQ_SUCCESS;
}

ErrCode StreamReaderImpl::setOnDataAvailable(IProcedure* callback)
{
    std::scoped_lock lock(mutex);

    readCallback = callback;
    return OPENDAQ_SUCCESS;
}

ErrCode StreamReaderImpl::getValueTransformFunction(IFunction** transform)
{
    std::scoped_lock lock(mutex);

    *transform = valueReader->getTransformFunction().addRefAndReturn();
    return OPENDAQ_SUCCESS;
}

ErrCode StreamReaderImpl::setValueTransformFunction(IFunction* transform)
{
    std::scoped_lock lock(mutex);

    valueReader->setTransformFunction(transform);
    return OPENDAQ_SUCCESS;
}

ErrCode StreamReaderImpl::getDomainTransformFunction(IFunction** transform)
{
    std::scoped_lock lock(mutex);

    *transform = domainReader->getTransformFunction().addRefAndReturn();
    return OPENDAQ_SUCCESS;
}

ErrCode StreamReaderImpl::setDomainTransformFunction(IFunction* transform)
{
    std::scoped_lock lock(mutex);

    domainReader->setTransformFunction(transform);
    return OPENDAQ_SUCCESS;
}

ErrCode StreamReaderImpl::getReadMode(ReadMode* mode)
{
    OPENDAQ_PARAM_NOT_NULL(mode);

    *mode = readMode;
    return OPENDAQ_SUCCESS;
}

template <>
struct ObjectCreator<IStreamReader>
{
    static ErrCode Create(IStreamReader** out,
                          IStreamReader* toCopy,
                          SampleType valueReadType,
                          SampleType domainReadType
                         ) noexcept
    {
        OPENDAQ_PARAM_NOT_NULL(out);

        if (toCopy == nullptr)
        {
            return makeErrorInfo(OPENDAQ_ERR_ARGUMENT_NULL, "Existing reader must not be null", nullptr);
        }

        ReadMode mode;
        toCopy->getReadMode(&mode);

        auto old = ReaderConfigPtr::Borrow(toCopy);
        auto impl = dynamic_cast<StreamReaderImpl*>(old.getObject());

        return impl != nullptr
            ? createObject<IStreamReader, StreamReaderImpl>(out, impl, valueReadType, domainReadType)
            : createObject<IStreamReader, StreamReaderImpl>(out, old, valueReadType, domainReadType, mode);
    }
};

OPENDAQ_DEFINE_CLASS_FACTORY(
    LIBRARY_FACTORY, StreamReader,
    ISignal*, signal,
    SampleType, valueReadType,
    SampleType, domainReadType,
    ReadMode, readMode,
    ReadTimeoutType, timeoutType
)

OPENDAQ_DEFINE_CLASS_FACTORY_WITH_INTERFACE_AND_CREATEFUNC(
    LIBRARY_FACTORY, StreamReader,
    IStreamReader, createStreamReaderFromPort,
    IInputPortConfig*, port,
    SampleType, valueReadType,
    SampleType, domainReadType,
    ReadMode, readMode,
    ReadTimeoutType, timeoutType
)

OPENDAQ_DEFINE_CUSTOM_CLASS_FACTORY_WITH_INTERFACE_AND_CREATEFUNC_OBJ(
    LIBRARY_FACTORY, IStreamReader, createStreamReaderFromExisting,
    IStreamReader*, invalidatedReader,
    SampleType, valueReadType,
    SampleType, domainReadType
)

extern "C"
daq::ErrCode PUBLIC_EXPORT createStreamReaderFromBuilder(IStreamReader** objTmp, IStreamReaderBuilder* builder)
{
    return daq::createObject<IStreamReader, StreamReaderImpl>(objTmp, builder);
}

END_NAMESPACE_OPENDAQ
