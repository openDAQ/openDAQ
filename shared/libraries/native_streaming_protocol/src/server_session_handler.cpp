#include <native_streaming_protocol/server_session_handler.h>
#include <native_streaming_protocol/native_streaming_protocol_types.h>

#include <opendaq/custom_log.h>

#include <coretypes/json_serializer_factory.h>

BEGIN_NAMESPACE_OPENDAQ_NATIVE_STREAMING_PROTOCOL

using namespace daq::native_streaming;

ServerSessionHandler::ServerSessionHandler(const ContextPtr& daqContext,
                                           const std::shared_ptr<boost::asio::io_context>& ioContextPtr,
                                           SessionPtr session,
                                           const std::string& clientId,
                                           OnFindSignalCallback findSignalHandler,
                                           OnSignalSubscriptionCallback signalSubscriptionHandler,
                                           OnSessionErrorCallback errorHandler,
                                           SizeT streamingPacketSendTimeout)
    : BaseSessionHandler(daqContext, session, ioContextPtr, errorHandler, "NativeProtocolServerSessionHandler", streamingPacketSendTimeout)
    , findSignalHandler(findSignalHandler)
    , signalSubscriptionHandler(signalSubscriptionHandler)
    , transportLayerPropsHandler(nullptr)
    , clientId(clientId)
    , reconnected(false)
    , useConfigProtocol(false)
{
}

void ServerSessionHandler::sendSignalAvailable(const SignalNumericIdType& signalNumericId,
                                               const SignalPtr& signal)
{
    if (!hasUserAccessToSignal(signal))
        return;

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

    auto jsonSerializer = JsonSerializer(False);
    signal.serialize(jsonSerializer);
    auto serializedSignal = jsonSerializer.getOutput();
    LOG_T("Serialized signal:\n{}", serializedSignal);
    tasks.push_back(createWriteStringTask(serializedSignal.toStdString()));

    // create write task for transport header
    size_t payloadSize = calculatePayloadSize(tasks);
    auto writeHeaderTask = createWriteHeaderTask(PayloadType::PAYLOAD_TYPE_STREAMING_SIGNAL_AVAILABLE, payloadSize);
    tasks.insert(tasks.begin(), writeHeaderTask);

    session->scheduleWrite(std::move(tasks));
}

void ServerSessionHandler::sendSignalUnavailable(const SignalNumericIdType& signalNumericId,
                                                 const SignalPtr& signal)
{
    if (!hasUserAccessToSignal(signal))
        return;

    std::vector<WriteTask> tasks;

    // create write task for signal numeric ID
    tasks.push_back(createWriteNumberTask<SignalNumericIdType>(signalNumericId));

    // create write task for signal string ID
    tasks.push_back(createWriteStringTask(signal.getGlobalId().toStdString()));

    // create write task for transport header
    size_t payloadSize = calculatePayloadSize(tasks);
    auto writeHeaderTask = createWriteHeaderTask(PayloadType::PAYLOAD_TYPE_STREAMING_SIGNAL_UNAVAILABLE, payloadSize);
    tasks.insert(tasks.begin(), writeHeaderTask);

    session->scheduleWrite(std::move(tasks));
}

void ServerSessionHandler::sendStreamingInitDone()
{
    std::vector<WriteTask> tasks;

    tasks.push_back(createWriteHeaderTask(PayloadType::PAYLOAD_TYPE_STREAMING_PROTOCOL_INIT_DONE, 0));

    session->scheduleWrite(std::move(tasks));
}

void ServerSessionHandler::sendSubscribingDone(const SignalNumericIdType signalNumericId)
{
    std::vector<WriteTask> tasks;

    // create write task for signal numeric ID
    tasks.push_back(createWriteNumberTask<SignalNumericIdType>(signalNumericId));

    // create write task for transport header
    size_t payloadSize = calculatePayloadSize(tasks);
    auto writeHeaderTask = createWriteHeaderTask(PayloadType::PAYLOAD_TYPE_STREAMING_SIGNAL_SUBSCRIBE_ACK, payloadSize);
    tasks.insert(tasks.begin(), writeHeaderTask);

    session->scheduleWrite(std::move(tasks));
}

void ServerSessionHandler::sendUnsubscribingDone(const SignalNumericIdType signalNumericId)
{
    std::vector<WriteTask> tasks;

    // create write task for signal numeric ID
    tasks.push_back(createWriteNumberTask<SignalNumericIdType>(signalNumericId));

    // create write task for transport header
    size_t payloadSize = calculatePayloadSize(tasks);
    auto writeHeaderTask = createWriteHeaderTask(PayloadType::PAYLOAD_TYPE_STREAMING_SIGNAL_UNSUBSCRIBE_ACK, payloadSize);
    tasks.insert(tasks.begin(), writeHeaderTask);

    session->scheduleWrite(std::move(tasks));
}

ReadTask ServerSessionHandler::readSignalSubscribe(const void* data, size_t size)
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
        errorHandler(std::string("Protocol error - readSignalSubscribe - ") + e.what(), session);
        return createReadStopTask();
    }

    try
    {
        const auto signal = findSignalHandler(signalIdString);
        const auto hasAccess = hasUserAccessToSignal(signal);

        if (hasAccess && signalSubscriptionHandler(signalNumericId, signal, true, clientId))
            sendSubscribingDone(signalNumericId);
    }
    catch (const NativeStreamingProtocolException& e)
    {
        LOG_W("Protocol warning: {}", e.what());
    }

    return createReadHeaderTask();
}

