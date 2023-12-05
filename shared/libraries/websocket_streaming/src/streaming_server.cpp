#include <nlohmann/json.hpp>

#include <streaming_protocol/Defines.h>
#include <streaming_protocol/Logging.hpp>
#include <streaming_protocol/jsonrpc_defines.hpp>

#include "websocket_streaming/streaming_server.h"
#include <opendaq/event_packet_ids.h>
#include <opendaq/event_packet_params.h>
#include <opendaq/custom_log.h>

BEGIN_NAMESPACE_OPENDAQ_WEBSOCKET_STREAMING

using namespace daq::streaming_protocol;
using namespace daq::stream;

StreamingServer::StreamingServer(const ContextPtr& context)
    : work(ioContext.get_executor())
    , logger(context.getLogger())
{
    if (!this->logger.assigned())
        throw ArgumentNullException("Logger must not be null");
    loggerComponent = this->logger.getOrAddComponent("StreamingServer");
    logCallback = [this](spdlog::source_loc location, spdlog::level::level_enum level, const char* msg) {
        this->loggerComponent.logMessage(SourceLocation{location.filename, location.line, location.funcname}, msg, static_cast<LogLevel>(level));
    };
}

StreamingServer::~StreamingServer()
{
    stop();
    logger.removeComponent("StreamingServer");
}

void StreamingServer::start(uint16_t port, uint16_t controlPort)
{
    this->port = port;

    ioContext.restart();

    auto acceptFunc = [this](StreamPtr stream) { this->onAcceptInternal(stream); };

    this->server = std::make_unique<daq::stream::WebsocketServer>(ioContext, acceptFunc, port);
    this->server->start();

    auto controlCommandCb = [this](const std::string& streamId,
                                   const std::string& command,
                                   const daq::streaming_protocol::SignalIds& signalIds,
                                   std::string& errorMessage)
    {
        return onControlCommand(streamId, command, signalIds, errorMessage);
    };
    this->controlServer =
        std::make_unique<daq::streaming_protocol::ControlServer>(ioContext,
                                                                 controlPort,
                                                                 controlCommandCb,
                                                                 logCallback);
    this->controlServer->start();

    this->serverThread = std::thread([this]() { this->ioContext.run(); });
}

void StreamingServer::stop()
{
    ioContext.stop();

    if (!serverThread.joinable())
        return;

    this->server->stop();
    serverThread.join();
    this->server.reset();
}

void StreamingServer::onAccept(const OnAcceptCallback& callback)
{
    onAcceptCallback = callback;
}

void StreamingServer::onSubscribe(const OnSubscribeCallback& callback)
{
    onSubscribeCallback = callback;
}

void StreamingServer::onUnsubscribe(const OnUnsubscribeCallback& callback)
{
    onUnsubscribeCallback = callback;
}

void StreamingServer::unicastPacket(const daq::streaming_protocol::StreamWriterPtr& client,
                                    const std::string& signalId,
                                    const PacketPtr& packet)
{
    auto& signals = clients[client];
    if (signals.count(signalId) > 0)
        signals[signalId]->write(packet);
}

void StreamingServer::broadcastPacket(const std::string& signalId, const PacketPtr& packet)
{
    for (auto& [_, client] : clients)
        if (client.count(signalId) > 0)
            client[signalId]->write(packet);
}

void StreamingServer::sendPacketToSubscribers(const std::string& signalId, const PacketPtr& packet)
{
    for (auto& [_, client] : clients)
        if (auto signalIter = client.find(signalId); signalIter != client.end())
        {
            if (signalIter->second->isSubscribed())
                signalIter->second->write(packet);
        }
}

void StreamingServer::onAcceptInternal(const daq::stream::StreamPtr& stream)
{
    auto writer = std::make_shared<StreamWriter>(stream);
    writeProtocolInfo(writer);
    writeInit(writer);

    auto signals = List<ISignal>();
    if (onAcceptCallback)
        signals = onAcceptCallback(writer);

    auto outputSignals = std::unordered_map<std::string, OutputSignalPtr>();
    for (const auto& signal : signals)
    {
        // TODO: We skip domain signals for now.
        if (!signal.getDomainSignal().assigned())
            continue;

        try
        {
            const auto outputSignal = std::make_shared<OutputSignal>(writer, signal, logCallback);
            outputSignals.insert({signal.getGlobalId(), outputSignal});
        }
        catch (const DaqException&)
        {
            LOG_W("Failed to create an ouput websocket signal.");
        }
    }

    LOG_I("New client connected. Stream Id: {}", writer->id());
    clients.insert({writer, outputSignals});

    writeSignalsAvailable(writer, signals);
}

