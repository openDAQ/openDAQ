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

NativeStreamingServerBaseImpl::NativeStreamingServerBaseImpl(const DevicePtr& rootDevice, const PropertyObjectPtr& config, const ContextPtr& context)
    : id("OpenDAQNativeStreaming")
    , config(config)
    , rootDeviceRef(rootDevice)
    , context(context)
    , readThreadActive(false)
    , readThreadSleepTime(std::chrono::milliseconds(20))
    , transportIOContextPtr(std::make_shared<boost::asio::io_context>())
    , processingIOContextPtr(std::make_shared<boost::asio::io_context>())
    , processingStrand(*processingIOContextPtr)
    , rootDeviceGlobalId(rootDevice.getGlobalId().toStdString())
    , logger(context.getLogger())
    , loggerComponent(logger.getOrAddComponent(id))
    , serverStopped(false)
{
    auto info = rootDevice.getInfo();
    if (info.hasServerCapability("OpenDAQNativeStreaming"))
        DAQ_THROW_EXCEPTION(InvalidStateException, fmt::format("Device \"{}\" already has an OpenDAQNativeStreaming server capability.", info.getName()));
    if (info.hasServerCapability("OpenDAQNativeConfiguration"))
        DAQ_THROW_EXCEPTION(InvalidStateException, fmt::format("Device \"{}\" already has an OpenDAQNativeConfiguration server capability.", info.getName()));

    startProcessingOperations();
    startTransportOperations();

    prepareServerHandler();
    const uint16_t port = config.getPropertyValue("NativeStreamingPort");
    serverHandler->startServer(port);

    StringPtr path = config.getPropertyValue("Path");

    ServerCapabilityConfigPtr serverCapabilityStreaming =
        ServerCapability("OpenDAQNativeStreaming", "OpenDAQNativeStreaming", ProtocolType::Streaming)
            .setPrefix("daq.ns")
            .setConnectionType("TCP/IP")
            .setPort(port);

    serverCapabilityStreaming.addProperty(StringProperty("Path", path == "/" ? "" : path));
    info.asPtr<IDeviceInfoInternal>(true).addServerCapability(serverCapabilityStreaming);

    ServerCapabilityConfigPtr serverCapabilityConfig =
        ServerCapability("OpenDAQNativeConfiguration", "OpenDAQNativeConfiguration", ProtocolType::ConfigurationAndStreaming)
            .setPrefix("daq.nd")
            .setConnectionType("TCP/IP")
            .setPort(port)
            .setProtocolVersion(std::to_string(GetLatestConfigProtocolVersion()));

    serverCapabilityConfig.addProperty(StringProperty("Path", path == "/" ? "" : path));
    info.asPtr<IDeviceInfoInternal>(true).addServerCapability(serverCapabilityConfig);

    this->context.getOnCoreEvent() += event(this, &NativeStreamingServerBaseImpl::coreEventCallback);

    const uint16_t pollingPeriod = config.getPropertyValue("StreamingDataPollingPeriod");
    readThreadSleepTime = std::chrono::milliseconds(pollingPeriod);

    maxPacketReadCount = config.getPropertyValue("MaxPacketReadCount");
    packetBuf.resize(maxPacketReadCount);
    startReading();
}

NativeStreamingServerBaseImpl::~NativeStreamingServerBaseImpl()
{
    stopServerInternal();
}

std::shared_ptr<ConfigProtocolServer> NativeStreamingServerBaseImpl::createConfigProtocolServer(NotificationReadyCallback sendConfigPacketCb,
                                                                                                const UserPtr& user,
                                                                                                ClientType connectionType)
{
    return nullptr;
}

PropertyObjectPtr NativeStreamingServerBaseImpl::createDefaultConfig(const ContextPtr& context)
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

ServerTypePtr NativeStreamingServerBaseImpl::createType(const ContextPtr& context)
{
    return ServerType(
        "OpenDAQNativeStreaming",
        "openDAQ Native Streaming server",
        "Publishes device structure over openDAQ native configuration protocol and streams data over openDAQ native streaming protocol",
        NativeStreamingServerBaseImpl::createDefaultConfig(context));
}

PropertyObjectPtr NativeStreamingServerBaseImpl::populateDefaultConfig(const PropertyObjectPtr& config, const ContextPtr& context)
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

PropertyObjectPtr NativeStreamingServerBaseImpl::onGetDiscoveryConfig()
{
    auto discoveryConfig = PropertyObject();
    discoveryConfig.addProperty(StringProperty("ServiceName", "_opendaq-streaming-native._tcp.local."));
    discoveryConfig.addProperty(StringProperty("ServiceCap", "OPENDAQ_NS"));
    discoveryConfig.addProperty(StringProperty("Path", config.getPropertyValue("Path")));
    discoveryConfig.addProperty(IntProperty("Port", config.getPropertyValue("NativeStreamingPort")));
    discoveryConfig.addProperty(StringProperty("ProtocolVersion", std::to_string(GetLatestConfigProtocolVersion())));
    return discoveryConfig;
}

