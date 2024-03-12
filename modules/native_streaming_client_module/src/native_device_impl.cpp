#include <native_streaming_client_module/native_device_impl.h>
#include <native_streaming_client_module/native_streaming_impl.h>

#include <opendaq/custom_log.h>
#include <regex>
#include <boost/asio/dispatch.hpp>

#include <opendaq/ids_parser.h>

BEGIN_NAMESPACE_OPENDAQ_NATIVE_STREAMING_CLIENT_MODULE

using namespace opendaq_native_streaming_protocol;
using namespace config_protocol;

static std::chrono::milliseconds requestTimeout = std::chrono::milliseconds(10000);

NativeDeviceHelper::NativeDeviceHelper(const ContextPtr& context,
                                       NativeStreamingClientHandlerPtr transportProtocolClient,
                                       std::shared_ptr<boost::asio::io_context> processingIOContextPtr)
    : processingIOContextPtr(processingIOContextPtr)
    , processingStrand(*(this->processingIOContextPtr))
    , loggerComponent(context.getLogger().getOrAddComponent("NativeDevice"))
    , transportClientHandler(transportProtocolClient)
{
    setupProtocolClients(context);
}

NativeDeviceHelper::~NativeDeviceHelper()
{
    transportClientHandler->resetConfigHandlers();
    processingIOContextPtr->stop();
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

void NativeDeviceHelper::addStreaming(const StreamingPtr& streaming)
{
    this->streaming = streaming;
}

void NativeDeviceHelper::componentAdded(const ComponentPtr& sender, const CoreEventArgsPtr& eventArgs)
{
    auto device = deviceRef.assigned() ? deviceRef.getRef() : nullptr;
    if (!device.assigned())
        return;

    ComponentPtr addedComponent = eventArgs.getParameters().get("Component");

    auto deviceGlobalId = device.getGlobalId().toStdString();
    auto addedComponentGlobalId = addedComponent.getGlobalId().toStdString();
    if (deviceGlobalId != addedComponentGlobalId && !IdsParser::isNestedComponentId(deviceGlobalId, addedComponentGlobalId))
        return;

    LOG_I("Added Component: {};", addedComponentGlobalId);

    if (addedComponent.supportsInterface<ISignal>())
    {
        addSignalsToStreaming({addedComponent.asPtr<ISignal>()});
    }
    else if (addedComponent.supportsInterface<IFolder>())
    {
        auto nestedComponents = addedComponent.asPtr<IFolder>().getItems(search::Recursive(search::Any()));
        for (const auto& nestedComponent : nestedComponents)
        {
            if (nestedComponent.supportsInterface<ISignal>())
            {
                addSignalsToStreaming({nestedComponent.asPtr<ISignal>()});
                LOG_I("Signal: {}; added to streaming", nestedComponent.getGlobalId());
            }
        }
    }
}

void NativeDeviceHelper::addSignalsToStreaming(const ListPtr<ISignal>& signals)
{
    streaming.addSignals(signals);
    for (const auto& signal : signals)
    {
        if (signal.getPublic())
        {
            auto mirroredSignalConfigPtr = signal.template asPtr<IMirroredSignalConfig>();
            mirroredSignalConfigPtr.setActiveStreamingSource(streaming.getConnectionString());
        }
    }
}

void NativeDeviceHelper::coreEventCallback(ComponentPtr& sender, CoreEventArgsPtr& eventArgs)
{
    switch (static_cast<CoreEventId>(eventArgs.getEventId()))
    {
        case CoreEventId::ComponentAdded:
            componentAdded(sender, eventArgs);
            break;
        default:
            break;
    }
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
    transportClientHandler->setConfigHandlers(receiveConfigPacketCb,
                                              [](ClientReconnectionStatus) {});
}

PacketBuffer NativeDeviceHelper::doConfigRequest(const PacketBuffer& reqPacket)
{
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
        throw GeneralErrorException("Native configuration protocol request timed out");
    }
}

void NativeDeviceHelper::processConfigPacket(PacketBuffer&& packet)
{
    if (packet.getPacketType() == ServerNotification)
    {
        configProtocolClient->triggerNotificationPacket(packet);
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
}

NativeDeviceImpl::~NativeDeviceImpl()
{
    if (this->deviceHelper)
    {
        this->deviceHelper->unsubscribeFromCoreEvent(this->context);
    }
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

END_NAMESPACE_OPENDAQ_NATIVE_STREAMING_CLIENT_MODULE
