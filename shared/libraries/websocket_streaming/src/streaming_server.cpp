#include <nlohmann/json.hpp>

#include <streaming_protocol/Defines.h>
#include <streaming_protocol/Logging.hpp>
#include <streaming_protocol/jsonrpc_defines.hpp>

#include "websocket_streaming/streaming_server.h"
#include <opendaq/event_packet_ids.h>
#include <opendaq/event_packet_params.h>
#include <opendaq/custom_log.h>
#include <opendaq/ids_parser.h>

#include <opendaq/data_descriptor_factory.h>

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

void StreamingServer::onStartSignalsRead(const OnStartSignalsReadCallback& callback)
{
    onStartSignalsReadCallback = callback;
}

void StreamingServer::onStopSignalsRead(const OnStopSignalsReadCallback& callback)
{
    onStopSignalsReadCallback = callback;
}

void StreamingServer::broadcastPacket(const std::string& signalId, const PacketPtr& packet)
{
    std::scoped_lock lock(sync);

    for (auto& [_, client] : clients)
    {
        auto writer = client.first;
        auto& outputSignals = client.second;

        if (auto signalIter = outputSignals.find(signalId); signalIter != outputSignals.end())
        {
            auto outputSignal = signalIter->second;

            if (auto eventPacket = packet.asPtrOrNull<IEventPacket>(); eventPacket.assigned())
            {
                if (eventPacket.getEventId() == event_packet_id::DATA_DESCRIPTOR_CHANGED)
                {
                    handleDataDescriptorChanges(outputSignal, outputSignals, writer, eventPacket);
                }
                else
                {
                    STREAMING_PROTOCOL_LOG_W("Event type {} is not supported by streaming.", eventPacket.getEventId());
                }
            }
            else if (auto dataPacket = packet.asPtrOrNull<IDataPacket>(); dataPacket.assigned())
            {
                outputSignal->writeDaqDataPacket(dataPacket);
            }
        }
    }
}

DataRuleType StreamingServer::getSignalRuleType(const SignalPtr& signal)
{
    auto descriptor = signal.getDescriptor();
    if (!descriptor.assigned() || !descriptor.getRule().assigned())
    {
        throw InvalidParameterException(R"(Signal "{}" has incomplete  descriptor - unknown signal rule)", signal.getGlobalId());
    }
    return descriptor.getRule().getType();
}

OutputDomainSignalBasePtr StreamingServer::addUpdateOrFindDomainSignal(const SignalPtr& domainSignal,
                                                                       SignalMap& outputSignals,
                                                                       const StreamWriterPtr& writer)
{
    auto domainSignalId = domainSignal.getGlobalId();

    OutputDomainSignalBasePtr outputDomainSignal;
    if (const auto& outputSignalIt = outputSignals.find(domainSignalId); outputSignalIt != outputSignals.end())
    {
        auto outputSignal = outputSignalIt->second;

        if (std::dynamic_pointer_cast<OutputNullSignal>(outputSignal))
        {
            // replace previously added incomplete placeholder signal
            outputDomainSignal = createOutputDomainSignal(domainSignal, domainSignal.getGlobalId(), writer);
            outputSignals[domainSignalId] = outputDomainSignal;
        }
        else
        {
            // find previously added complete output domain signal
            outputDomainSignal = std::dynamic_pointer_cast<OutputDomainSignalBase>(outputSignal);
            if (!outputDomainSignal)
                throw NoInterfaceException("Previously registered domain output signal {} is not of domain type", domainSignalId);
        }
    }
    else
    {
        // signal wasn't added before so add it now
        outputDomainSignal = createOutputDomainSignal(domainSignal, domainSignal.getGlobalId(), writer);
        outputSignals.insert({domainSignalId, outputDomainSignal});
    }

    return outputDomainSignal;
}

