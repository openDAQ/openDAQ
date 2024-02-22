#include <native_streaming_client_module/native_device_impl.h>
#include <native_streaming_client_module/native_streaming_impl.h>

#include <opendaq/custom_log.h>
#include <regex>

BEGIN_NAMESPACE_OPENDAQ_NATIVE_STREAMING_CLIENT_MODULE

using namespace opendaq_native_streaming_protocol;
using namespace config_protocol;

static std::chrono::milliseconds requestTimeout = std::chrono::milliseconds(10000);

NativeDeviceHelper::NativeDeviceHelper(const ContextPtr& context,
                                       NativeStreamingClientHandlerPtr transportProtocolClient)
    : loggerComponent(context.getLogger().getOrAddComponent("NativeDevice"))
    , transportProtocolClient(transportProtocolClient)
{
    setupProtocolClients(context);
}

NativeDeviceHelper::~NativeDeviceHelper()
{
    // reset transport protocol handler of received config packets
    if (transportProtocolClient)
    {
        auto receiveConfigPacketCb = [](const PacketBuffer& packet) {};
        transportProtocolClient->setConfigPacketHandler(receiveConfigPacketCb);
    }
}

DevicePtr NativeDeviceHelper::connectAndGetDevice(const ComponentPtr& parent)
{
    return configProtocolClient->connect(parent);
}

void NativeDeviceHelper::setupProtocolClients(const ContextPtr& context)
{
    SendRequestCallback sendRequestCallback =
        [this](PacketBuffer& packet)
    {
        return this->doConfigRequest(packet);
    };
    configProtocolClient = std::make_unique<ConfigProtocolClient<NativeDeviceImpl>>(context, sendRequestCallback, nullptr);

    auto receiveConfigPacketCb =
        [this](const PacketBuffer& packet)
    {
        return this->receiveConfigPacket(packet);
    };
    transportProtocolClient->setConfigPacketHandler(receiveConfigPacketCb);
}

PacketBuffer NativeDeviceHelper::doConfigRequest(const PacketBuffer& reqPacket)
{
    // future/promise mechanism is used since transport client works asynchronously
    auto reqId = reqPacket.getId();
    replyPackets.insert({reqId, std::promise<PacketBuffer>()});
    std::future<PacketBuffer> future = replyPackets.at(reqId).get_future();
    transportProtocolClient->sendConfigRequest(reqPacket);

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

void NativeDeviceHelper::receiveConfigPacket(const PacketBuffer& packet)
{
    if (packet.getPacketType() == serverNotification)
    {
        configProtocolClient->triggerNotificationPacket(packet);
    }
    else if(auto it = replyPackets.find(packet.getId()); it != replyPackets.end())
    {
        it->second.set_value(PacketBuffer(packet.getBuffer(), true));
    }
    else
    {
        LOG_E("Received reply for unknown request id {}, reply type {:#x}", packet.getId(), packet.getPacketType());
    }
}

NativeDeviceImpl::NativeDeviceImpl(const config_protocol::ConfigProtocolClientCommPtr& configProtocolClientComm,
                                   const std::string& remoteGlobalId,
                                   const ContextPtr& ctx,
                                   const ComponentPtr& parent,
                                   const StringPtr& localId)
    : Super(configProtocolClientComm, remoteGlobalId, ctx, parent, localId)
{
}

// IDevice

ErrCode NativeDeviceImpl::getInfo(IDeviceInfo** info)
{
    OPENDAQ_PARAM_NOT_NULL(info);

    *info = deviceInfo.addRefAndReturn();
    return OPENDAQ_SUCCESS;
}

// INativeDevicePrivate

void NativeDeviceImpl::attachNativeStreaming(const StreamingPtr& streaming)
{
    nativeStreaming = streaming;

    auto self = this->borrowPtr<DevicePtr>();
    const auto signals = self.getSignals(search::Recursive(search::Any()));
    nativeStreaming.addSignals(signals);
    nativeStreaming.setActive(true);

    for (const auto& signal : signals)
    {
        if (signal.getPublic())
        {
            auto mirroredSignalConfigPtr = signal.template asPtr<IMirroredSignalConfig>();
            mirroredSignalConfigPtr.setActiveStreamingSource(nativeStreaming.getConnectionString());
        }
    }
}

void NativeDeviceImpl::attachDeviceHelper(std::unique_ptr<NativeDeviceHelper> deviceHelper)
{
    this->deviceHelper = std::move(deviceHelper);
}

void NativeDeviceImpl::setConnectionString(const StringPtr& connectionString)
{
    if (deviceInfo.assigned())
        return;

    deviceInfo = DeviceInfo(connectionString, "NativeConfigDevice");
    deviceInfo.freeze();
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
