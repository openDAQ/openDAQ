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

    auto messageCallback = [this](const SubscribedSignal& subscribedSignal, uint64_t timeStamp, const uint8_t* data, size_t valueCount)
    { onMessage(subscribedSignal, timeStamp, data, valueCount); };

    signalContainer.setSignalMetaCb(signalMetaCallback);
    signalContainer.setDataAsValueCb(messageCallback);

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
        for (const auto& [signalId,_] : availableSignals)
        {
            signalIds.push_back(signalId);
            std::promise<void> signalInitPromise;
            std::future<void> signalInitFuture = signalInitPromise.get_future();
            availableSigInitStatus.insert_or_assign(
                signalId,
                std::make_tuple(
                    std::move(signalInitPromise),
                    std::move(signalInitFuture),
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

        for (const auto& [id, params] : availableSigInitStatus)
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
    if (clientThread.get_id() != std::this_thread::get_id())
    {
        if (clientThread.joinable())
        {
            clientThread.join();
            LOG_I("Websocket streaming client thread joined");
        }
        else
        {
            LOG_W("Websocket streaming client thread is not joinable");
        }
    }
    else
    {
        LOG_C("Websocket streaming client thread cannot join itself");
    }

    connected = false;
}

void StreamingClient::onPacket(const OnPacketCallback& callack)
{
    onPacketCallback = callack;
}

void StreamingClient::onAvailableSignalInit(const OnSignalCallback& callback)
{
    onAvailableSignalInitCb = callback;
}

void StreamingClient::onSignalUpdated(const OnSignalCallback& callback)
{
    onSignalUpdatedCallback = callback;
}

void StreamingClient::onDomainSingalInit(const OnDomainSignalInitCallback& callback)
{
    onDomainSignalInitCallback = callback;
}

void StreamingClient::onAvailableStreamingSignals(const OnAvailableSignalsCallback& callback)
{
    onAvailableStreamingSignalsCb = callback;
}

void StreamingClient::onAvailableDeviceSignals(const OnAvailableSignalsCallback& callback)
{
    onAvailableDeviceSignalsCb = callback;
}

void StreamingClient::onHiddenStreamingSignal(const OnSignalCallback& callback)
{
    onHiddenStreamingSignalCb = callback;
}

void StreamingClient::onHiddenDeviceSignal(const OnSignalCallback& callback)
{
    onHiddenDeviceSignalInitCb = callback;
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

void StreamingClient::parseConnectionString(const std::string& url)
{
    // this is not great but it is convenient until we have a way to pass configuration parameters to a client device

    host = "";
    port = 7414;
    target = "/";

    std::smatch match;

    // parsing connection string to four groups: prefix, host, port, path
    auto regexIpv6Hostname = std::regex(R"(^(.*://)?(?:\[([a-fA-F0-9:]+)\])(?::(\d+))?(/.*)?$)");
    auto regexIpv4Hostname = std::regex(R"(^(.*://)?([^:/\s]+)(?::(\d+))?(/.*)?$)");

    bool parsed = false;
    parsed = std::regex_search(url, match, regexIpv6Hostname);
    if (!parsed)
        parsed = std::regex_search(url, match, regexIpv4Hostname);

    if (parsed)
    {
        host = match[2];
        if (match[3].matched)
            port = std::stoi(match[3]);
        if (match[4].matched)
            target = match[4];
    }
}

void StreamingClient::onSignalMeta(const SubscribedSignal& subscribedSignal, const std::string& method, const nlohmann::json& params)
{
    if (method == daq::streaming_protocol::META_METHOD_SIGNAL)
        onSignal(subscribedSignal, params);

    std::string signalId = subscribedSignal.signalId();

    // triggers ack only for available signals, but not for hidden ones
    if (method == daq::streaming_protocol::META_METHOD_SUBSCRIBE)
    {
        if (auto it = availableSignals.find(signalId); it != availableSignals.end())
        {
            auto inputSignal = it->second;
            // skips the first subscribe ACK from server as inputSignal is not initialized at the moment
            if (inputSignal)
            {
                // ignores ACKs for domain signals
                if (!inputSignal->isDomainSignal())
                    onSubscriptionAckCallback(subscribedSignal.signalId(), true);
            }
        }
    }
    else if (method == daq::streaming_protocol::META_METHOD_UNSUBSCRIBE)
    {
        if (auto it = availableSigInitStatus.find(signalId); it != availableSigInitStatus.end())
        {
            // skips the first unsubscribe ACK from server
            if (std::get<2>(it->second))
            {
                // ignores ACKs for domain signals
                if (!subscribedSignal.isTimeSignal())
                    onSubscriptionAckCallback(subscribedSignal.signalId(), false);
            }
            else
            {
                std::get<2>(it->second) = true;
            }
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
        auto availableSignalsArray = params.find(META_SIGNALIDS);

        if (availableSignalsArray != params.end() && availableSignalsArray->is_array())
        {
            for (const auto& arrayItem : *availableSignalsArray)
            {
                std::string signalId = arrayItem;
                signalIds.push_back(signalId);

                if (auto signalIt = availableSignals.find(signalId); signalIt == availableSignals.end())
                {
                    availableSignals.insert({signalId, nullptr});
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

void StreamingClient::subscribeSignal(const std::string& signalId)
{
    if (auto it = availableSignals.find(signalId); it != availableSignals.end())
        protocolHandler->subscribe({signalId});
}

void StreamingClient::unsubscribeSignal(const std::string& signalId)
{
    if (auto it = availableSignals.find(signalId); it != availableSignals.end())
        protocolHandler->unsubscribe({signalId});
}

void StreamingClient::onMessage(const daq::streaming_protocol::SubscribedSignal& subscribedSignal,
                                uint64_t timeStamp,
                                const uint8_t* data,
                                size_t valueCount)
{
    std::string id = subscribedSignal.signalId();

    InputSignalBasePtr inputSignal = nullptr;

    if (auto availableSigIt = availableSignals.find(id); availableSigIt != availableSignals.end())
        inputSignal = availableSigIt->second;
    else if (auto hiddenSigIt = hiddenSignals.find(id); hiddenSigIt != hiddenSignals.end())
        inputSignal = hiddenSigIt->second;

    if (inputSignal &&
        inputSignal->hasDescriptors() &&
        inputSignal->getSignalDescriptor().getSampleType() != daq::SampleType::Struct)
    {
        if (inputSignal->isCountable())
        {
            DataPacketPtr domainPacket;
            if (inputSignal->isDomainSignal())
            {
                domainPacket = inputSignal->generateDataPacket(timeStamp, data, valueCount, nullptr);
                if (domainPacket.assigned())
                    onPacketCallback(id, domainPacket);
            }
            else
            {
                domainPacket =
                    inputSignal->getInputDomainSignal()->generateDataPacket(timeStamp, nullptr, valueCount, nullptr);
                if (domainPacket.assigned())
                    onPacketCallback(inputSignal->getInputDomainSignal()->getSignalId(), domainPacket);
                auto packet = inputSignal->generateDataPacket(timeStamp, data, valueCount, domainPacket);
                if (packet.assigned())
                    onPacketCallback(id, packet);
            }

            auto relatedDataSignals = findDataSignalsByTableId(inputSignal->getTableId());

            // trigger packet generation for each related signal which is not countable (is implicit)
            for (auto& relatedSignal : relatedDataSignals)
            {
                if (!relatedSignal->isCountable())
                {
                    auto packet = relatedSignal->generateDataPacket(timeStamp, nullptr, valueCount, domainPacket);
                    if (packet.assigned())
                        onPacketCallback(relatedSignal->getSignalId(), packet);
                }
            }
        }
        else
        {
            inputSignal->processSamples(timeStamp, data, valueCount);
        }
    }
}

void StreamingClient::setDataSignal(const daq::streaming_protocol::SubscribedSignal& subscribedSignal)
{
    auto sInfo = SignalDescriptorConverter::ToDataDescriptor(subscribedSignal);
    const auto signalId = subscribedSignal.signalId();
    const auto tableId = subscribedSignal.tableId();
    bool available = false;

    auto domainInputSignal = findTimeSignalByTableId(tableId);
    if (!domainInputSignal)
        throw NotFoundException("Unknown domain signal for data signal {}, table {}", signalId, tableId);

    InputSignalBasePtr inputSignal = nullptr;
    if (auto availableSigIt = availableSignals.find(signalId); availableSigIt != availableSignals.end())
    {
        inputSignal = availableSigIt->second;
        available = true;
    }
    else if (auto hiddenSigIt = hiddenSignals.find(signalId); hiddenSigIt != hiddenSignals.end())
    {
        inputSignal = hiddenSigIt->second;
        available = false;
    }

    if (!inputSignal && !available)
    {
        inputSignal = InputSignal(signalId, tableId, sInfo, false, domainInputSignal, logCallback);
        hiddenSignals.insert({signalId, inputSignal});
        onHiddenStreamingSignalCb(signalId, sInfo);
        onHiddenDeviceSignalInitCb(signalId, sInfo);
        onDomainSignalInitCallback(signalId, domainInputSignal->getSignalId());
    }
    else if (available && !inputSignal)
    {
        inputSignal = InputSignal(signalId, tableId, sInfo, false, domainInputSignal, logCallback);
        availableSignals[signalId] = inputSignal;
        onAvailableSignalInitCb(signalId, sInfo);
        onDomainSignalInitCallback(signalId, domainInputSignal->getSignalId());
        setSignalInitSatisfied(signalId);
    }
    else
    {
        if (sInfo.dataDescriptor != inputSignal->getSignalDescriptor())
        {
            inputSignal->setDataDescriptor(sInfo.dataDescriptor);
            publishSignalChanges(inputSignal, true, false);
        }
        onSignalUpdatedCallback(signalId, sInfo);
    }
}

void StreamingClient::setTimeSignal(const daq::streaming_protocol::SubscribedSignal& subscribedSignal)
{
    auto sInfo = SignalDescriptorConverter::ToDataDescriptor(subscribedSignal);
    const std::string tableId = subscribedSignal.tableId();
    const std::string timeSignalId = subscribedSignal.signalId();
    bool available = false;

    InputSignalBasePtr inputSignal = nullptr;
    if (auto availableSigIt = availableSignals.find(timeSignalId); availableSigIt != availableSignals.end())
    {
        inputSignal = availableSigIt->second;
        available = true;
    }
    else if (auto hiddenSigIt = hiddenSignals.find(timeSignalId); hiddenSigIt != hiddenSignals.end())
    {
        inputSignal = hiddenSigIt->second;
        available = false;
    }

    if (!inputSignal && !available)
    {
        // the time signal was not published as available by server, add as hidden
        inputSignal = InputSignal(timeSignalId, tableId, sInfo, true, nullptr, logCallback);
        hiddenSignals.insert({timeSignalId, inputSignal});
        onHiddenStreamingSignalCb(timeSignalId, sInfo);
        onHiddenDeviceSignalInitCb(timeSignalId, sInfo);
    }
    else if (available && !inputSignal)
    {
        inputSignal = InputSignal(timeSignalId, tableId, sInfo, true, nullptr, logCallback);
        availableSignals[timeSignalId] = inputSignal;
        // the time signal is published as available by server,
        // so do the initialization of its mirrored copy
        onAvailableSignalInitCb(timeSignalId, sInfo);
        setSignalInitSatisfied(timeSignalId);
    }
    else
    {
        if (sInfo.dataDescriptor != inputSignal->getSignalDescriptor())
        {
            inputSignal->setDataDescriptor(sInfo.dataDescriptor);

            // publish new domain descriptor for all input data signals with known tableId
            for (const auto& inputDataSignal : findDataSignalsByTableId(tableId))
            {
                publishSignalChanges(inputDataSignal, false, true);
            }
        }
        onSignalUpdatedCallback(timeSignalId, sInfo);
    }
}

void StreamingClient::publishSignalChanges(const InputSignalBasePtr& signal, bool valueChanged, bool domainChanged)
{
    // signal meta information is always received by pairs of META_METHOD_SIGNAL:
    // one is meta for data signal, another is meta for time signal.
    // we generate event packet only after both meta are received
    // and all signal descriptors are assigned.
    if (!signal->hasDescriptors())
        return;

    auto eventPacket = signal->createDecriptorChangedPacket(valueChanged, domainChanged);
    onPacketCallback(signal->getSignalId(), eventPacket);
}

std::vector<InputSignalBasePtr> StreamingClient::findDataSignalsByTableId(const std::string& tableId)
{
    std::vector<InputSignalBasePtr> result;
    for (const auto& [_, inputSignal] : availableSignals)
    {
        if (inputSignal && tableId == inputSignal->getTableId() && !inputSignal->isDomainSignal())
        {
            result.push_back(inputSignal);
        }
    }
    for (const auto& [_, inputSignal] : hiddenSignals)
    {
        if (inputSignal && tableId == inputSignal->getTableId() && !inputSignal->isDomainSignal())
        {
            result.push_back(inputSignal);
        }
    }
    return result;
}

InputSignalBasePtr StreamingClient::findTimeSignalByTableId(const std::string& tableId)
{
    std::vector<std::pair<std::string, InputSignalPtr>> result;
    for (const auto& [_, inputSignal] : availableSignals)
    {
        if (inputSignal && tableId == inputSignal->getTableId() && inputSignal->isDomainSignal())
        {
            return inputSignal;
        }
    }
    for (const auto& [_, inputSignal] : hiddenSignals)
    {
        if (inputSignal && tableId == inputSignal->getTableId() && inputSignal->isDomainSignal())
        {
            return inputSignal;
        }
    }
    return nullptr;
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
            setTimeSignal(subscribedSignal);
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
    if (auto iterator = availableSigInitStatus.find(signalId); iterator != availableSigInitStatus.end())
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

END_NAMESPACE_OPENDAQ_WEBSOCKET_STREAMING