void NativeStreamingServerBaseImpl::doStopServer()
{
    stopServerInternal();
}

void NativeStreamingServerBaseImpl::prepareServerHandler()
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
            auto configServer = createConfigProtocolServer(sendConfigPacketCb, user, connectionType);
            processConfigRequestCb =
                [this, configServer, sendConfigPacketCb](PacketBuffer&& packetBuffer)
            {
                auto packetBufferPtr = std::make_shared<PacketBuffer>(std::move(packetBuffer));
                boost::asio::dispatch(
                    *processingIOContextPtr,
                    processingStrand.wrap(
                        [configServer, sendConfigPacketCb, packetBufferPtr]()
                        {
                            if (packetBufferPtr->getPacketType() == config_protocol::PacketType::NoReplyRpc)
                            {
                                configServer->processNoReplyRequest(*packetBufferPtr);
                            }
                            else
                            {
                                auto replyPacketBuffer = configServer->processRequestAndGetReply(*packetBufferPtr);
                                sendConfigPacketCb(replyPacketBuffer);
                            }
                        }
                    )
                );
            };

            auto packetStreamingClient = std::make_shared<packet_streaming::PacketStreamingClient>();
            packetBufferReceivedHandler =
                [this, packetStreamingClient, configServer](const packet_streaming::PacketBufferPtr& packetBufferPtr)
            {
                boost::asio::dispatch(
                    *processingIOContextPtr,
                    processingStrand.wrap(
                        [configServer, packetStreamingClient, packetBufferPtr]()
                        {
                            packetStreamingClient->addPacketBuffer(packetBufferPtr);

                            auto [signalNumericId, packet] = packetStreamingClient->getNextDaqPacket();
                            while (packet.assigned())
                            {
                                configServer->processClientToServerStreamingPacket(signalNumericId, packet);
                                std::tie(signalNumericId, packet) = packetStreamingClient->getNextDaqPacket();
                            }
                        }
                    )
                );
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
                    ? ConnectedClientInfo(address, ProtocolType::Streaming, "OpenDAQNativeStreaming", "", hostName)
                    : ConnectedClientInfo(address, ProtocolType::Configuration, "OpenDAQNativeConfiguration", ClientTypeTools::ClientTypeToString(clientType), hostName);
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

void NativeStreamingServerBaseImpl::startReading()
{
    readThreadActive = true;
    this->readThread = std::thread([this]()
                                   {
                                       daqNameThread("NatSrvStreamRead");
                                       this->startReadThread();
                                       LOG_I("Reading thread finished");
                                   });
}

void NativeStreamingServerBaseImpl::stopReading()
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

void NativeStreamingServerBaseImpl::startReadThread()
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

void NativeStreamingServerBaseImpl::addReader(SignalPtr signalToRead)
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

void NativeStreamingServerBaseImpl::removeReader(SignalPtr signalToRead)
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

void NativeStreamingServerBaseImpl::clearIndices()
{
    for (auto& [signalId, _] : packetIndices)
    {
        auto& data = packetIndices[signalId];
        data.reset();
    }
}

void NativeStreamingServerBaseImpl::startTransportOperations()
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

void NativeStreamingServerBaseImpl::stopTransportOperations()
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

void NativeStreamingServerBaseImpl::startProcessingOperations()
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

void NativeStreamingServerBaseImpl::stopProcessingOperations()
{
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

void NativeStreamingServerBaseImpl::stopServerInternal()
{
    if (serverStopped)
        return;

    serverStopped = true;

    this->context.getOnCoreEvent() -= event(this, &NativeStreamingServerBaseImpl::coreEventCallback);
    if (const DevicePtr rootDevice = this->rootDeviceRef.assigned() ? this->rootDeviceRef.getRef() : nullptr;
        rootDevice.assigned() && !rootDevice.isRemoved())
    {
        const auto info = rootDevice.getInfo();
        const auto infoInternal = info.asPtr<IDeviceInfoInternal>();
        if (info.hasServerCapability("OpenDAQNativeStreaming"))
            infoInternal.removeServerCapability("OpenDAQNativeStreaming");
        if (info.hasServerCapability("OpenDAQNativeConfiguration"))
            infoInternal.removeServerCapability("OpenDAQNativeConfiguration");
        for (const auto& [_, clientNumber] : registeredClientIds)
        {
            if (clientNumber != 0)
                infoInternal.removeConnectedClient(clientNumber);
        }
    }
    registeredClientIds.clear();
    disconnectedClientIds.clear();

    stopReading();
    serverHandler->stopServer();
    stopTransportOperations();
    stopProcessingOperations();
}

void NativeStreamingServerBaseImpl::addSignalsOfComponent(ComponentPtr& component)
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

void NativeStreamingServerBaseImpl::componentAdded(ComponentPtr& sender, CoreEventArgsPtr& eventArgs)
{
    ComponentPtr addedComponent = eventArgs.getParameters().get("Component");

    auto addedComponentGlobalId = addedComponent.getGlobalId().toStdString();
    if (addedComponentGlobalId.find(rootDeviceGlobalId) != 0)
        return;

    LOG_I("Added Component: {};", addedComponentGlobalId);
    addSignalsOfComponent(addedComponent);
}

void NativeStreamingServerBaseImpl::componentRemoved(ComponentPtr& sender, CoreEventArgsPtr& eventArgs)
{
    StringPtr removedComponentLocalId = eventArgs.getParameters().get("Id");

    auto removedComponentGlobalId =
        sender.getGlobalId().toStdString() + "/" + removedComponentLocalId.toStdString();
    if (removedComponentGlobalId.find(rootDeviceGlobalId) != 0)
        return;

    LOG_I("Component: {}; is removed", removedComponentGlobalId);
    serverHandler->removeComponentSignals(removedComponentGlobalId);
}

void NativeStreamingServerBaseImpl::componentUpdated(ComponentPtr& updatedComponent)
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

void NativeStreamingServerBaseImpl::coreEventCallback(ComponentPtr& sender, CoreEventArgsPtr& eventArgs)
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

void NativeStreamingServerBaseImpl::populateDefaultConfigFromProvider(const ContextPtr& context, const PropertyObjectPtr& config)
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

NativeStreamingToDeviceServerImpl::NativeStreamingToDeviceServerImpl(const DevicePtr& rootDevice, const PropertyObjectPtr& config, const ContextPtr& context)
    : daq::StreamingToDeviceServer("OpenDAQNativeStreaming", config, rootDevice, context, nullptr)
    , NativeStreamingServerBaseImpl(rootDevice, config, context)
{
    // TODO - refactor and move it to initializer list
    this->streaming = createWithImplementation<IStreaming, NativeServerStreamingImpl>(this->serverHandler, this->processingIOContextPtr, context);
    this->streaming.asPtr<INativeServerStreamingPrivate>()->upgradeToSafeProcessingCallbacks();
}

NativeStreamingServerBasicImpl::NativeStreamingServerBasicImpl(const DevicePtr& rootDevice, const PropertyObjectPtr& config, const ContextPtr& context)
    : daq::StreamingServer("OpenDAQNativeStreaming", config, rootDevice, context)
    , NativeStreamingServerBaseImpl(rootDevice, config, context)
{}

std::shared_ptr<ConfigProtocolServer> NativeStreamingServerBasicImpl::createConfigProtocolServer(config_protocol::NotificationReadyCallback sendConfigPacketCb, const UserPtr& user, ClientType connectionType)
{
    if (const DevicePtr rootDevice = NativeStreamingServerBaseImpl::rootDeviceRef.assigned() ? NativeStreamingServerBaseImpl::rootDeviceRef.getRef() : nullptr; rootDevice.assigned())
        return std::make_shared<config_protocol::ConfigProtocolServer>(rootDevice, sendConfigPacketCb, user, connectionType, this->signals);
    return nullptr;
}

PropertyObjectPtr NativeStreamingServerBasicImpl::getDiscoveryConfig()
{
    return onGetDiscoveryConfig();
}

void NativeStreamingServerBasicImpl::onStopServer()
{
    doStopServer();
}

std::shared_ptr<ConfigProtocolServer> NativeStreamingToDeviceServerImpl::createConfigProtocolServer(config_protocol::NotificationReadyCallback sendConfigPacketCb, const UserPtr&  user, ClientType connectionType)
{
    if (const DevicePtr rootDevice = NativeStreamingServerBaseImpl::rootDeviceRef.assigned() ? NativeStreamingServerBaseImpl::rootDeviceRef.getRef() : nullptr; rootDevice.assigned())
        return std::make_shared<config_protocol::ConfigProtocolServer>(rootDevice, sendConfigPacketCb, user, connectionType, this->signals);
    return nullptr;
}

void NativeStreamingToDeviceServerImpl::onStopServer()
{
    doStopServer();
}

PropertyObjectPtr NativeStreamingToDeviceServerImpl::getDiscoveryConfig()
{
    return onGetDiscoveryConfig();
}

OPENDAQ_DEFINE_CLASS_FACTORY_WITH_INTERFACE(
    INTERNAL_FACTORY, NativeStreamingServerBasic, daq::IStreamingServer,
    daq::DevicePtr, rootDevice,
    PropertyObjectPtr, config,
    const ContextPtr&, context
)

OPENDAQ_DEFINE_CLASS_FACTORY_WITH_INTERFACE(
    INTERNAL_FACTORY, NativeStreamingToDeviceServer, daq::IStreamingToDeviceServer,
    daq::DevicePtr, rootDevice,
    PropertyObjectPtr, config,
    const ContextPtr&, context
)

END_NAMESPACE_OPENDAQ_NATIVE_STREAMING_SERVER_MODULE
