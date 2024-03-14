#include "websocket_streaming/streaming_client.h"
#include <websocket_streaming/signal_descriptor_converter.h>
#include <regex>
#include <streaming_protocol/Logging.hpp>
#include <streaming_protocol/ProtocolHandler.hpp>
#include "stream/WebsocketClientStream.hpp"
#include "stream/TcpClientStream.hpp"
#include "streaming_protocol/SignalContainer.hpp"
#include "opendaq/custom_log.h"
#include <opendaq/packet_factory.h>

BEGIN_NAMESPACE_OPENDAQ_WEBSOCKET_STREAMING

using namespace daq::stream;
using namespace daq::streaming_protocol;

StreamingClient::StreamingClient(const ContextPtr& context, const std::string& connectionString, bool useRawTcpConnection)
    : logger(context.getLogger())
    , loggerComponent( logger.assigned() ? logger.getOrAddComponent("StreamingClient") : throw ArgumentNullException("Logger must not be null") )
    , logCallback( [this](spdlog::source_loc location, spdlog::level::level_enum level, const char* msg) {
        this->loggerComponent.logMessage(SourceLocation{location.filename, location.line, location.funcname}, msg, static_cast<LogLevel>(level));
    })
    , signalContainer(logCallback)
    , useRawTcpConnection(useRawTcpConnection)
{
    parseConnectionString(connectionString);
}

StreamingClient::StreamingClient(const ContextPtr& context, const std::string& host, uint16_t port, const std::string& target, bool useRawTcpConnection)
    : logger(context.getLogger())
    , loggerComponent( logger.assigned() ? logger.getOrAddComponent("StreamingClient") : throw ArgumentNullException("Logger must not be null") )
    , logCallback( [this](spdlog::source_loc location, spdlog::level::level_enum level, const char* msg) {
        this->loggerComponent.logMessage(SourceLocation{location.filename, location.line, location.funcname}, msg, static_cast<LogLevel>(level));
    })
    , host(host)
    , port(port)
    , target(target)
    , signalContainer(logCallback)
    , useRawTcpConnection(useRawTcpConnection)
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

    std::unique_ptr<Stream> clientStream;
    if (useRawTcpConnection)
        clientStream = std::make_unique<TcpClientStream>(ioContext, host, std::to_string(port));
    else
        clientStream = std::make_unique<WebsocketClientStream>(ioContext, host, std::to_string(port), target);

    protocolHandler = std::make_shared<ProtocolHandler>(ioContext, signalContainer, protocolMetaCallback, logCallback);
    std::unique_lock<std::mutex> lock(clientMutex);
    protocolHandler->startWithSyncInit(std::move(clientStream));

    ioContext.restart();
    clientThread = std::thread([this]() { ioContext.run(); });

    conditionVariable.wait_for(lock, connectTimeout, [this]() { return connected; });

    if (connected)
    {
        std::vector<std::string> signalIds;
        for (const auto& [signalId,_] : signals)
        {
            signalIds.push_back(signalId);
            std::promise<void> signalInitPromise;
            std::future<void> signalInitFuture = signalInitPromise.get_future();
            signalInitializedStatus.insert_or_assign(
                signalId,
                std::make_tuple(
                    std::move(signalInitPromise),
                    std::move(signalInitFuture),
                    false,
                    false
                )
            );
        }

        // signal meta-information (signal description, tableId, related signals, etc.)
        // is published only for subscribed signals.
        // as workaround we temporarily subscribe all signals to receive signal meta-info
        // and initialize signal descriptors
        protocolHandler->subscribe(signalIds);

        const auto timeout = std::chrono::seconds(1);
        auto timeoutExpired = std::chrono::system_clock::now() + timeout;

        for (const auto& [id, params] : signalInitializedStatus)
        {
            auto status = std::get<1>(params).wait_until(timeoutExpired);
            if (status != std::future_status::ready)
            {
                LOG_W("signal {} has incomplete descriptors", id);
            }
        }

        // unsubscribe previously subscribed signals
        protocolHandler->unsubscribe(signalIds);
    }

    return connected;
}

void StreamingClient::disconnect()
{
    ioContext.stop();
    if (clientThread.joinable())
        clientThread.join();
    connected = false;
}

void StreamingClient::onPacket(const OnPacketCallback& callack)
{
    onPacketCallback = callack;
}

