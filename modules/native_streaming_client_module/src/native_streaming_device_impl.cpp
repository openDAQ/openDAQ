#include <native_streaming_client_module/native_streaming_device_impl.h>
#include <native_streaming_client_module/native_streaming_signal_impl.h>
#include <native_streaming_client_module/native_streaming_impl.h>

#include <opendaq/device_info_factory.h>
#include <opendaq/component_deserialize_context_factory.h>
#include <opendaq/deserialize_component_ptr.h>
#include <opendaq/component_status_container_private_ptr.h>

#include <coretypes/function_factory.h>

#include <coreobjects/property_object_protected_ptr.h>
#include <opendaq/custom_log.h>

BEGIN_NAMESPACE_OPENDAQ_NATIVE_STREAMING_CLIENT_MODULE
using namespace opendaq_native_streaming_protocol;

NativeStreamingDeviceImpl::NativeStreamingDeviceImpl(const ContextPtr& ctx,
                                                     const ComponentPtr& parent,
                                                     const StringPtr& localId,
                                                     const StringPtr& connectionString,
                                                     NativeStreamingClientHandlerPtr transportProtocolClient,
                                                     std::shared_ptr<boost::asio::io_context> processingIOContextPtr,
                                                     Int initTimeout,
                                                     const DeviceTypePtr& type)
: Device(ctx, parent, localId)
  , connectionString(connectionString)
  , connectionStatus(Enumeration("ConnectionStatusType", "Connected", this->context.getTypeManager()))
  , deviceType(type)
{
    if (!this->connectionString.assigned())
        DAQ_THROW_EXCEPTION(ArgumentNullException, "connectionString cannot be null");
    this->name = "NativeStreamingClientPseudoDevice";

    createNativeStreaming(transportProtocolClient,
                          processingIOContextPtr,
                          initTimeout);
    activateStreaming();
    this->connectionStatusContainer.addStreamingConnectionStatus(connectionString, connectionStatus, nativeStreaming);
    this->statusContainer.asPtr<IComponentStatusContainerPrivate>().addStatus("ConnectionStatus", connectionStatus);
    
    // Set up callback to get alternative addresses from device info for reconnection
    setupAlternativeAddressesCallback(transportProtocolClient);
}

