#include <native_streaming_server_module/native_streaming_server_impl.h>
#include <coretypes/impl.h>
#include <coreobjects/property_object_factory.h>
#include <coreobjects/property_factory.h>
#include <opendaq/server_type_factory.h>
#include <opendaq/device_private.h>
#include <opendaq/reader_factory.h>
#include <opendaq/search_filter_factory.h>
#include <opendaq/custom_log.h>
#include <opendaq/event_packet_ids.h>
#include <opendaq/device_info_factory.h>
#include <opendaq/device_info_internal_ptr.h>
#include <native_streaming_protocol/native_streaming_server_handler.h>
#include <native_streaming_protocol/native_streaming_constants.h>
#include <config_protocol/config_protocol_server.h>

#include <boost/asio/dispatch.hpp>
#include <opendaq/input_port_factory.h>
#include <opendaq/thread_name.h>

#include <native_streaming_server_module/native_server_streaming_impl.h>

BEGIN_NAMESPACE_OPENDAQ_NATIVE_STREAMING_SERVER_MODULE
using namespace daq;
using namespace opendaq_native_streaming_protocol;
using namespace config_protocol;

static constexpr size_t DEFAULT_MAX_PACKET_READ_COUNT = 5000;
static constexpr size_t DEFAULT_POLLING_PERIOD = 20;

NativeStreamingServerImpl::NativeStreamingServerImpl(const DevicePtr& rootDevice,
                                                     const PropertyObjectPtr& config,
                                                     const ContextPtr& context)
    : Server(CONST_NATIVE_SERVER_TYPE_ID, config, rootDevice, context)
    , readThreadActive(false)
    , readThreadSleepTime(std::chrono::milliseconds(20))
    , transportIOContextPtr(std::make_shared<boost::asio::io_context>())
    , processingIOContextPtr(std::make_shared<boost::asio::io_context>())
    , processingStrand(*processingIOContextPtr)
    , rootDeviceGlobalId(rootDevice.getGlobalId().toStdString())
    , logger(context.getLogger())
    , loggerComponent(logger.getOrAddComponent(id))
    , serverStopped(false)
    , workerPool(nullptr)
    , plainChannelEnabled(false)
    , tlsChannelEnabled(false)

{
    plainChannelEnabled = config.getPropertyValue(PROPERTY_ENABLE_PORT_SERVER);
#if NATIVE_STREAMING_ENABLE_TLS
    tlsChannelEnabled = config.getPropertyValue(PROPERTY_ENABLE_TLS_PORT_SERVER);
#endif

    validateChannelConfig(config);

    auto info = rootDevice.getInfo();
    const auto refuseExisting = [&info](const char* protocolId)
    {
        if (info.hasServerCapability(protocolId))
            DAQ_THROW_EXCEPTION(InvalidStateException,
                                fmt::format("Device \"{}\" already has an {} server capability.", info.getName(), protocolId));
    };

    if (plainChannelEnabled)
    {
        refuseExisting(CONST_NATIVE_STREAMING_ID);
        refuseExisting(CONST_NATIVE_CONFIG_ID);
    }
    if (tlsChannelEnabled)
    {
        refuseExisting(CONST_NATIVE_STREAMING_SECURE_ID);
        refuseExisting(CONST_NATIVE_CONFIG_SECURE_ID);
    }

    initWorkerPool();
    startProcessingOperations();
    startTransportOperations();

    prepareServerHandler();
    streaming = createWithImplementation<IStreaming, NativeServerStreamingImpl>(serverHandler, processingIOContextPtr, context);
    streaming.asPtr<INativeServerStreamingPrivate>()->upgradeToSafeProcessingCallbacks();
    streaming.setActive(true);

    const StringPtr path = config.getPropertyValue(PROPERTY_PATH_SERVER);
    const auto infoInternal = info.asPtr<IDeviceInfoInternal>(true);

    if (plainChannelEnabled)
    {
        const uint16_t port = config.getPropertyValue(PROPERTY_PORT_SERVER);
        serverHandler->startServer(port);
        addServerCapabilities(infoInternal, port, path, false);
    }

#if NATIVE_STREAMING_ENABLE_TLS
    if (tlsChannelEnabled)
    {
        const uint16_t tlsPort = config.getPropertyValue(PROPERTY_TLS_PORT_SERVER);
        const bool mutualTls = config.getPropertyValue(PROPERTY_ENABLE_MTLS_SERVER);
        const auto readPath = [&config](const char* name)
        { return StringPtr(config.getPropertyValue(name)).toStdString(); };

        // an empty CA file is how the transport is told not to ask clients for a certificate
        serverHandler->startTlsServer(tlsPort,
                                      readPath(PROPERTY_CERT_FILE_PATH_SERVER),
                                      readPath(PROPERTY_KEY_FILE_PATH_SERVER),
                                      mutualTls ? readPath(PROPERTY_CA_CERT_FILE_PATH_SERVER) : std::string());
        addServerCapabilities(infoInternal, tlsPort, path, true);
    }
#endif

    this->context.getOnCoreEvent() += event(&NativeStreamingServerImpl::coreEventCallback);

    const uint16_t pollingPeriod = config.getPropertyValue("StreamingDataPollingPeriod");
    readThreadSleepTime = std::chrono::milliseconds(pollingPeriod);

    maxPacketReadCount = config.getPropertyValue("MaxPacketReadCount");
    packetBuf.resize(maxPacketReadCount);
    startReading();
}

