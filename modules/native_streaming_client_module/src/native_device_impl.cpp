#include <native_streaming_client_module/native_device_impl.h>
#include <native_streaming_client_module/native_streaming_impl.h>

#include <opendaq/custom_log.h>
#include <opendaq/device_info_internal_ptr.h>
#include <regex>
#include <boost/asio/dispatch.hpp>

#include <opendaq/ids_parser.h>
#include <opendaq/mirrored_device_ptr.h>
#include <opendaq/component_exceptions.h>
#include <opendaq/exceptions.h>

BEGIN_NAMESPACE_OPENDAQ_NATIVE_STREAMING_CLIENT_MODULE

using namespace opendaq_native_streaming_protocol;
using namespace config_protocol;

NativeDeviceHelper::NativeDeviceHelper(const ContextPtr& context,
                                       NativeStreamingClientHandlerPtr transportProtocolClient,
                                       SizeT configProtocolRequestTimeout,
                                       Bool restoreClientConfigOnReconnect,
                                       std::shared_ptr<boost::asio::io_context> processingIOContextPtr,
                                       std::shared_ptr<boost::asio::io_context> reconnectionProcessingIOContextPtr,
                                       std::thread::id reconnectionProcessingThreadId,
                                       const StringPtr& connectionString,
                                       Int reconnectionPeriod)
    : processingIOContextPtr(processingIOContextPtr)
    , reconnectionProcessingIOContextPtr(reconnectionProcessingIOContextPtr)
    , reconnectionProcessingThreadId(reconnectionProcessingThreadId)
    , loggerComponent(context.getLogger().getOrAddComponent("NativeDevice"))
    , transportClientHandler(transportProtocolClient)
    , connectionStatus(Enumeration("ConnectionStatusType", "Connected", context.getTypeManager()))
    , acceptNotificationPackets(true)
    , configProtocolRequestTimeout(std::chrono::milliseconds(configProtocolRequestTimeout))
    , restoreClientConfigOnReconnect(restoreClientConfigOnReconnect)
    , connectionString(connectionString)
    , configProtocolReconnectionRetryTimer(std::make_shared<boost::asio::steady_timer>(*reconnectionProcessingIOContextPtr))
    , reconnectionPeriod(std::chrono::milliseconds(reconnectionPeriod))
{
}

NativeDeviceHelper::~NativeDeviceHelper()
{
    configProtocolReconnectionRetryTimer->cancel();
    closeConnectionOnRemoval();
}

DevicePtr NativeDeviceHelper::connectAndGetDevice(const ComponentPtr& parent, uint16_t protocolVersion)
{
    auto device = configProtocolClient->connect(parent, protocolVersion);
    deviceRef = device;
    return device;
}

uint16_t NativeDeviceHelper::getProtocolVersion() const
{
    return configProtocolClient->getProtocolVersion();
}

void NativeDeviceHelper::subscribeToCoreEvent(const ContextPtr& context)
{
    context.getOnCoreEvent() += event(this, &NativeDeviceHelper::coreEventCallback);
}

void NativeDeviceHelper::unsubscribeFromCoreEvent(const ContextPtr& context)
{
    context.getOnCoreEvent() -= event(this, &NativeDeviceHelper::coreEventCallback);
}

void NativeDeviceHelper::closeConnectionOnRemoval()
{
    configProtocolReconnectionRetryTimer->cancel();

    if (transportClientHandler)
    {
        transportClientHandler->resetConfigHandlers();
    }

    if (!processingIOContextPtr->stopped())
    {
        processingIOContextPtr->stop();
    }

    if (!reconnectionProcessingIOContextPtr->stopped())
    {
        reconnectionProcessingIOContextPtr->stop();
    }

    {
        std::scoped_lock lock(sync);
        configProtocolClient.reset();
        transportClientHandler.reset();
    }

    cancelPendingConfigRequests(ComponentRemovedException());
}

