#include <native_streaming_protocol/server_session_handler.h>
#include <native_streaming_protocol/native_streaming_protocol_types.h>

#include <opendaq/custom_log.h>

#include <coretypes/json_serializer_factory.h>

BEGIN_NAMESPACE_OPENDAQ_NATIVE_STREAMING_PROTOCOL

using namespace daq::native_streaming;
using namespace packet_streaming;

ServerSessionHandler::ServerSessionHandler(const ContextPtr& context,
                                           SessionPtr session,
                                           OnSignalSubscriptionCallback signalSubscriptionHandler,
                                           OnErrorCallback errorHandler)
    : BaseSessionHandler(session, errorHandler)
    , signalSubscriptionHandler(signalSubscriptionHandler)
    , logger(context.getLogger())
    , packetStreamingServer(10)
    , jsonSerializer(JsonSerializer(False))
{
    if (!this->logger.assigned())
        throw ArgumentNullException("Logger must not be null");
    loggerComponent = this->logger.getOrAddComponent("NativeStreamingServerSessionHandler");
}

ServerSessionHandler::~ServerSessionHandler()
{
}

void ServerSessionHandler::sendSignalAvailable(const SignalNumericIdType& signalNumericId,
                                               const SignalPtr& signal)
{
    std::vector<WriteTask> tasks;
    SizeT signalStringIdMaxSize = std::numeric_limits<uint16_t>::max();

    // create write task for signal numeric ID
    tasks.push_back(createWriteNumberTask<SignalNumericIdType>(signalNumericId));

    auto signalStringId = signal.getGlobalId();
    // create write task for signal string ID size
    SizeT signalStringIdSize = signalStringId.getLength();
    if (signalStringIdSize > signalStringIdMaxSize)
        throw NativeStreamingProtocolException("Size of signal string id exceeds limit");
    tasks.push_back(createWriteNumberTask<uint16_t>(static_cast<uint16_t>(signalStringIdSize)));
    // create write task for signal string ID itself
    tasks.push_back(createWriteStringTask(signalStringId.toStdString()));

    if (signal.getDomainSignal().assigned())
    {
        auto domainSignalStringId = signal.getDomainSignal().getGlobalId();
        // create write task for domain signal string ID size
        auto domainSignalStringIdSize = domainSignalStringId.getLength();
        if (domainSignalStringIdSize > signalStringIdMaxSize)
            throw NativeStreamingProtocolException("Size of domain signal string id exceeds limit");
        tasks.push_back(createWriteNumberTask<uint16_t>(static_cast<uint16_t>(domainSignalStringIdSize)));
        // create write task for domain signal string ID itself
        tasks.push_back(createWriteStringTask(domainSignalStringId.toStdString()));
    }
    else
    {
        // create write task for domain signal string ID size 0 - indicates absence on domain signal
        auto domainSignalStringIdSize = 0;
        tasks.push_back(createWriteNumberTask<uint16_t>(static_cast<uint16_t>(domainSignalStringIdSize)));
    }

    // create write tasks for signal properties
    auto namePropertyValue = signal.getName();
    if (namePropertyValue.getLength() > signalStringIdMaxSize)
        throw NativeStreamingProtocolException("Size of property \"Name\" exceeds limit");
    auto descriptionPropertyValue = signal.getDescription();
    if (descriptionPropertyValue.getLength() > signalStringIdMaxSize)
        throw NativeStreamingProtocolException("Size of property \"Description\" exceeds limit");
    // create write task for property "Name" value string size
    tasks.push_back(createWriteNumberTask<uint16_t>(static_cast<uint16_t>(namePropertyValue.getLength())));
    // create write task for property "Name" value string itself
    if (namePropertyValue.getLength() > 0)
        tasks.push_back(createWriteStringTask(namePropertyValue.toStdString()));
    // create write task for property "Description" value string size
    tasks.push_back(createWriteNumberTask<uint16_t>(static_cast<uint16_t>(descriptionPropertyValue.getLength())));
    // create write task for property "Description" value string itself
    if (descriptionPropertyValue.getLength() > 0)
        tasks.push_back(createWriteStringTask(descriptionPropertyValue.toStdString()));

    // create write task for signal serialized descriptor
    if (signal.getDescriptor().assigned())
    {
        jsonSerializer.reset();
        signal.getDescriptor().serialize(jsonSerializer);
        auto serializedDescriptor = jsonSerializer.getOutput();
        LOG_T("Serialized descriptor:\n{}", serialized);
        tasks.push_back(createWriteStringTask(serializedDescriptor.toStdString()));
    }
    else
    {
        LOG_W("Signal {} does not have descriptor", signalStringId);
    }

    // create write task for transport header
    size_t payloadSize = calculatePayloadSize(tasks);
    auto writeHeaderTask = createWriteHeaderTask(PayloadType::PAYLOAD_TYPE_SIGNAL_AVAILABLE, payloadSize);
    tasks.insert(tasks.begin(), writeHeaderTask);

    session->scheduleWrite(tasks);
}