void NativeStreamingServerImpl::validateChannelConfig(const PropertyObjectPtr& config) const
{
    if (!plainChannelEnabled && !tlsChannelEnabled)
    {
#if NATIVE_STREAMING_ENABLE_TLS
        DAQ_THROW_EXCEPTION(InvalidParameterException,
                            "Neither \"{}\" nor \"{}\" is enabled, so the server would listen nowhere.",
                            PROPERTY_ENABLE_PORT_SERVER,
                            PROPERTY_ENABLE_TLS_PORT_SERVER);
#else
        DAQ_THROW_EXCEPTION(InvalidParameterException,
                            "\"{}\" is not enabled, and it is the only listener this build has: the TLS channel "
                            "needs OPENDAQ_ENABLE_NATIVE_STREAMING_WITH_TLS.",
                            PROPERTY_ENABLE_PORT_SERVER);
#endif
    }

#if NATIVE_STREAMING_ENABLE_TLS
    if (!tlsChannelEnabled)
        return;

    const auto isEmpty = [&config](const char* name)
    { return StringPtr(config.getPropertyValue(name)).getLength() == 0; };

    if (isEmpty(PROPERTY_CERT_FILE_PATH_SERVER) || isEmpty(PROPERTY_KEY_FILE_PATH_SERVER))
        DAQ_THROW_EXCEPTION(InvalidParameterException,
                            "\"{}\" and \"{}\" are both required when \"{}\" is on.",
                            PROPERTY_CERT_FILE_PATH_SERVER,
                            PROPERTY_KEY_FILE_PATH_SERVER,
                            PROPERTY_ENABLE_TLS_PORT_SERVER);

    const bool mutualTls = config.getPropertyValue(PROPERTY_ENABLE_MTLS_SERVER);
    if (mutualTls && isEmpty(PROPERTY_CA_CERT_FILE_PATH_SERVER))
        DAQ_THROW_EXCEPTION(InvalidParameterException,
                            "\"{}\" is required to verify client certificates. Set it, or turn \"{}\" off to "
                            "stop asking clients for one.",
                            PROPERTY_CA_CERT_FILE_PATH_SERVER,
                            PROPERTY_ENABLE_MTLS_SERVER);
#endif
}

