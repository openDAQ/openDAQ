#include "websocket_streaming/streaming_client.h"
#include <websocket_streaming/signal_descriptor_converter.h>
#include <regex>
#include <streaming_protocol/Logging.hpp>
#include <streaming_protocol/ProtocolHandler.hpp>
#include "stream/WebsocketClientStream.hpp"
#include "streaming_protocol/SignalContainer.hpp"
#include "opendaq/custom_log.h"

BEGIN_NAMESPACE_OPENDAQ_WEBSOCKET_STREAMING

using namespace daq::stream;
using namespace daq::streaming_protocol;

StreamingClient::StreamingClient(const ContextPtr& context, const std::string& connectionString)
    : logger(context.getLogger())
    , loggerComponent( logger.assigned() ? logger.getOrAddComponent("StreamingClient") : throw ArgumentNullException("Logger must not be null") )
    , logCallback( [this](spdlog::source_loc location, spdlog::level::level_enum level, const char* msg) {
        this->loggerComponent.logMessage(SourceLocation{location.filename, location.line, location.funcname}, msg, static_cast<LogLevel>(level));
    })
    , signalContainer(logCallback)
{
    parseConnectionString(connectionString);
}

StreamingClient::StreamingClient(const ContextPtr& context, const std::string& host, uint16_t port, const std::string& target)
    : logger(context.getLogger())
    , loggerComponent( logger.assigned() ? logger.getOrAddComponent("StreamingClient") : throw ArgumentNullException("Logger must not be null") )
    , logCallback( [this](spdlog::source_loc location, spdlog::level::level_enum level, const char* msg) {
        this->loggerComponent.logMessage(SourceLocation{location.filename, location.line, location.funcname}, msg, static_cast<LogLevel>(level));
    })
    , host(host)
    , port(port)
    , target(target)
    , signalContainer(logCallback)
{
}

StreamingClient::~StreamingClient()
{
    disconnect();
}

bool StreamingClient::connect()
{
    if (connected)
        return true;
    if (host.empty() || port == 0)
        return false;

    connected = false;

    auto signalMetaCallback = [this](const SubscribedSignal& subscribedSignal, const std::string& method, const nlohmann::json& params)
    { onSignalMeta(subscribedSignal, method, params); };

    auto protocolMetaCallback = [this](ProtocolHandler& protocolHandler, const std::string& method, const nlohmann::json& params)
    { onProtocolMeta(protocolHandler, method, params); };

    auto messageCallback = [this](const SubscribedSignal& subscribedSignal, uint64_t timeStamp, const uint8_t* data, size_t size)
    { onMessage(subscribedSignal, timeStamp, data, size); };

    signalContainer.setSignalMetaCb(signalMetaCallback);
    signalContainer.setDataAsRawCb(messageCallback);

    auto clientStream = std::make_unique<WebsocketClientStream>(ioContext, host, std::to_string(port), target);
    protocolHandler = std::make_shared<ProtocolHandler>(ioContext, signalContainer, protocolMetaCallback, logCallback);
    std::unique_lock<std::mutex> lock(clientMutex);
    protocolHandler->startWithSyncInit(std::move(clientStream));

    ioContext.restart();
    clientThread = std::thread([this]() { ioContext.run(); });

    conditionVariable.wait_for(lock, connectTimeout, [this]() { return connected; });
    return connected;
}

void StreamingClient::disconnect()
{
    if (clientThread.joinable())
    {
        ioContext.stop();
        clientThread.join();
        connected = false;
    }
}

void StreamingClient::onPacket(const OnPacketCallback& callack)
{
    onPacketCallback = callack;
}

void StreamingClient::onNewSignal(const OnSignalCallback& callback)
{
    onNewSignalCallback = callback;
}

void StreamingClient::onSignalUpdated(const OnSignalCallback& callback)
{
    onSignalUpdatedCallback = callback;
}

void StreamingClient::onDomainDescriptor(const OnDomainDescriptorCallback& callback)
{
    onDomainDescriptorCallback = callback;
}

void StreamingClient::onAvailableStreamingSignals(const OnAvailableSignalsCallback& callback)
{
    onAvailableStreamingSignalsCb = callback;
}

void StreamingClient::onAvailableDeviceSignals(const OnAvailableSignalsCallback& callback)
{
    onAvailableDeviceSignalsCb = callback;
}

void StreamingClient::onFindSignal(const OnFindSignalCallback& callback)
{
    onFindSignalCallback = callback;
}

std::string StreamingClient::getHost()
{
    return host;
}

uint16_t StreamingClient::getPort()
{
    return port;
}

std::string StreamingClient::getTarget()
{
    return target;
}

bool StreamingClient::isConnected()
{
    return connected;
}

void StreamingClient::setConnectTimeout(std::chrono::milliseconds timeout)
{
    this->connectTimeout = timeout;
}