void ServerSessionHandler::sendSignalUnavailable(const SignalNumericIdType& signalNumericId,
                                                 const SignalPtr& signal)
{
    std::vector<WriteTask> tasks;

    // create write task for signal numeric ID
    tasks.push_back(createWriteNumberTask<SignalNumericIdType>(signalNumericId));

    // create write task for signal string ID
    tasks.push_back(createWriteStringTask(signal.getGlobalId().toStdString()));

    // create write task for transport header
    size_t payloadSize = calculatePayloadSize(tasks);
    auto writeHeaderTask = createWriteHeaderTask(PayloadType::PAYLOAD_TYPE_SIGNAL_UNAVAILABLE, payloadSize);
    tasks.insert(tasks.begin(), writeHeaderTask);

    session->scheduleWrite(tasks);
}

void ServerSessionHandler::sendInitializationDone()
{
    std::vector<WriteTask> tasks;

    tasks.push_back(createWriteHeaderTask(PayloadType::PAYLOAD_TYPE_PROTOCOL_INIT_DONE, 0));

    session->scheduleWrite(tasks);
}

void ServerSessionHandler::sendPacket(const SignalNumericIdType signalId, const PacketPtr& packet)
{
    packetStreamingServer.addDaqPacket(signalId, packet);
    while (const auto packetBuffer = packetStreamingServer.getNextPacketBuffer())
    {
        sendPacketBuffer(packetBuffer);
    }
}

void ServerSessionHandler::sendSubscribingDone(const SignalNumericIdType signalNumericId)
{
    std::vector<WriteTask> tasks;

    // create write task for signal numeric ID
    tasks.push_back(createWriteNumberTask<SignalNumericIdType>(signalNumericId));

    // create write task for transport header
    size_t payloadSize = calculatePayloadSize(tasks);
    auto writeHeaderTask = createWriteHeaderTask(PayloadType::PAYLOAD_TYPE_SIGNAL_SUBSCRIBE_ACK, payloadSize);
    tasks.insert(tasks.begin(), writeHeaderTask);

    session->scheduleWrite(tasks);
}

void ServerSessionHandler::sendUnsubscribingDone(const SignalNumericIdType signalNumericId)
{
    std::vector<WriteTask> tasks;

    // create write task for signal numeric ID
    tasks.push_back(createWriteNumberTask<SignalNumericIdType>(signalNumericId));

    // create write task for transport header
    size_t payloadSize = calculatePayloadSize(tasks);
    auto writeHeaderTask = createWriteHeaderTask(PayloadType::PAYLOAD_TYPE_SIGNAL_UNSUBSCRIBE_ACK, payloadSize);
    tasks.insert(tasks.begin(), writeHeaderTask);

    session->scheduleWrite(tasks);
}