void StreamingServer::addToOutputSignals(const SignalPtr& signal,
                                         SignalMap& outputSignals,
                                         const StreamWriterPtr& writer)
{
    auto domainSignal = signal.getDomainSignal();

    if (domainSignal.assigned())
    {
        // if domain is assigned then signal is considered as value signal
        OutputDomainSignalBasePtr outputDomainSignal = addUpdateOrFindDomainSignal(domainSignal, outputSignals, writer);

        auto tableId = domainSignal.getGlobalId().toStdString();
        const auto domainSignalRuleType = getSignalRuleType(domainSignal);
        if (domainSignalRuleType == DataRuleType::Linear)
        {
            auto outputValueSignal = createOutputValueSignal(signal, outputDomainSignal, tableId, writer);
            outputSignals.insert_or_assign(signal.getGlobalId(), outputValueSignal);
        }
        else
        {
            throw InvalidParameterException("Unsupported domain signal rule type - only domain signals with linear rule type are supported in LT-streaming");
        }
    }
    else
    {
        // if domain is not assigned then signal is considered as domain signal itself
        addUpdateOrFindDomainSignal(signal, outputSignals, writer);
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

void StreamingServer::startReadSignals(const ListPtr<ISignal>& signals)
{
    if (onStartSignalsReadCallback && signals.getCount() > 0)
        onStartSignalsReadCallback(signals);
}

void StreamingServer::stopReadSignals(const ListPtr<ISignal>& signals)
{
    if (onStopSignalsReadCallback && signals.getCount() > 0)
        onStopSignalsReadCallback(signals);
}

void StreamingServer::removeClient(const std::string& clientId)
{
    LOG_I("client with id {} disconnected", clientId);

    auto signalsToStopRead = List<ISignal>();
    {
        std::scoped_lock lock(sync);
        if (auto iter = clients.find(clientId); iter != clients.end())
        {
            const auto& outputSignals = iter->second.second;
            for (const auto& [signalId, outputSignal] : outputSignals)
            {
                if (unsubscribeHandler(signalId, outputSignal) && outputSignal->isDataSignal())
                    signalsToStopRead.pushBack(outputSignal->getDaqSignal());
            }
            clients.erase(iter);
        }
    }

    stopReadSignals(signalsToStopRead);
}

void StreamingServer::onAcceptInternal(const daq::stream::StreamPtr& stream)
{
    auto writer = std::make_shared<StreamWriter>(stream);
    writeProtocolInfo(writer);
    writeInit(writer);

    auto signals = List<ISignal>();

    if (onAcceptCallback)
        signals = onAcceptCallback(writer);

    auto clientId = stream->endPointUrl();
    LOG_I("New client connected. Stream Id: {}", clientId);
    {
        std::scoped_lock lock(sync);
        clients.insert({clientId, {writer, std::unordered_map<std::string, OutputSignalBasePtr>()}});
        auto& outputSignals = clients.at(clientId).second;
        publishSignalsToClient(writer, signals, outputSignals);
    }

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
    if (command != "subscribe" && command != "unsubscribe")
    {
        LOG_W("Unknown control command: {}", command);
        errorMessage = "Unknown command: " + command;
        return -1;
    }

    size_t unknownSignalsCount = 0;
    std::string message = "Command '" + command + "' failed for unknown signals:\n";

    auto signalsToStartRead = List<ISignal>();
    auto signalsToStopRead = List<ISignal>();

    {
        std::scoped_lock lock(sync);

        auto clientIter = clients.find(streamId);
        if (clientIter == std::end(clients))
        {
            LOG_W("Unknown streamId: {}, reject command", streamId);
            errorMessage = "Unknown streamId:  '" + streamId + "'";
            return -1;
        }

        for (const auto& signalId : signalIds)
        {
            auto& outputSignals = clientIter->second.second;
            if (auto signalIter = outputSignals.find(signalId); signalIter != outputSignals.end())
            {
                auto outputSignal = signalIter->second;
                if (command == "subscribe")
                {
                    if (subscribeHandler(signalId, outputSignal) && outputSignal->isDataSignal())
                        signalsToStartRead.pushBack(outputSignal->getDaqSignal());
                }
                else if (command == "unsubscribe")
                {
                    if (unsubscribeHandler(signalId, outputSignal) && outputSignal->isDataSignal())
                        signalsToStopRead.pushBack(outputSignal->getDaqSignal());
                }
            }
            else
            {
                unknownSignalsCount++;
                message.append(signalId + "\n");
            }
        }
    }

    if (command == "subscribe")
        startReadSignals(signalsToStartRead);

    if (command == "unsubscribe")
        stopReadSignals(signalsToStopRead);

    if (unknownSignalsCount > 0)
    {
        LOG_W("{}", message);
        errorMessage = message;
        return -1;
    }
    else
    {
        return 0;
    }
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

bool StreamingServer::subscribeHandler(const std::string& signalId, OutputSignalBasePtr signal)
{
    // returns true if signal wasn't subscribed by any client
    bool result = !isSignalSubscribed(signalId);
    signal->setSubscribed(true);

    return result;
}

bool StreamingServer::unsubscribeHandler(const std::string& signalId, OutputSignalBasePtr signal)
{
    if (!signal->isSubscribed())
        return false;
    signal->setSubscribed(false);

    // returns true if signal became not subscribed by any client
    return !isSignalSubscribed(signalId);
}

void StreamingServer::addSignals(const ListPtr<ISignal>& signals)
{
    std::scoped_lock lock(sync);
    for (auto& [_, client] : clients)
    {
        auto writer = client.first;
        auto& outputSignals = client.second;

        publishSignalsToClient(writer, signals, outputSignals);
    }
}

void StreamingServer::removeComponentSignals(const StringPtr& componentId)
{
    auto signalsToStopRead = List<ISignal>();
    {
        std::scoped_lock lock(sync);
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
                    if (unsubscribeHandler(signalId, outputSignal) && outputSignal->isDataSignal())
                        signalsToStopRead.pushBack(outputSignal->getDaqSignal());
                }
            }

            if (!signalsToRemove.empty())
            {
                writeSignalsUnavailable(writer, signalsToRemove);
                for (const auto& signalId : signalsToRemove)
                    outputSignals.erase(signalId);
            }
        }
    }

    stopReadSignals(signalsToStopRead);
}

