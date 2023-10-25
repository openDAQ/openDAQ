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
    loggerComponent = this->logger.addComponent("StreamingServer");
    logCallback = [this](spdlog::source_loc location, spdlog::level::level_enum level, const char* msg) {
        this->loggerComponent.logMessage(SourceLocation{location.filename, location.line, location.funcname}, msg, static_cast<LogLevel>(level));
    };
}

StreamingServer::~StreamingServer()
{
    stop();
    logger.removeComponent("StreamingServer");
}

void StreamingServer::start(uint16_t port)
{
    this->port = port;

    ioContext.restart();

    auto acceptFunc = [this](StreamPtr stream) { this->onAcceptInternal(stream); };

    this->server = std::make_unique<daq::stream::WebsocketServer>(ioContext, acceptFunc, port);
    this->server->start();
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

void StreamingServer::onAcceptInternal(const daq::stream::StreamPtr& stream)
{
    auto writer = std::make_shared<StreamWriter>(stream);
    writeProtocolInfo(writer);

    auto signals = List<ISignal>();
    if (onAcceptCallback)
        signals = onAcceptCallback(writer);

    writeSignalsAvailable(writer, signals);

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

    clients.insert({writer, outputSignals});
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

END_NAMESPACE_OPENDAQ_WEBSOCKET_STREAMING