void NativeStreamingDeviceImpl::setupAlternativeAddressesCallback(opendaq_native_streaming_protocol::NativeStreamingClientHandlerPtr transportProtocolClient)
{
    if (!transportProtocolClient)
        return;

    auto loggerComponent =  this->context.getLogger().getOrAddComponent("NativeStreamingDevice");

    WeakRefPtr<IDevice> deviceWeak = this->template borrowPtr<DevicePtr>();
    opendaq_native_streaming_protocol::GetAlternativeAddressesCallback callback =
        [deviceWeak, loggerComponent]() -> ListPtr<IString>
    {
        DevicePtr deviceSelf = deviceWeak.assigned() ? deviceWeak.getRef() : nullptr;
        if (!deviceSelf.assigned())
        {
            LOG_D("Streaming device weak reference is not assigned");
            return List<IString>();
        }

        try
        {
            // First try to get addresses from parent device (real device) if available
            auto parent = deviceSelf.getParent();
            if (parent.assigned())
            {
                auto parentDevice = parent.asPtrOrNull<IDevice>();
                if (parentDevice.assigned())
                {
                    auto parentDeviceInfo = parentDevice.getInfo();
                    if (parentDeviceInfo.assigned())
                    {
                        // Try to get streaming capability from parent device first
                        auto streamingCapability = parentDeviceInfo.getServerCapability("OpenDAQNativeStreaming");
                        if (streamingCapability.assigned())
                        {
                            auto addressInfos = streamingCapability.getAddressInfo();
                            if (addressInfos.assigned() && addressInfos.getCount() > 0)
                            {
                                // Return only addresses (port and path are the same for all addresses of the same device)
                                ListPtr<IString> addresses = List<IString>();
                                for (const auto& addressInfo : addressInfos)
                                {
                                    auto address = addressInfo.getAddress();
                                    if (address.assigned())
                                        addresses.pushBack(address);
                                }
                                if (addresses.getCount() > 0)
                                {
                                    LOG_D("Streaming: Found {} alternative addresses from parent device streaming capability", addresses.getCount());
                                    return addresses;
                                }
                            }
                        }
                        
                        // Fallback: use config protocol addresses (same transport layer)
                        auto configCapability = parentDeviceInfo.getServerCapability("OpenDAQNativeConfiguration");
                        if (configCapability.assigned())
                        {
                            auto addressInfos = configCapability.getAddressInfo();
                            if (addressInfos.assigned() && addressInfos.getCount() > 0)
                            {
                                // Return only addresses (port and path are the same for all addresses of the same device)
                                ListPtr<IString> addresses = List<IString>();
                                for (const auto& addressInfo : addressInfos)
                                {
                                    auto address = addressInfo.getAddress();
                                    if (address.assigned())
                                        addresses.pushBack(address);
                                }
                                if (addresses.getCount() > 0)
                                {
                                    LOG_D("Streaming: Found {} alternative addresses from parent device config capability", addresses.getCount());
                                    return addresses;
                                }
                            }
                        }
                    }
                }
            }
            else
            {
                LOG_D("Streaming device has no parent device");
            }

            // Fallback to streaming device's own info
            auto deviceInfo = deviceSelf.getInfo();
            if (!deviceInfo.assigned())
            {
                LOG_D("Streaming device info is not assigned");
                return List<IString>();
            }

            // Get streaming capability (OpenDAQNativeStreaming)
            auto capability = deviceInfo.getServerCapability("OpenDAQNativeStreaming");
            if (!capability.assigned())
            {
                LOG_D("Streaming capability not found in device info");
                return List<IString>();
            }

            // Get all address info entries
            auto addressInfos = capability.getAddressInfo();
            if (!addressInfos.assigned() || addressInfos.getCount() == 0)
            {
                LOG_D("Streaming capability has {} address info entries", addressInfos.assigned() ? addressInfos.getCount() : 0);
                return List<IString>();
            }

            // Return only addresses (port and path are the same for all addresses of the same device)
            ListPtr<IString> addresses = List<IString>();
            for (const auto& addressInfo : addressInfos)
            {
                auto address = addressInfo.getAddress();
                if (address.assigned())
                    addresses.pushBack(address);
            }

            if (addresses.getCount() > 0)
            {
                LOG_D("Streaming: Found {} alternative addresses from own device info", addresses.getCount());
            }

            return addresses;
        }
        catch (const std::exception& e)
        {
            LOG_W("Exception while getting alternative connection strings: {}", e.what());
            return List<IString>();
        }
    };

    transportProtocolClient->setAlternativeAddressesCallback(callback);
}

void NativeStreamingDeviceImpl::removed()
{
    this->connectionStatusContainer.removeStreamingConnectionStatus(connectionString);
    nativeStreaming.release();
    Device::removed();
}

void NativeStreamingDeviceImpl::createNativeStreaming(NativeStreamingClientHandlerPtr transportProtocolClient,
                                                      std::shared_ptr<boost::asio::io_context> processingIOContextPtr,
                                                      Int initTimeout)
{
    ProcedurePtr onSignalAvailableCallback =
        Procedure([this](const StringPtr& signalStringId,
                         const StringPtr& serializedSignal)
                  {
                      signalAvailableHandler(signalStringId, serializedSignal);
                  });

    ProcedurePtr onSignalUnavailableCallback =
        Procedure([this](const StringPtr& signalStringId)
                  {
                      signalUnavailableHandler(signalStringId);
                  });

    OnConnectionStatusChangedCallback onConnectionStatusChangedCallback =
        [this](const EnumerationPtr& status, const StringPtr& statusMessage)
        {
            connectionStatusChangedHandler(status, statusMessage);
        };

    nativeStreaming =
        createWithImplementation<IStreaming, NativeStreamingBasicImpl>(connectionString,
                                                                       context,
                                                                       transportProtocolClient,
                                                                       processingIOContextPtr,
                                                                       initTimeout,
                                                                       onSignalAvailableCallback,
                                                                       onSignalUnavailableCallback,
                                                                       onConnectionStatusChangedCallback);
    nativeStreaming.asPtr<INativeStreamingPrivate>()->upgradeToSafeProcessingCallbacks();
}

