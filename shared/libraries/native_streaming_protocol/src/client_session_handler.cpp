#include <native_streaming_protocol/client_session_handler.h>
#include <native_streaming_protocol/native_streaming_protocol_types.h>

#include <opendaq/custom_log.h>
#include <opendaq/signal_factory.h>

BEGIN_NAMESPACE_OPENDAQ_NATIVE_STREAMING_PROTOCOL

using namespace daq::native_streaming;
using namespace packet_streaming;

ClientSessionHandler::ClientSessionHandler(const ContextPtr& daqContext,
                                           const std::shared_ptr<boost::asio::io_context>& ioContextPtr,
                                           SessionPtr session,
                                           OnSignalCallback signalReceivedHandler,
                                           OnPacketReceivedCallback packetReceivedHandler,
                                           OnStreamingInitDoneCallback protocolInitDoneHandler,
                                           OnSubscriptionAckCallback subscriptionAckHandler,
                                           OnSessionErrorCallback errorHandler)
    : BaseSessionHandler(daqContext, session, ioContextPtr, errorHandler, "NativeProtocolClientSessionHandler")
    , signalReceivedHandler(signalReceivedHandler)
    , packetReceivedHandler(packetReceivedHandler)
    , streamingInitDoneHandler(protocolInitDoneHandler)
    , subscriptionAckHandler(subscriptionAckHandler)
{
}

void ClientSessionHandler::sendSignalSubscribe(const SignalNumericIdType& signalNumericId, const std::string& signalStringId)
{
    std::vector<WriteTask> tasks;

    // create write task for signal numeric ID
    tasks.push_back(createWriteNumberTask<SignalNumericIdType>(signalNumericId));

    // create write task for signal string ID
    tasks.push_back(createWriteStringTask(signalStringId));

    // create write task for transport header
    size_t payloadSize = calculatePayloadSize(tasks);
    auto writeHeaderTask = createWriteHeaderTask(PayloadType::PAYLOAD_TYPE_STREAMING_SIGNAL_SUBSCRIBE_COMMAND, payloadSize);
    tasks.insert(tasks.begin(), writeHeaderTask);

    session->scheduleWrite(tasks);
}

void ClientSessionHandler::sendSignalUnsubscribe(const SignalNumericIdType& signalNumericId, const std::string& signalStringId)
{
    std::vector<WriteTask> tasks;

    // create write task for signal numeric ID
    tasks.push_back(createWriteNumberTask<SignalNumericIdType>(signalNumericId));

    // create write task for signal string ID
    tasks.push_back(createWriteStringTask(signalStringId));

    // create write task for transport header
    size_t payloadSize = calculatePayloadSize(tasks);
    auto writeHeaderTask = createWriteHeaderTask(PayloadType::PAYLOAD_TYPE_STREAMING_SIGNAL_UNSUBSCRIBE_COMMAND, payloadSize);
    tasks.insert(tasks.begin(), writeHeaderTask);

    session->scheduleWrite(tasks);
}

void ClientSessionHandler::sendTransportLayerProperties(const PropertyObjectPtr& properties)
{
    std::vector<WriteTask> tasks;

    auto jsonSerializer = JsonSerializer(False);
    properties.serialize(jsonSerializer);
    auto serializedProperties = jsonSerializer.getOutput();
    LOG_T("Serialized properties:\n{}", serializedProperties);
    tasks.push_back(createWriteStringTask(serializedProperties.toStdString()));

    // create write task for transport header
    size_t payloadSize = calculatePayloadSize(tasks);
    auto writeHeaderTask = createWriteHeaderTask(PayloadType::PAYLOAD_TYPE_TRANSPORT_LAYER_PROPERTIES, payloadSize);
    tasks.insert(tasks.begin(), writeHeaderTask);

    session->scheduleWrite(tasks);
}

void ClientSessionHandler::sendStreamingRequest()
{
    std::vector<WriteTask> tasks;

    tasks.push_back(createWriteHeaderTask(PayloadType::PAYLOAD_TYPE_STREAMING_PROTOCOL_INIT_REQUEST, 0));

    session->scheduleWrite(tasks);
}

