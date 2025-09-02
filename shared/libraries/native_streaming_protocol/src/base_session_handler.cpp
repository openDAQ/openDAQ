#include <native_streaming_protocol/base_session_handler.h>
#include <opendaq/custom_log.h>

BEGIN_NAMESPACE_OPENDAQ_NATIVE_STREAMING_PROTOCOL

using namespace daq::native_streaming;
using namespace packet_streaming;

BaseSessionHandler::BaseSessionHandler(const ContextPtr& daqContext,
                                       SessionPtr session,
                                       const std::shared_ptr<boost::asio::io_context>& ioContextPtr,
                                       OnSessionErrorCallback errorHandler,
                                       OnSignalCallback signalReceivedHandler,
                                       OnSubscriptionAckCallback subscriptionAckHandler,
                                       OnFindSignalCallback findSignalHandler,
                                       OnSignalSubscriptionCallback signalSubscriptionHandler,
                                       ConstCharPtr loggerComponentName,
                                       SizeT streamingPacketSendTimeout)
    : session(session)
    , configPacketReceivedHandler(nullptr)
    , packetBufferReceivedHandler(nullptr)
    , errorHandler(errorHandler)
    , ioContextPtr(ioContextPtr)
    , connectionInactivityTimer(std::make_shared<boost::asio::steady_timer>(*(this->ioContextPtr)))
    , loggerComponent(daqContext.getLogger().getOrAddComponent(loggerComponentName))
    , streamingPacketSendTimeout(streamingPacketSendTimeout != UNLIMITED_PACKET_SEND_TIME
                                     ? std::chrono::milliseconds(streamingPacketSendTimeout)
                                     : std::chrono::milliseconds(0))
    , signalReceivedHandler(signalReceivedHandler)
    , subscriptionAckHandler(subscriptionAckHandler)
    , findSignalHandler(findSignalHandler)
    , signalSubscriptionHandler(signalSubscriptionHandler)
{
}

void BaseSessionHandler::startConnectionActivityMonitoring(Int heartbeatPeriod, Int inactivityTimeout)
{
    if (connectionActivityMonitoringStarted)
    {
        LOG_W("Connection activity monitoring is already running");
        return;
    }

    auto inactivityTimeoutMs = std::chrono::milliseconds(inactivityTimeout);
    auto heartbeatPeriodMs = std::chrono::milliseconds(heartbeatPeriod);

    std::weak_ptr<Session> session_weak = session;
    std::weak_ptr<boost::asio::steady_timer> connectionInactivityTimer_weak = connectionInactivityTimer;

    OnConnectionAliveCallback onConnectionAliveCallback =
        [connectionInactivityTimer_weak, session_weak, errorHandler = errorHandler, inactivityTimeoutMs]()
    {
        if (auto timer = connectionInactivityTimer_weak.lock())
        {
            timer->cancel();
            timer->expires_from_now(inactivityTimeoutMs);

            timer->async_wait(
                [errorHandler, session_weak](const boost::system::error_code& ec)
                {
                    if (ec)
                        return;
                    if (auto session = session_weak.lock())
                        errorHandler("Connection activity timeout error", session);
                });
        }
    };
    session->startConnectionActivityMonitoring(onConnectionAliveCallback, heartbeatPeriodMs);
    connectionActivityMonitoringStarted = true;
}

ReadTask BaseSessionHandler::readHeader(const void *data, size_t size)
{
    return createReadStopTask();
}

void BaseSessionHandler::setConfigPacketReceivedHandler(const ProcessConfigProtocolPacketCb& configPacketReceivedHandler)
{
    this->configPacketReceivedHandler = configPacketReceivedHandler;
}

void BaseSessionHandler::setPacketBufferReceivedHandler(const OnPacketBufferReceivedCallback& packetBufferReceivedHandler)
{
    this->packetBufferReceivedHandler = packetBufferReceivedHandler;
}