void StreamingClient::onSignalInit(const OnSignalCallback& callback)
{
    onSignalInitCallback = callback;
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

void StreamingClient::onSubscriptionAck(const OnSubsciptionAckCallback& callback)
{
    onSubscriptionAckCallback = callback;
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

EventPacketPtr StreamingClient::getDataDescriptorChangedEventPacket(const StringPtr& signalStringId)
{
    if (auto it = signals.find(signalStringId); it == signals.end())
        return DataDescriptorChangedEventPacket(nullptr, nullptr);
    else
        return it->second->createDecriptorChangedPacket();
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

    std::string signalId = subscribedSignal.signalId();
    if (auto it = signalInitializedStatus.find(signalId); it != signalInitializedStatus.end())
    {
        if (method == daq::streaming_protocol::META_METHOD_SUBSCRIBE)
        {
            // skips the first subscribe ACK from server and ignores ACKs for domain signals
            if (std::get<2>(it->second) && !subscribedSignal.isTimeSignal())
                onSubscriptionAckCallback(subscribedSignal.signalId(), true);
            else
                std::get<2>(it->second) = true;
        }
        else if (method == daq::streaming_protocol::META_METHOD_UNSUBSCRIBE)
        {
            // skips the first unsubscribe ACK from server and ignores ACKs for domain signals
            if (std::get<3>(it->second) && !subscribedSignal.isTimeSignal())
                onSubscriptionAckCallback(subscribedSignal.signalId(), false);
            else
                std::get<3>(it->second) = true;
        }
    }
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
            {
                std::string signalId = arrayItem;
                signalIds.push_back(signalId);

                if (auto signalIt = signals.find(signalId); signalIt == signals.end())
                {
                    auto inputSignal = std::make_shared<InputSignal>();
                    signals.insert({signalId, inputSignal});
                }
                else
                {
                    LOG_E("Received duplicate of available signal. ID is {}.", signalId);
                }
            }

            onAvailableDeviceSignalsCb(signalIds);
            onAvailableStreamingSignalsCb(signalIds);
        }

        std::unique_lock<std::mutex> lock(clientMutex);
        connected = true;
        conditionVariable.notify_all();
    }
}

void StreamingClient::subscribeSignals(const std::vector<std::string>& signalIds)
{
    protocolHandler->subscribe(signalIds);
}

void StreamingClient::unsubscribeSignals(const std::vector<std::string>& signalIds)
{
    protocolHandler->unsubscribe(signalIds);
}

void StreamingClient::onMessage(const daq::streaming_protocol::SubscribedSignal& subscribedSignal,
                                uint64_t timeStamp,
                                const uint8_t* data,
                                size_t size)
{
    std::string id = subscribedSignal.signalId();
    const auto& signalIter = signals.find(id);
    if (signalIter != signals.end() &&
        signalIter->second->hasDescriptors() &&
        signalIter->second->getSignalDescriptor().getSampleType() != daq::SampleType::Struct)
    {
        auto packet = signalIter->second->createDataPacket(timeStamp, data, size);
        onPacketCallback(id, packet);
    }
}

void StreamingClient::setDataSignal(const daq::streaming_protocol::SubscribedSignal& subscribedSignal)
{
    const auto id = subscribedSignal.signalId();

    if (auto signalIt = signals.find(id); signalIt != signals.end())
    {
        auto inputSignal = signalIt->second;

        DataDescriptorPtr dataDescriptor = inputSignal->getSignalDescriptor();
        DataDescriptorPtr domainDescriptor = inputSignal->getDomainSignalDescriptor();

        auto sInfo = SignalDescriptorConverter::ToDataDescriptor(subscribedSignal);
        if (!inputSignal->getSignalDescriptor().assigned())
            onSignalInitCallback(id, sInfo);
        else
            onSignalUpdatedCallback(id, sInfo);
        inputSignal->setDataDescriptor(sInfo.dataDescriptor);

        const auto tableId = subscribedSignal.tableId();
        if (inputSignal->getTableId() != tableId)
        {
            inputSignal->setTableId(tableId);
            if (auto domainDescIt = cachedDomainDescriptors.find(tableId); domainDescIt != cachedDomainDescriptors.end())
            {
                setDomainDescriptor(id, inputSignal, domainDescIt->second);
            }
        }

        if (dataDescriptor != inputSignal->getSignalDescriptor() ||
            domainDescriptor != inputSignal->getDomainSignalDescriptor())
        {
            publishSignalChanges(id, inputSignal);
        }
    }
}