void StreamingServer::updateComponentSignals(const DictPtr<IString, ISignal>& signals, const StringPtr& componentId)
{
    auto signalsToStopRead = List<ISignal>();
    {
        std::scoped_lock lock(sync);
        auto updatedComponentId = componentId.toStdString();

        for (auto& [_, client] : clients)
        {
            auto writer = client.first;
            auto& outputSignals = client.second;

            auto signalsToAdd = List<ISignal>();
            for (const auto& [signalId, signal] : signals)
            {
                if (auto iter = outputSignals.find(signalId.toStdString()); iter == outputSignals.end())
                    signalsToAdd.pushBack(signal);
            }
            if (signalsToAdd.getCount() > 0)
                publishSignalsToClient(writer, signalsToAdd, outputSignals);

            std::vector<std::string> signalsToRemove;
            for (const auto& [signalId, outputSignal] : outputSignals)
            {
                // signal is a descendant of updated component
                if (IdsParser::isNestedComponentId(updatedComponentId, signalId) &&
                    (!signals.hasKey(signalId) || !signals.get(signalId).getPublic()))
                {
                    signalsToRemove.push_back(signalId);
                    if (unsubscribeHandler(signalId, outputSignal) && outputSignal->isDataSignal())
                        signalsToStopRead.pushBack(outputSignal->getDaqSignal());
                }
            }
            if (!signalsToRemove.empty())
            {
                writeSignalsUnavailable(writer, signalsToRemove);
                for (const auto& signalId : signalsToRemove)
                    outputSignals.erase(signalId);
            }
        }
    }

    stopReadSignals(signalsToStopRead);
}