ReadTask BaseSessionHandler::readConfigurationPacket(const void* data, size_t size)
{
    if (!configPacketReceivedHandler)
        return discardPayload(data, size);

    size_t bytesDone = 0;

    config_protocol::PacketHeader* packetBufferHeader;

    try
    {
        auto errorGuard = DAQ_ERROR_GUARD();
        decltype(config_protocol::PacketHeader::headerSize) headerSize;

        // Get packet buffer header size from received buffer
        copyData(&headerSize, data, sizeof(headerSize), bytesDone, size);
        LOG_T("Received config packet buffer header size: {}", headerSize);

        if (headerSize != sizeof(config_protocol::PacketHeader))
        {
            LOG_E("Unsupported config packet buffer header size: {}. Skipping payload.", headerSize);
            return createReadHeaderTask();
        }

        // Get packet buffer header from received buffer
        packetBufferHeader = static_cast<config_protocol::PacketHeader*>(std::malloc(headerSize));
        copyData(packetBufferHeader, data, headerSize, bytesDone, size);
        LOG_T("Received config packet buffer header: header size {}, payload size {}",
              packetBufferHeader->headerSize, packetBufferHeader->payloadSize);
        bytesDone += headerSize;

        // Get packet buffer payload from received buffer
        if (packetBufferHeader->payloadSize > 0)
        {
            void* oldPtr = static_cast<void*>(packetBufferHeader);
            packetBufferHeader =
                static_cast<config_protocol::PacketHeader*>(
                    std::realloc(oldPtr, packetBufferHeader->headerSize + packetBufferHeader->payloadSize)
                );
            copyData(packetBufferHeader + 1, data, packetBufferHeader->payloadSize, bytesDone, size);
        }
    }
    catch (const DaqException& e)
    {
        LOG_E("Protocol error: {}", e.what());
        errorHandler(std::string("Protocol error - readConfigurationPacket - ") + e.what(), session);
        return createReadHeaderTask();
    }

    auto packet = config_protocol::PacketBuffer(
        static_cast<void*>(packetBufferHeader),
        config_protocol::DeleterCallback([](void* ptr)
                                         {
                                             config_protocol::PacketBuffer::deallocateMem(ptr);
                                         })
    );

    configPacketReceivedHandler(std::move(packet));

    return createReadHeaderTask();
}

BaseSessionHandler::~BaseSessionHandler()
{
    connectionInactivityTimer->cancel();
}

ReadTask BaseSessionHandler::createReadHeaderTask()
{
    auto thisWeakPtr = this->weak_from_this();
    return ReadTask(
        [thisWeakPtr](const void* data, size_t size)
        {
            if (const auto thisPtr = thisWeakPtr.lock())
                return thisPtr->readHeader(data, size);
            return ReadTask();
        },
        TransportHeader::PACKED_HEADER_SIZE
    );
}

void BaseSessionHandler::startReading()
{
    session->scheduleRead(createReadHeaderTask());
}

const SessionPtr BaseSessionHandler::getSession() const
{
    return session;
}

void BaseSessionHandler::sendConfigurationPacket(const config_protocol::PacketBuffer& packetBuffer)
{
    std::vector<WriteTask> tasks;
    tasks.reserve(3);

    auto packetBufferPtr =
        std::make_shared<config_protocol::PacketBuffer>(packetBuffer.getBuffer(), true);

    // create write task for packet buffer header
    boost::asio::const_buffer packetBufferHeader(packetBufferPtr->getBuffer(),
                                                 sizeof(config_protocol::PacketHeader));

    if (packetBufferPtr->getPayloadSize() > 0)
    {
        WriteHandler packetBufferHeaderHandler = []() {};
        tasks.push_back(WriteTask(packetBufferHeader, packetBufferHeaderHandler));

        // create write task for packet buffer payload
        boost::asio::const_buffer packetBufferPayload(packetBufferPtr->getPayload(),
                                                      packetBufferPtr->getPayloadSize());
        WriteHandler packetBufferPayloadHandler = [packetBufferPtr]() {};
        tasks.push_back(WriteTask(packetBufferPayload, packetBufferPayloadHandler));
    }
    else
    {
        WriteHandler packetBufferHeaderHandler = [packetBufferPtr]() {};
        tasks.push_back(WriteTask(packetBufferHeader, packetBufferHeaderHandler));
    }

    // create write task for transport header
    size_t payloadSize = calculatePayloadSize(tasks);
    auto writeHeaderTask = createWriteHeaderTask(PayloadType::PAYLOAD_TYPE_CONFIGURATION_PACKET, payloadSize);
    tasks.insert(tasks.begin(), writeHeaderTask);

    session->scheduleWrite(std::move(tasks));
}

