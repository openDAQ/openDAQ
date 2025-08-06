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
#include <opendaq/mirrored_device_config_ptr.h>
#include <opendaq/address_info_factory.h>

BEGIN_NAMESPACE_OPENDAQ_NATIVE_STREAMING_CLIENT_MODULE

using namespace opendaq_native_streaming_protocol;
using namespace config_protocol;

static const std::regex RegexIpv6Hostname(R"(^(.+://)?(\[[a-fA-F0-9:]+(?:\%[a-zA-Z0-9_\.-~]+)?\])(?::(\d+))?(/.*)?$)");
static const std::regex RegexIpv4Hostname(R"(^(.+://)([^:/\s]+))");
static const std::regex RegexPort(":(\\d+)");

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
    , acceptNotificationPackets(false) // ignore notification packets by default until the device is connected
    , subscribedToCoreEvent(false)
    , configProtocolRequestTimeout(std::chrono::milliseconds(configProtocolRequestTimeout))
    , restoreClientConfigOnReconnect(restoreClientConfigOnReconnect)
    , connectionString(connectionString)
    , configProtocolReconnectionRetryTimer(std::make_shared<boost::asio::steady_timer>(*reconnectionProcessingIOContextPtr))
    , reconnectionPeriod(std::chrono::milliseconds(reconnectionPeriod))
{
}

NativeDeviceHelper::~NativeDeviceHelper()
{
    closeConnectionOnRemoval();
}

DevicePtr NativeDeviceHelper::connectAndGetDevice(const ComponentPtr& parent, uint16_t& protocolVersion)
{
    auto device = configProtocolClient->connect(parent, protocolVersion);
    protocolVersion = configProtocolClient->getProtocolVersion();
    startAcceptNotificationPackets();
    deviceRef = device;
    return device;
}

void NativeDeviceHelper::subscribeToCoreEvent(const ContextPtr& context)
{
    std::scoped_lock lock(flagsSync);
    if (!subscribedToCoreEvent)
    {
        context.getOnCoreEvent() += event(this, &NativeDeviceHelper::coreEventCallback);
        subscribedToCoreEvent = true;
    }
}

void NativeDeviceHelper::unsubscribeFromCoreEvent(const ContextPtr& context)
{
    std::scoped_lock lock(flagsSync);
    if (subscribedToCoreEvent)
    {
        context.getOnCoreEvent() -= event(this, &NativeDeviceHelper::coreEventCallback);
        subscribedToCoreEvent = false;
    }
}

void NativeDeviceHelper::closeConnectionOnRemoval()
{
    // Stop handling any further incoming notifications
    stopAcceptNotificationPackets();

    // Cancel ongoing reconnection attempt, if still pending
    configProtocolReconnectionRetryTimer->cancel();

    // Replace existing handlers with no-op stubs
    // This detaches the callbacks associated with the removed device from the transport layer client
    if (transportClientHandler)
    {
        transportClientHandler->resetConfigHandlers();
    }

    // Gracefully stop the thread that processes incoming config protocol packets
    if (!processingIOContextPtr->stopped())
    {
        processingIOContextPtr->stop();
    }

    // Gracefully stop the thread responsible for handling reconnection
    if (!reconnectionProcessingIOContextPtr->stopped())
    {
        reconnectionProcessingIOContextPtr->stop();
    }

    // Release the transport layer client - this also automatically closes the connection if it wasn't shared
    // i.e., if it was used exclusively for config protocol communication (which is typically the case)
    transportClientHandler.reset();

    // Immediately fail all pending async config requests with an exception
    cancelPendingConfigRequests(ComponentRemovedException());
}

void NativeDeviceHelper::componentUpdated(const ComponentPtr& sender, const CoreEventArgsPtr& eventArgs)
{
    auto deviceSelf = deviceRef.assigned() ? deviceRef.getRef() : nullptr;
    if (!deviceSelf.assigned())
        return;

    ComponentPtr updatedComponent = sender;

    auto deviceSelfGlobalId = deviceSelf.getGlobalId().toStdString();
    auto updatedComponentGlobalId = updatedComponent.getGlobalId().toStdString();
    if (deviceSelfGlobalId == updatedComponentGlobalId ||
        IdsParser::isNestedComponentId(updatedComponentGlobalId, deviceSelfGlobalId))
    {
        deviceSelf.asPtr<INativeDevicePrivate>(true)->updateDeviceInfo(connectionString);
    }
}

void NativeDeviceHelper::coreEventCallback(ComponentPtr& sender, CoreEventArgsPtr& eventArgs)
{
    switch (static_cast<CoreEventId>(eventArgs.getEventId()))
    {
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
        stopAcceptNotificationPackets();
        cancelPendingConfigRequests(ConnectionLostException());
        configProtocolClient->disconnectExternalSignals();

        updateConnectionStatus(status, statusMessage);
    }
}