void NativeStreamingServerImpl::addServerCapabilities(const DeviceInfoInternalPtr& infoInternal,
                                                      uint16_t port,
                                                      const StringPtr& path,
                                                      bool secure) const
{
    const auto securityLevel = secure ? CONST_NATIVE_SECURE_SECURITY_LVL : CONST_NATIVE_SECURITY_LVL;
    const StringPtr publishedPath = path == "/" ? "" : path;

    ServerCapabilityConfigPtr streamingCapability =
        ServerCapability(secure ? CONST_NATIVE_STREAMING_SECURE_ID : CONST_NATIVE_STREAMING_ID,
                         secure ? CONST_NATIVE_STREAMING_SECURE_ID : CONST_NATIVE_STREAMING_ID,
                         ProtocolType::Streaming)
            .setPrefix(secure ? CONST_NATIVE_STREAMING_SECURE_PREFIX : CONST_NATIVE_STREAMING_PREFIX)
            .setConnectionType("TCP/IP")
            .setPort(port)
            .setProtocolGroupId(CONST_NATIVE_PROTOCOL_GROUP_ID)
            .setProtocolSecurityLevel(securityLevel);

    streamingCapability.addProperty(StringProperty("Path", publishedPath));
    infoInternal.addServerCapability(streamingCapability);

    ServerCapabilityConfigPtr configCapability =
        ServerCapability(secure ? CONST_NATIVE_CONFIG_SECURE_ID : CONST_NATIVE_CONFIG_ID,
                         secure ? CONST_NATIVE_CONFIG_SECURE_ID : CONST_NATIVE_CONFIG_ID,
                         ProtocolType::ConfigurationAndStreaming)
            .setPrefix(secure ? CONST_NATIVE_CONFIG_SECURE_PREFIX : CONST_NATIVE_CONFIG_PREFIX)
            .setConnectionType("TCP/IP")
            .setPort(port)
            .setProtocolVersion(std::to_string(GetLatestConfigProtocolVersion()))
            .setProtocolGroupId(CONST_NATIVE_PROTOCOL_GROUP_ID)
            .setProtocolSecurityLevel(securityLevel);

    configCapability.addProperty(StringProperty("Path", publishedPath));
    infoInternal.addServerCapability(configCapability);
}

NativeStreamingServerImpl::~NativeStreamingServerImpl()
{
    stopServerInternal();
}

void NativeStreamingServerImpl::addSignalsOfComponent(ComponentPtr& component)
{
    if (component.supportsInterface<ISignal>())
    {
        serverHandler->addSignal(component.asPtr<ISignal>(true));
    }
    else if (component.supportsInterface<IFolder>())
    {
        auto nestedComponents = component.asPtr<IFolder>().getItems(search::Recursive(search::Any()));
        for (const auto& nestedComponent : nestedComponents)
        {
            if (nestedComponent.supportsInterface<ISignal>())
            {
                LOG_I("Added Signal: {};", nestedComponent.getGlobalId());
                serverHandler->addSignal(nestedComponent.asPtr<ISignal>(true));
            }
        }
    }
}

void NativeStreamingServerImpl::componentAdded(ComponentPtr& /*sender*/, CoreEventArgsPtr& eventArgs)
{
    ComponentPtr addedComponent = eventArgs.getParameters().get("Component");

    auto addedComponentGlobalId = addedComponent.getGlobalId().toStdString();
    if (addedComponentGlobalId.find(rootDeviceGlobalId) != 0)
        return;

    LOG_I("Added Component: {};", addedComponentGlobalId);
    addSignalsOfComponent(addedComponent);
}

void NativeStreamingServerImpl::componentRemoved(ComponentPtr& sender, CoreEventArgsPtr& eventArgs)
{
    StringPtr removedComponentLocalId = eventArgs.getParameters().get("Id");

    auto removedComponentGlobalId =
        sender.getGlobalId().toStdString() + "/" + removedComponentLocalId.toStdString();
    if (removedComponentGlobalId.find(rootDeviceGlobalId) != 0)
        return;

    LOG_I("Component: {}; is removed", removedComponentGlobalId);
    serverHandler->removeComponentSignals(removedComponentGlobalId);
}

void NativeStreamingServerImpl::componentUpdated(ComponentPtr& updatedComponent)
{
    auto updatedComponentGlobalId = updatedComponent.getGlobalId().toStdString();
    if (updatedComponentGlobalId.find(rootDeviceGlobalId) != 0)
        return;

    LOG_I("Component: {}; is updated", updatedComponentGlobalId);

    // remove all registered signal of updated component since those might be modified or removed
    serverHandler->removeComponentSignals(updatedComponentGlobalId);

    // add updated versions of signals
    addSignalsOfComponent(updatedComponent);
}

