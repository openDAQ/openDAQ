#include <native_streaming_protocol/base_session_handler.h>

BEGIN_NAMESPACE_OPENDAQ_NATIVE_STREAMING_PROTOCOL

using namespace daq::native_streaming;

static std::chrono::milliseconds heartbeatPeriod = std::chrono::milliseconds(1000);
static std::chrono::milliseconds heartbeatTimeout = std::chrono::milliseconds(1500);

BaseSessionHandler::BaseSessionHandler(SessionPtr session,
                                       boost::asio::io_context& ioContext,
                                       OnSessionErrorCallback errorHandler)
    : session(session)
    , errorHandler(errorHandler)
    , heartbeatTimer(std::make_shared<boost::asio::steady_timer>(ioContext))
{
    initHeartbeat();
}

void BaseSessionHandler::initHeartbeat()
{
    OnHeartbeatCallback onHeartbeatCallback = [this]()
    {
        heartbeatTimer->cancel();
        heartbeatTimer->expires_from_now(heartbeatTimeout);
        heartbeatTimer->async_wait(
            [this](const boost::system::error_code& ec)
            {
                if (ec)
                    return;
                this->errorHandler("Heartbeat timeout error", session);
            }
        );
    };
    session->startHeartbeat(onHeartbeatCallback, heartbeatPeriod);
}

ReadTask BaseSessionHandler::readHeader(const void *data, size_t size)
{
    return createReadStopTask();
}

BaseSessionHandler::~BaseSessionHandler()
{
    heartbeatTimer->cancel();
}

ReadTask BaseSessionHandler::createReadHeaderTask()
{
    return ReadTask(
        [this](const void* data, size_t size)
        {
            return readHeader(data, size);
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

END_NAMESPACE_OPENDAQ_NATIVE_STREAMING_PROTOCOL