/// Due to the type-specific "hardcoded" implementations of output signals,
/// when a signal descriptor change is incompatible with the existing output signal (e.g. sample type or rule changed),
/// the signal is replaced with a newly created one. To handle this correctly on both the server and client sides,
/// the old signal is marked as unavailable by server, and the new one is made available under the same signal ID.
/// This ensures any cached signal details across server and client implementations are cleared.
/// As a result, the corresponding signal in the LT pseudo device is also removed and the new one appeared.
void StreamingServer::handleDataDescriptorChanges(OutputSignalBasePtr& outputSignal,
                                                  SignalMap& outputSignals,
                                                  const StreamWriterPtr& writer,
                                                  const EventPacketPtr& packet)
{
    const auto params = packet.getParameters();
    DataDescriptorPtr valueDescriptorParam = params[event_packet_param::DATA_DESCRIPTOR];
    DataDescriptorPtr domainDescriptorParam = params[event_packet_param::DOMAIN_DATA_DESCRIPTOR];
    const bool valueDescriptorChanged = valueDescriptorParam.assigned();
    const bool domainDescriptorChanged = domainDescriptorParam.assigned();
    const DataDescriptorPtr newValueDescriptor = valueDescriptorParam != NullDataDescriptor() ? valueDescriptorParam : nullptr;
    const DataDescriptorPtr newDomainDescriptor = domainDescriptorParam != NullDataDescriptor() ? domainDescriptorParam : nullptr;
    bool subscribed = outputSignal->isSubscribed();

    if (auto placeholderValueSignal = std::dynamic_pointer_cast<OutputNullSignal>(outputSignal))
    {
        if (valueDescriptorChanged && newValueDescriptor.assigned() ||
            domainDescriptorChanged && newDomainDescriptor.assigned())
            updateOutputPlaceholderSignal(outputSignal, outputSignals, writer, subscribed);
    }
    else
    {
        const auto daqValueSignal = outputSignal->getDaqSignal();
        const auto valueSignalId = daqValueSignal.getGlobalId();

        if (valueDescriptorChanged)
        {
            try
            {
                outputSignal->writeValueDescriptorChanges(newValueDescriptor);
            }
            catch (const DaqException& e)
            {
                writeSignalsUnavailable(writer, {valueSignalId});
                LOG_W("Failed to change value descriptor for signal {}, reason: {}", valueSignalId, e.what());
                outputSignals.insert_or_assign(valueSignalId, std::make_shared<OutputNullSignal>(daqValueSignal, logCallback));
                outputSignal = outputSignals.at(valueSignalId);
                if (newValueDescriptor.assigned())
                    updateOutputPlaceholderSignal(outputSignal, outputSignals, writer, false);
                writeSignalsAvailable(writer, {valueSignalId});
                outputSignal->setSubscribed(subscribed);
            }
        }

        if (domainDescriptorChanged)
        {
            if (const auto daqDomainSignal = daqValueSignal.getDomainSignal(); daqDomainSignal.assigned())
            {
                try
                {
                    outputSignal->writeDomainDescriptorChanges(newDomainDescriptor);
                }
                catch (const DaqException& e)
                {
                    LOG_W("Failed to change domain descriptor for signal {}, reason: {}", valueSignalId, e.what());
                    const auto domainSignalId = daqDomainSignal.getGlobalId();
                    writeSignalsUnavailable(writer, {domainSignalId, valueSignalId});
                    outputSignals.insert_or_assign(valueSignalId, std::make_shared<OutputNullSignal>(daqValueSignal, logCallback));
                    outputSignals.insert_or_assign(domainSignalId, std::make_shared<OutputNullSignal>(daqDomainSignal, logCallback));
                    outputSignal = outputSignals.at(valueSignalId);
                    if (newDomainDescriptor.assigned())
                        updateOutputPlaceholderSignal(outputSignal, outputSignals, writer, false);
                    writeSignalsAvailable(writer, {valueSignalId, domainSignalId});
                    outputSignal->setSubscribed(subscribed);
                }
            }
        }
    }
}

void StreamingServer::updateOutputPlaceholderSignal(OutputSignalBasePtr& outputSignal,
                                                    SignalMap& outputSignals,
                                                    const StreamWriterPtr& writer,
                                                    bool subscribed)
{
    auto daqSignal = outputSignal->getDaqSignal();
    auto signalId = daqSignal.getGlobalId().toStdString();

    LOG_I("Parameters of unsupported signal {} has been changed, check if it is supported now ...", daqSignal.getGlobalId());
    try
    {
        addToOutputSignals(daqSignal, outputSignals, writer);
        outputSignal = outputSignals.at(signalId);
        outputSignal->setSubscribed(subscribed);
    }
    catch (const DaqException& e)
    {
        LOG_W("Failed to re-create an output LT streaming signal for {}, reason: {}", daqSignal.getGlobalId(), e.what());
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
            LOG_W("Failed to create an output LT streaming signal for {}, reason: {}", signalId, e.what());
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