void NativeStreamingServerImpl::coreEventCallback(ComponentPtr& sender, CoreEventArgsPtr& eventArgs)
{
    switch (static_cast<CoreEventId>(eventArgs.getEventId()))
    {
        case CoreEventId::ComponentAdded:
            componentAdded(sender, eventArgs);
            break;
        case CoreEventId::ComponentRemoved:
            componentRemoved(sender, eventArgs);
            break;
        case CoreEventId::ComponentUpdateEnd:
            componentUpdated(sender);
            break;
        default:
            break;
    }
}

void NativeStreamingServerImpl::initWorkerPool()
{
    if (config.hasProperty("ConfigurationRpcWorkerCount"))
    {
        size_t workerCount = config.getPropertyValue("ConfigurationRpcWorkerCount");
        LOG_I("\"ConfigurationRpcWorkerCount\" property value: {}", workerCount);

        if (workerCount == 0)
            workerCount = std::thread::hardware_concurrency();

        if (workerCount > 1)
            workerPool = std::make_unique<boost::asio::thread_pool>(workerCount);

        LOG_I("Config protocol worker count: {}", workerCount);
    }
    else
    {
        LOG_W("\"ConfigurationRpcWorkerCount\" property is missing - server will process all config protocol request in a single thread");
    }
}

void NativeStreamingServerImpl::startTransportOperations()
{
    transportThread = std::thread(
        [this]()
        {
            daqNameThread("NatSrvStreamTrans");
            using namespace boost::asio;
            auto workGuard = make_work_guard(*transportIOContextPtr);
            transportIOContextPtr->run();
            LOG_I("Transport IO thread finished");
        });
}

void NativeStreamingServerImpl::stopTransportOperations()
{
    transportIOContextPtr->stop();
    if (transportThread.get_id() != std::this_thread::get_id())
    {
        if (transportThread.joinable())
        {
            transportThread.join();
            LOG_I("Transport IO thread joined");
        }
        else
        {
            LOG_W("Native server - transport IO thread is not joinable");
        }
    }
    else
    {
        LOG_C("Native server - transport IO thread cannot join itself");
    }
}

void NativeStreamingServerImpl::startProcessingOperations()
{
    processingThread = std::thread(
        [this]()
        {
            daqNameThread("NatSrvProc");

            using namespace boost::asio;
            auto workGuard = make_work_guard(*processingIOContextPtr);
            processingIOContextPtr->run();
            LOG_I("Processing thread finished");
        }
    );
}

void NativeStreamingServerImpl::stopProcessingOperations()
{
    if (workerPool)
    {
        workerPool->join();
        LOG_I("Config protocol worker pool joined");
    }
    processingIOContextPtr->stop();
    if (processingThread.get_id() != std::this_thread::get_id())
    {
        if (processingThread.joinable())
        {
            processingThread.join();
            LOG_I("Processing thread joined");
        }
        else
        {
            LOG_W("Native server - processing thread is not joinable");
        }
    }
    else
    {
        LOG_C("Native server - processing thread cannot join itself");
    }
}

void NativeStreamingServerImpl::stopServerInternal()
{
    stopReading();
    if (serverStopped)
        return;

    serverStopped = true;

    this->context.getOnCoreEvent() -= event(&NativeStreamingServerImpl::coreEventCallback);
    if (const DevicePtr rootDevice = this->rootDeviceRef.assigned() ? this->rootDeviceRef.getRef() : nullptr;
        rootDevice.assigned() && !rootDevice.isRemoved())
    {
        const auto info = rootDevice.getInfo();
        const auto infoInternal = info.asPtr<IDeviceInfoInternal>();

        const auto removeIfPresent = [&info, &infoInternal](const char* protocolId)
        {
            if (info.hasServerCapability(protocolId))
                infoInternal.removeServerCapability(protocolId);
        };

        if (plainChannelEnabled)
        {
            removeIfPresent(CONST_NATIVE_STREAMING_ID);
            removeIfPresent(CONST_NATIVE_CONFIG_ID);
        }
        if (tlsChannelEnabled)
        {
            removeIfPresent(CONST_NATIVE_STREAMING_SECURE_ID);
            removeIfPresent(CONST_NATIVE_CONFIG_SECURE_ID);
        }
        for (const auto& [_, clientNumber] : registeredClientIds)
        {
            if (clientNumber != 0)
                infoInternal.removeConnectedClient(clientNumber);
        }
    }
    registeredClientIds.clear();
    disconnectedClientIds.clear();

    serverHandler->stopServer();
    stopTransportOperations();
    stopProcessingOperations();
}