void NativeDeviceHelper::tryConfigProtocolReconnect()
{
    try
    {
        startAcceptNotificationPackets();
        configProtocolClient->reconnect(restoreClientConfigOnReconnect);
    }
    catch(const std::exception& e)
    {
        stopAcceptNotificationPackets();
        const auto statusMessage = String(fmt::format("Configuration protocol reconnection failed: {}.", e.what()));
        LOG_E("{}", statusMessage);

        updateConnectionStatus(connectionStatus, statusMessage);

        configProtocolReconnectionRetryTimer->expires_from_now(reconnectionPeriod);
        configProtocolReconnectionRetryTimer->async_wait(
            [deviceHelperWeak = weak_from_this()](const boost::system::error_code& ec)
            {
                if (ec)
                    return;
                if (auto deviceHelperSelf = deviceHelperWeak.lock())
                {
                    auto deviceSelf = deviceHelperSelf->deviceRef.assigned() ? deviceHelperSelf->deviceRef.getRef() : nullptr;
                    if (deviceSelf.assigned())
                        deviceHelperSelf->tryConfigProtocolReconnect();
                }
            }
        );
        return;
    }

    // use tmp var to implicitly copy the enumeration type
    auto tmpStatusValue = connectionStatus;
    tmpStatusValue = "Connected";
    updateConnectionStatus(tmpStatusValue, "");
}

void NativeDeviceHelper::startAcceptNotificationPackets()
{
    acceptNotificationPackets = true;
}

void NativeDeviceHelper::stopAcceptNotificationPackets()
{
    acceptNotificationPackets = false;
}

