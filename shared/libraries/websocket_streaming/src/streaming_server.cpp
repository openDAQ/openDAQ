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
}

void StreamingServer::start(uint16_t port, uint16_t controlPort)
{
    if (serverRunning)
        return;

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

    this->serverThread = std::thread([this]()
                                     {
                                         this->ioContext.run();
                                         LOG_I("Websocket streaming server thread finished");
                                     });

    serverRunning = true;
}

void StreamingServer::stop()
{
    if (!serverRunning)
        return;

    ioContext.stop();

    if (this->server)
        this->server->stop();
    if (this->controlServer)
        this->controlServer->stop();

    if (serverThread.get_id() != std::this_thread::get_id())
    {
        if (!serverThread.joinable())
        {
            LOG_W("Websocket streaming server thread is not joinable");
        }
        else
        {
            serverThread.join();
            LOG_I("Websocket streaming server thread joined");
        }
    }
    else
    {
        LOG_C("Websocket streaming server thread cannot join itself");
    }
    serverRunning = false;

    this->server.reset();
    this->controlServer.reset();
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

void StreamingServer::unicastPacket(const std::string& streamId,
                                    const std::string& signalId,
                                    const PacketPtr& packet)
{
    if (auto clientIt = clients.find(streamId); clientIt != clients.end())
    {
        auto signals = clientIt->second.second;
        if (auto signalIt = signals.find(streamId); signalIt != signals.end())
            signalIt->second->writeDaqPacket(packet);
    }
}

void StreamingServer::broadcastPacket(const std::string& signalId, const PacketPtr& packet)
{
    for (auto& [_, client] : clients)
    {
        auto signals = client.second;
        if (auto signalIter = signals.find(signalId); signalIter != signals.end())
        {
            signalIter->second->writeDaqPacket(packet);
        }
    }
}

DataRuleType StreamingServer::getSignalRuleType(const SignalPtr& signal)
{
    auto descriptor = signal.getDescriptor();
    if (!descriptor.assigned() || !descriptor.getRule().assigned())
    {
        throw InvalidParameterException("Unknown signal rule");
    }
    return descriptor.getRule().getType();
}

void StreamingServer::addToOutputSignals(const SignalPtr& signal,
                                         SignalMap& outputSignals,
                                         const StreamWriterPtr& writer)
{
    auto createOutputDomainSignal = [&writer, this](const SignalPtr& domainSignal)
        -> std::shared_ptr<OutputDomainSignalBase>
    {
        auto tableId = domainSignal.getGlobalId();
        const auto domainSignalRuleType = getSignalRuleType(domainSignal);

        if (domainSignalRuleType == DataRuleType::Linear)
        {
            return std::make_shared<OutputLinearDomainSignal>(writer, domainSignal, tableId, logCallback);
        }
        else
        {
            throw InvalidParameterException("Unsupported domain signal rule type");
        }
    };

    auto domainSignal = signal.getDomainSignal();
    if (domainSignal.assigned())
    {
        auto domainSignalId = domainSignal.getGlobalId();

        OutputDomainSignaBaselPtr outputDomainSignal;
        if (const auto& outputSignalIt = outputSignals.find(domainSignalId); outputSignalIt != outputSignals.end())
        {
            outputDomainSignal = std::dynamic_pointer_cast<OutputDomainSignalBase>(outputSignalIt->second);
            if (!outputDomainSignal)
                throw NoInterfaceException("Registered output signal {} is not of domain type", domainSignalId);
        }
        else
        {
            outputDomainSignal = createOutputDomainSignal(domainSignal);
            outputSignals.insert({domainSignalId, outputDomainSignal});
        }

        auto tableId = domainSignalId;

        const auto domainSignalRuleType = getSignalRuleType(domainSignal);
        if (domainSignalRuleType == DataRuleType::Linear)
        {
            const auto valueSignalRuleType = getSignalRuleType(signal);
            if (valueSignalRuleType == DataRuleType::Explicit)
            {
                auto outputValueSignal =
                    std::make_shared<OutputSyncValueSignal>(writer, signal, outputDomainSignal, tableId, logCallback);
                outputSignals.insert({signal.getGlobalId(), outputValueSignal});
            }
            else if (valueSignalRuleType == DataRuleType::Constant)
            {
                auto outputValueSignal =
                    std::make_shared<OutputConstValueSignal>(writer, signal, outputDomainSignal, tableId, logCallback);
                outputSignals.insert({signal.getGlobalId(), outputValueSignal});
            }
            else
            {
                throw InvalidParameterException("Unsupported value signal rule type");
            }
        }
        else
        {
            throw InvalidParameterException("Unsupported domain signal rule type");
        }
    }
    else
    {
        if (const auto& outputSignalIt = outputSignals.find(signal.getGlobalId()); outputSignalIt == outputSignals.end())
        {
            outputSignals.insert({signal.getGlobalId(), createOutputDomainSignal(signal)});
        }
    }
}

void StreamingServer::doRead(const std::string& clientId, const daq::stream::StreamPtr& stream)
{
    std::weak_ptr<daq::stream::Stream> stream_weak = stream;

    // The callback is to be called in the thread that remains active as long as this object exists,
    // ensuring that the captured 'this' pointer is always valid.
    auto readDoneCallback = [this, stream_weak, clientId](const boost::system::error_code& ec, std::size_t bytesRead)
    {
        if (auto stream = stream_weak.lock())
            this->onReadDone(clientId, stream, ec, bytesRead);
    };
    stream->asyncReadSome(readDoneCallback);
}

void StreamingServer::onReadDone(const std::string& clientId,
                                 const daq::stream::StreamPtr& stream,
                                 const boost::system::error_code& ec,
                                 std::size_t bytesRead)
{
    if (ec) {
        removeClient(clientId);
        return;
    }

    // any incoming data is ignored
    stream->consume(bytesRead);
    doRead(clientId, stream);
}

void StreamingServer::removeClient(const std::string& clientId)
{
    LOG_I("client with id {} disconnected", clientId);

    if (auto iter = clients.find(clientId); iter != clients.end())
    {
        auto outputSignals = iter->second.second;
        for (const auto& [signalId, outputSignal] : outputSignals)
        {
            unsubscribeHandler(signalId, outputSignal);
        }
        clients.erase(iter);
    }
}

void StreamingServer::onAcceptInternal(const daq::stream::StreamPtr& stream)
{
    auto writer = std::make_shared<StreamWriter>(stream);
    writeProtocolInfo(writer);
    writeInit(writer);

    auto signals = List<ISignal>();
    auto filteredSignals = List<ISignal>();
    if (onAcceptCallback)
        signals = onAcceptCallback(writer);

    auto outputSignals = std::unordered_map<std::string, OutputSignalBasePtr>();
    for (const auto& signal : signals)
    {
        if (!signal.getPublic())
            continue;

        filteredSignals.pushBack(signal);
        
        try
        {
            addToOutputSignals(signal, outputSignals, writer);
        }
        catch (const DaqException& e)
        {
            LOG_W("Failed to create an output websocket signal: {}", e.what());
        }
    }

    auto clientId = stream->endPointUrl();
    LOG_I("New client connected. Stream Id: {}", clientId);
    clients.insert({clientId, {writer, outputSignals}});

    writeSignalsAvailable(writer, filteredSignals);

    doRead(clientId, stream);
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

    auto clientIter = clients.find(streamId);
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
            auto signals = clientIter->second.second;
            if (auto signalIter = signals.find(signalId); signalIter != signals.end())
            {
                if (command == "subscribe")
                    subscribeHandler(signalId, signalIter->second);
                else if (command == "unsubscribe")
                    unsubscribeHandler(signalId, signalIter->second);
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
    jsonRpcHttp["Port"] = std::to_string(controlServer->getPort());

    nlohmann::json commandInterfaces;
    commandInterfaces["jsonrpc-http"] = jsonRpcHttp;

    initMeta[PARAMS][COMMANDINTERFACES] = commandInterfaces;
    writer->writeMetaInformation(0, initMeta);
}

bool StreamingServer::isSignalSubscribed(const std::string& signalId) const
{
    bool result = false;
    for (const auto& [_, client] : clients)
    {
        auto signals = client.second;
        if (auto iter = signals.find(signalId); iter != signals.end())
            result = result || iter->second->isSubscribed();
    }
    return result;
}

void StreamingServer::subscribeHandler(const std::string& signalId, OutputSignalBasePtr signal)
{
    // wasn't subscribed by any client
    if (!isSignalSubscribed(signalId))
    {
        if (signal->isDataSignal() && onSubscribeCallback)
            onSubscribeCallback(signal->getDaqSignal());
    }

    signal->setSubscribed(true);
}

void StreamingServer::unsubscribeHandler(const std::string& signalId, OutputSignalBasePtr signal)
{
    signal->setSubscribed(false);

    // became not subscribed by any client
    if (!isSignalSubscribed(signalId))
    {
        if (signal->isDataSignal() && onUnsubscribeCallback)
            onUnsubscribeCallback(signal->getDaqSignal());
    }
}

END_NAMESPACE_OPENDAQ_WEBSOCKET_STREAMING