void NativeStreamingServerImpl::prepareServerHandler()
{
    auto signalSubscribedHandler = [this](const SignalPtr& signal)
    {
        std::scoped_lock lock(readersSync);
        addReader(signal);
    };
    auto signalUnsubscribedHandler = [this](const SignalPtr& signal)
    {
        std::scoped_lock lock(readersSync);
        removeReader(signal);
    };

    // The Callback establishes two objects for each connected client:
    // a new native configuration server and
    // a new packet streaming client (used for client to device streaming);
    // and transfers ownership of these objects to the transport layer session
    SetUpConfigProtocolServerCb createConfigServerCb =
        [this](SendConfigProtocolPacketCb sendConfigPacketCb, const UserPtr& user, ClientType connectionType)
    {
        ProcessConfigProtocolPacketCb processConfigRequestCb = [](PacketBuffer&& packetBuffer) {};
        OnPacketBufferReceivedCallback packetBufferReceivedHandler = [](const packet_streaming::PacketBufferPtr& packetBufferPtr) {};

        if (const DevicePtr rootDevice = this->rootDeviceRef.assigned() ? this->rootDeviceRef.getRef() : nullptr; rootDevice.assigned())
        {
            auto configServer = std::make_shared<ConfigProtocolServer>(rootDevice, sendConfigPacketCb, user, connectionType, this->signals);
            processConfigRequestCb =
                [this, configServer, sendConfigPacketCb](PacketBuffer&& packetBuffer)
            {
                dispatchClientConfigRequest(configServer, sendConfigPacketCb, std::move(packetBuffer));
            };

            auto packetStreamingClient = std::make_shared<packet_streaming::PacketStreamingClient>();
            packetBufferReceivedHandler =
                [this, packetStreamingClient, configServer](const packet_streaming::PacketBufferPtr& packetBufferPtr)
            {
                dispatchClientToDeviceStreamingPacket(configServer, packetStreamingClient, packetBufferPtr);
            };
        }

        return std::make_pair(processConfigRequestCb, packetBufferReceivedHandler);
    };

    auto rootDeviceSignals = List<ISignal>();
    if (const DevicePtr rootDevice = this->rootDeviceRef.assigned() ? this->rootDeviceRef.getRef() : nullptr; rootDevice.assigned())
        rootDeviceSignals = rootDevice.getSignals(search::Recursive(search::Any()));

    auto clientConnectedHandler = [this](const std::string& clientId, const std::string& address, bool isStreamingConnection, ClientType clientType, const std::string& hostName)
    {
        SizeT clientNumber = 0;
        Bool reconnected = False;
        if (auto it = disconnectedClientIds.find(clientId); it != disconnectedClientIds.end())
        {
            reconnected = True;
            clientNumber = it->second;
            disconnectedClientIds.erase(it);
        }
        if (const DevicePtr rootDevice = this->rootDeviceRef.assigned() ? this->rootDeviceRef.getRef() : nullptr;
            rootDevice.assigned() && !rootDevice.isRemoved())
        {
            const auto clientInfo =
                isStreamingConnection
                    ? ConnectedClientInfo(address, ProtocolType::Streaming, CONST_NATIVE_STREAMING_ID, "", hostName)
                    : ConnectedClientInfo(address, ProtocolType::Configuration, CONST_NATIVE_CONFIG_ID, ClientTypeTools::ClientTypeToString(clientType), hostName);
            clientInfo.addProperty(StringProperty("Reconnected", reconnected ? "Yes" : "No"));
            rootDevice.getInfo().asPtr<IDeviceInfoInternal>(true).addConnectedClient(&clientNumber, clientInfo);
        }
        registeredClientIds.insert({clientId, clientNumber});
    };

    auto clientDisconnectedHandler = [this](const std::string& clientId)
    {
        if (auto it = registeredClientIds.find(clientId); it != registeredClientIds.end())
        {
            if (const DevicePtr rootDevice = this->rootDeviceRef.assigned() ? this->rootDeviceRef.getRef() : nullptr;
                rootDevice.assigned() && !rootDevice.isRemoved())
            {
                rootDevice.getInfo().asPtr<IDeviceInfoInternal>(true).removeConnectedClient(it->second);
            }
            disconnectedClientIds.emplace(clientId, it->second);
            registeredClientIds.erase(it);
        }
    };

    serverHandler = std::make_shared<NativeStreamingServerHandler>(context,
                                                                   transportIOContextPtr,
                                                                   rootDeviceSignals,
                                                                   signalSubscribedHandler,
                                                                   signalUnsubscribedHandler,
                                                                   createConfigServerCb,
                                                                   clientConnectedHandler,
                                                                   clientDisconnectedHandler,
                                                                   config);
}