void ServerSessionHandler::sendPacketBuffer(const PacketBufferPtr& packetBuffer)
{
    std::vector<WriteTask> tasks;

    // create write task for packet buffer header
    boost::asio::const_buffer packetBufferHeader(packetBuffer->packetHeader,
                                            packetBuffer->packetHeader->size);
    WriteHandler packetBufferHeaderHandler = [packetBuffer]() {};
    tasks.push_back(WriteTask(packetBufferHeader, packetBufferHeaderHandler));

    if (packetBuffer->packetHeader->payloadSize > 0)
    {
        // create write task for packet buffer payload
        boost::asio::const_buffer packetBufferPayload(packetBuffer->payload,
                                                     packetBuffer->packetHeader->payloadSize);
        WriteHandler packetBufferPayloadHandler = [packetBuffer]() {};
        tasks.push_back(WriteTask(packetBufferPayload, packetBufferPayloadHandler));
    }

    // create write task for transport header
    size_t payloadSize = calculatePayloadSize(tasks);
    auto writeHeaderTask = createWriteHeaderTask(PayloadType::PAYLOAD_TYPE_PACKET, payloadSize);
    tasks.insert(tasks.begin(), writeHeaderTask);

    session->scheduleWrite(tasks);
}

ReadTask ServerSessionHandler::readSignalSubscribe(const void *data, size_t size)
{
    size_t bytesDone = 0;

    SignalNumericIdType signalNumericId;
    std::string signalIdString;

    try
    {
        // Get signal numeric ID from received buffer
        copyData(&signalNumericId, data, sizeof(signalNumericId), bytesDone, size);
        LOG_T("Received signal numeric ID: {}", signalNumericId);
        bytesDone += sizeof(signalNumericId);

        // Get signal string ID from received buffer
        signalIdString = getStringFromData(data, size - bytesDone, bytesDone, size);
        LOG_T("Received signal string ID: {}", signalIdString);
    }
    catch (const DaqException& e)
    {
        LOG_E("Protocol error: {}", e.what());
        errorHandler(e.what(), session);
        return createReadStopTask();
    }

    signalSubscriptionHandler(signalNumericId, signalIdString, true, session);
    sendSubscribingDone(signalNumericId);
    return createReadHeaderTask();
}

ReadTask ServerSessionHandler::readSignalUnsubscribe(const void *data, size_t size)
{
    size_t bytesDone = 0;

    SignalNumericIdType signalNumericId;
    std::string signalIdString;

    try
    {
        // Get signal numeric ID from received buffer
        copyData(&signalNumericId, data, sizeof(signalNumericId), bytesDone, size);
        LOG_T("Received signal numeric ID: {}", signalNumericId);
        bytesDone += sizeof(signalNumericId);

        // Get signal string ID from received buffer
        signalIdString = getStringFromData(data, size - bytesDone, bytesDone, size);
        LOG_T("Received signal string ID: {}", signalIdString);
    }
    catch (const DaqException& e)
    {
        LOG_E("Protocol error: {}", e.what());
        errorHandler(e.what(), session);
        return createReadStopTask();
    }

    signalSubscriptionHandler(signalNumericId, signalIdString, false, session);
    sendUnsubscribingDone(signalNumericId);
    return createReadHeaderTask();
}

ReadTask ServerSessionHandler::readHeader(const void* data, size_t size)
{
    TransportHeader header(static_cast<const PackedHeaderType*>(data));
    PayloadType payloadType = header.getPayloadType();
    size_t payloadSize = header.getPayloadSize();

    LOG_T("Received header: type {}, size {}", convertPayloadTypeToString(payloadType), payloadSize);

    if (payloadType == PayloadType::PAYLOAD_TYPE_SIGNAL_SUBSCRIBE_COMMAND)
    {
        return ReadTask(
            [this](const void* data, size_t size)
            {
                return readSignalSubscribe(data, size);
            },
            payloadSize
        );
    }
    else if (payloadType == PayloadType::PAYLOAD_TYPE_SIGNAL_UNSUBSCRIBE_COMMAND)
    {
        return ReadTask(
            [this](const void* data, size_t size)
            {
                return readSignalUnsubscribe(data, size);
            },
            payloadSize
        );
    }
    else
    {
        LOG_W("Received type: {} cannot be handled by server side", convertPayloadTypeToString(payloadType));
        return ReadTask(
            [this](const void* data, size_t size)
            {
                return discardPayload(data, size);
            },
            payloadSize
        );
    }
}

END_NAMESPACE_OPENDAQ_NATIVE_STREAMING_PROTOCOL
