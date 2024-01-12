#include <nlohmann/json.hpp>

#include <streaming_protocol/Defines.h>
#include <streaming_protocol/Logging.hpp>
#include <streaming_protocol/jsonrpc_defines.hpp>

#include "mock_streaming_server.h"
#include <opendaq/event_packet_ids.h>
#include <opendaq/event_packet_params.h>
#include <opendaq/custom_log.h>

#include <streaming_protocol/Unit.hpp>
#include <websocket_streaming/signal_descriptor_converter.h>

namespace streaming_test_helpers
{

using namespace daq;
using namespace daq::streaming_protocol;
using namespace daq::stream;

MockStreamingServer::MockStreamingServer(const ContextPtr& context)
    : work(ioContext.get_executor())
    , logger(context.getLogger())
{
    loggerComponent = this->logger.getOrAddComponent("MockStreamingServer");
    logCallback = [this](spdlog::source_loc location, spdlog::level::level_enum level, const char* msg) {
        this->loggerComponent.logMessage(SourceLocation{location.filename, location.line, location.funcname}, msg, static_cast<LogLevel>(level));
    };
}

MockStreamingServer::~MockStreamingServer()
{
    stop();
    logger.removeComponent("MockStreamingServer");
}

void MockStreamingServer::start(uint16_t port, uint16_t controlPort)
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

void MockStreamingServer::stop()
{
    ioContext.stop();

    if (!serverThread.joinable())
        return;

    this->server->stop();
    serverThread.join();
    this->server.reset();
}

void MockStreamingServer::onAccept(const OnAcceptCallback& callback)
{
    onAcceptCallback = callback;
}

void MockStreamingServer::onAcceptInternal(const daq::stream::StreamPtr& stream)
{
    auto writer = std::make_shared<StreamWriter>(stream);
    writeProtocolInfo(writer);
    writeInit(writer);

    auto signals = List<ISignal>();
    if (onAcceptCallback)
        signals = onAcceptCallback();

    auto outputSignals = std::unordered_map<std::string, std::pair<SizeT, SignalPtr>>();
    SizeT signalNumber = 0;
    for (const auto& signal : signals)
    {
        bool isDomainSignal = !signal.getDomainSignal().assigned();
        auto descriptor = signal.getDescriptor();

        if (!descriptor.assigned() ||
            !isDomainSignal && descriptor.getSampleType() != daq::SampleType::Float64 ||
            !isDomainSignal && descriptor.getRule().getType() != daq::DataRuleType::Explicit ||
            isDomainSignal && descriptor.getSampleType() != daq::SampleType::UInt64 ||
            isDomainSignal && descriptor.getRule().getType() != daq::DataRuleType::Linear)
        {
            LOG_E("Unsupported type of signal {}; data signal should be explicit float64, "
                  "domain signal linear unsigned64", signal.getGlobalId());
            throw GeneralErrorException("Unsupported type of signal");
        }
        outputSignals.insert({signal.getGlobalId().toStdString(), {++signalNumber, signal}});
    }

    LOG_I("New client connected. Stream Id: {}", writer->id());
    clients.insert({writer, outputSignals});

    writeSignalsAvailable(writer, signals);
}

int MockStreamingServer::onControlCommand(const std::string& streamId,
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

    auto writer = clientIter->first;
    auto signalMap = clientIter->second;

    if (command == "subscribe")
    {
        subscribeSignals(signalIds, signalMap, writer);
    }
    else if (command == "unsubscribe")
    {
        unsubscribeSignals(signalIds, signalMap, writer);
    }
    else
    {
        LOG_W("Unknown control command: {}", command);
        errorMessage = "Unknown command: " + command;
        return -1;
    }

    size_t unknownSignalsCount = 0;
    std::string message = "Command '" + command + "' failed for unknown signals:\n";
    for (const auto& signalId : signalIds)
    {
        if (auto signalMapIter = signalMap.find(signalId); signalMapIter == signalMap.end())
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

    return 0;
}

void MockStreamingServer::subscribeSignals(const daq::streaming_protocol::SignalIds& signalIds,
                                           const SignalMap& signalMap,
                                           const daq::streaming_protocol::StreamWriterPtr& writer)
{
    for (const auto& signalId : signalIds)
    {
        if (auto signalMapIter = signalMap.find(signalId); signalMapIter != signalMap.end())
        {
            auto signalNumber = signalMapIter->second.first;
            auto signalPtr = signalMapIter->second.second;

            if (!signalPtr.hasProperty("tableId"))
                throw GeneralErrorException("Unknown table id of signal");

            std::string tableId = signalPtr.getPropertyValue("tableId").asPtr<IString>().toStdString();

            if (!signalPtr.getDomainSignal().assigned())
            {
                // considered as domain signal
                writeSignalSubscribe(signalId, signalNumber, writer);
                writeTimeSignalMeta(tableId, signalNumber, signalPtr, writer);
            }
            else
            {
                // considered as data signal
                writeSignalSubscribe(signalId, signalNumber, writer);
                writeDataSignalMeta(tableId, signalNumber, signalPtr, writer);

                auto domainSignal = signalPtr.getDomainSignal();
                std::string domainSignalId = domainSignal.getGlobalId().toStdString();
                if (auto domainSignalMapIter = signalMap.find(domainSignalId);
                    domainSignalMapIter == signalMap.end())
                {
                    throw GeneralErrorException("Domain signal should be registered as available");
                }
            }
        }
    }
}

void MockStreamingServer::unsubscribeSignals(const daq::streaming_protocol::SignalIds& signalIds,
                                             const SignalMap& signalMap,
                                             const daq::streaming_protocol::StreamWriterPtr& writer)
{
    for (const auto& signalId : signalIds)
    {
        if (auto signalMapIter = signalMap.find(signalId); signalMapIter != signalMap.end())
        {
            auto signalNumber = signalMapIter->second.first;
            auto signalPtr = signalMapIter->second.second;

            writeSignalUnsubscribe(signalId, signalNumber, writer);
        }
    }
}

void MockStreamingServer::writeProtocolInfo(const daq::streaming_protocol::StreamWriterPtr& writer)
{
    nlohmann::json msg;
    msg[METHOD] = META_METHOD_APIVERSION;
    msg[PARAMS][VERSION] = OPENDAQ_LT_STREAM_VERSION;
    writer->writeMetaInformation(0, msg);
}

void MockStreamingServer::writeSignalsAvailable(const daq::streaming_protocol::StreamWriterPtr& writer,
                                                const ListPtr<ISignal>& signals)
{
    std::vector<std::string> signalIds;

    for (const auto& signal : signals)
        signalIds.push_back(signal.getGlobalId());

    nlohmann::json msg;
    msg[METHOD] = META_METHOD_AVAILABLE;
    msg[PARAMS][META_SIGNALIDS] = signalIds;
    writer->writeMetaInformation(0, msg);
}

void MockStreamingServer::writeInit(const streaming_protocol::StreamWriterPtr& writer)
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

void MockStreamingServer::writeSignalSubscribe(const std::string& signalId,
                                               SizeT signalNumber,
                                               const streaming_protocol::StreamWriterPtr& writer)
{
    nlohmann::json subscribeData;
    subscribeData[METHOD] = META_METHOD_SUBSCRIBE;
    subscribeData[PARAMS][META_SIGNALID] = signalId;
    int result = writer->writeMetaInformation(signalNumber, subscribeData);
    if (result < 0)
        throw GeneralErrorException("Write Subscribe Meta Information failure");
}

void MockStreamingServer::writeDataSignalMeta(const std::string& tableId,
                                              SizeT signalNumber,
                                              const SignalPtr& signal,
                                              const streaming_protocol::StreamWriterPtr& writer)
{
    nlohmann::json dataSignal;
    dataSignal[METHOD] = META_METHOD_SIGNAL;
    dataSignal[PARAMS][META_TABLEID] = tableId;

    dataSignal[PARAMS][META_DEFINITION][META_NAME] = signal.getName();
    dataSignal[PARAMS][META_DEFINITION][META_DATATYPE] = DATA_TYPE_REAL64;
    dataSignal[PARAMS][META_DEFINITION][META_RULE] = META_RULETYPE_EXPLICIT;

    nlohmann::json interpretationObject;
    websocket_streaming::SignalDescriptorConverter::EncodeInterpretationObject(
        signal.getDescriptor(), interpretationObject);
    dataSignal[PARAMS][META_INTERPRETATION] = interpretationObject;

    int result = writer->writeMetaInformation(signalNumber, dataSignal);
    if (result < 0)
        throw GeneralErrorException("Write Data Signal Meta Information failure");
}

void MockStreamingServer::writeTimeSignalMeta(const std::string& tableId,
                                              SizeT signalNumber,
                                              const SignalPtr& signal,
                                              const streaming_protocol::StreamWriterPtr& writer)
{
    auto descriptor = signal.getDescriptor();
    auto resolution = descriptor.getTickResolution();

    nlohmann::json timeSignal;
    timeSignal[METHOD] = META_METHOD_SIGNAL;
    timeSignal[PARAMS][META_TABLEID] = tableId;
    timeSignal[PARAMS][META_DEFINITION][META_NAME] = signal.getName();
    timeSignal[PARAMS][META_DEFINITION][META_RULE] = META_RULETYPE_LINEAR;

    timeSignal[PARAMS][META_DEFINITION][META_RULETYPE_LINEAR][META_DELTA] =
        (uint64_t)descriptor.getRule().getParameters().get("delta");
    timeSignal[PARAMS][META_DEFINITION][META_DATATYPE] = DATA_TYPE_UINT64;

    timeSignal[PARAMS][META_DEFINITION][META_UNIT][META_UNIT_ID] = daq::streaming_protocol::Unit::UNIT_ID_SECONDS;
    timeSignal[PARAMS][META_DEFINITION][META_UNIT][META_DISPLAY_NAME] = "s";
    timeSignal[PARAMS][META_DEFINITION][META_UNIT][META_QUANTITY] = META_TIME;
    timeSignal[PARAMS][META_DEFINITION][META_ABSOLUTE_REFERENCE] = UNIX_EPOCH;
    timeSignal[PARAMS][META_DEFINITION][META_RESOLUTION][META_NUMERATOR] = 1;
    timeSignal[PARAMS][META_DEFINITION][META_RESOLUTION][META_DENOMINATOR] =
        resolution.getDenominator() / resolution.getNumerator();

    nlohmann::json interpretationObject;
    websocket_streaming::SignalDescriptorConverter::EncodeInterpretationObject(
        descriptor, interpretationObject);
    timeSignal[PARAMS][META_INTERPRETATION] = interpretationObject;

    int result = writer->writeMetaInformation(signalNumber, timeSignal);
    if (result < 0)
        throw GeneralErrorException("Write Time Signal Meta Information failure");
}

void MockStreamingServer::writeSignalUnsubscribe(const std::string& signalId,
                                             SizeT signalNumber,
                                             const streaming_protocol::StreamWriterPtr& writer)
{
    nlohmann::json unsubscribe;
    unsubscribe[METHOD] = META_METHOD_UNSUBSCRIBE;
    int result = writer->writeMetaInformation(signalNumber, unsubscribe);
    if (result < 0)
        throw GeneralErrorException("Write Unsubscribe Meta Information failure");
}

}