void StreamingClient::setTimeSignal(const daq::streaming_protocol::SubscribedSignal& subscribedSignal)
{
    std::string tableId = subscribedSignal.tableId();
    auto sInfo = SignalDescriptorConverter::ToDataDescriptor(subscribedSignal);

    const auto timeSignalId = subscribedSignal.signalId();
    if (auto signalIt = signals.find(timeSignalId); signalIt != signals.end())
    {
        // the time signal is published as available by server,
        // so do the initialization of its mirrored copy
        auto inputSignal = signalIt->second;
        if (!inputSignal->getSignalDescriptor().assigned())
        {
            onSignalInitCallback(timeSignalId, sInfo);
            setSignalInitSatisfied(timeSignalId);
        }
        else
        {
            onSignalUpdatedCallback(timeSignalId, sInfo);
        }
        inputSignal->setDataDescriptor(sInfo.dataDescriptor);
        inputSignal->setIsDomainSignal(true);
        inputSignal->setTableId(tableId);
    }

    // set domain descriptor for all input data signals with known tableId
    for (const auto& [dataSignalId, inputSignal] : findDataSignalsByTableId(tableId))
    {
        setDomainDescriptor(dataSignalId, inputSignal, sInfo.dataDescriptor);
    }

    // some data signals can have unknown tableId at the moment, save domain descriptor for future
    cachedDomainDescriptors.insert_or_assign(tableId, sInfo.dataDescriptor);
}

void StreamingClient::publishSignalChanges(const std::string& signalId, const InputSignalPtr& signal)
{
    // signal meta information is always received by pairs of META_METHOD_SIGNAL:
    // one is meta for data signal, another is meta for time signal.
    // we generate event packet only after both meta are received
    // and all signal descriptors are assigned.
    if (!signal->hasDescriptors())
        return;

    auto eventPacket = signal->createDecriptorChangedPacket();
    onPacketCallback(signalId, eventPacket);
}

std::vector<std::pair<std::string, InputSignalPtr>> StreamingClient::findDataSignalsByTableId(const std::string& tableId)
{
    std::vector<std::pair<std::string, InputSignalPtr>> result;
    for (const auto& [id, inputSignal] : signals)
    {
        if (tableId == inputSignal->getTableId() && !inputSignal->getIsDomainSignal())
        {
            result.push_back({id, inputSignal});
        }
    }
    return result;
}

void StreamingClient::onSignal(const daq::streaming_protocol::SubscribedSignal& subscribedSignal, const nlohmann::json& params)
{
    try
    {
        {
            LOG_I("Signal #{}; signalId {}; tableId {}; name {}; value type {}; Json parameters: \n\n{}\n",
                  subscribedSignal.signalNumber(),
                  subscribedSignal.signalId(),
                  subscribedSignal.tableId(),
                  subscribedSignal.memberName(),
                  static_cast<int>(subscribedSignal.dataValueType()),
                  params.dump());
        }

        if (subscribedSignal.isTimeSignal())
        {
            std::unordered_map<std::string, std::pair<DataDescriptorPtr, DataDescriptorPtr>> descriptors;
            std::string tableId = subscribedSignal.tableId();
            for (const auto& [id,signal] : findDataSignalsByTableId(tableId))
            {
                std::pair<DataDescriptorPtr, DataDescriptorPtr> pair(signal->getSignalDescriptor(),
                                                                     signal->getDomainSignalDescriptor());
                descriptors.insert_or_assign(id, pair);
            }

            setTimeSignal(subscribedSignal);

            for (const auto& [id, signalDescriptors] : descriptors)
            {
                if (auto it = signals.find(id); it != signals.end())
                {
                    auto inputSignal = it->second;
                    if (signalDescriptors.first != inputSignal->getSignalDescriptor() ||
                        signalDescriptors.second != inputSignal->getDomainSignalDescriptor())
                    {
                        // descriptors changed - generate event packet and send it to signal listeners
                        publishSignalChanges(id, inputSignal);
                    }
                }
            }
        }
        else
        {
            setDataSignal(subscribedSignal);
        }
    }
    catch (const DaqException& e)
    {
        LOG_W("Failed to interpret received input signal: {}.", e.what());        
    }
}

void StreamingClient::setSignalInitSatisfied(const std::string& signalId)
{
    if (auto iterator = signalInitializedStatus.find(signalId); iterator != signalInitializedStatus.end())
    {
        try
        {
            std::get<0>(iterator->second).set_value();
        }
        catch (std::future_error& e)
        {
            if (e.code() == std::make_error_code(std::future_errc::promise_already_satisfied))
            {
                LOG_D("signal {} is already initialized", signalId);
            }
            else
            {
                LOG_E("signal {} initialization error {}", signalId, e.what());
            }
        }
    }
}

void StreamingClient::setDomainDescriptor(const std::string& signalId,
                                          const InputSignalPtr& inputSignal,
                                          const DataDescriptorPtr& domainDescriptor)
{
    // Sets the descriptors of pseudo device signal when first connecting
    if (!inputSignal->getDomainSignalDescriptor().assigned())
    {
        onDomainDescriptorCallback(signalId, domainDescriptor);
        setSignalInitSatisfied(signalId);
    }

    inputSignal->setDomainDescriptor(domainDescriptor);
}

END_NAMESPACE_OPENDAQ_WEBSOCKET_STREAMING