ReadTask BaseSessionHandler::discardPayload(const void* /*data*/, size_t /*size*/)
{
    return createReadHeaderTask();
}

WriteTask BaseSessionHandler::createWriteHeaderTask(PayloadType payloadType, size_t payloadSize)
{
    auto header = std::make_shared<TransportHeader>(payloadType, payloadSize);
    boost::asio::const_buffer headerBuffer(header->getPackedHeaderPtr(), TransportHeader::PACKED_HEADER_SIZE);
    WriteHandler headerHandler = [header]() {};
    return WriteTask(headerBuffer, headerHandler);
}

WriteTask BaseSessionHandler::createWriteStringTask(const std::string& str)
{
    auto strCopy = std::make_shared<std::string>(str);
    boost::asio::const_buffer stringPayload(strCopy->data(), strCopy->size());
    WriteHandler stringPayloadHandler = [strCopy]() {};
    return WriteTask(stringPayload, stringPayloadHandler);
}

size_t BaseSessionHandler::calculatePayloadSize(const std::vector<daq::native_streaming::WriteTask>& writePayloadTasks)
{
    size_t result = 0;
    for (const auto& task : writePayloadTasks)
    {
        result += task.getBuffer().size();
    }
    if (result > TransportHeader::MAX_PAYLOAD_SIZE)
    {
        throw NativeStreamingProtocolException("Size of message payload exceeds limit");
    }
    return result;
}

ReadTask BaseSessionHandler::createReadStopTask()
{
    return ReadTask();
}

void BaseSessionHandler::copyData(void* destination, const void* source, size_t bytesToCopy, size_t sourceOffset, size_t sourceSize)
{
    if ( (bytesToCopy + sourceOffset) > sourceSize)
    {
        DAQ_THROW_EXCEPTION(GeneralErrorException,
                            R"(Failed to copy {} bytes from offset {} bytes of received data with size {} bytes)",
                            bytesToCopy,
                            sourceOffset,
                            sourceSize);
    }

    const char* sourcePtr = static_cast<const char*>(source);
    memcpy(destination, sourcePtr + sourceOffset, bytesToCopy);
}

std::string BaseSessionHandler::getStringFromData(const void* source, size_t stringSize, size_t sourceOffset, size_t sourceSize)
{
    if ( (stringSize + sourceOffset) > sourceSize)
    {
        DAQ_THROW_EXCEPTION(GeneralErrorException,
                            R"(Failed to get string with size {} bytes from offset {} bytes of received data with size {} bytes)",
                            stringSize,
                            sourceOffset,
                            sourceSize);
    }

    const char* sourcePtr = static_cast<const char*>(source);
    return std::string(sourcePtr + sourceOffset, stringSize);
}

ReadTask BaseSessionHandler::readPacketBuffer(const void* data, size_t size)
{
    if (!packetBufferReceivedHandler)
        return discardPayload(data, size);

    size_t bytesDone = 0;

    GenericPacketHeader* packetBufferHeader {};
    void* packetBufferPayload;

    try
    {
        auto errorGuard = DAQ_ERROR_GUARD();
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
                                       },
                                       false);

    packetBufferReceivedHandler(recvPacketBuffer);

    return createReadHeaderTask();
}