void NativeStreamingDeviceImpl::activateStreaming()
{
    auto self = this->borrowPtr<DevicePtr>();
    const auto signals = self.getSignals(search::Any());
    nativeStreaming.addSignals(signals);
    nativeStreaming.setActive(true);

    for (const auto& signal : signals)
    {
        auto mirroredSignalConfigPtr = signal.template asPtr<IMirroredSignalConfig>();
        mirroredSignalConfigPtr.setActiveStreamingSource(nativeStreaming.getConnectionString());
    }
}

DeviceInfoPtr NativeStreamingDeviceImpl::onGetInfo()
{
    auto info = DeviceInfo(connectionString, "NativeStreamingClientPseudoDevice");
    info.setDeviceType(deviceType);
    return info;
}

SignalPtr NativeStreamingDeviceImpl::createSignal(const StringPtr& signalStringId,
                                                  const StringPtr& serializedSignal)
{
    const auto deserializer = JsonDeserializer();
    const auto deserializeContext = ComponentDeserializeContext(context, nullptr, this->signals, signalStringId);

    auto signal = deserializer.deserialize(
        serializedSignal,
        deserializeContext,
        [](const StringPtr& typeId, const SerializedObjectPtr& object, const BaseObjectPtr& context, const FunctionPtr& factoryCallback) -> BaseObjectPtr
        {
            if (typeId != "Signal")
                return nullptr;
            BaseObjectPtr obj;
            checkErrorInfo(NativeStreamingSignalImpl::Deserialize(object, context, factoryCallback, &obj));
            return obj;
        });

    if (nativeStreaming.assigned())
    {
        nativeStreaming.addSignals({signal});
        signal.template asPtr<IMirroredSignalConfig>().setActiveStreamingSource(nativeStreaming.getConnectionString());
    }
    return signal;
}

void NativeStreamingDeviceImpl::addToDeviceSignals(const StringPtr& signalStringId,
                                                   const StringPtr& serializedSignal)
{
    if (auto iter = deviceSignals.find(signalStringId); iter != deviceSignals.end())
        DAQ_THROW_EXCEPTION(AlreadyExistsException, "Signal with id {} already exists in native streaming device", signalStringId);

    auto signalToAdd = createSignal(signalStringId, serializedSignal);
    StringPtr domainSignalStringId = signalToAdd.asPtr<IDeserializeComponent>(true).getDeserializedParameter("domainSignalId");

    // recreate signal -> domainSignal relations in the same way as on server
    for (const auto& item : deviceSignals)
    {
        auto addedSignalId = item.first;
        auto [addedSignal, domainSignalId] = item.second;
        if (domainSignalId == signalStringId)
        {
            addedSignal.asPtr<INativeStreamingSignalPrivate>()->assignDomainSignal(signalToAdd);
        }
        if (domainSignalStringId == addedSignalId)
        {
            signalToAdd.asPtr<INativeStreamingSignalPrivate>()->assignDomainSignal(addedSignal);
        }
    }

    this->addSignal(signalToAdd);
    deviceSignals.insert({signalStringId, {signalToAdd, domainSignalStringId}});
}

