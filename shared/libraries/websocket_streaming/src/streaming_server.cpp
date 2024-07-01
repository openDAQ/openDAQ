#include <nlohmann/json.hpp>

#include <streaming_protocol/Defines.h>
#include <streaming_protocol/Logging.hpp>
#include <streaming_protocol/jsonrpc_defines.hpp>

#include "websocket_streaming/streaming_server.h"
#include <opendaq/event_packet_ids.h>
#include <opendaq/event_packet_params.h>
#include <opendaq/custom_log.h>
#include <opendaq/ids_parser.h>

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

void StreamingServer::broadcastPacket(const std::string& signalId, const PacketPtr& packet)
{
    for (auto& [_, client] : clients)
    {
        auto writer = client.first;
        auto& outputSignals = client.second;

        if (auto signalIter = outputSignals.find(signalId); signalIter != outputSignals.end())
        {
            auto outputSignal = signalIter->second;
            auto eventPacket = packet.asPtrOrNull<IEventPacket>();

            if (eventPacket.assigned() && eventPacket.getEventId() == event_packet_id::DATA_DESCRIPTOR_CHANGED)
            {
                updateOutputValueSignal(outputSignal, outputSignals, writer);
            }
            outputSignal->writeDaqPacket(packet);
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
    auto domainSignal = signal.getDomainSignal();
    if (domainSignal.assigned())
    {
        auto domainSignalId = domainSignal.getGlobalId();

        OutputDomainSignalBasePtr outputDomainSignal;
        if (const auto& outputSignalIt = outputSignals.find(domainSignalId); outputSignalIt != outputSignals.end())
        {
            auto outputSignal = outputSignalIt->second;

            if (std::dynamic_pointer_cast<OutputNullSignal>(outputSignal))
            {
                outputDomainSignal = createOutputDomainSignal(domainSignal, domainSignal.getGlobalId(), writer);
                outputSignals[domainSignalId] = outputDomainSignal;
            }
            else
            {
                outputDomainSignal = std::dynamic_pointer_cast<OutputDomainSignalBase>(outputSignal);
                if (!outputDomainSignal)
                    throw NoInterfaceException("Registered output signal {} is not of domain type", domainSignalId);
            }
        }
        else
        {
            outputDomainSignal = createOutputDomainSignal(domainSignal, domainSignal.getGlobalId(), writer);
            outputSignals.insert({domainSignalId, outputDomainSignal});
        }

        auto tableId = domainSignalId.toStdString();

        const auto domainSignalRuleType = getSignalRuleType(domainSignal);
        if (domainSignalRuleType == DataRuleType::Linear)
        {
            auto outputValueSignal = createOutputValueSignal(signal, outputDomainSignal, tableId, writer);
            outputSignals.insert_or_assign(signal.getGlobalId(), outputValueSignal);
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
            auto outputDomainSignal = createOutputDomainSignal(signal, signal.getGlobalId(), writer);
            outputSignals.insert({signal.getGlobalId(), outputDomainSignal});
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

    if (onAcceptCallback)
        signals = onAcceptCallback(writer);

    auto outputSignals = std::unordered_map<std::string, OutputSignalBasePtr>();

    publishSignalsToClient(writer, signals, outputSignals);

    auto clientId = stream->endPointUrl();
    LOG_I("New client connected. Stream Id: {}", clientId);
    clients.insert({clientId, {writer, outputSignals}});
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

void StreamingServer::writeSignalsAvailable(const daq::streaming_protocol::StreamWriterPtr& writer,
                                            const std::vector<std::string>& signalIds)
{
    nlohmann::json msg;
    msg[METHOD] = META_METHOD_AVAILABLE;
    msg[PARAMS][META_SIGNALIDS] = signalIds;
    writer->writeMetaInformation(0, msg);
}

void StreamingServer::writeSignalsUnavailable(const daq::streaming_protocol::StreamWriterPtr& writer,
                                              const std::vector<std::string>& signalIds)
{
    nlohmann::json msg;
    msg[METHOD] = META_METHOD_UNAVAILABLE;
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

void StreamingServer::addSignals(const ListPtr<ISignal>& signals)
{
    for (auto& [_, client] : clients)
    {
        auto writer = client.first;
        auto& outputSignals = client.second;

        publishSignalsToClient(writer, signals, outputSignals);
    }
}

void StreamingServer::removeComponentSignals(const StringPtr& componentId)
{
    auto removedComponentId = componentId.toStdString();

    for (auto& [_, client] : clients)
    {
        auto writer = client.first;
        auto& outputSignals = client.second;

        std::vector<std::string> signalsToRemove;

        for (const auto& [signalId, outputSignal] : outputSignals)
        {
            // removed component is a signal, or signal is a descendant of removed component
            if (signalId == removedComponentId || IdsParser::isNestedComponentId(removedComponentId, signalId))
            {
                signalsToRemove.push_back(signalId);
                unsubscribeHandler(signalId, outputSignal);
            }
        }
        for (const auto& signalId : signalsToRemove)
        {
            outputSignals.erase(signalId);
        }

        writeSignalsUnavailable(writer, signalsToRemove);
    }
}

void StreamingServer::updateOutputValueSignal(OutputSignalBasePtr& outputSignal,
                                              SignalMap& outputSignals,
                                              const StreamWriterPtr& writer)
{
    auto placeholderSignal = std::dynamic_pointer_cast<OutputNullSignal>(outputSignal);

    if (placeholderSignal)
    {
        auto daqSignal = outputSignal->getDaqSignal();
        auto signalId = daqSignal.getGlobalId().toStdString();

        LOG_I("Parameters of unsupported signal {} has been changed, check if it is supported now ...", daqSignal.getGlobalId());
        try
        {
            addToOutputSignals(daqSignal, outputSignals, writer);
            outputSignal = outputSignals.at(signalId);

            if (placeholderSignal->isSubscribed())
                outputSignal->setSubscribed(true);
        }
        catch (const DaqException& e)
        {
            LOG_W("Failed to create an output LT streaming signal for {}: {}", daqSignal.getGlobalId(), e.what());
        }
    }
}

void StreamingServer::publishSignalsToClient(const StreamWriterPtr& writer,
                                             const ListPtr<ISignal>& signals,
                                             SignalMap& outputSignals)
{
    std::vector<std::string> filteredSignalsIds;
    for (const auto& daqSignal : signals)
    {
        if (!daqSignal.getPublic())
            continue;

        auto signalId = daqSignal.getGlobalId().toStdString();
        filteredSignalsIds.push_back(signalId);

        try
        {
            addToOutputSignals(daqSignal, outputSignals, writer);
        }
        catch (const DaqException& e)
        {
            LOG_W("Failed to create an output LT streaming signal for {}: {}", signalId, e.what());
            auto placeholderSignal = std::make_shared<OutputNullSignal>(daqSignal, logCallback);
            outputSignals.insert({signalId, placeholderSignal});
        }
    }

    writeSignalsAvailable(writer, filteredSignalsIds);
}

OutputDomainSignalBasePtr StreamingServer::createOutputDomainSignal(const SignalPtr& daqDomainSignal,
                                                                    const std::string& tableId,
                                                                    const StreamWriterPtr& writer)
{
    const auto domainSignalRuleType = getSignalRuleType(daqDomainSignal);

    if (domainSignalRuleType == DataRuleType::Linear)
        return std::make_shared<OutputLinearDomainSignal>(writer, daqDomainSignal, tableId, logCallback);
    else
        throw InvalidParameterException("Unsupported domain signal rule type");
}

OutputSignalBasePtr StreamingServer::createOutputValueSignal(const SignalPtr& daqSignal,
                                                             const OutputDomainSignalBasePtr& outputDomainSignal,
                                                             const std::string& tableId,
                                                             const StreamWriterPtr& writer)
{
    const auto valueSignalRuleType = getSignalRuleType(daqSignal);

    if (valueSignalRuleType == DataRuleType::Explicit)
        return std::make_shared<OutputSyncValueSignal>(writer, daqSignal, outputDomainSignal, tableId, logCallback);
    else if (valueSignalRuleType == DataRuleType::Constant)
        return std::make_shared<OutputConstValueSignal>(writer, daqSignal, outputDomainSignal, tableId, logCallback);
    else
        throw InvalidParameterException("Unsupported value signal rule type");
}

END_NAMESPACE_OPENDAQ_WEBSOCKET_STREAMING