void NativeDeviceHelper::enableStreamingForNewComponent(const ComponentPtr& component)
{
    auto device = deviceRef.getRef();
    if (!device.assigned())
        return;

    // collect all related streaming sources for component by getting sources of all devices
    // which are ancestors of the component
    auto allStreamingSources = List<IStreaming>();
    StreamingPtr activeStreamingSource;
    ComponentPtr ancestorComponent = device.asPtr<IComponent>();

    auto config = device.asPtr<IDevicePrivate>().getDeviceConfig();
    PropertyObjectPtr general = config.getPropertyValue("General");
    bool minHopsEnabled = general.getPropertyValue("StreamingConnectionHeuristic") == 1;

    do
    {
        auto mirroredDevice = ancestorComponent.asPtrOrNull<IMirroredDevice>();
        if (mirroredDevice.assigned())
        {
            auto streamingSources = mirroredDevice.getStreamingSources();
            for (const auto& streaming : streamingSources)
                allStreamingSources.pushBack(streaming);

            if (streamingSources.empty())
            {
                // TODO try to establish new streaming connections using server caps if MinHops enabled
            }

            // streaming sources were created and ordered by priority on the device connection
            // cache first source of the in-tree-deepest ancestor device to be active for signals of new component
            if (!streamingSources.empty())
                activeStreamingSource = streamingSources[0];
        }

        if (minHopsEnabled && ancestorComponent.supportsInterface<IFolder>())
        {
            auto nestedComponents = ancestorComponent.asPtr<IFolder>().getItems();
            for (const auto& nestedComponent : nestedComponents)
            {
                // next-level nested is an ancestor of newly added or the added itself
                if (IdsParser::isNestedComponentId(nestedComponent.getGlobalId(), component.getGlobalId()) ||
                    nestedComponent.getGlobalId() == component.getGlobalId())
                {
                    ancestorComponent = nestedComponent;
                    break;
                }
            }
        }
        else
        {
            break;
        }
    }
    while(ancestorComponent != component);

    if (!activeStreamingSource.assigned())
        return;

    auto setupStreamingForSignal = [this, allStreamingSources, activeStreamingSource](const SignalPtr& signal)
    {
        for (const auto& streaming : allStreamingSources)
            tryAddSignalToStreaming(signal, streaming);
        setSignalActiveStreamingSource(signal, activeStreamingSource);
    };

    // setup streaming sources for all signals of the new component
    if (component.supportsInterface<ISignal>())
    {
        setupStreamingForSignal(component.asPtr<ISignal>());
    }
    else if (component.supportsInterface<IFolder>())
    {
        auto nestedComponents = component.asPtr<IFolder>().getItems(search::Recursive(search::InterfaceId(ISignal::Id)));
        for (const auto& nestedComponent : nestedComponents)
        {
            if (nestedComponent.supportsInterface<ISignal>())
            {
                setupStreamingForSignal(nestedComponent.asPtr<ISignal>());
            }
        }
    }
}

void NativeDeviceHelper::enableStreamingForExistingComponent(const ComponentPtr& component)
{
    auto device = deviceRef.getRef();
    if (!device.assigned())
        return;

    const auto deviceHasStreamingCaps = [](const DevicePtr& dev)
    {
        for (const auto& cap : dev.getInfo().getServerCapabilities())
        {
            if ((cap.getProtocolType() == ProtocolType::Streaming || cap.getProtocolType() == ProtocolType::ConfigurationAndStreaming) &&
                cap.getConnectionString().assigned() &&
                cap.getConnectionString() != "")
                return true;
        }
        return false;
    };

    // assign streaming sources for all nested devices which do not have any but have corresponding caps
    // considering these are newly added devices
    // this will automatically assign streaming sources for all signals of the device
    if (auto mirroredDevice = component.asPtrOrNull<IMirroredDevice>(); mirroredDevice.assigned())
    {
        if (mirroredDevice.getStreamingSources().getCount() == 0 && deviceHasStreamingCaps(mirroredDevice))
            enableStreamingForNewComponent(mirroredDevice);

        for (const auto& nestedDevice : mirroredDevice.getDevices(search::Recursive(search::Any())))
        {
            if (nestedDevice.asPtr<IMirroredDevice>().getStreamingSources().getCount() == 0 &&
                deviceHasStreamingCaps(nestedDevice))
                enableStreamingForNewComponent(nestedDevice);
        }
    }

    // assign streaming sources for all signals which do not have any,
    // considering these are newly added signals
    if (auto signal = component.asPtrOrNull<IMirroredSignalConfig>();
        signal.assigned() && signal.getStreamingSources().getCount() == 0)
    {
        enableStreamingForNewComponent(signal);
    }
    else if (component.supportsInterface<IFolder>())
    {
        auto nestedComponents = component.asPtr<IFolder>().getItems(search::Recursive(search::InterfaceId(ISignal::Id)));
        for (const auto& nestedComponent : nestedComponents)
            if (auto nestedSignal = nestedComponent.asPtrOrNull<IMirroredSignalConfig>();
                nestedSignal.assigned() && nestedSignal.getStreamingSources().getCount() == 0)
                enableStreamingForNewComponent(nestedSignal);
    }
}

