#include <native_streaming_protocol/base_session_handler.h>
#include <opendaq/custom_log.h>

BEGIN_NAMESPACE_OPENDAQ_NATIVE_STREAMING_PROTOCOL

using namespace daq::native_streaming;
using namespace packet_streaming;

BaseSessionHandler::BaseSessionHandler(const ContextPtr& daqContext,
                                       SessionPtr session,
                                       const std::shared_ptr<boost::asio::io_context>& ioContextPtr,
                                       OnSessionErrorCallback errorHandler,
                                       ConstCharPtr loggerComponentName)
    : session(session)
    , configPacketReceivedHandler(nullptr)
    , packetBufferReceivedHandler(nullptr)
    , errorHandler(errorHandler)
    , ioContextPtr(ioContextPtr)
    , connectionInactivityTimer(std::make_shared<boost::asio::steady_timer>(*(this->ioContextPtr)))
    , loggerComponent(daqContext.getLogger().getOrAddComponent(loggerComponentName))
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
        throw DaqException(OPENDAQ_ERR_GENERALERROR,
                           fmt::format(R"(Failed to copy {} bytes from offset {} bytes of received data with size {} bytes)",
                                       bytesToCopy,
                                       sourceOffset,
                                       sourceSize));
    }

    const char* sourcePtr = static_cast<const char*>(source);
    memcpy(destination, sourcePtr + sourceOffset, bytesToCopy);
}

std::string BaseSessionHandler::getStringFromData(const void* source, size_t stringSize, size_t sourceOffset, size_t sourceSize)
{
    if ( (stringSize + sourceOffset) > sourceSize)
    {
        throw DaqException(OPENDAQ_ERR_GENERALERROR,
                           fmt::format(R"(Failed to get string with size {} bytes from offset {} bytes of received data with size {} bytes)",
                                       stringSize,
                                       sourceOffset,
                                       sourceSize));
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

    packetBufferReceivedHandler(recvPacketBuffer);

    return createReadHeaderTask();
}

void BaseSessionHandler::sendPacketBuffer(const PacketBufferPtr& packetBuffer)
{
    std::vector<WriteTask> tasks;
    tasks.reserve(3);

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
    auto writeHeaderTask = createWriteHeaderTask(PayloadType::PAYLOAD_TYPE_STREAMING_PACKET, payloadSize);
    tasks.insert(tasks.begin(), writeHeaderTask);

    session->scheduleWrite(std::move(tasks));
}

END_NAMESPACE_OPENDAQ_NATIVE_STREAMING_PROTOCOL
