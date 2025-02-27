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

BEGIN_NAMESPACE_OPENDAQ_NATIVE_STREAMING_SERVER_MODULE

using namespace daq;
using namespace opendaq_native_streaming_protocol;
using namespace config_protocol;

NativeStreamingServerImpl::NativeStreamingServerImpl(const DevicePtr& rootDevice,
                                                     const PropertyObjectPtr& config,
                                                     const ContextPtr& context)
    : Server("OpenDAQNativeStreaming", config, rootDevice, context)
    , readThreadActive(false)
    , readThreadSleepTime(std::chrono::milliseconds(20))
    , transportIOContextPtr(std::make_shared<boost::asio::io_context>())
    , processingStrand(processingIOContext)
    , rootDeviceGlobalId(rootDevice.getGlobalId().toStdString())
    , logger(context.getLogger())
    , loggerComponent(logger.getOrAddComponent(id))
    , serverStopped(false)
{
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
    rootDevice.getInfo().asPtr<IDeviceInfoInternal>(true).addServerCapability(serverCapabilityStreaming);

    ServerCapabilityConfigPtr serverCapabilityConfig =
        ServerCapability("OpenDAQNativeConfiguration", "OpenDAQNativeConfiguration", ProtocolType::ConfigurationAndStreaming)
        .setPrefix("daq.nd")
        .setConnectionType("TCP/IP")
        .setPort(port)
        .setProtocolVersion(std::to_string(GetLatestConfigProtocolVersion()));

    serverCapabilityConfig.addProperty(StringProperty("Path", path == "/" ? "" : path));
    rootDevice.getInfo().asPtr<IDeviceInfoInternal>(true).addServerCapability(serverCapabilityConfig);

    this->context.getOnCoreEvent() += event(&NativeStreamingServerImpl::coreEventCallback);

    const uint16_t pollingPeriod = config.getPropertyValue("StreamingDataPollingPeriod");
    readThreadSleepTime = std::chrono::milliseconds(pollingPeriod);
    startReading();
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

void NativeStreamingServerImpl::startTransportOperations()
{
    transportThread = std::thread(
        [this]()
        {
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
            using namespace boost::asio;
            auto workGuard = make_work_guard(processingIOContext);
            processingIOContext.run();
            LOG_I("Processing thread finished");
        }
    );
}

void NativeStreamingServerImpl::stopProcessingOperations()
{
    processingIOContext.stop();
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
    if (serverStopped)
        return;

    serverStopped = true;

    this->context.getOnCoreEvent() -= event(&NativeStreamingServerImpl::coreEventCallback);
    if (const DevicePtr rootDevice = this->rootDeviceRef.assigned() ? this->rootDeviceRef.getRef() : nullptr; rootDevice.assigned())
    {
        const auto info = rootDevice.getInfo();
        const auto infoInternal = info.asPtr<IDeviceInfoInternal>();
        if (info.hasServerCapability("OpenDAQNativeStreaming"))
            infoInternal.removeServerCapability("OpenDAQNativeStreaming");
        if (info.hasServerCapability("OpenDAQNativeConfiguration"))
            infoInternal.removeServerCapability("OpenDAQNativeConfiguration");
    }

    stopReading();
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
                auto packetBufferPtr = std::make_shared<PacketBuffer>(std::move(packetBuffer));
                boost::asio::dispatch(
                    processingIOContext,
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
                    processingIOContext,
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

    const SizeT maxAllowedConfigConnections = this->config.getPropertyValue("MaxAllowedConfigConnections");
    const SizeT streamingPacketSendTimeout = this->config.getPropertyValue("StreamingPacketSendTimeout");

    serverHandler = std::make_shared<NativeStreamingServerHandler>(context,
                                                                   transportIOContextPtr,
                                                                   rootDeviceSignals,
                                                                   signalSubscribedHandler,
                                                                   signalUnsubscribedHandler,
                                                                   createConfigServerCb,
                                                                   maxAllowedConfigConnections,
                                                                   streamingPacketSendTimeout);
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

PropertyObjectPtr NativeStreamingServerImpl::createDefaultConfig(const ContextPtr& context)
{
    constexpr Int minPortValue = 0;
    constexpr Int maxPortValue = 65535;

    auto defaultConfig = PropertyObject();

    const auto portProp = IntPropertyBuilder("NativeStreamingPort", 7420)
        .setMinValue(minPortValue)
        .setMaxValue(maxPortValue)
        .build();
    defaultConfig.addProperty(portProp);
    defaultConfig.addProperty(StringProperty("Path", "/"));

    // default value "UNLIMITED_CONFIGURATION_CONNECTIONS = 0" stands for unlimited count of concurrent connections
    const auto configConnectionsLimitProp = IntPropertyBuilder("MaxAllowedConfigConnections", UNLIMITED_CONFIGURATION_CONNECTIONS)
                                                .setMinValue(0)
                                                .build();
    defaultConfig.addProperty(configConnectionsLimitProp);

    const auto pollingPeriodProp = IntPropertyBuilder("StreamingDataPollingPeriod", 20)
                                       .setMinValue(1)
                                       .setMaxValue(65535)
                                       .setDescription("Polling period in milliseconds "
                                                       "which specifies how often the server collects and sends "
                                                       "subscribed signals' data to clients")
                                       .build();
    defaultConfig.addProperty(pollingPeriodProp);

    const auto streamingPacketSendTimeoutPropDescription =
        "Defines the timeout for sending streaming packets, measured in milliseconds. "
        "If a timeout is set with this property, and the lifetime of the queued PacketBuffer awaiting transmission exceeds "
        "the specified limit since the buffer's creation before starting the low-level write operation, the server will reset "
        "the connection for the corresponding client that owns the queue, effectively clearing it. "
        "The default value '0' signifies that the streaming server time to send out the packets is not limited, "
        "i.e. the lifetime of PacketBuffers awaiting for transmission, along with the memory allocated for the queue, are both unlimited.";
    const auto streamingPacketSendTimeoutProp = IntPropertyBuilder("StreamingPacketSendTimeout", UNLIMITED_PACKET_SEND_TIME)
                                                    .setMinValue(0)
                                                    .setDescription(streamingPacketSendTimeoutPropDescription)
                                                    .build();
    defaultConfig.addProperty(streamingPacketSendTimeoutProp);

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

PropertyObjectPtr NativeStreamingServerImpl::getDiscoveryConfig()
{
    auto discoveryConfig = PropertyObject();
    discoveryConfig.addProperty(StringProperty("ServiceName", "_opendaq-streaming-native._tcp.local."));
    discoveryConfig.addProperty(StringProperty("ServiceCap", "OPENDAQ_NS"));
    discoveryConfig.addProperty(StringProperty("Path", config.getPropertyValue("Path")));
    discoveryConfig.addProperty(IntProperty("Port", config.getPropertyValue("NativeStreamingPort")));
    discoveryConfig.addProperty(StringProperty("ProtocolVersion", std::to_string(GetLatestConfigProtocolVersion())));
    return discoveryConfig;
}

ServerTypePtr NativeStreamingServerImpl::createType(const ContextPtr& context)
{
    return ServerType(
        "OpenDAQNativeStreaming",
        "openDAQ Native Streaming server",
        "Publishes device structure over openDAQ native configuration protocol and streams data over openDAQ native streaming protocol",
        NativeStreamingServerImpl::createDefaultConfig(context));
}

void NativeStreamingServerImpl::onStopServer()
{
    stopServerInternal();
}

void NativeStreamingServerImpl::startReading()
{
    readThreadActive = true;
    this->readThread = std::thread([this]()
    {
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

    signalReaders.clear();
}

void NativeStreamingServerImpl::startReadThread()
{
    while (readThreadActive)
    {
        {
            std::scoped_lock lock(readersSync);

            bool hasPacketsToSend = false;
            for (const auto& [_, signalGlobalId, reader] : signalReaders)
            {
                PacketPtr packet = reader.read();
                while (packet.assigned())
                {
                    hasPacketsToSend = true;
                    serverHandler->processStreamingPacket(signalGlobalId, std::move(packet));
                    packet = reader.read();
                }
            }
            if (hasPacketsToSend)
                serverHandler->scheduleStreamingWriteTasks();
        }

        std::this_thread::sleep_for(readThreadSleepTime);
    }
}

void NativeStreamingServerImpl::addReader(SignalPtr signalToRead)
{
    auto it = std::find_if(signalReaders.begin(),
                           signalReaders.end(),
                           [&signalToRead](const std::tuple<SignalPtr, std::string, PacketReaderPtr>& element)
                           {
                               return std::get<0>(element) == signalToRead;
                           });
    if (it != signalReaders.end())
        return;

    LOG_I("Add reader for signal {}", signalToRead.getGlobalId());
    auto reader = PacketReader(signalToRead);
    signalReaders.push_back(std::tuple<SignalPtr, std::string, PacketReaderPtr>({signalToRead, signalToRead.getGlobalId().toStdString(), reader}));
}

void NativeStreamingServerImpl::removeReader(SignalPtr signalToRead)
{
    auto it = std::find_if(signalReaders.begin(),
                           signalReaders.end(),
                           [&signalToRead](const std::tuple<SignalPtr, std::string, PacketReaderPtr>& element)
                           {
                               return std::get<0>(element) == signalToRead;
                           });
    if (it == signalReaders.end())
        return;

    LOG_I("Remove reader for signal {}", signalToRead.getGlobalId());
    signalReaders.erase(it);
}

OPENDAQ_DEFINE_CLASS_FACTORY_WITH_INTERFACE(
    INTERNAL_FACTORY, NativeStreamingServer, daq::IServer,
    daq::DevicePtr, rootDevice,
    PropertyObjectPtr, config,
    const ContextPtr&, context
)

END_NAMESPACE_OPENDAQ_NATIVE_STREAMING_SERVER_MODULE
