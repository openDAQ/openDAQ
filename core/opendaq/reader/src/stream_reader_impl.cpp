#include <opendaq/stream_reader_impl.h>
#include <opendaq/data_descriptor_ptr.h>
#include <opendaq/event_packet_ids.h>
#include <opendaq/event_packet_params.h>
#include <opendaq/packet_factory.h>
#include <opendaq/reader_errors.h>

#include <coretypes/validation.h>
#include <coretypes/function.h>

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
    changeCallback = readerConfig.getOnDescriptorChanged();

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
{
    std::scoped_lock lock(old->mutex);
    old->invalid = true;

    info = old->info;
    timeoutType = old->timeoutType;

    valueReader = createReaderForType(valueReadType, old->valueReader->getTransformFunction());
    domainReader = createReaderForType(domainReadType, old->domainReader->getTransformFunction());

    inputPort = old->inputPort;
    connection = inputPort.getConnection();
    changeCallback = old->changeCallback;

    this->internalAddRef();
    readDescriptorFromPort();
}

StreamReaderImpl::~StreamReaderImpl()
{
    if (inputPort.assigned())
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

    const auto signal = inputPort.getSignal();
    if (!signal.assigned())
    {
        throw InvalidStateException("Input port must already have a signal assigned");
    }

    const auto descriptor = signal.getDescriptor();
    if (!descriptor.assigned())
    {
        throw InvalidStateException("Input port connected signal must have a descriptor assigned.");
    }
    handleDescriptorChanged(DataDescriptorChangedEventPacket(descriptor, nullptr));
}

void StreamReaderImpl::connectSignal(const SignalPtr& signal)
{
    inputPort = InputPort(signal.getContext(), nullptr, "readsig");
    inputPort.setListener(this->thisPtr<InputPortNotificationsPtr>());
    inputPort.setNotificationMethod(PacketReadyNotification::SameThread);

    inputPort.connect(signal);
    connection = inputPort.getConnection();

    handleDescriptorChanged(connection.dequeue());
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

    return OPENDAQ_SUCCESS;
}

ErrCode StreamReaderImpl::disconnected(IInputPort* port)
{
    OPENDAQ_PARAM_NOT_NULL(port);

    return OPENDAQ_SUCCESS;
}

ErrCode StreamReaderImpl::packetReceived(IInputPort* port)
{
    OPENDAQ_PARAM_NOT_NULL(port);

    onPacketReady();
    return OPENDAQ_SUCCESS;
}

void StreamReaderImpl::onPacketReady()
{
    notify.condition.notify_one();
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
            *count = info.dataPacket.getSampleCount() - info.prevSampleIndex;
        }

        *count += connection.getAvailableSamples();
    });
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

void StreamReaderImpl::handleDescriptorChanged(const EventPacketPtr& eventPacket)
{
    if (!eventPacket.assigned())
        return;


    auto params = eventPacket.getParameters();
    DataDescriptorPtr newValueDescriptor = params[event_packet_param::DATA_DESCRIPTOR];
    DataDescriptorPtr newDomainDescriptor = params[event_packet_param::DOMAIN_DATA_DESCRIPTOR];

    // Check if value is stil convertible
    if (newValueDescriptor.assigned())
    {
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

    // Check if domain is stil convertible
    if (newDomainDescriptor.assigned())
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

ErrCode StreamReaderImpl::readPacketData()
{
    auto remainingSampleCount = info.dataPacket.getSampleCount() - info.prevSampleIndex;
    SizeT toRead = std::min(info.remainingToRead, remainingSampleCount);

    ErrCode errCode = valueReader->readData(info.dataPacket.getData(), info.prevSampleIndex, &info.values, toRead);
    if (OPENDAQ_FAILED(errCode))
    {
        return errCode;
    }

    if (info.domainValues != nullptr)
    {
        auto dataPacket = info.dataPacket;
        if (!dataPacket.getDomainPacket().assigned())
        {
            return makeErrorInfo(OPENDAQ_ERR_INVALIDSTATE, "Packets must have an associated domain packets to read domain data.");
        }

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
        std::unique_lock lock(notify.mutex);
        info.reset();
    }

    info.remainingToRead -= toRead;
    return OPENDAQ_SUCCESS;
}

ErrCode StreamReaderImpl::readPackets()
{
    bool firstData = false;
    ErrCode errCode = OPENDAQ_SUCCESS;

    ReadInfo::Duration remainingTime = info.timeout;
    while (info.remainingToRead > 0 && remainingTime.count() >= 0)
    {
        PacketPtr packet;
        {
            std::unique_lock lock(notify.mutex);

            packet = connection.dequeue();
        }

        if (!packet.assigned())
        {
            // Don't wait for any data if we already read some
            if (timeoutType == ReadTimeoutType::Any && firstData)
            {
                break; 
            }

            std::unique_lock notifyLock(notify.mutex);
            if (notify.condition.wait_for(notifyLock, remainingTime, [this]
            {
                return connection.peek().assigned();
            }))
            {
                packet = connection.dequeue();
            }
            else
            {
                break;
            }
        }

        assert(packet.assigned());
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
                    errCode = wrapHandler(this, &StreamReaderImpl::handleDescriptorChanged, eventPacket);
                    if (OPENDAQ_FAILED(errCode))
                    {
                        invalid = true;

                        return this->makeErrorInfo(
                            OPENDAQ_ERR_INVALID_DATA,
                            "Exception occurred while processing a signal descriptor change"
                        );
                    }

                    if (invalid)
                    {
                        return this->makeErrorInfo(
                            OPENDAQ_ERR_INVALID_DATA,
                            "Packet samples are no longer convertible to the read type"
                        );
                    }
                }
                break;
            }
            case PacketType::None:
                break;
        }

        if (info.timeout.count() != 0)
            remainingTime = info.timeout - info.durationFromStart();
    }

    return errCode;
}

