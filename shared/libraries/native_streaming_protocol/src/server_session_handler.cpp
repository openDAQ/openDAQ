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
                                           OnSignalCallback signalReceivedHandler,
                                           OnSubscriptionAckCallback subscriptionAckHandler,
                                           OnFindSignalCallback findSignalHandler,
                                           OnSignalSubscriptionCallback signalSubscriptionHandler,
                                           OnSessionErrorCallback errorHandler,
                                           SizeT streamingPacketSendTimeout)
    : BaseSessionHandler(daqContext,
                         session,
                         ioContextPtr,
                         errorHandler,
                         signalReceivedHandler,
                         subscriptionAckHandler,
                         findSignalHandler,
                         signalSubscriptionHandler,
                         "NativeProtocolServerSessionHandler",
                         streamingPacketSendTimeout)
    , transportLayerPropsHandler(nullptr)
    , clientId(clientId)
    , reconnected(false)
    , useConfigProtocol(false)
{
}

void ServerSessionHandler::sendStreamingInitDone()
{
    std::vector<WriteTask> tasks;

    tasks.push_back(createWriteHeaderTask(PayloadType::PAYLOAD_TYPE_STREAMING_PROTOCOL_INIT_DONE, 0));

    session->scheduleWrite(std::move(tasks));
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

void ServerSessionHandler::setClientType(ClientType clientType)
{
    this->clientType = clientType;
}

ClientType ServerSessionHandler::getClientType()
{
    return clientType;
}

void ServerSessionHandler::setExclusiveControlDropOthers(bool enabled)
{
    exclusiveControlDropOthers = enabled;
}

bool ServerSessionHandler::isExclusiveControlDropOthersEnabled()
{
    return exclusiveControlDropOthers;
}

void ServerSessionHandler::setClientId(const std::string& clientId)
{
    this->clientId = clientId;
}

std::string ServerSessionHandler::getClientId()
{
    return clientId;
}

void ServerSessionHandler::setClientHostName(const std::string& hostName)
{
    this->clientHostName = hostName;
}

std::string ServerSessionHandler::getClientHostName()
{
    return clientHostName;
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
    else if (payloadType == PayloadType::PAYLOAD_TYPE_STREAMING_SIGNAL_SUBSCRIBE_ACK)
    {
        return ReadTask(
            [thisWeakPtr](const void* data, size_t size)
            {
                if (const auto thisPtr = std::static_pointer_cast<ServerSessionHandler>(thisWeakPtr.lock()))
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
                if (const auto thisPtr = std::static_pointer_cast<ServerSessionHandler>(thisWeakPtr.lock()))
                    return thisPtr->readSignalUnsubscribedAck(data, size);
                return ReadTask();
            },
            payloadSize
        );
    }
    else if (payloadType == PayloadType::PAYLOAD_TYPE_STREAMING_SIGNAL_AVAILABLE)
    {
        return ReadTask(
            [thisWeakPtr](const void* data, size_t size)
            {
                if (const auto thisPtr = std::static_pointer_cast<ServerSessionHandler>(thisWeakPtr.lock()))
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
                if (const auto thisPtr = std::static_pointer_cast<ServerSessionHandler>(thisWeakPtr.lock()))
                    return thisPtr->readSignalUnavailable(data, size);
                return ReadTask();
            },
            payloadSize
        );
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
