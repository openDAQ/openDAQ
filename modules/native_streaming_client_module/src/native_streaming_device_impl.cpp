#include <native_streaming_client_module/native_streaming_device_impl.h>
#include <native_streaming_client_module/native_streaming_signal_factory.h>
#include <native_streaming_client_module/native_streaming_factory.h>

#include <opendaq/device_info_factory.h>

#include <coretypes/function_factory.h>

#include <coreobjects/property_object_protected_ptr.h>

BEGIN_NAMESPACE_OPENDAQ_NATIVE_STREAMING_CLIENT_MODULE

NativeStreamingDeviceImpl::NativeStreamingDeviceImpl(const ContextPtr& ctx,
                                                     const ComponentPtr& parent,
                                                     const StringPtr& localId,
                                                     const StringPtr& connectionString,
                                                     const StringPtr& host,
                                                     const StringPtr& port,
                                                     const StringPtr& path)
    : Device(ctx, parent, localId)
    , connectionString(connectionString)
{
    if (!this->connectionString.assigned())
        throw ArgumentNullException("connectionString cannot be null");

    createNativeStreaming(host, port, path);
    activateStreaming();
}

void NativeStreamingDeviceImpl::createNativeStreaming(const StringPtr& host,
                                                      const StringPtr& port,
                                                      const StringPtr& path)
{
    ProcedurePtr onSignalAvailableCallback =
        Procedure([this](const StringPtr& signalStringId,
                         const StringPtr& domainSignalStringId,
                         const DataDescriptorPtr& signalDescriptor,
                         const StringPtr& name,
                         const StringPtr& description)
                  {
                      signalAvailableHandler(signalStringId, domainSignalStringId, signalDescriptor, name, description);
                  });

    ProcedurePtr onSignalUnavailableCallback =
        Procedure([this](const StringPtr& signalStringId)
                  {
                      signalUnavailableHandler(signalStringId);
                  });
    nativeStreaming = NativeStreaming(connectionString,
                                      host,
                                      port,
                                      path,
                                      context,
                                      onSignalAvailableCallback,
                                      onSignalUnavailableCallback);
}

void NativeStreamingDeviceImpl::activateStreaming()
{
    auto self = this->borrowPtr<DevicePtr>();
    const auto signals = self.getSignals();
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
    if (deviceInfo != nullptr)
        return deviceInfo;

    deviceInfo = DeviceInfo(connectionString, "NativeStreamingClientPseudoDevice");
    deviceInfo.freeze();
    return deviceInfo;
}

void NativeStreamingDeviceImpl::updateSignalProperties(const SignalPtr& signal,
                                                       const StringPtr& name,
                                                       const StringPtr& description)
{
    auto compPrivate = signal.asPtr<IComponentPrivate>();

    compPrivate.unlockAllAttributes();
    signal.setName(name);
    signal.setDescription(description);
    compPrivate.lockAllAttributes();
}

void NativeStreamingDeviceImpl::signalAvailableHandler(const StringPtr& signalStringId,
                                                       const StringPtr& domainSignalStringId,
                                                       const DataDescriptorPtr& signalDescriptor,
                                                       const StringPtr& name,
                                                       const StringPtr& description)
{
    if (auto iter = deviceSignals.find(signalStringId); iter != deviceSignals.end())
        throw AlreadyExistsException("Signal with id {} already exists in native streaming device", signalStringId);

    auto signalToAdd = NativeStreamingSignal(this->context, this->signals, signalDescriptor, signalStringId);
    updateSignalProperties(signalToAdd, name, description);

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

void NativeStreamingDeviceImpl::signalUnavailableHandler(const StringPtr& signalStringId)
{
    if (auto iter = deviceSignals.find(signalStringId); iter == deviceSignals.end())
        throw NotFoundException("Signal with id {} is not found in native streaming device", signalStringId);

    auto [signalToRemove,_] = deviceSignals.at(signalStringId);

    // recreate signal -> domainSignal relations in the same way as on server
    for (const auto& item : deviceSignals)
    {
        auto addedSignalId = item.first;
        auto [addedSignal, domainSignalId] = item.second;
        if (domainSignalId == signalStringId)
        {
            addedSignal.asPtr<INativeStreamingSignalPrivate>()->removeDomainSignal();
        }
    }

    this->removeSignal(signalToRemove);
    deviceSignals.erase(signalStringId);
}

END_NAMESPACE_OPENDAQ_NATIVE_STREAMING_CLIENT_MODULE