void NativeDeviceHelper::componentAdded(const ComponentPtr& sender, const CoreEventArgsPtr& eventArgs)
{
    auto device = deviceRef.assigned() ? deviceRef.getRef() : nullptr;
    if (!device.assigned())
        return;

    ComponentPtr addedComponent = eventArgs.getParameters().get("Component");

    auto deviceGlobalId = device.getGlobalId().toStdString();
    auto addedComponentGlobalId = addedComponent.getGlobalId().toStdString();
    if (!IdsParser::isNestedComponentId(deviceGlobalId, addedComponentGlobalId))
        return;

    LOG_I("Added Component: {};", addedComponentGlobalId);

    enableStreamingForNewComponent(addedComponent);
}

void NativeDeviceHelper::componentUpdated(const ComponentPtr& sender, const CoreEventArgsPtr& eventArgs)
{
    auto device = deviceRef.assigned() ? deviceRef.getRef() : nullptr;
    if (!device.assigned())
        return;

    ComponentPtr updatedComponent = sender;

    auto deviceGlobalId = device.getGlobalId().toStdString();
    auto updatedComponentGlobalId = updatedComponent.getGlobalId().toStdString();
    if (deviceGlobalId == updatedComponentGlobalId ||
        IdsParser::isNestedComponentId(deviceGlobalId, updatedComponentGlobalId) ||
        IdsParser::isNestedComponentId(updatedComponentGlobalId, deviceGlobalId))
    {
        LOG_I("Updated Component: {};", updatedComponentGlobalId);

        if (deviceGlobalId == updatedComponentGlobalId ||
            IdsParser::isNestedComponentId(updatedComponentGlobalId, deviceGlobalId))
        {
            device.asPtr<INativeDevicePrivate>(true)->updateDeviceInfo(connectionString);
            enableStreamingForExistingComponent(device);
        }
        else
        {
            enableStreamingForExistingComponent(updatedComponent);
        }
    }
}

void NativeDeviceHelper::tryAddSignalToStreaming(const SignalPtr& signal, const StreamingPtr& streaming)
{
    if (!signal.getPublic())
        return;

    ErrCode errCode =
        daqTry([&]()
                {
                    streaming.addSignals({signal});
                });
    if (OPENDAQ_SUCCEEDED(errCode))
    {
        LOG_I("Signal {} added to streaming {};", signal.getGlobalId(), streaming.getConnectionString());
    }
    else if (errCode != OPENDAQ_ERR_DUPLICATEITEM)
    {
        checkErrorInfo(errCode);
    }
}

void NativeDeviceHelper::setSignalActiveStreamingSource(const SignalPtr& signal, const StreamingPtr& streaming)
{
    if (!signal.getPublic())
        return;

    auto mirroredSignalConfigPtr = signal.template asPtr<IMirroredSignalConfig>();
    mirroredSignalConfigPtr.setActiveStreamingSource(streaming.getConnectionString());
}

void NativeDeviceHelper::coreEventCallback(ComponentPtr& sender, CoreEventArgsPtr& eventArgs)
{
    switch (static_cast<CoreEventId>(eventArgs.getEventId()))
    {
        case CoreEventId::ComponentAdded:
            componentAdded(sender, eventArgs);
            break;
        case CoreEventId::ComponentUpdateEnd:
            componentUpdated(sender, eventArgs);
            break;
        default:
            break;
    }
}

void NativeDeviceHelper::transportConnectionStatusChangedHandler(const EnumerationPtr& status, const StringPtr& statusMessage)
{
    if (status == "Connected")
    {
        tryConfigProtocolReconnect();
    }
    else
    {
        configProtocolReconnectionRetryTimer->cancel();
        acceptNotificationPackets = false;
        cancelPendingConfigRequests(ConnectionLostException());
        configProtocolClient->disconnectExternalSignals();

        updateConnectionStatus(status, statusMessage);
    }
}

