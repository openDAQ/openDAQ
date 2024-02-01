#include <native_streaming_client_module/native_device_impl.h>
#include <native_streaming_client_module/native_streaming_impl.h>

#include <opendaq/custom_log.h>
#include <regex>

BEGIN_NAMESPACE_OPENDAQ_NATIVE_STREAMING_CLIENT_MODULE

using namespace opendaq_native_streaming_protocol;
using namespace config_protocol;

static std::chrono::milliseconds requestTimeout = std::chrono::milliseconds(10000);

NativeDeviceImpl::NativeDeviceImpl(const ContextPtr& context,
                                   const ComponentPtr& parent,
                                   const StringPtr& connectionString,
                                   const StringPtr& host,
                                   const StringPtr& port,
                                   const StringPtr& path)
    : DeviceWrapperImpl()
    , loggerComponent(context.getLogger().getOrAddComponent("NativeConfigurationDevice"))
    , context(context)
    , deviceInfo(DeviceInfo(connectionString, "NativeConfigDevice"))
    , transportProtocolClient(std::make_shared<NativeStreamingClientHandler>(context))
{
    deviceInfo.freeze();

    std::string streamingConnectionString = std::regex_replace(connectionString.toStdString(),
                                                               std::regex(NativeConfigurationDevicePrefix),
                                                               NativeStreamingPrefix);
    nativeStreaming =
        createWithImplementation<IStreaming, NativeStreamingImpl>(streamingConnectionString,
                                                                  host,
                                                                  port,
                                                                  path,
                                                                  context,
                                                                  transportProtocolClient,
                                                                  nullptr,
                                                                  nullptr,
                                                                  nullptr);
    setupProtocolClients();
    configProtocolClient->connect(parent);
    wrappedDevice = configProtocolClient->getDevice();

    validateWrapperInterfaces();

    setDomainSignals();
    activateStreaming();
}

NativeDeviceImpl::~NativeDeviceImpl()
{
    // reset transport protocol handler of received config packets
    auto receiveConfigPacketCb = [](const PacketBuffer& packet) {};
    transportProtocolClient->setConfigPacketHandler(receiveConfigPacketCb);
}

// IDevice

ErrCode NativeDeviceImpl::getInfo(IDeviceInfo** info)
{
    OPENDAQ_PARAM_NOT_NULL(info);

    *info =  deviceInfo.addRefAndReturn();
    return OPENDAQ_SUCCESS;
}

void NativeDeviceImpl::setupProtocolClients()
{
    SendRequestCallback sendRequestCallback =
        [this](PacketBuffer& packet)
    {
        return this->doConfigRequest(packet);
    };
    configProtocolClient = std::make_unique<ConfigProtocolClient>(context, sendRequestCallback, nullptr);

    auto receiveConfigPacketCb =
        [this](const PacketBuffer& packet)
    {
        return this->receiveConfigPacket(packet);
    };
    transportProtocolClient->setConfigPacketHandler(receiveConfigPacketCb);
}

// TODO get rid of device wrapper; or move this validation to tests:
void NativeDeviceImpl::validateWrapperInterfaces()
{
    InspectablePtr wrappedPtr = wrappedDevice.asPtr<IInspectable>();
    auto wrappedInstanceInfsIds = wrappedPtr.getInterfaceIds();

    InspectablePtr thisPtr = this->template borrowPtr<InspectablePtr>();
    auto wrapperInstanceInfsIds = thisPtr.getInterfaceIds();

    for (const auto& id : wrappedInstanceInfsIds)
    {
        if (!thisPtr.supportsInterface(id))
            throw GeneralErrorException("Some interfaces are missed");
    }
    for (const auto& id : wrapperInstanceInfsIds)
    {
        if (!wrappedDevice.supportsInterface(id))
            throw GeneralErrorException("Some interfaces are redundant");
    }
}

/// workaround which recreates signal->domainSignal relations on client side
void NativeDeviceImpl::setDomainSignals()
{
    const auto signals = wrappedDevice.getSignals(search::Recursive(search::Any()));

    for (const auto& signal : signals)
    {
        const auto deserializedDomainSignalId = signal.asPtr<IDeserializeComponent>(true).getDeserializedParameter("domainSignalId");
        if (deserializedDomainSignalId.assigned())
        {
            for (const auto& domainSignal : signals)
            {
                if (domainSignal.asPtr<IMirroredSignalConfig>().getRemoteId() == deserializedDomainSignalId)
                {
                    signal.asPtr<IMirroredSignalPrivate>()->assignDomainSignal(domainSignal);
                }
            }
        }
    }
}

void NativeDeviceImpl::activateStreaming()
{
    const auto signals = wrappedDevice.getSignals(search::Recursive(search::Any()));
    nativeStreaming.addSignals(signals);
    nativeStreaming.setActive(true);

    for (const auto& signal : signals)
    {
        auto mirroredSignalConfigPtr = signal.template asPtr<IMirroredSignalConfig>();
        mirroredSignalConfigPtr.setActiveStreamingSource(nativeStreaming.getConnectionString());
    }
}

PacketBuffer NativeDeviceImpl::doConfigRequest(const PacketBuffer& reqPacket)
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

void NativeDeviceImpl::receiveConfigPacket(const PacketBuffer& packet)
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

END_NAMESPACE_OPENDAQ_NATIVE_STREAMING_CLIENT_MODULE