ReadTask ClientSessionHandler::readPacket(const void* data, size_t size)
{
    size_t bytesDone = 0;

    GenericPacketHeader* packetBufferHeader {};
    void* packetBufferPayload;

    try
    {
        decltype(GenericPacketHeader::size) headerSize;

        // Get packet buffer header size from received buffer
        copyData(&headerSize, data, sizeof(headerSize), bytesDone, size);
        LOG_T("Received packet buffer header size: {}", headerSize);

        if (headerSize < sizeof(GenericPacketHeader))
        {
            LOG_E("Unsupported streaming packet buffer header size: {}. Skipping payload.", headerSize);
            return createReadHeaderTask();
        }

        // Get packet buffer header from received buffer
        packetBufferHeader = static_cast<GenericPacketHeader*>(std::malloc(headerSize));
        copyData(packetBufferHeader, data, headerSize, bytesDone, size);
        LOG_T("Received packet buffer header: header size {}, payload size {}",
              packetBufferHeader->size, packetBufferHeader->payloadSize);
        bytesDone += headerSize;

        // Get packet buffer payload from received buffer
        if (packetBufferHeader->payloadSize > 0)
        {
            packetBufferPayload = std::malloc(packetBufferHeader->payloadSize);
            copyData(packetBufferPayload, data, packetBufferHeader->payloadSize, bytesDone, size);
        }
        else
        {
            packetBufferPayload = nullptr;
        }
    }
    catch (const DaqException& e)
    {
        LOG_E("Protocol error: {}", e.what());
        errorHandler(std::string("Protocol error - readPacket - ") + e.what(), session);
        return createReadStopTask();
    }

    auto recvPacketBuffer =
        std::make_shared<PacketBuffer>(packetBufferHeader,
                                       packetBufferPayload,
                                       [packetBufferHeader, packetBufferPayload]()
                                       {
                                           std::free(packetBufferHeader);
                                           if (packetBufferPayload != nullptr)
                                               std::free(packetBufferPayload);
                                       });

    packetStreamingClient.addPacketBuffer(recvPacketBuffer);

    processReceivedPackets();

    return createReadHeaderTask();
}

void ClientSessionHandler::processReceivedPackets()
{
    auto [signalId, packet] = packetStreamingClient.getNextDaqPacket();
    while (packet.assigned())
    {
        packetReceivedHandler(signalId, packet);
        std::tie(signalId, packet) = packetStreamingClient.getNextDaqPacket();
    }
}

ReadTask ClientSessionHandler::readSignalAvailable(const void* data, size_t size)
{
    size_t bytesDone = 0;

    SignalNumericIdType signalNumericId;
    uint16_t signalIdStringSize;
    StringPtr signalIdString;

    StringPtr serializedSignal;

    try
    {
        // Get signal numeric ID from received buffer
        copyData(&signalNumericId, data, sizeof(signalNumericId), bytesDone, size);
        LOG_T("Received signal numeric ID: {}", signalNumericId);
        bytesDone += sizeof(signalNumericId);

        // Get size of signal string ID from received buffer
        copyData(&signalIdStringSize, data, sizeof(signalIdStringSize), bytesDone, size);
        LOG_T("Received signal string ID size: {}", signalIdStringSize);
        bytesDone += sizeof(signalIdStringSize);

        // Get signal string ID from received buffer
        signalIdString = String(getStringFromData(data, signalIdStringSize, bytesDone, size));
        LOG_T("Received signal string ID: {}", signalIdString);
        bytesDone += signalIdStringSize;

        // Get serialized signal from received buffer
        serializedSignal = String(getStringFromData(data, size - bytesDone, bytesDone, size));
        LOG_T("Received serialized signal:\n{}", serializedSignal);
    }
    catch (const DaqException& e)
    {
        LOG_E("Protocol error: {}", e.what());
        errorHandler(std::string("Protocol error - readSignalAvailable - ") + e.what(), session);
        return createReadStopTask();
    }

    signalReceivedHandler(signalNumericId, signalIdString, serializedSignal, true);
    return createReadHeaderTask();
}