void NativeDeviceHelper::tryConfigProtocolReconnect()
{
    try
    {
        acceptNotificationPackets = true;
        configProtocolClient->reconnect(restoreClientConfigOnReconnect);
    }
    catch(const std::exception& e)
    {
        acceptNotificationPackets = false;
        const auto statusMessage = String(fmt::format("Configuration protocol reconnection failed: {}.", e.what()));
        LOG_E("{}", statusMessage);

        updateConnectionStatus(connectionStatus, statusMessage);

        configProtocolReconnectionRetryTimer->expires_from_now(reconnectionPeriod);
        configProtocolReconnectionRetryTimer->async_wait(
            [this, weak_self = weak_from_this()](const boost::system::error_code& ec)
            {
                if (ec)
                    return;
                if (auto shared_self = weak_self.lock())
                    this->tryConfigProtocolReconnect();
            }
        );
        return;
    }

    // use tmp var to implicitly copy the enumeration type
    auto tmpStatusValue = connectionStatus;
    tmpStatusValue = "Connected";
    updateConnectionStatus(tmpStatusValue, "");
}

void NativeDeviceHelper::updateConnectionStatus(const EnumerationPtr& status, const StringPtr& statusMessage)
{
    connectionStatus = status;

    auto device = deviceRef.assigned() ? deviceRef.getRef() : nullptr;
    if (!device.assigned())
        return;
    device.asPtr<INativeDevicePrivate>()->publishConnectionStatus(connectionStatus, statusMessage);
}

void NativeDeviceHelper::setupProtocolClients(const ContextPtr& context)
{
    SendRequestCallback sendRequestCallback =
        [this](const PacketBuffer& packet)
    {
        return this->doConfigRequestAndGetReply(packet);
    };
    SendNoReplyRequestCallback sendNoReplyRequestCallback =
        [this](const PacketBuffer& packet)
    {
        this->doConfigNoReplyRequest(packet);
    };
    HandleDaqPacketCallback handleDaqPacketCallback =
        [this](PacketPtr&& packet, uint32_t signalNumericId)
    {
        transportClientHandler->sendStreamingPacket(signalNumericId, std::move(packet));
    };
    configProtocolClient =
        std::make_unique<ConfigProtocolClient<NativeDeviceImpl>>(
            context,
            sendRequestCallback,
            sendNoReplyRequestCallback,
            handleDaqPacketCallback,
            nullptr,
            nullptr
        );

    ProcessConfigProtocolPacketCb receiveConfigPacketCb =
        [this](PacketBuffer&& packetBuffer)
    {
        auto packetBufferPtr = std::make_shared<PacketBuffer>(std::move(packetBuffer));
        boost::asio::dispatch(
            *processingIOContextPtr,
            [this, packetBufferPtr, weak_self = weak_from_this()]()
            {
                if (auto shared_self = weak_self.lock())
                    this->processConfigPacket(std::move(*packetBufferPtr));
            }
        );
    };

    OnConnectionStatusChangedCallback transportConnectionStatusChangedCb =
        [this](const EnumerationPtr& status, const StringPtr& statusMessage)
    {
        boost::asio::dispatch(
            *reconnectionProcessingIOContextPtr,
            [this, status, statusMessage, weak_self = weak_from_this()]()
            {
                if (auto shared_self = weak_self.lock())
                    this->transportConnectionStatusChangedHandler(status, statusMessage);
            }
        );
    };

    transportClientHandler->setConfigHandlers(receiveConfigPacketCb,
                                              transportConnectionStatusChangedCb);
}