ReadTask ServerSessionHandler::readSignalUnsubscribe(const void* data, size_t size)
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
        errorHandler(std::string("Protocol error - readSignalUnsubscribe - ") + e.what(), session);
        return createReadStopTask();
    }

    try
    {
        const auto signal = findSignalHandler(signalIdString);
        const auto hasAccess = hasUserAccessToSignal(signal);

        if (hasAccess && signalSubscriptionHandler(signalNumericId, signal, false, clientId))
            sendUnsubscribingDone(signalNumericId);
    }
    catch (const NativeStreamingProtocolException& e)
    {
        LOG_W("Protocol warning: {}", e.what());
    }

    return createReadHeaderTask();
}

ReadTask ServerSessionHandler::readTransportLayerProperties(const void* data, size_t size)
{
    PropertyObjectPtr propertyObject;

    try
    {
        // Get serialized properties from received buffer
        auto serializedProperties = String(getStringFromData(data, size, 0, size));
        LOG_D("Received properties:\n{}", serializedProperties);

        try
        {
            const auto deserializer = JsonDeserializer();
            propertyObject = deserializer.deserialize(serializedProperties, nullptr);
        }
        catch (const std::exception& e)
        {
            LOG_E("Fail to deserialize property object: {}", e.what());
        }
    }
    catch (const DaqException& e)
    {
        LOG_E("Protocol error: {}", e.what());
        errorHandler(std::string("Protocol error - readTransportLayerProperties - ") + e.what(), session);
        return createReadStopTask();
    }

    if (transportLayerPropsHandler && propertyObject.assigned())
        transportLayerPropsHandler(propertyObject);
    return createReadHeaderTask();
}

bool ServerSessionHandler::hasUserAccessToSignal(const SignalPtr& signal)
{
    const auto user = getUser();
    return signal.getPermissionManager().isAuthorized(user, Permission::Read);
}

void ServerSessionHandler::setTransportLayerPropsHandler(const OnTrasportLayerPropertiesCallback& transportLayerPropsHandler)
{
    this->transportLayerPropsHandler = transportLayerPropsHandler;
}

void ServerSessionHandler::setStreamingInitHandler(const OnStreamingRequestCallback& streamingInitHandler)
{
    this->streamingInitHandler = streamingInitHandler;
}

void ServerSessionHandler::setReconnected(bool reconnected)
{
    this->reconnected = reconnected;
}

bool ServerSessionHandler::getReconnected()
{
    return this->reconnected;
}

void ServerSessionHandler::triggerUseConfigProtocol()
{
    this->useConfigProtocol = true;
}

bool ServerSessionHandler::isConfigProtocolUsed()
{
    return this->useConfigProtocol;
}

UserPtr ServerSessionHandler::getUser()
{
    auto user = (IUser*) session->getUserContext().get();
    UserPtr userPtr = user;
    return userPtr;
}

void ServerSessionHandler::setClientId(const std::string& clientId)
{
    this->clientId = clientId;
}

std::string ServerSessionHandler::getClientId()
{
    return clientId;
}

ReadTask ServerSessionHandler::readHeader(const void* data, size_t size)
{
    TransportHeader header(static_cast<const PackedHeaderType*>(data));
    PayloadType payloadType = header.getPayloadType();
    size_t payloadSize = header.getPayloadSize();

    LOG_T("Received header: type {}, size {}", convertPayloadTypeToString(payloadType), payloadSize);

    const auto thisWeakPtr = this->weak_from_this();

    if (payloadType == PayloadType::PAYLOAD_TYPE_STREAMING_SIGNAL_SUBSCRIBE_COMMAND)
    {
        return ReadTask(
            [thisWeakPtr](const void* data, size_t size)
            {
                if (const auto thisPtr = std::static_pointer_cast<ServerSessionHandler>(thisWeakPtr.lock()))
                    return thisPtr->readSignalSubscribe(data, size);
                return ReadTask();
            },
            payloadSize
        );
    }
    else if (payloadType == PayloadType::PAYLOAD_TYPE_STREAMING_SIGNAL_UNSUBSCRIBE_COMMAND)
    {
        return ReadTask(
            [thisWeakPtr](const void* data, size_t size)
            {
                if (const auto thisPtr = std::static_pointer_cast<ServerSessionHandler>(thisWeakPtr.lock()))
                    return thisPtr->readSignalUnsubscribe(data, size);
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
                if (const auto thisPtr = std::static_pointer_cast<ServerSessionHandler>(thisWeakPtr.lock()))
                    return thisPtr->readConfigurationPacket(data, size);
                return ReadTask();
            },
            payloadSize
        );
    }
    else if (payloadType == PayloadType::PAYLOAD_TYPE_TRANSPORT_LAYER_PROPERTIES)
    {
        return ReadTask(
            [thisWeakPtr](const void* data, size_t size)
            {
                if (const auto thisPtr = std::static_pointer_cast<ServerSessionHandler>(thisWeakPtr.lock()))
                    return thisPtr->readTransportLayerProperties(data, size);
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
                if (const auto thisPtr = std::static_pointer_cast<ServerSessionHandler>(thisWeakPtr.lock()))
                    return thisPtr->readPacketBuffer(data, size);
                return ReadTask();
            },
            payloadSize
        );
    }
    else if (payloadType == PayloadType::PAYLOAD_TYPE_STREAMING_PROTOCOL_INIT_REQUEST)
    {
        if (streamingInitHandler)
            streamingInitHandler();
        return createReadHeaderTask();
    }
    else
    {
        LOG_W("Received type: {} cannot be handled by server side", convertPayloadTypeToString(payloadType));
        return ReadTask(
            [thisWeakPtr](const void* data, size_t size)
            {
                if (const auto thisPtr = std::static_pointer_cast<ServerSessionHandler>(thisWeakPtr.lock()))
                    return thisPtr->discardPayload(data, size);
                return ReadTask();
            },
            payloadSize
        );
    }
}

END_NAMESPACE_OPENDAQ_NATIVE_STREAMING_PROTOCOL