void NativeDeviceHelper::updateConnectionStatus(const EnumerationPtr& status, const StringPtr& statusMessage)
{
    connectionStatus = status;

    auto deviceSelf = deviceRef.assigned() ? deviceRef.getRef() : nullptr;
    if (!deviceSelf.assigned())
        return;
    deviceSelf.asPtr<INativeDevicePrivate>()->publishConnectionStatus(connectionStatus, statusMessage);
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
        // send packet using a temporary copy of the transport client
        // to allow safe disposal of the member variable during device removal.
        if (auto transportClientHandlerTemp = this->transportClientHandler; transportClientHandlerTemp)
        {
            transportClientHandlerTemp->sendStreamingPacket(signalNumericId, std::move(packet));
        }
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
            [packetBufferPtr, deviceHelperWeak = weak_from_this()]()
            {
                if (auto deviceHelperSelf = deviceHelperWeak.lock())
                {
                    auto deviceSelf = deviceHelperSelf->deviceRef.assigned() ? deviceHelperSelf->deviceRef.getRef() : nullptr;
                    // if the device connection is in progress or completed and device is still alive
                    if (!deviceHelperSelf->deviceRef.assigned() || deviceSelf.assigned())
                        deviceHelperSelf->processConfigPacket(std::move(*packetBufferPtr));
                }
            }
        );
    };

    OnConnectionStatusChangedCallback transportConnectionStatusChangedCb =
        [this](const EnumerationPtr& status, const StringPtr& statusMessage)
    {
        boost::asio::dispatch(
            *reconnectionProcessingIOContextPtr,
            [status, statusMessage, deviceHelperWeak = weak_from_this()]()
            {
                if (auto deviceHelperSelf = deviceHelperWeak.lock())
                {
                    auto deviceSelf = deviceHelperSelf->deviceRef.assigned() ? deviceHelperSelf->deviceRef.getRef() : nullptr;
                    if (deviceSelf.assigned())
                        deviceHelperSelf->transportConnectionStatusChangedHandler(status, statusMessage);
                }
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
    std::scoped_lock lock(requestReplySync);
    replyPackets.insert({requestId, std::promise<PacketBuffer>()});
    return replyPackets.at(requestId).get_future();
}

void NativeDeviceHelper::unregisterConfigRequest(uint64_t requestId)
{
    std::scoped_lock lock(requestReplySync);
    replyPackets.erase(requestId);
}

void NativeDeviceHelper::cancelPendingConfigRequests(const DaqException& e)
{
    std::scoped_lock lock(requestReplySync);
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
        std::scoped_lock lock(requestReplySync);
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
    disconnectAndCleanUp();
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

    const ErrCode errCode = daqTry([&obj, &serialized, &context, &factoryCallback]()
    {
        *obj = Super::Super::template DeserializeConfigComponent<IDevice, NativeDeviceImpl>(serialized, context, factoryCallback).detach();
    });
    OPENDAQ_RETURN_IF_FAILED(errCode);
    return errCode;
}

void NativeDeviceImpl::removed()
{
    disconnectAndCleanUp();
    Super::removed();
}

// retrieves the local configuration object without triggering an RPC call
ErrCode NativeDeviceImpl::getComponentConfig(IPropertyObject** config)
{
    OPENDAQ_PARAM_NOT_NULL(config);
    *config = this->componentConfig.addRefAndReturn();

    return OPENDAQ_SUCCESS;
}

bool NativeDeviceImpl::isAddedToLocalComponentTree()
{
    return true;
}

void NativeDeviceImpl::attachDeviceHelper(std::shared_ptr<NativeDeviceHelper> deviceHelper)
{
    this->deviceHelper = deviceHelper;
}

void NativeDeviceImpl::disconnectAndCleanUp()
{
    if (this->deviceHelper)
    {
        this->deviceHelper->unsubscribeFromCoreEvent(this->context);
        this->deviceHelper->closeConnectionOnRemoval();
    }
}

void NativeDeviceImpl::updateDeviceInfo(const StringPtr& connectionString)
{
    uint16_t configProtocolVersion = clientComm->getProtocolVersion();

    if (clientComm->getProtocolVersion() < 8)
    {
        auto changeableFields = List<IString>();
        if (deviceInfo.hasProperty("userName"))
            changeableFields.pushBack("userName");
        if (deviceInfo.hasProperty("location"))
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

    // Set the connection info for the device
    ServerCapabilityConfigPtr connectionInfo = deviceInfo.getConfigurationConnectionInfo();

    auto host = ConnectionStringUtils::GetHost(connectionString);
    const auto addressInfo = AddressInfoBuilder().setAddress(host)
                                 .setReachabilityStatus(AddressReachabilityStatus::Reachable)
                                 .setType(ConnectionStringUtils::GetHostType(connectionString))
                                 .setConnectionString(connectionString)
                                 .build();

    connectionInfo.setProtocolId(NativeConfigurationDeviceTypeId)
        .setProtocolName("OpenDAQNativeConfiguration")
        .setProtocolType(ProtocolType::ConfigurationAndStreaming)
        .setConnectionType("TCP/IP")
        .addAddress(host)
        .setPort(std::stoi(ConnectionStringUtils::GetPort(connectionString).toStdString()))
        .setPrefix("daq.nd")
        .setConnectionString(connectionString)
        .setProtocolVersion(std::to_string(configProtocolVersion))
        .addAddressInfo(addressInfo)
        .freeze();
}

StringPtr ConnectionStringUtils::GetHostType(const StringPtr& url)
{
    std::string urlString = url.toStdString();
    std::smatch match;

    if (std::regex_search(urlString, match, RegexIpv6Hostname))
        return String("IPv6");
    if (std::regex_search(urlString, match, RegexIpv4Hostname))
        return String("IPv4");
    DAQ_THROW_EXCEPTION(InvalidParameterException, "Host type not found in url: {}", url);
}

StringPtr ConnectionStringUtils::GetHost(const StringPtr& url)
{
    std::string urlString = url.toStdString();
    std::smatch match;

    if (std::regex_search(urlString, match, RegexIpv6Hostname))
        return String(match[2]);
    if (std::regex_search(urlString, match, RegexIpv4Hostname))
        return String(match[2]);
    DAQ_THROW_EXCEPTION(InvalidParameterException, "Host name not found in url: {}", url);
}

StringPtr ConnectionStringUtils::GetPort(const StringPtr& url, const PropertyObjectPtr& config)
{
    std::string outPort;
    std::string urlString = url.toStdString();
    std::smatch match;

    std::string host = GetHost(url).toStdString();
    std::string suffix = urlString.substr(urlString.find(host) + host.size());

    if (std::regex_search(suffix, match, RegexPort))
        outPort = match[1];
    else
        outPort = "7420";

    if (config.assigned())
    {
        std::string ctxPort = config.getPropertyValue("Port");
        if (ctxPort != "7420")
            outPort = ctxPort;
    }

    return outPort;
}

StringPtr ConnectionStringUtils::GetPath(const StringPtr& url)
{
    std::string urlString = url.toStdString();

    std::string host = GetHost(url).toStdString();
    std::string suffix = urlString.substr(urlString.find(host) + host.size());
    auto pos = suffix.find("/");

    if (pos != std::string::npos)
    {
        return String(suffix.substr(pos));
    }
    else
    {
        return String("/");
    }
}

END_NAMESPACE_OPENDAQ_NATIVE_STREAMING_CLIENT_MODULE