PacketBuffer NativeDeviceHelper::doConfigRequestAndGetReply(const PacketBuffer& reqPacket)
{
    auto reqId = reqPacket.getId();

    // future/promise mechanism is used since transport client works asynchronously
    // register the request first to ensure connection loss or device removal will be reported
    auto future = registerConfigRequest(reqId);

    // using a thread id is a hacky way to disable all config requests
    // except those related to reconnection until reconnection is finished
    if (connectionStatus != "Connected" &&
        std::this_thread::get_id() != reconnectionProcessingThreadId)
    {
        unregisterConfigRequest(reqId);
        DAQ_THROW_EXCEPTION(ConnectionLostException);
    }

    // send packet using a temporary copy of the transport client
    // to allow safe disposal of the member variable during device removal.
    if (auto transportClientHandlerTemp = this->transportClientHandler; transportClientHandlerTemp)
    {
        transportClientHandlerTemp->sendConfigRequest(reqPacket);
    }
    else
    {
        unregisterConfigRequest(reqId);
        DAQ_THROW_EXCEPTION(ComponentRemovedException);
    }

    if (future.wait_for(configProtocolRequestTimeout) == std::future_status::ready)
    {
        return future.get();
    }
    else // std::future_status::timeout
    {
        unregisterConfigRequest(reqId);
        LOG_E("Native configuration protocol request id {} timed out", reqId);
        if (connectionStatus == "Connected")
            DAQ_THROW_EXCEPTION(GeneralErrorException, "Native configuration protocol request id {} timed out", reqId);
        else
            DAQ_THROW_EXCEPTION(ConnectionLostException, "Native configuration protocol request id {} timed out due to disconnection", reqId);
    }
}

std::future<PacketBuffer> NativeDeviceHelper::registerConfigRequest(uint64_t requestId)
{
    std::scoped_lock lock(sync);
    replyPackets.insert({requestId, std::promise<PacketBuffer>()});
    return replyPackets.at(requestId).get_future();
}

void NativeDeviceHelper::unregisterConfigRequest(uint64_t requestId)
{
    std::scoped_lock lock(sync);
    replyPackets.erase(requestId);
}

void NativeDeviceHelper::cancelPendingConfigRequests(const DaqException& e)
{
    std::scoped_lock lock(sync);
    for (auto it = replyPackets.begin(); it != replyPackets.end(); )
    {
        auto& replyPromise = it->second;
        LOG_W("Cancel config request id {}: {}", it->first, e.what());
        replyPromise.set_exception(std::make_exception_ptr(e));
        it = replyPackets.erase(it);
    }
}

void NativeDeviceHelper::processConfigPacket(PacketBuffer&& packet)
{
    std::scoped_lock lock(sync);
    if (packet.getPacketType() == ServerNotification)
    {
        // allow server notifications only if connected / reconnection started
        if (acceptNotificationPackets && configProtocolClient != nullptr)
        {
            configProtocolClient->triggerNotificationPacket(packet);
        }
        else
        {
            LOG_W("Notification packet from server ignored: \n{}\n", packet.parseServerNotification());
        }
    }
    else
    {
        if(auto it = replyPackets.find(packet.getId()); it != replyPackets.end())
        {
            it->second.set_value(std::move(packet));
            replyPackets.erase(it);
        }
        else
        {
            LOG_E("Received reply for unknown request id {}, reply type {:#x} [{}]",
                packet.getId(),
                static_cast<uint8_t>(packet.getPacketType()),
                packet.getPacketType()
            );
        }
    }
}

void NativeDeviceHelper::doConfigNoReplyRequest(const config_protocol::PacketBuffer& reqPacket)
{
    sendConfigRequest(reqPacket);
}

void NativeDeviceHelper::sendConfigRequest(const config_protocol::PacketBuffer& reqPacket)
{
    // using a thread id is a hacky way to disable all config requests
    // except those related to reconnection until reconnection is finished
    if (connectionStatus != "Connected" &&
        std::this_thread::get_id() != reconnectionProcessingThreadId)
    {
        DAQ_THROW_EXCEPTION(ConnectionLostException);
    }

    // send packet using a temporary copy of the transport client
    // to allow safe disposal of the member variable during device removal.
    if (auto transportClientHandlerTemp = this->transportClientHandler; transportClientHandlerTemp)
        transportClientHandlerTemp->sendConfigRequest(reqPacket);
    else
        DAQ_THROW_EXCEPTION(ComponentRemovedException);
}

NativeDeviceImpl::NativeDeviceImpl(const config_protocol::ConfigProtocolClientCommPtr& configProtocolClientComm,
                                   const std::string& remoteGlobalId,
                                   const ContextPtr& ctx,
                                   const ComponentPtr& parent,
                                   const StringPtr& localId,
                                   const StringPtr& className)
    : Super(configProtocolClientComm, remoteGlobalId, ctx, parent, localId, className)
{
}

