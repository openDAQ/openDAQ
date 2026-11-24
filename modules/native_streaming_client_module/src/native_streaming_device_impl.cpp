#include <native_streaming_client_module/native_streaming_device_impl.h>
#include <native_streaming_protocol/native_streaming_constants.h>
#include <native_streaming_client_module/native_streaming_signal_impl.h>
#include <native_streaming_client_module/native_streaming_impl.h>
#include <native_streaming_client_module/native_device_utils.h>

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
                                                     const DeviceTypePtr& type,
                                                     std::shared_ptr<std::atomic<bool>> callbacksHoldDevice)
: Device(ctx, parent, localId)
  , connectionString(connectionString)
  , connectionStatus(Enumeration("ConnectionStatusType", "Connected", this->context.getTypeManager()))
  , connectedStatusMessage(buildConnectedStatusMessage(transportProtocolClient->getTransport()))
  , deviceType(type)
  , transportProtocolClient(transportProtocolClient)
  , callbacksHoldDevice(std::move(callbacksHoldDevice))
{
    if (!this->connectionString.assigned())
        DAQ_THROW_EXCEPTION(ArgumentNullException, "connectionString cannot be null");
    this->name = "NativeStreamingClientPseudoDevice";

    createNativeStreaming(transportProtocolClient,
                          processingIOContextPtr,
                          initTimeout);
    activateStreaming();
    this->connectionStatusContainer.addStreamingConnectionStatus(connectionString, connectionStatus, nativeStreaming);
    // the status is added with an empty message, while the "Connected" one names the transport
    this->connectionStatusContainer.asPtr<IConnectionStatusContainerPrivate>().updateConnectionStatusWithMessage(
        connectionString,
        connectionStatus,
        nativeStreaming,
        connectedStatusMessage
    );
    this->statusContainer.asPtr<IComponentStatusContainerPrivate>().addStatus("ConnectionStatus", connectionStatus);
}

ErrCode NativeStreamingDeviceImpl::setComponentConfig(IPropertyObject* config)
{
    OPENDAQ_RETURN_IF_FAILED(Super::setComponentConfig(config));

    DeviceInfoPtr deviceInfo;
    OPENDAQ_RETURN_IF_FAILED(getInfo(&deviceInfo));

    ListPtr<IString> alternativeAddresses;
    const ErrCode errCode = collectAlternativeAddresses(componentConfig, 
                                                        deviceInfo,
                                                        CONST_NATIVE_STREAMING_ID, 
                                                        alternativeAddresses);
    
    OPENDAQ_RETURN_IF_FAILED(errCode);
    
    if (errCode == OPENDAQ_SUCCESS)
        transportProtocolClient->setAlternativeAddresses(alternativeAddresses);

    return OPENDAQ_SUCCESS;
}

void NativeStreamingDeviceImpl::removed()
{
    this->connectionStatusContainer.removeStreamingConnectionStatus(connectionString);
    nativeStreaming.release();
    transportProtocolClient.reset();
    Device::removed();
}

void NativeStreamingDeviceImpl::createNativeStreaming(NativeStreamingClientHandlerPtr transportProtocolClient,
                                                      std::shared_ptr<boost::asio::io_context> processingIOContextPtr,
                                                      Int initTimeout)
{
    // The streaming object can call these when the device is removed or released: a call holds the device, or is dropped
    const auto thisRef = this->template getWeakRefInternal<IDevice>();
    const auto runOnDevice = [thisRef, required = callbacksHoldDevice](const auto& handler)
    {
        const DevicePtr device = thisRef.getRef();
        if (!device.assigned() && *required)
            return;

        handler();
    };

    // Signal changes hold the device lock and are dropped once the device is removed
    const auto runLockedOnDevice = [this, runOnDevice](const auto& handler)
    {
        runOnDevice(
            [&]
            {
                const auto lock = this->getRecursiveConfigLock2();
                if (!this->isComponentRemoved)
                    handler();
            });
    };

    ProcedurePtr onSignalAvailableCallback =
        Procedure([this, runLockedOnDevice](const StringPtr& signalStringId,
                                            const StringPtr& serializedSignal)
                  {
                      runLockedOnDevice([&] { signalAvailableHandler(signalStringId, serializedSignal); });
                  });

    ProcedurePtr onSignalUnavailableCallback =
        Procedure([this, runLockedOnDevice](const StringPtr& signalStringId)
                  {
                      runLockedOnDevice([&] { signalUnavailableHandler(signalStringId); });
                  });

    OnConnectionStatusChangedCallback onConnectionStatusChangedCallback =
        [this, runOnDevice](const EnumerationPtr& status, const StringPtr& statusMessage)
        {
            runOnDevice([&] { connectionStatusChangedHandler(status, statusMessage); });
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
    StreamingPtr streaming;
    {
        const auto lock = this->getRecursiveConfigLock2();
        if (this->isComponentRemoved)
            return;

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
        streaming = nativeStreaming;
    }

    // Published without the device lock, which only the signal changes above need, against a removal at the same time
    this->statusContainer.asPtr<IComponentStatusContainerPrivate>().setStatusWithMessage("ConnectionStatus", status, statusMessage);

    // A removal since the lock was released has dropped the connection status, so there is nothing to update
    const ErrCode errCode =
        this->connectionStatusContainer->updateConnectionStatusWithMessage(this->connectionString,
                                                                      status,
                                                                      streaming,
                                                                      status == "Connected" && statusMessage == "" ? connectedStatusMessage : statusMessage);
    checkErrorInfoExcept(errCode, OPENDAQ_ERR_NOTFOUND);
}

END_NAMESPACE_OPENDAQ_NATIVE_STREAMING_CLIENT_MODULE
