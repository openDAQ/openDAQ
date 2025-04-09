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

BEGIN_NAMESPACE_OPENDAQ_NATIVE_STREAMING_SERVER_MODULE
using namespace daq;
using namespace opendaq_native_streaming_protocol;
using namespace config_protocol;

static constexpr size_t DEFAULT_MAX_PACKET_READ_COUNT = 5000;
static constexpr size_t DEFAULT_POLLING_PERIOD = 20;

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

    this->context.getOnCoreEvent() += event(&NativeStreamingServerImpl::coreEventCallback);

    const uint16_t pollingPeriod = config.getPropertyValue("StreamingDataPollingPeriod");
    readThreadSleepTime = std::chrono::milliseconds(pollingPeriod);

    maxPacketReadCount = config.getPropertyValue("MaxPacketReadCount");
    packetBuf.resize(maxPacketReadCount);
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

    serverHandler = std::make_shared<NativeStreamingServerHandler>(context,
                                                                   transportIOContextPtr,
                                                                   rootDeviceSignals,
                                                                   signalSubscribedHandler,
                                                                   signalUnsubscribedHandler,
                                                                   createConfigServerCb,
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