NativeDeviceImpl::~NativeDeviceImpl()
{
    if (this->deviceHelper)
    {
        this->deviceHelper->unsubscribeFromCoreEvent(this->context);
    }
}

// INativeDevicePrivate
void NativeDeviceImpl::publishConnectionStatus(const EnumerationPtr& status, const StringPtr& statusMessage)
{
    this->statusContainer.asPtr<IComponentStatusContainerPrivate>().setStatusWithMessage("ConnectionStatus", status, statusMessage);
    this->connectionStatusContainer.updateConnectionStatusWithMessage(deviceInfo.getConnectionString(), status, nullptr, statusMessage);
}

void NativeDeviceImpl::completeInitialization(std::shared_ptr<NativeDeviceHelper> deviceHelper, const StringPtr& connectionString)
{
    attachDeviceHelper(deviceHelper);
    updateDeviceInfo(connectionString);

    const auto statusInitValue = Enumeration("ConnectionStatusType", "Connected", this->context.getTypeManager());
    this->statusContainer.asPtr<IComponentStatusContainerPrivate>().addStatus("ConnectionStatus", statusInitValue);
    this->connectionStatusContainer.addConfigurationConnectionStatus(deviceInfo.getConnectionString(), statusInitValue);
}

ErrCode NativeDeviceImpl::Deserialize(ISerializedObject* serialized,
                                      IBaseObject* context,
                                      IFunction* factoryCallback,
                                      IBaseObject** obj)
{
    OPENDAQ_PARAM_NOT_NULL(context);

    return daqTry([&obj, &serialized, &context, &factoryCallback]()
    {
        *obj = Super::Super::template DeserializeConfigComponent<IDevice, NativeDeviceImpl>(serialized, context, factoryCallback).detach();
    });
}

ErrCode NativeDeviceImpl::getComponentConfig(IPropertyObject** config)
{
    OPENDAQ_PARAM_NOT_NULL(config);
    *config = this->componentConfig.addRefAndReturn();
    return OPENDAQ_SUCCESS;
}

void NativeDeviceImpl::removed()
{
    if (this->deviceHelper)
    {
        this->deviceHelper->unsubscribeFromCoreEvent(this->context);
        this->deviceHelper->closeConnectionOnRemoval();
    }

    Super::removed();
}

void NativeDeviceImpl::attachDeviceHelper(std::shared_ptr<NativeDeviceHelper> deviceHelper)
{
    this->deviceHelper = std::move(deviceHelper);
}

void NativeDeviceImpl::updateDeviceInfo(const StringPtr& connectionString)
{
    if (clientComm->getProtocolVersion() < 8)
    {
        auto changeableFields = List<IString>();
        PropertyPtr userNameProp;
        deviceInfo->getProperty(String("userName"), &userNameProp);
        if (userNameProp.assigned())
            changeableFields.pushBack("userName");
        
        PropertyPtr locationProp;
        deviceInfo->getProperty(String("location"), &locationProp);
        if (locationProp.assigned())
            changeableFields.pushBack("location");
        
        auto newDeviceInfo = DeviceInfoWithChanegableFields(changeableFields);
        auto deviceInfoInternal = newDeviceInfo.asPtr<IPropertyObjectProtected>(true);
    
        for (const auto& prop : deviceInfo.getAllProperties())
        {
            const auto propName = prop.getName();
            if (!newDeviceInfo.hasProperty(propName))
                if (const auto internalProp = prop.asPtrOrNull<IPropertyInternal>(true); internalProp.assigned())
                    newDeviceInfo.addProperty(internalProp.clone());                

            if (const auto propValue = deviceInfo.getPropertyValue(propName); propValue.assigned())
                deviceInfoInternal->setProtectedPropertyValue(propName, propValue);
        }
    
        deviceInfo = newDeviceInfo;
    }

    deviceInfo.asPtr<IPropertyObjectProtected>(true)->setProtectedPropertyValue(String("connectionString"), connectionString);

    if (!deviceInfo.hasProperty("NativeConfigProtocolVersion"))
    {
        auto propBuilder = IntPropertyBuilder("NativeConfigProtocolVersion", clientComm->getProtocolVersion()).setReadOnly(true);
        deviceInfo.addProperty(propBuilder.build());
    }
}

END_NAMESPACE_OPENDAQ_NATIVE_STREAMING_CLIENT_MODULE