void NativeStreamingDeviceImpl::addToDeviceSignalsOnReconnection(const StringPtr& signalStringId,
                                                                 const StringPtr& serializedSignal)
{
    SignalPtr signalToAdd;

    if (auto iter1 = deviceSignals.find(signalStringId); iter1 != deviceSignals.end())
    {
        signalToAdd = iter1->second.first;
        // TODO update existing signal
    }
    else
    {
        if (auto iter2 = deviceSignalsReconnection.find(signalStringId); iter2 == deviceSignalsReconnection.end())
            signalToAdd = createSignal(signalStringId, serializedSignal);
        else
            DAQ_THROW_EXCEPTION(AlreadyExistsException, "Signal with id {} already exists in native streaming device", signalStringId);
    }

    StringPtr domainSignalStringId = signalToAdd.asPtr<IDeserializeComponent>(true).getDeserializedParameter("domainSignalId");

    // remove domain signal if it is no longer assigned after reconnection
    if (signalToAdd.getDomainSignal().assigned() && !domainSignalStringId.assigned())
    {
        signalToAdd.asPtr<INativeStreamingSignalPrivate>()->assignDomainSignal(nullptr);
    }
    // recreate signal -> domainSignal relations in the same way as on server
    for (const auto& item : deviceSignalsReconnection)
    {
        auto addedSignalId = item.first;
        auto [addedSignal, domainSignalId] = item.second;
        if (domainSignalId == signalStringId)
        {
            addedSignal.asPtr<INativeStreamingSignalPrivate>()->assignDomainSignal(signalToAdd);
        }
        if (domainSignalStringId == addedSignalId)
        {
            signalToAdd.asPtr<INativeStreamingSignalPrivate>()->assignDomainSignal(addedSignal);
        }
    }

    // add only new signals under device
    if (auto iter = deviceSignals.find(signalStringId); iter == deviceSignals.end())
        this->addSignal(signalToAdd);
    deviceSignalsReconnection.insert({signalStringId, {signalToAdd, domainSignalStringId}});
}

void NativeStreamingDeviceImpl::signalAvailableHandler(const StringPtr& signalStringId,
                                                       const StringPtr& serializedSignal)
{
    if (connectionStatus == "Reconnecting")
    {
        addToDeviceSignalsOnReconnection(signalStringId, serializedSignal);
    }
    else
    {
        addToDeviceSignals(signalStringId, serializedSignal);
    }
}

void NativeStreamingDeviceImpl::signalUnavailableHandler(const StringPtr& signalStringId)
{
    if (auto iter = deviceSignals.find(signalStringId); iter == deviceSignals.end())
        DAQ_THROW_EXCEPTION(NotFoundException, "Signal with id {} is not found in native streaming device", signalStringId);

    auto [signalToRemove,_] = deviceSignals.at(signalStringId);

    // recreate signal -> domainSignal relations in the same way as on server
    for (const auto& item : deviceSignals)
    {
        auto [addedSignal, domainSignalId] = item.second;
        if (domainSignalId == signalStringId)
        {
            addedSignal.asPtr<INativeStreamingSignalPrivate>()->assignDomainSignal(nullptr);
        }
    }

    this->removeSignal(signalToRemove);
    deviceSignals.erase(signalStringId);
}

void NativeStreamingDeviceImpl::connectionStatusChangedHandler(const EnumerationPtr& status, const StringPtr& statusMessage)
{
    if (status == "Connected")
    {
        // remove signals which no longer exist after reconnection
        for (const auto& item : deviceSignals)
        {
            auto signalId = item.first;
            auto signal = item.second.first;

            if (auto iter = deviceSignalsReconnection.find(signalId); iter == deviceSignalsReconnection.end())
            {
                this->removeSignal(signal);
            }
        }

        // replace signal container with new one
        deviceSignals = deviceSignalsReconnection;
        deviceSignalsReconnection.clear();
    }
    connectionStatus = status;

    this->statusContainer.asPtr<IComponentStatusContainerPrivate>().setStatusWithMessage("ConnectionStatus", connectionStatus, statusMessage);
    this->connectionStatusContainer.asPtr<IConnectionStatusContainerPrivate>().updateConnectionStatusWithMessage(
        this->connectionString,
        connectionStatus,
        nativeStreaming,
        statusMessage
    );
}

END_NAMESPACE_OPENDAQ_NATIVE_STREAMING_CLIENT_MODULE