void BaseSessionHandler::sendPacketBuffer(PacketBufferPtr&& packetBuffer)
{
    std::vector<WriteTask> tasks;
    tasks.reserve((packetBuffer->packetHeader->payloadSize > 0) ? 2 : 1);

    auto deadlineTime =
        packetBuffer->timeStamp.has_value() && streamingPacketSendTimeout != std::chrono::milliseconds(0)
            ? std::optional(packetBuffer->timeStamp.value() + streamingPacketSendTimeout)
            : std::nullopt;

    createAndPushPacketBufferTasks(std::move(packetBuffer), tasks);

    session->scheduleWrite(std::move(tasks), std::move(deadlineTime));
}

void BaseSessionHandler::schedulePacketBufferWriteTasks(std::vector<native_streaming::WriteTask>&& tasks,
                                                        std::optional<std::chrono::steady_clock::time_point>&& timeStamp)
{
    auto deadlineTime =
        timeStamp.has_value() && streamingPacketSendTimeout != std::chrono::milliseconds(0)
            ? std::optional(timeStamp.value() + streamingPacketSendTimeout)
            : std::nullopt;

    session->scheduleWrite(std::move(tasks), std::move(deadlineTime));
}

void BaseSessionHandler::copyHeadersToBuffer(const packet_streaming::PacketBufferPtr& packetBuffer, char* bufferDestPtr)
{
    size_t payloadSize = packetBuffer->packetHeader->size + packetBuffer->packetHeader->payloadSize;
    if (payloadSize > TransportHeader::MAX_PAYLOAD_SIZE)
    {
        throw NativeStreamingProtocolException("Size of message payload exceeds limit");
    }
    auto transportHeader = TransportHeader(PayloadType::PAYLOAD_TYPE_STREAMING_PACKET, payloadSize);
    // Copy each header into the linear buffer
    std::memcpy(bufferDestPtr, transportHeader.getPackedHeaderPtr(), TransportHeader::PACKED_HEADER_SIZE);
    std::memcpy(bufferDestPtr + TransportHeader::PACKED_HEADER_SIZE, packetBuffer->packetHeader, packetBuffer->packetHeader->size);
}

void BaseSessionHandler::createAndPushPacketBufferTasks(packet_streaming::PacketBufferPtr&& packetBuffer,
                                                        std::vector<native_streaming::WriteTask>& tasks)
{
    auto headersBuffer = std::make_shared<std::vector<char>>(TransportHeader::PACKED_HEADER_SIZE + packetBuffer->packetHeader->size);
    copyHeadersToBuffer(packetBuffer, headersBuffer->data());

    boost::asio::const_buffer headersTaskBuffer(headersBuffer->data(), headersBuffer->size());
    WriteHandler headersHandler = [headersBuffer]() {};
    tasks.push_back(WriteTask(headersTaskBuffer, headersHandler));

    if (packetBuffer->packetHeader->payloadSize > 0)
    {
        // create write task for packet buffer payload
        boost::asio::const_buffer packetBufferPayload(packetBuffer->payload,
                                                      packetBuffer->packetHeader->payloadSize);
        WriteHandler packetBufferPayloadHandler = [packetBuffer]() {};
        tasks.push_back(WriteTask(packetBufferPayload, packetBufferPayloadHandler));
    }
}

void BaseSessionHandler::sendSignalSubscribe(const SignalNumericIdType& signalNumericId, const std::string& signalStringId)
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

void BaseSessionHandler::sendSignalUnsubscribe(const SignalNumericIdType& signalNumericId, const std::string& signalStringId)
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

