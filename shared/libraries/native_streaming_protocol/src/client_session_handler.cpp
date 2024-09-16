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
                                           OnStreamingInitDoneCallback protocolInitDoneHandler,
                                           OnSubscriptionAckCallback subscriptionAckHandler,
                                           OnSessionErrorCallback errorHandler)
    : BaseSessionHandler(daqContext, session, ioContextPtr, errorHandler, "NativeProtocolClientSessionHandler")
    , signalReceivedHandler(signalReceivedHandler)
    , streamingInitDoneHandler(protocolInitDoneHandler)
    , subscriptionAckHandler(subscriptionAckHandler)
{
}

void ClientSessionHandler::sendSignalSubscribe(const SignalNumericIdType& signalNumericId, const std::string& signalStringId)
{
    std::vector<WriteTask> tasks;
    tasks.reserve(3);

    // create write task for signal numeric ID
    tasks.push_back(createWriteNumberTask<SignalNumericIdType>(signalNumericId));

    // create write task for signal string ID
    tasks.push_back(createWriteStringTask(signalStringId));

    // create write task for transport header
    size_t payloadSize = calculatePayloadSize(tasks);
    auto writeHeaderTask = createWriteHeaderTask(PayloadType::PAYLOAD_TYPE_STREAMING_SIGNAL_SUBSCRIBE_COMMAND, payloadSize);
    tasks.insert(tasks.begin(), writeHeaderTask);

    session->scheduleWrite(std::move(tasks));
}

void ClientSessionHandler::sendSignalUnsubscribe(const SignalNumericIdType& signalNumericId, const std::string& signalStringId)
{
    std::vector<WriteTask> tasks;
    tasks.reserve(3);

    // create write task for signal numeric ID
    tasks.push_back(createWriteNumberTask<SignalNumericIdType>(signalNumericId));

    // create write task for signal string ID
    tasks.push_back(createWriteStringTask(signalStringId));

    // create write task for transport header
    size_t payloadSize = calculatePayloadSize(tasks);
    auto writeHeaderTask = createWriteHeaderTask(PayloadType::PAYLOAD_TYPE_STREAMING_SIGNAL_UNSUBSCRIBE_COMMAND, payloadSize);
    tasks.insert(tasks.begin(), writeHeaderTask);

    session->scheduleWrite(std::move(tasks));
}

void ClientSessionHandler::sendTransportLayerProperties(const PropertyObjectPtr& properties)
{
    std::vector<WriteTask> tasks;
    tasks.reserve(2);

    auto jsonSerializer = JsonSerializer(False);
    properties.serialize(jsonSerializer);
    auto serializedProperties = jsonSerializer.getOutput();
    LOG_T("Serialized properties:\n{}", serializedProperties);
    tasks.push_back(createWriteStringTask(serializedProperties.toStdString()));

    // create write task for transport header
    size_t payloadSize = calculatePayloadSize(tasks);
    auto writeHeaderTask = createWriteHeaderTask(PayloadType::PAYLOAD_TYPE_TRANSPORT_LAYER_PROPERTIES, payloadSize);
    tasks.insert(tasks.begin(), writeHeaderTask);

    session->scheduleWrite(std::move(tasks));
}

void ClientSessionHandler::sendStreamingRequest()
{
    std::vector<WriteTask> tasks;

    tasks.push_back(createWriteHeaderTask(PayloadType::PAYLOAD_TYPE_STREAMING_PROTOCOL_INIT_REQUEST, 0));

    session->scheduleWrite(std::move(tasks));
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

    const auto thisWeakPtr = this->weak_from_this();

    if (payloadType == PayloadType::PAYLOAD_TYPE_STREAMING_SIGNAL_AVAILABLE)
    {
        return ReadTask(
            [thisWeakPtr](const void* data, size_t size)
            {
                if (const auto thisPtr = std::static_pointer_cast<ClientSessionHandler>(thisWeakPtr.lock())) 
                    return thisPtr->readSignalAvailable(data, size);
                return ReadTask();
            },
            payloadSize
        );
    }
    else if (payloadType == PayloadType::PAYLOAD_TYPE_STREAMING_SIGNAL_UNAVAILABLE)
    {
        return ReadTask(
            [thisWeakPtr](const void* data, size_t size)
            {
                if (const auto thisPtr = std::static_pointer_cast<ClientSessionHandler>(thisWeakPtr.lock()))
                    return thisPtr->readSignalUnavailable(data, size);
                return ReadTask();
            },
            payloadSize
        );
    }
    else if (payloadType == PayloadType::PAYLOAD_TYPE_STREAMING_PACKET)
    {
        return ReadTask(
            [thisWeakPtr](const void* data, size_t size)
            {
                if (const auto thisPtr = std::static_pointer_cast<ClientSessionHandler>(thisWeakPtr.lock()))
                    return thisPtr->readPacketBuffer(data, size);
                return ReadTask();
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
            [thisWeakPtr](const void* data, size_t size)
            {
                if (const auto thisPtr = std::static_pointer_cast<ClientSessionHandler>(thisWeakPtr.lock()))
                    return thisPtr->readSignalSubscribedAck(data, size);
                return ReadTask();
            },
            payloadSize
        );
    }
    else if (payloadType == PayloadType::PAYLOAD_TYPE_STREAMING_SIGNAL_UNSUBSCRIBE_ACK)
    {
        return ReadTask(
            [thisWeakPtr](const void* data, size_t size)
            {
                if (const auto thisPtr = std::static_pointer_cast<ClientSessionHandler>(thisWeakPtr.lock()))
                    return thisPtr->readSignalUnsubscribedAck(data, size);
                return ReadTask();
            },
            payloadSize
        );
    }
    else if (payloadType == PayloadType::PAYLOAD_TYPE_CONFIGURATION_PACKET)
    {
        return ReadTask(
            [thisWeakPtr](const void* data, size_t size)
            {
                if (const auto thisPtr = std::static_pointer_cast<ClientSessionHandler>(thisWeakPtr.lock()))
                    return thisPtr->readConfigurationPacket(data, size);
                return ReadTask();
            },
            payloadSize
        );
    }
    else
    {
        LOG_W("Received type: {} cannot be handled by client side", convertPayloadTypeToString(payloadType));
        return ReadTask(
            [thisWeakPtr](const void* data, size_t size)
            {
                if (const auto thisPtr = std::static_pointer_cast<ClientSessionHandler>(thisWeakPtr.lock()))
                    return thisPtr->discardPayload(data, size);
                return ReadTask();
            },
            payloadSize
        );
    }
}

END_NAMESPACE_OPENDAQ_NATIVE_STREAMING_PROTOCOL
