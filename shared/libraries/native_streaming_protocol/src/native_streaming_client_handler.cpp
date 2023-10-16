#include <native_streaming_protocol/native_streaming_client_handler.h>
#include <native_streaming/client.hpp>

#include <opendaq/custom_log.h>

BEGIN_NAMESPACE_OPENDAQ_NATIVE_STREAMING_PROTOCOL

using namespace daq::native_streaming;

NativeStreamingClientHandler::NativeStreamingClientHandler(
    const ContextPtr& context,
    OnSignalAvailableCallback signalAvailableHandler,
    OnSignalUnavailableCallback signalUnavailableHandler,
    OnPacketCallback packetHandler)
    : context(context)
    , logger(context.getLogger())
    , signalAvailableHandler(signalAvailableHandler)
    , signalUnavailableHandler(signalUnavailableHandler)
    , packetHandler(packetHandler)
{
    if (!this->logger.assigned())
        throw ArgumentNullException("Logger must not be null");
    loggerComponent = this->logger.getOrAddComponent("NativeStreamingClientHandler");
}

bool NativeStreamingClientHandler::connect(std::shared_ptr<boost::asio::io_context> ioContextPtr,
                                           std::string host,
                                           std::string port,
                                           std::string path)
{
    initClient(ioContextPtr, host, port, path);
    std::future<bool> connectedFuture = connectedPromise.get_future();
    client->connect();

    auto status = connectedFuture.wait_for(std::chrono::seconds(2));
    if (status == std::future_status::ready)
    {
        return connectedFuture.get();
    }
    else
    {
        client.reset();
        return false;
    }
}

void NativeStreamingClientHandler::subscribeSignal(const StringPtr& signalStringId)
{
    const auto it = std::find_if(std::begin(signalIds),
                                 std::end(signalIds),
                                 [signalStringId](const auto& pair)
                                 {
                                     return signalStringId == pair.second;
                                 });

    if (it != std::end(signalIds))
    {
        sessionHandler->sendSignalSubscribe(it->first, signalStringId.toStdString());
    }
}

void NativeStreamingClientHandler::unsubscribeSignal(const StringPtr& signalStringId)
{
    const auto it = std::find_if(std::begin(signalIds),
                                 std::end(signalIds),
                                 [signalStringId](const auto& pair)
                                 {
                                     return signalStringId == pair.second;
                                 });

    if (it != std::end(signalIds))
    {
        sessionHandler->sendSignalUnsubscribe(it->first, signalStringId.toStdString());
    }
}

void NativeStreamingClientHandler::initClientSessionHandler(SessionPtr session)
{
    LOG_D("Client connected");

    OnErrorCallback errorHandler = [this](const std::string& errorMessage, SessionPtr session)
    {
        LOG_W("Client connection lost: {}", errorMessage);
        session->close();
    };

    OnSignalCallback signalReceivedHandler =
        [this](const SignalNumericIdType& signalNumericId,
               const StringPtr& signalStringId,
               const StringPtr& domainSignalStringId,
               const DataDescriptorPtr& signalDescriptor,
               const StringPtr& name,
               const StringPtr& description,
               bool available)
    {
        handleSignal(signalNumericId, signalStringId, domainSignalStringId, signalDescriptor, name, description, available);
    };

    OnPacketReceivedCallback packetReceivedHandler =
        [this](const SignalNumericIdType& signalNumericId, const PacketPtr& packet)
    {
        handlePacket(signalNumericId, packet);
    };

    OnProtocolInitDoneCallback protocolInitDoneHandler =
        [this]()
    {
        connectedPromise.set_value(true);
    };

    sessionHandler = std::make_shared<ClientSessionHandler>(context,
                                                            session,
                                                            signalReceivedHandler,
                                                            packetReceivedHandler,
                                                            protocolInitDoneHandler,
                                                            errorHandler);
    sessionHandler->initErrorHandlers();
    sessionHandler->startReading();
}

void NativeStreamingClientHandler::initClient(std::shared_ptr<boost::asio::io_context> ioContextPtr,
                                              std::string host,
                                              std::string port,
                                              std::string path)
{
    OnNewSessionCallback onNewSessionCallback =
        [this](SessionPtr session)
    {
        initClientSessionHandler(session);
    };
    OnCompleteCallback onConnectionFailedCallback =
        [this](const boost::system::error_code& ec)
    {
        LOG_E("Client connection failed: {}", ec.message());
        connectedPromise.set_value(false);
    };
    LogCallback logCallback =
        [this](spdlog::source_loc location, spdlog::level::level_enum level, const char* msg)
    {
        loggerComponent.logMessage(SourceLocation{location.filename, location.line, location.funcname},
                                   msg,
                                   static_cast<LogLevel>(level));
    };

    client = std::make_shared<Client>(host,
                                      port,
                                      path,
                                      onNewSessionCallback,
                                      onConnectionFailedCallback,
                                      ioContextPtr,
                                      logCallback);
}

void NativeStreamingClientHandler::handlePacket(const SignalNumericIdType& signalNumericId, const PacketPtr& packet)
{
    packetHandler(signalIds.at(signalNumericId), packet);
}

void NativeStreamingClientHandler::handleSignal(const SignalNumericIdType& signalNumericId,
                                                const StringPtr& signalStringId,
                                                const StringPtr& domainSignalStringId,
                                                const DataDescriptorPtr& signalDescriptor,
                                                const StringPtr& name,
                                                const StringPtr& description,
                                                bool available)
{
    if (available)
    {
        signalIds.insert({signalNumericId, signalStringId});
        signalAvailableHandler(signalStringId, domainSignalStringId, signalDescriptor, name, description);
    }
    else
    {
        signalIds.erase(signalNumericId);
        signalUnavailableHandler(signalStringId);
    }
}

END_NAMESPACE_OPENDAQ_NATIVE_STREAMING_PROTOCOL