ReadTask ClientSessionHandler::readSignalUnavailable(const void *data, size_t size)
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
        errorHandler(std::string("Protocol error - readSignalUnavailable - ") + e.what(), session);
        return createReadStopTask();
    }

    signalReceivedHandler(signalNumericId, signalIdString, nullptr, false);
    return createReadHeaderTask();
}

ReadTask ClientSessionHandler::readSignalSubscribedAck(const void *data, size_t size)
{
    SignalNumericIdType signalNumericId;

    try
    {
        // Get signal numeric ID from received buffer
        copyData(&signalNumericId, data, sizeof(signalNumericId), 0, size);
        LOG_T("Received signal numeric ID: {}", signalNumericId);
    }
    catch (const DaqException& e)
    {
        LOG_E("Protocol error: {}", e.what());
        errorHandler(std::string("Protocol error - readSignalSubscribedAck - ") + e.what(), session);
        return createReadStopTask();
    }

    subscriptionAckHandler(signalNumericId, true);
    return createReadHeaderTask();
}

ReadTask ClientSessionHandler::readSignalUnsubscribedAck(const void *data, size_t size)
{
    SignalNumericIdType signalNumericId;

    try
    {
        // Get signal numeric ID from received buffer
        copyData(&signalNumericId, data, sizeof(signalNumericId), 0, size);
        LOG_T("Received signal numeric ID: {}", signalNumericId);
    }
    catch (const DaqException& e)
    {
        LOG_E("Protocol error: {}", e.what());
        errorHandler(std::string("Protocol error - readSignalUnsubscribedAck - ") + e.what(), session);
        return createReadStopTask();
    }

    subscriptionAckHandler(signalNumericId, false);
    return createReadHeaderTask();
}

ReadTask ClientSessionHandler::readHeader(const void* data, size_t size)
{
    TransportHeader header(static_cast<const PackedHeaderType*>(data));
    PayloadType payloadType = header.getPayloadType();
    size_t payloadSize = header.getPayloadSize();

    LOG_T("Received header: type {}, size {}", convertPayloadTypeToString(payloadType), payloadSize);

    if (payloadType == PayloadType::PAYLOAD_TYPE_STREAMING_SIGNAL_AVAILABLE)
    {
        return ReadTask(
            [this](const void* data, size_t size)
            {
                return readSignalAvailable(data, size);
            },
            payloadSize
        );
    }
    else if (payloadType == PayloadType::PAYLOAD_TYPE_STREAMING_SIGNAL_UNAVAILABLE)
    {
        return ReadTask(
            [this](const void* data, size_t size)
            {
                return readSignalUnavailable(data, size);
            },
            payloadSize
        );
    }
    else if (payloadType == PayloadType::PAYLOAD_TYPE_STREAMING_PACKET)
    {
        return ReadTask(
            [this](const void* data, size_t size)
            {
                return readPacket(data, size);
            },
            payloadSize
        );
    }
    else if (payloadType == PayloadType::PAYLOAD_TYPE_STREAMING_PROTOCOL_INIT_DONE)
    {
        streamingInitDoneHandler();
        return createReadHeaderTask();
    }
    else if (payloadType == PayloadType::PAYLOAD_TYPE_STREAMING_SIGNAL_SUBSCRIBE_ACK)
    {
        return ReadTask(
            [this](const void* data, size_t size)
            {
                return readSignalSubscribedAck(data, size);
            },
            payloadSize
        );
    }
    else if (payloadType == PayloadType::PAYLOAD_TYPE_STREAMING_SIGNAL_UNSUBSCRIBE_ACK)
    {
        return ReadTask(
            [this](const void* data, size_t size)
            {
                return readSignalUnsubscribedAck(data, size);
            },
            payloadSize
        );
    }
    else if (payloadType == PayloadType::PAYLOAD_TYPE_CONFIGURATION_PACKET)
    {
        return ReadTask(
            [this](const void* data, size_t size)
            {
                return readConfigurationPacket(data, size);
            },
            payloadSize
        );
    }
    else
    {
        LOG_W("Received type: {} cannot be handled by client side", convertPayloadTypeToString(payloadType));
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
