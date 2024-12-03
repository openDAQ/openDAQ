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
                                       const StringPtr& connectionString)
    : processingIOContextPtr(processingIOContextPtr)
    , reconnectionProcessingIOContextPtr(reconnectionProcessingIOContextPtr)
    , reconnectionProcessingThreadId(reconnectionProcessingThreadId)
    , loggerComponent(context.getLogger().getOrAddComponent("NativeDevice"))
    , transportClientHandler(transportProtocolClient)
    , connectionStatus(ClientConnectionStatus::Connected)
    , acceptNotificationPackets(true)
    , configProtocolRequestTimeout(std::chrono::milliseconds(configProtocolRequestTimeout))
    , restoreClientConfigOnReconnect(restoreClientConfigOnReconnect)
    , connectionString(connectionString)
{
}

NativeDeviceHelper::~NativeDeviceHelper()
{
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

    configProtocolClient.reset();
    transportClientHandler.reset();

    cancelPendingConfigRequests(ComponentRemovedException());
}

void NativeDeviceHelper::enableStreamingForComponent(const ComponentPtr& component)
{
    auto device = deviceRef.getRef();
    if (!device.assigned())
        return;

    // collect all related streaming sources for component by getting sources of all devices
    // which are ancestors of the component
    auto allStreamingSources = List<IStreaming>();
    StreamingPtr activeStreamingSource;
    ComponentPtr ancestorComponent = device.asPtr<IComponent>();

    do
    {
        auto mirroredDevice = ancestorComponent.asPtrOrNull<IMirroredDevice>();
        if (mirroredDevice.assigned())
        {
            auto streamingSources = mirroredDevice.getStreamingSources();
            for (const auto& streaming : streamingSources)
                allStreamingSources.pushBack(streaming);

            // streaming sources are ordered by priority - cache first to be active
            if (!streamingSources.empty())
                activeStreamingSource = streamingSources[0];
        }

        if (ancestorComponent.supportsInterface<IFolder>())
        {
            auto nestedComponents = ancestorComponent.asPtr<IFolder>().getItems();
            for (const auto& nestedComponent : nestedComponents)
            {
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

    // setup streaming sources for all signals of the component
    if (component.supportsInterface<ISignal>())
    {
        setupStreamingForSignal(component.asPtr<ISignal>());
    }
    else if (component.supportsInterface<IFolder>())
    {
        auto nestedComponents = component.asPtr<IFolder>().getItems(search::Recursive(search::Any()));
        for (const auto& nestedComponent : nestedComponents)
        {
            if (nestedComponent.supportsInterface<ISignal>())
            {
                setupStreamingForSignal(nestedComponent.asPtr<ISignal>());
            }
        }
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

    enableStreamingForComponent(addedComponent);
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
            enableStreamingForComponent(device);
        }
        else
        {
            enableStreamingForComponent(updatedComponent);
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

void NativeDeviceHelper::connectionStatusChangedHandler(ClientConnectionStatus status)
{
    if (status == ClientConnectionStatus::Connected)
    {
        try
        {
            acceptNotificationPackets = true;
            configProtocolClient->reconnect(restoreClientConfigOnReconnect);
        }
        catch(const std::exception& e)
        {
            acceptNotificationPackets = false;
            LOG_W("Reconnection failed: {}", e.what());
            return;
        }
    }
    else
    {
        acceptNotificationPackets = false;
        cancelPendingConfigRequests(ConnectionLostException());
        configProtocolClient->disconnectExternalSignals();
    }

    connectionStatus = status;

    auto device = deviceRef.assigned() ? deviceRef.getRef() : nullptr;
    if (!device.assigned())
        return;
    device.asPtr<INativeDevicePrivate>()->publishConnectionStatus(convertConnectionStatusToString(connectionStatus));
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
    SendDaqPacketCallback sendDaqPacketCallback =
        [this](const PacketPtr& packet, uint32_t signalNumericId)
    {
        transportClientHandler->sendStreamingPacket(signalNumericId, packet);
    };
    configProtocolClient =
        std::make_unique<ConfigProtocolClient<NativeDeviceImpl>>(
            context,
            sendRequestCallback,
            sendNoReplyRequestCallback,
            sendDaqPacketCallback,
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

    OnConnectionStatusChangedCallback connectionStatusChangedCb =
        [this](ClientConnectionStatus status)
    {
        boost::asio::dispatch(
            *reconnectionProcessingIOContextPtr,
            [this, status, weak_self = weak_from_this()]()
            {
                if (auto shared_self = weak_self.lock())
                    this->connectionStatusChangedHandler(status);
            }
        );
    };

    transportClientHandler->setConfigHandlers(receiveConfigPacketCb,
                                              connectionStatusChangedCb);
}

PacketBuffer NativeDeviceHelper::doConfigRequestAndGetReply(const PacketBuffer& reqPacket)
{
    auto reqId = reqPacket.getId();

    // future/promise mechanism is used since transport client works asynchronously
    // register the request first to ensure connection loss or device removal will be reported
    auto future = registerConfigRequest(reqId);

    // using a thread id is a hacky way to disable all config requests
    // except those related to reconnection until reconnection is finished
    if (connectionStatus != ClientConnectionStatus::Connected &&
        std::this_thread::get_id() != reconnectionProcessingThreadId)
    {
        unregisterConfigRequest(reqId);
        throw ConnectionLostException();
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
        throw ComponentRemovedException();
    }

    if (future.wait_for(configProtocolRequestTimeout) == std::future_status::ready)
    {
        return future.get();
    }
    else // std::future_status::timeout
    {
        unregisterConfigRequest(reqId);
        LOG_E("Native configuration protocol request id {} timed out", reqId);
        if (connectionStatus == ClientConnectionStatus::Connected)
            throw GeneralErrorException("Native configuration protocol request id {} timed out", reqId);
        else
            throw ConnectionLostException("Native configuration protocol request id {} timed out due to disconnection", reqId);
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
    if (packet.getPacketType() == ServerNotification)
    {
        // allow server notifications only if connected / reconnection started
        if (acceptNotificationPackets)
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
        std::scoped_lock lock(sync);
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
    if (connectionStatus != ClientConnectionStatus::Connected &&
        std::this_thread::get_id() != reconnectionProcessingThreadId)
    {
        throw ConnectionLostException();
    }

    // send packet using a temporary copy of the transport client
    // to allow safe disposal of the member variable during device removal.
    if (auto transportClientHandlerTemp = this->transportClientHandler; transportClientHandlerTemp)
        transportClientHandlerTemp->sendConfigRequest(reqPacket);
    else
        throw ComponentRemovedException();
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

void NativeDeviceImpl::initStatuses()
{
    if (!this->context.getTypeManager().hasType("ConnectionStatusType"))
    {
        const auto statusType = EnumerationType("ConnectionStatusType", List<IString>("Connected",
                                                                                      "Reconnecting",
                                                                                      "Unrecoverable"));
        this->context.getTypeManager().addType(statusType);
    }
    const auto statusInitValue = Enumeration("ConnectionStatusType", "Connected", this->context.getTypeManager());
    this->statusContainer.asPtr<IComponentStatusContainerPrivate>().addStatus("ConnectionStatus", statusInitValue);
}

// INativeDevicePrivate
void NativeDeviceImpl::publishConnectionStatus(ConstCharPtr statusValue)
{
    auto newStatusValue = this->statusContainer.getStatus("ConnectionStatus");
    newStatusValue = statusValue;

    this->statusContainer.asPtr<IComponentStatusContainerPrivate>().setStatus("ConnectionStatus", newStatusValue);
}

void NativeDeviceImpl::completeInitialization(std::shared_ptr<NativeDeviceHelper> deviceHelper, const StringPtr& connectionString)
{
    initStatuses();
    attachDeviceHelper(deviceHelper);
    updateDeviceInfo(connectionString);
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
    const auto newDeviceInfo = DeviceInfo(connectionString, deviceInfo.getName());

    for (const auto& prop : deviceInfo.getAllProperties())
    {
        const auto propName = prop.getName();
        if (!newDeviceInfo.hasProperty(propName))
        {
            const auto internalProp = prop.asPtrOrNull<IPropertyInternal>(true);
            if (!internalProp.assigned())
                continue;

            newDeviceInfo.addProperty(internalProp.clone());
        }

        if (propName != "connectionString" && propName != "name")
        {
            const auto propValue = deviceInfo.getPropertyValue(propName);
            if (propValue.assigned())
                newDeviceInfo.asPtr<IPropertyObjectProtected>(true).setProtectedPropertyValue(propName, propValue);
        }
    }

    if (!newDeviceInfo.hasProperty("NativeConfigProtocolVersion"))
    {
        newDeviceInfo.addProperty(IntProperty("NativeConfigProtocolVersion", clientComm->getProtocolVersion()));
    }

    newDeviceInfo.asPtr<IDeviceInfoInternal>(true).setChangeableProperties(deviceInfo.asPtr<IDeviceInfoInternal>().getChangeableProperties());
    newDeviceInfo.asPtr<IOwnable>(true).setOwner(this->objPtr);
    newDeviceInfo.freeze();

    deviceInfo = newDeviceInfo;
}

END_NAMESPACE_OPENDAQ_NATIVE_STREAMING_CLIENT_MODULE