void NativeStreamingServerImpl::populateDefaultConfigFromProvider(const ContextPtr& context, const PropertyObjectPtr& config)
{
    if (!context.assigned())
        return;
    if (!config.assigned())
        return;

    auto options = context.getModuleOptions("OpenDAQNativeStreamingServerModule");
    for (const auto& [key, value] : options)
    {
        if (config.hasProperty(key))
        {
            config->setPropertyValue(key, value);
        }
    }
}

void NativeStreamingServerImpl::processConfigRequestAndSendReply(const ConfigServerPtr& configServerPtr, const PacketBufferPtr& packetBufferPtr, SendConfigProtocolPacketCb sendConfigPacketCb)
{
    auto replyPacketBuffer = configServerPtr->processRequestAndGetReply(*packetBufferPtr);
    sendConfigPacketCb(replyPacketBuffer);
}

void NativeStreamingServerImpl::dispatchClientConfigRequest(const ConfigServerPtr& configServerPtr, SendConfigProtocolPacketCb sendConfigPacketCb, PacketBuffer&& packetBuffer)
{
    // create a ptr to capture
    auto packetBufferPtr = std::make_shared<PacketBuffer>(std::move(packetBuffer));

    // one main processing context & thread + strand - preserves the correct execution order of client's request
    boost::asio::dispatch(
        *processingIOContextPtr,
        processingStrand.wrap(
            [this, configServerPtr, sendConfigPacketCb, packetBufferPtr]()
            {
                processClientConfigRequest(configServerPtr, packetBufferPtr, sendConfigPacketCb);
            }
        )
    );
}

void NativeStreamingServerImpl::dispatchClientToDeviceStreamingPacket(const ConfigServerPtr& configServerPtr, const PacketStreamingClientPtr& packetStreamingClientPtr, const packet_streaming::PacketBufferPtr& packetBufferPtr)
{
    boost::asio::dispatch(
        *processingIOContextPtr,
        processingStrand.wrap(
            [configServerPtr, packetStreamingClientPtr, packetBufferPtr]()
            {
                packetStreamingClientPtr->addPacketBuffer(packetBufferPtr);

                auto [signalNumericId, packet] = packetStreamingClientPtr->getNextDaqPacket();
                while (packet.assigned())
                {
                    configServerPtr->processClientToServerStreamingPacket(signalNumericId, packet);
                    std::tie(signalNumericId, packet) = packetStreamingClientPtr->getNextDaqPacket();
                }
            }
        )
    );
}

void NativeStreamingServerImpl::processClientConfigRequest(const ConfigServerPtr& configServerPtr, const PacketBufferPtr& packetBufferPtr, SendConfigProtocolPacketCb sendConfigPacketCb)
{
    if (packetBufferPtr->getPacketType() == config_protocol::PacketType::Rpc)
    {
        if (workerPool)
        {
            // parallelize only processing of the RPCs which imply replying to client,
            // in this case execution order is orchestrated by client side
            // as client waits for server's reply anyway before triggering subsequent RPC
            boost::asio::dispatch(
                *workerPool,
                [configServerPtr, packetBufferPtr, sendConfigPacketCb] ()
                {
                    processConfigRequestAndSendReply(configServerPtr, packetBufferPtr, sendConfigPacketCb);
                }
            );
        }
        else
        {
            // or process request in the main thread if pool is not available
            processConfigRequestAndSendReply(configServerPtr, packetBufferPtr, sendConfigPacketCb);
        }
    }
    else if (packetBufferPtr->getPacketType() == config_protocol::PacketType::NoReplyRpc)
    {
        // process RPC request in the main thread if it doesn't imply replying to the client
        // in this case execution order is controlled by the server as it cannot be controlled by the client
        configServerPtr->processNoReplyRequest(*packetBufferPtr);
    }
    else
    {
        // process non-RPC requests (e.g protocol connection negotiations) in the main processing thread
        processConfigRequestAndSendReply(configServerPtr, packetBufferPtr, sendConfigPacketCb);
    }
}

