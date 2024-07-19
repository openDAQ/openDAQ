#include <native_streaming_client_module/native_device_impl.h>
#include <native_streaming_client_module/native_streaming_impl.h>

#include <opendaq/custom_log.h>
#include <regex>
#include <boost/asio/dispatch.hpp>

#include <opendaq/ids_parser.h>
#include <opendaq/mirrored_device_ptr.h>
#include <opendaq/component_exceptions.h>

BEGIN_NAMESPACE_OPENDAQ_NATIVE_STREAMING_CLIENT_MODULE

using namespace opendaq_native_streaming_protocol;
using namespace config_protocol;

static std::chrono::milliseconds requestTimeout = std::chrono::milliseconds(10000);

NativeDeviceHelper::NativeDeviceHelper(const ContextPtr& context,
                                       NativeStreamingClientHandlerPtr transportProtocolClient,
                                       std::shared_ptr<boost::asio::io_context> processingIOContextPtr,
                                       std::shared_ptr<boost::asio::io_context> reconnectionProcessingIOContextPtr,
                                       std::thread::id reconnectionProcessingThreadId)
    : processingIOContextPtr(processingIOContextPtr)
    , processingStrand(*(this->processingIOContextPtr))
    , reconnectionProcessingIOContextPtr(reconnectionProcessingIOContextPtr)
    , reconnectionProcessingStrand(*(this->reconnectionProcessingIOContextPtr))
    , reconnectionProcessingThreadId(reconnectionProcessingThreadId)
    , loggerComponent(context.getLogger().getOrAddComponent("NativeDevice"))
    , transportClientHandler(transportProtocolClient)
    , connectionStatus(ClientConnectionStatus::Connected)
{
    setupProtocolClients(context);
}

NativeDeviceHelper::~NativeDeviceHelper()
{
    closeConnectionOnRemoval();
}

DevicePtr NativeDeviceHelper::connectAndGetDevice(const ComponentPtr& parent)
{
    auto device = configProtocolClient->connect(parent);
    deviceRef = device;
    return device;
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

    processingIOContextPtr->stop();
    reconnectionProcessingIOContextPtr->stop();

    configProtocolClient.reset();
    transportClientHandler.reset();
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

            // streaming sources are ordered by priority - cash first to be active
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
    if (deviceGlobalId != updatedComponentGlobalId && !IdsParser::isNestedComponentId(deviceGlobalId, updatedComponentGlobalId))
        return;

    LOG_I("Updated Component: {};", updatedComponentGlobalId);

    enableStreamingForComponent(updatedComponent);
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
            configProtocolClient->reconnect();
        }
        catch(const std::exception& e)
        {
            LOG_W("Reconnection failed: {}", e.what());
            return;
        }
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
        return this->doConfigRequest(packet);
    };
    configProtocolClient = std::make_unique<ConfigProtocolClient<NativeDeviceImpl>>(context, sendRequestCallback, nullptr);

    ProcessConfigProtocolPacketCb receiveConfigPacketCb =
        [this](PacketBuffer&& packetBuffer)
    {
        auto packetBufferPtr = std::make_shared<PacketBuffer>(std::move(packetBuffer));
        boost::asio::dispatch(*processingIOContextPtr, processingStrand.wrap(
            [this, packetBufferPtr]()
            {
                this->processConfigPacket(std::move(*packetBufferPtr));
            }));
    };

    OnConnectionStatusChangedCallback connectionStatusChangedCb =
        [this](ClientConnectionStatus status)
    {
        boost::asio::dispatch(*reconnectionProcessingIOContextPtr, reconnectionProcessingStrand.wrap(
            [this, status]()
            {
                this->connectionStatusChangedHandler(status);
            }));
    };

    transportClientHandler->setConfigHandlers(receiveConfigPacketCb,
                                              connectionStatusChangedCb);
}