ReadTask BaseSessionHandler::readSignalAvailable(const void* data, size_t size)
{
    if (!signalReceivedHandler)
        return discardPayload(data, size);

    size_t bytesDone = 0;

    SignalNumericIdType signalNumericId;
    uint16_t signalIdStringSize;
    StringPtr signalIdString;

    StringPtr serializedSignal;

    try
    {
        auto errorGuard = DAQ_ERROR_GUARD();
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

    signalReceivedHandler(signalNumericId, signalIdString, serializedSignal, true, getClientId());
    return createReadHeaderTask();
}

ReadTask BaseSessionHandler::readSignalUnavailable(const void* data, size_t size)
{
    if (!signalReceivedHandler)
        return discardPayload(data, size);

    size_t bytesDone = 0;

    SignalNumericIdType signalNumericId;
    std::string signalIdString;

    try
    {
        auto errorGuard = DAQ_ERROR_GUARD();
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

    signalReceivedHandler(signalNumericId, signalIdString, nullptr, false, getClientId());
    return createReadHeaderTask();
}

ReadTask BaseSessionHandler::readSignalSubscribedAck(const void* data, size_t size)
{
    if (!subscriptionAckHandler)
        return discardPayload(data, size);

    SignalNumericIdType signalNumericId;

    try
    {
        auto errorGuard = DAQ_ERROR_GUARD();
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

    subscriptionAckHandler(signalNumericId, true, getClientId());
    return createReadHeaderTask();
}

ReadTask BaseSessionHandler::readSignalUnsubscribedAck(const void* data, size_t size)
{
    if (!subscriptionAckHandler)
        return discardPayload(data, size);

    SignalNumericIdType signalNumericId;

    try
    {
        auto errorGuard = DAQ_ERROR_GUARD();
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

    subscriptionAckHandler(signalNumericId, false, getClientId());
    return createReadHeaderTask();
}

ReadTask BaseSessionHandler::readSignalSubscribe(const void* data, size_t size)
{
    if (!signalSubscriptionHandler || !findSignalHandler)
        return discardPayload(data, size);

    size_t bytesDone = 0;

    SignalNumericIdType signalNumericId;
    std::string signalIdString;

    try
    {
        auto errorGuard = DAQ_ERROR_GUARD();
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
        auto errorGuard = DAQ_ERROR_GUARD();
        const auto signal = findSignalHandler(signalIdString);
        const auto hasAccess = hasUserAccessToSignal(signal);

        if (hasAccess && signalSubscriptionHandler(signalNumericId, signal, true, getClientId()))
            sendSubscribingDone(signalNumericId);
    }
    catch (const NativeStreamingProtocolException& e)
    {
        LOG_W("Protocol warning: {}", e.what());
    }

    return createReadHeaderTask();
}

ReadTask BaseSessionHandler::readSignalUnsubscribe(const void* data, size_t size)
{
    if (!signalSubscriptionHandler || !findSignalHandler)
        return discardPayload(data, size);

    size_t bytesDone = 0;

    SignalNumericIdType signalNumericId;
    std::string signalIdString;

    try
    {
        auto errorGuard = DAQ_ERROR_GUARD();
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
        auto errorGuard = DAQ_ERROR_GUARD();
        const auto signal = findSignalHandler(signalIdString);
        const auto hasAccess = hasUserAccessToSignal(signal);

        if (hasAccess && signalSubscriptionHandler(signalNumericId, signal, false, getClientId()))
            sendUnsubscribingDone(signalNumericId);
    }
    catch (const NativeStreamingProtocolException& e)
    {
        LOG_W("Protocol warning: {}", e.what());
    }

    return createReadHeaderTask();
}

bool BaseSessionHandler::hasUserAccessToSignal(const SignalPtr& signal)
{
    return true;
}

std::string BaseSessionHandler::getClientId()
{
    return std::string();
}

void BaseSessionHandler::sendSignalAvailable(const SignalNumericIdType& signalNumericId,
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

void BaseSessionHandler::sendSignalUnavailable(const SignalNumericIdType& signalNumericId,
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

void BaseSessionHandler::sendSubscribingDone(const SignalNumericIdType signalNumericId)
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

void BaseSessionHandler::sendUnsubscribingDone(const SignalNumericIdType signalNumericId)
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

END_NAMESPACE_OPENDAQ_NATIVE_STREAMING_PROTOCOL