PropertyObjectPtr NativeStreamingServerImpl::createDefaultConfig(const ContextPtr& context)
{
    auto defaultConfig = NativeStreamingServerHandler::createDefaultConfig();

    const auto pollingPeriodProp = IntPropertyBuilder("StreamingDataPollingPeriod", DEFAULT_POLLING_PERIOD)
                                       .setMinValue(1)
                                       .setMaxValue(65535)
                                       .setDescription("Polling period in milliseconds "
                                                       "which specifies how often the server collects and sends "
                                                       "subscribed signals' data to clients")
                                       .build();
    defaultConfig.addProperty(pollingPeriodProp);

    const auto maxPacketReadCountProp = IntPropertyBuilder("MaxPacketReadCount", DEFAULT_MAX_PACKET_READ_COUNT)
                                                .setMinValue(1)
                                                .setDescription("Specifies the size of a pre-allocated packet buffer into "
                                                                "which packets are dequeued. The size determines the amount of "
                                                                "packets that can be read in one dequeue call. Should be greater "
                                                                "than the amount of packets generated per polling period for best "
                                                                "performance.")
                                                .build();
    defaultConfig.addProperty(maxPacketReadCountProp);

    populateDefaultConfigFromProvider(context, defaultConfig);
    return defaultConfig;
}

PropertyObjectPtr NativeStreamingServerImpl::populateDefaultConfig(const PropertyObjectPtr& config, const ContextPtr& context)
{
    const auto defConfig = createDefaultConfig(context);
    for (const auto& prop : defConfig.getAllProperties())
    {
        const auto name = prop.getName();
        if (config.hasProperty(name))
            defConfig.setPropertyValue(name, config.getPropertyValue(name));
    }

    return defConfig;
}

ListPtr<IPropertyObject> NativeStreamingServerImpl::getDiscoveryConfigs()
{
    const auto makeRecord = [this](const char* serviceName, const char* portProperty)
    {
        auto discoveryConfig = PropertyObject();
        discoveryConfig.addProperty(StringProperty("ServiceName", serviceName));
        discoveryConfig.addProperty(StringProperty("ServiceCap", CONST_NATIVE_SERVICE_CAPABILITY));
        discoveryConfig.addProperty(StringProperty("Path", config.getPropertyValue(PROPERTY_PATH_SERVER)));
        discoveryConfig.addProperty(IntProperty("Port", config.getPropertyValue(portProperty)));
        discoveryConfig.addProperty(StringProperty("ProtocolVersion", std::to_string(GetLatestConfigProtocolVersion())));
        return discoveryConfig;
    };

    auto discoveryConfigs = List<IPropertyObject>();

    if (plainChannelEnabled)
        discoveryConfigs.pushBack(makeRecord(CONST_NATIVE_SERVICE_NAME, PROPERTY_PORT_SERVER));

#if NATIVE_STREAMING_ENABLE_TLS
    if (tlsChannelEnabled)
        discoveryConfigs.pushBack(makeRecord(CONST_NATIVE_TLS_SERVICE_NAME, PROPERTY_TLS_PORT_SERVER));
#endif

    return discoveryConfigs;
}

ServerTypePtr NativeStreamingServerImpl::createType(const ContextPtr& context)
{
    return ServerType(
        CONST_NATIVE_SERVER_TYPE_ID,
        "openDAQ Native Streaming server",
        "Publishes device structure over openDAQ native configuration protocol and streams data over openDAQ native streaming protocol",
        NativeStreamingServerImpl::createDefaultConfig(context));
}

void NativeStreamingServerImpl::onStopServer()
{
    stopServerInternal();
}

StreamingPtr NativeStreamingServerImpl::onGetStreaming()
{
    return streaming;
}