PacketBuffer NativeDeviceHelper::doConfigRequest(const PacketBuffer& reqPacket)
{
    if (!transportClientHandler)
    {
        throw ComponentRemovedException();
    }

    // using a thread id is a hacky way to disable all config requests
    // except those related to reconnection until reconnection is finished
    if (connectionStatus != ClientConnectionStatus::Connected &&
        std::this_thread::get_id() != reconnectionProcessingThreadId)
    {
        throw GeneralErrorException("Connection lost");
    }

    // future/promise mechanism is used since transport client works asynchronously
    auto reqId = reqPacket.getId();
    replyPackets.insert({reqId, std::promise<PacketBuffer>()});
    std::future<PacketBuffer> future = replyPackets.at(reqId).get_future();
    transportClientHandler->sendConfigRequest(reqPacket);

    if (future.wait_for(requestTimeout) == std::future_status::ready)
    {
        auto result = future.get();
        replyPackets.erase(reqId);
        return result;
    }
    else
    {
        replyPackets.erase(reqId);
        LOG_E("Native configuration protocol request id {} timed out", reqId);
        throw GeneralErrorException("Native configuration protocol request id {} timed out", reqId);
    }
}

void NativeDeviceHelper::processConfigPacket(PacketBuffer&& packet)
{
    if (packet.getPacketType() == ServerNotification)
    {
        // allow server notifications only if connected / reconnection finished
        if (connectionStatus == ClientConnectionStatus::Connected)
        {
            configProtocolClient->triggerNotificationPacket(packet);
        }
    }
    else if(auto it = replyPackets.find(packet.getId()); it != replyPackets.end())
    {
        it->second.set_value(std::move(packet));
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

NativeDeviceImpl::NativeDeviceImpl(const config_protocol::ConfigProtocolClientCommPtr& configProtocolClientComm,
                                   const std::string& remoteGlobalId,
                                   const ContextPtr& ctx,
                                   const ComponentPtr& parent,
                                   const StringPtr& localId,
                                   const StringPtr& className)
    : Super(configProtocolClientComm, remoteGlobalId, ctx, parent, localId, className)
    , deviceInfoSet(false)
{
    initStatuses(ctx);
}

NativeDeviceImpl::~NativeDeviceImpl()
{
    if (this->deviceHelper)
    {
        this->deviceHelper->unsubscribeFromCoreEvent(this->context);
    }
}

void NativeDeviceImpl::initStatuses(const ContextPtr& ctx)
{
    if (!this->context.getTypeManager().hasType("ConnectionStatusType"))
    {
        const auto statusType = EnumerationType("ConnectionStatusType", List<IString>("Connected",
                                                                                      "Reconnecting",
                                                                                      "Unrecoverable"));
        ctx.getTypeManager().addType(statusType);
    }
    const auto statusInitValue = Enumeration("ConnectionStatusType", "Connected", this->context.getTypeManager());
    this->statusContainer.asPtr<IComponentStatusContainerPrivate>().addStatus("ConnectionStatus", statusInitValue);
}

// INativeDevicePrivate

void NativeDeviceImpl::attachDeviceHelper(std::unique_ptr<NativeDeviceHelper> deviceHelper)
{
    this->deviceHelper = std::move(deviceHelper);
}

void NativeDeviceImpl::setConnectionString(const StringPtr& connectionString)
{
    if (deviceInfoSet)
        return;

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
                newDeviceInfo.asPtrOrNull<IPropertyObjectProtected>(true).setProtectedPropertyValue(propName, propValue);
        }
    }

    newDeviceInfo.freeze();

    deviceInfo = newDeviceInfo;
    deviceInfoSet = true;
}

void NativeDeviceImpl::publishConnectionStatus(ConstCharPtr statusValue)
{
    auto newStatusValue = this->statusContainer.getStatus("ConnectionStatus");
    newStatusValue = statusValue;

    this->statusContainer.asPtr<IComponentStatusContainerPrivate>().setStatus("ConnectionStatus", newStatusValue);
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

END_NAMESPACE_OPENDAQ_NATIVE_STREAMING_CLIENT_MODULE