int StreamingServer::onControlCommand(const std::string& streamId,
                                      const std::string& command,
                                      const daq::streaming_protocol::SignalIds& signalIds,
                                      std::string& errorMessage)
{
    if (signalIds.empty())
    {
        LOG_W("Signal list is empty, reject command", streamId);
        errorMessage = "Signal list is empty";
        return -1;
    }

    auto clientIter = std::find_if(std::begin(clients),
                                   std::end(clients),
                                   [&streamId](const auto& pair)
                                   {
                                       return pair.first->id() == streamId;
                                   });

    if (clientIter == std::end(clients))
    {
        LOG_W("Unknown streamId: {}, reject command", streamId);
        errorMessage = "Unknown streamId:  '" + streamId + "'";
        return -1;
    }

    if (command == "subscribe" || command == "unsubscribe")
    {
        size_t unknownSignalsCount = 0;
        std::string message = "Command '" + command + "' failed for unknown signals:\n";
        for (const auto& signalId : signalIds)
        {
            if (auto signalIter = clientIter->second.find(signalId); signalIter != clientIter->second.end())
            {
                // wasn't subscribed by requester client
                if (command == "subscribe" && !signalIter->second->isSubscribed())
                {
                    // wasn't subscribed by any client
                    if (!isSignalSubscribed(signalId) && onSubscribeCallback)
                    {
                        onSubscribeCallback(signalId);
                    }
                    signalIter->second->setSubscribed(true);
                }
                // was subscribed by requester client
                if (command == "unsubscribe" && signalIter->second->isSubscribed())
                {
                    signalIter->second->setSubscribed(false);
                    // became not subscribed by any client
                    if (!isSignalSubscribed(signalId) && onUnsubscribeCallback)
                    {
                        onUnsubscribeCallback(signalId);
                    }
                }
            }
            else
            {
                unknownSignalsCount++;
                message.append(signalId + "\n");
            }
        }

        if (unknownSignalsCount > 0)
        {
            LOG_W("{}", message);
            errorMessage = message;
            return -1;
        }
    }
    else
    {
        LOG_W("Unknown control command: {}", command);
        errorMessage = "Unknown command: " + command;
        return -1;
    }
    return 0;

}

void StreamingServer::writeProtocolInfo(const daq::streaming_protocol::StreamWriterPtr& writer)
{
    nlohmann::json msg;
    msg[METHOD] = META_METHOD_APIVERSION;
    msg[PARAMS][VERSION] = OPENDAQ_LT_STREAM_VERSION;
    writer->writeMetaInformation(0, msg);
}

void StreamingServer::writeSignalsAvailable(const daq::streaming_protocol::StreamWriterPtr& writer, const ListPtr<ISignal>& signals)
{
    std::vector<std::string> signalIds;

    for (const auto& signal : signals)
        signalIds.push_back(signal.getGlobalId());

    nlohmann::json msg;
    msg[METHOD] = META_METHOD_AVAILABLE;
    msg[PARAMS][META_SIGNALIDS] = signalIds;
    writer->writeMetaInformation(0, msg);
}

void StreamingServer::writeInit(const streaming_protocol::StreamWriterPtr& writer)
{
    nlohmann::json initMeta;
    initMeta[METHOD] = META_METHOD_INIT;
    initMeta[PARAMS][META_STREAMID] = writer->id();

    nlohmann::json jsonRpcHttp;
    jsonRpcHttp["httpMethod"] = "POST";
    jsonRpcHttp["httpPath"] = "/";
    jsonRpcHttp["httpVersion"] = "1.1";
    jsonRpcHttp["port"] = std::to_string(controlServer->getPort());

    nlohmann::json commandInterfaces;
    commandInterfaces["jsonrpc-http"] = jsonRpcHttp;

    initMeta[PARAMS][COMMANDINTERFACES] = commandInterfaces;
    writer->writeMetaInformation(0, initMeta);
}

bool StreamingServer::isSignalSubscribed(const std::string& signalId) const
{
    bool result = false;
    for (const auto& [_, signals] : clients)
    {
        if (auto iter = signals.find(signalId); iter != signals.end())
            result = result || iter->second->isSubscribed();
    }
    return result;
}

END_NAMESPACE_OPENDAQ_WEBSOCKET_STREAMING