void NativeStreamingServerImpl::startReading()
{
    readThreadActive = true;
    this->readThread = std::thread([this]()
    {
        daqNameThread("NatSrvStreamRead");
        this->startReadThread();
        LOG_I("Reading thread finished");
    });
}

void NativeStreamingServerImpl::stopReading()
{
    readThreadActive = false;
    if (readThread.joinable())
    {
        readThread.join();
        LOG_I("Reading thread joined");
    }

    auto ports = List<IInputPort>();
    for (const auto& [_, __, port, ___] : signalReaders)
        ports.pushBack(port);

    signalReaders.clear();

    for (const auto& port : ports)
        port.remove();
}

void NativeStreamingServerImpl::startReadThread()
{
    while (readThreadActive)
    {
        bool sendData = false;

        {
            std::scoped_lock lock(readersSync);
            bool repeatRead;
            do
            {
                repeatRead = false;
                SizeT read = 0;
                SizeT count = maxPacketReadCount;
                for (const auto& [_, signalGlobalId, port, connection] : signalReaders)
                {
                    connection->dequeueUpTo(packetBuf.data() + read, &count);
                    auto& packetData = packetIndices[signalGlobalId];
                    packetData.index = static_cast<int>(read);
                    packetData.count = static_cast<int>(count);
                    read += count;
                    count = maxPacketReadCount - read;

                    // Max packet read count exceeded; Send packets and re-read to not drop data.
                    if (count == 0)
                    {
                        repeatRead = true;
                        break;
                    }
                }

                if (read)
                    serverHandler->processStreamingPackets(packetIndices, packetBuf);

                sendData = sendData || read;
                clearIndices();
            }
            while (repeatRead);
        }

        if (sendData)
            serverHandler->sendAvailableStreamingPackets();

        std::this_thread::sleep_for(readThreadSleepTime);
    }
}

void NativeStreamingServerImpl::addReader(SignalPtr signalToRead)
{
    auto it = std::find_if(signalReaders.begin(),
                           signalReaders.end(),
                           [&signalToRead](const std::tuple<SignalPtr, std::string, InputPortPtr, ObjectPtr<IConnectionInternal>>& element)
                           {
                               return std::get<0>(element) == signalToRead;
                           });
    if (it != signalReaders.end())
        return;

    LOG_I("Add reader for signal {}", signalToRead.getGlobalId());

    auto port = InputPort(signalToRead.getContext(), nullptr, "readsig");
    port.connect(signalToRead);
    port.setNotificationMethod(PacketReadyNotification::None);
    auto connection = port.getConnection().asPtr<IConnectionInternal>();

    signalReaders.push_back(std::tuple<SignalPtr, std::string, InputPortPtr, ObjectPtr<IConnectionInternal>>(
        {signalToRead, signalToRead.getGlobalId().toStdString(), port, connection}));
    packetIndices.insert(std::make_pair(signalToRead.getGlobalId().toStdString(), PacketBufferData()));
}

void NativeStreamingServerImpl::removeReader(SignalPtr signalToRead)
{
    auto it = std::find_if(signalReaders.begin(),
                           signalReaders.end(),
                           [&signalToRead](const std::tuple<SignalPtr, std::string, InputPortPtr, ObjectPtr<IConnection>>& element)
                           {
                               return std::get<0>(element) == signalToRead;
                           });
    if (it == signalReaders.end())
        return;

    LOG_I("Remove reader for signal {}", signalToRead.getGlobalId());

    auto port = std::get<2>(*it);
    signalReaders.erase(it);
    packetIndices.erase(signalToRead.getGlobalId().toStdString());
    port.remove();
}

void NativeStreamingServerImpl::clearIndices()
{
    for (auto& [signalId, _] : packetIndices)
    {
        auto& data = packetIndices[signalId];
        data.reset();
    }
}

OPENDAQ_DEFINE_CLASS_FACTORY_WITH_INTERFACE(
    INTERNAL_FACTORY, NativeStreamingServer, daq::IServer,
    daq::DevicePtr, rootDevice,
    PropertyObjectPtr, config,
    const ContextPtr&, context
)

END_NAMESPACE_OPENDAQ_NATIVE_STREAMING_SERVER_MODULE