ErrCode StreamReaderImpl::read(void* samples, SizeT* count, SizeT timeoutMs)
{
    OPENDAQ_PARAM_NOT_NULL(samples);
    OPENDAQ_PARAM_NOT_NULL(count);

    std::scoped_lock lock(mutex);

    if (invalid)
        return makeErrorInfo(OPENDAQ_ERR_INVALID_DATA, "Packet samples are no longer convertible to the read type", nullptr);

    ErrCode errCode = OPENDAQ_SUCCESS;

    info.prepare(samples, *count, milliseconds(timeoutMs));
    if (info.dataPacket.assigned())
    {
        errCode = readPacketData();
    }

    bool shouldReturnEarly = timeoutType == ReadTimeoutType::Any
                             && info.remainingToRead != *count;

    if (OPENDAQ_SUCCEEDED(errCode) && info.remainingToRead <= *count && !shouldReturnEarly)
        errCode = readPackets();

    *count = *count - info.remainingToRead;
    return errCode;
}

ErrCode StreamReaderImpl::readWithDomain(void* samples,
                                         void* domain,
                                         SizeT* count,
                                         SizeT timeoutMs)
{
    OPENDAQ_PARAM_NOT_NULL(samples);
    OPENDAQ_PARAM_NOT_NULL(domain);
    OPENDAQ_PARAM_NOT_NULL(count);

    std::scoped_lock lock(mutex);

    if (invalid)
        return makeErrorInfo(OPENDAQ_ERR_INVALID_DATA, "Packet samples are no longer convertible to the read type.", nullptr);

    ErrCode errCode = OPENDAQ_SUCCESS;

    info.prepareWithDomain(samples, domain, *count, milliseconds(timeoutMs));
    if (info.dataPacket.assigned())
    {
        errCode = readPacketData();
    }

    bool shouldReturnEarly = timeoutType == ReadTimeoutType::Any
                             && info.remainingToRead != *count;

    if (OPENDAQ_SUCCEEDED(errCode) && info.remainingToRead <= *count && !shouldReturnEarly)
        errCode = readPackets();

    *count = *count - info.remainingToRead;
    return errCode;
}

ErrCode StreamReaderImpl::getOnDescriptorChanged(IFunction** callback)
{
    std::scoped_lock lock(mutex);

    *callback = changeCallback.addRefAndReturn();
    return OPENDAQ_SUCCESS;
}

ErrCode StreamReaderImpl::setOnDescriptorChanged(IFunction* callback)
{
    std::scoped_lock lock(mutex);

    changeCallback = callback;
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

OPENDAQ_DEFINE_CUSTOM_CLASS_FACTORY_WITH_INTERFACE_AND_CREATEFUNC_OBJ(
    LIBRARY_FACTORY, IStreamReader, createStreamReaderFromExisting,
    IStreamReader*, invalidatedReader,
    SampleType, valueReadType,
    SampleType, domainReadType
)

END_NAMESPACE_OPENDAQ