void StreamingClient::parseConnectionString(const std::string& url)
{
    // this is not great but it is convenient until we have a way to pass configuration parameters to a client device

    host = "";
    port = 7414;
    target = "/";

    std::smatch match;
    std::string suffix;

    auto regexHostname = std::regex("^(.*:\\/\\/)?([^:\\/\\s]+)");
    if (std::regex_search(url, match, regexHostname))
        host = match[2];
    else
        return;

    auto regexPort = std::regex("^:(\\d+)");
    suffix = match.suffix().str();
    if (std::regex_search(suffix, match, regexPort))
    {
        port = std::stoi(match[1]);
        suffix = match.suffix().str();
    }

    auto regexTarget = std::regex("^\\/?[^\\?]+");
    if (std::regex_search(suffix, match, regexTarget))
        target = match[0];
    else
        return;
}

void StreamingClient::onSignalMeta(const SubscribedSignal& subscribedSignal, const std::string& method, const nlohmann::json& params)
{
    if (method == daq::streaming_protocol::META_METHOD_SIGNAL)
        onSignal(subscribedSignal, params);
}

void StreamingClient::onProtocolMeta(daq::streaming_protocol::ProtocolHandler& protocolHandler,
                                     const std::string& method,
                                     const nlohmann::json& params)
{
    if (method == daq::streaming_protocol::META_METHOD_AVAILABLE)
    {
        std::vector<std::string> signalIds;
        auto availableSignals = params.find(META_SIGNALIDS);

        if (availableSignals != params.end() && availableSignals->is_array())
        {
            for (const auto& arrayItem : *availableSignals)
                signalIds.push_back(arrayItem);

            onAvailableDeviceSignalsCb(signalIds);
            onAvailableStreamingSignalsCb(signalIds);
        }

        std::unique_lock<std::mutex> lock(clientMutex);
        connected = true;
        conditionVariable.notify_all();
    }
}

void StreamingClient::onMessage(const daq::streaming_protocol::SubscribedSignal& subscribedSignal,
                                uint64_t timeStamp,
                                const uint8_t* data,
                                size_t size)
{
    std::string id = subscribedSignal.signalId();
    const auto& signalIter = signals.find(id);
    if (signalIter == signals.end())
        return;

    auto packet = signalIter->second->asPacket(timeStamp, data, size);
    onPacketCallback(id, packet);
}

void StreamingClient::setDataSignal(const daq::streaming_protocol::SubscribedSignal& subscribedSignal)
{
    const auto id = subscribedSignal.signalId();

    auto sInfo = SignalDescriptorConverter::ToDataDescriptor(subscribedSignal);
    if (signals.count(id) == 0)
    {
        auto descriptor = sInfo.dataDescriptor;
        onNewSignalCallback(id, sInfo);

        auto inputSignal = std::make_shared<InputSignal>();
        inputSignal->setDataDescriptor(subscribedSignal);
        signals[id] = inputSignal;
    }
    else
    {
        onSignalUpdatedCallback(id, sInfo);
        signals[id]->setDataDescriptor(subscribedSignal);
    }
}

void StreamingClient::setTimeSignal(const daq::streaming_protocol::SubscribedSignal& subscribedSignal)
{
    std::string tableId = subscribedSignal.tableId();

    if (signals.count(tableId) == 0)
        return;

    auto inputSignal = signals[tableId];
    inputSignal->setDomainDescriptor(subscribedSignal);

    // Sets the descriptors when first connecting
    if (!connected)
    {
        auto domainDescriptor = inputSignal->getDomainSignalDescriptor();
        onDomainDescriptorCallback(tableId, domainDescriptor);
    }
}

void StreamingClient::publishSignal(const std::string& signalId)
{
    if (signals.count(signalId) == 0)
        return;

    auto inputSignal = signals[signalId];
    if (!inputSignal->hasDescriptors())
        return;

    auto eventPacket = inputSignal->createDecriptorChangedPacket();
    onPacketCallback(signalId, eventPacket);
}

void StreamingClient::onSignal(const daq::streaming_protocol::SubscribedSignal& subscribedSignal, const nlohmann::json& params)
{
    try
    {
        if (subscribedSignal.isTimeSignal())
            setTimeSignal(subscribedSignal);
        else
            setDataSignal(subscribedSignal);

        // signal meta information is always received by pairs of META_METHOD_SIGNAL:
        // first is meta for data signal, second is meta for artificial time signal.
        // we call "publishSignal" which generates event packet only after both meta are received
        // and all signal descriptors are updated.
        if (subscribedSignal.isTimeSignal())
            publishSignal(subscribedSignal.tableId());
    }
    catch (const DaqException& e)
    {
        LOG_W("Failed to interpret received input signal: {}.", e.what());
    }
}

END_NAMESPACE_OPENDAQ_WEBSOCKET_STREAMING
