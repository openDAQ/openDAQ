#include <native_streaming_client_module/native_streaming_signal_impl.h>

#include <opendaq/event_packet_params.h>
#include <opendaq/mirrored_signal_config_ptr.h>
#include <opendaq/mirrored_signal_private_ptr.h>

#include <coreobjects/property_object_protected_ptr.h>

BEGIN_NAMESPACE_OPENDAQ_NATIVE_STREAMING_CLIENT_MODULE

static constexpr char delimeter = '*';

NativeStreamingSignalImpl::NativeStreamingSignalImpl(const ContextPtr& ctx,
                                                     const ComponentPtr& parent,
                                                     const StringPtr& streamingId)
    : MirroredSignalBase(ctx, parent, createLocalId(streamingId), nullptr)
    , streamingId(streamingId)
{
}

StringPtr NativeStreamingSignalImpl::onGetRemoteId() const
{
    return streamingId;
}

StringPtr NativeStreamingSignalImpl::createLocalId(const StringPtr& streamingId)
{
    std::string localId = streamingId;

    const char slash = '/';
    std::replace(localId.begin(), localId.end(), slash, delimeter);

    return String(localId);
}

Bool NativeStreamingSignalImpl::onTriggerEvent(const EventPacketPtr& eventPacket)
{
    return Super::onTriggerEvent(eventPacket);
}

void NativeStreamingSignalImpl::assignDomainSignal(const SignalPtr& domainSignal)
{
    if (domainSignal.assigned())
        if (domainSignal.asPtrOrNull<IMirroredSignalConfig>() == nullptr)
        {
            throw NoInterfaceException(
                fmt::format(R"(Domain signal "{}" does not implement IMirroredSignalConfig interface.)",
                            domainSignal.getGlobalId()));
        }

    setMirroredDomainSignal(domainSignal.asPtr<IMirroredSignalConfig>());
}

ErrCode NativeStreamingSignalImpl::Deserialize(ISerializedObject* serialized, IBaseObject* context, IFunction* factoryCallback, IBaseObject** obj)
{
    OPENDAQ_PARAM_NOT_NULL(obj);
    return daqTry(
        [&obj, &serialized, &context, &factoryCallback]()
        {
            *obj = Super::DeserializeComponent(
                       serialized,
                       context,
                       factoryCallback,
                       [](const SerializedObjectPtr& serialized,
                          const ComponentDeserializeContextPtr& deserializeContext,
                          const StringPtr& className)
                       {
                           return createWithImplementation<ISignal, NativeStreamingSignalImpl>(
                               deserializeContext.getContext(), deserializeContext.getParent(), deserializeContext.getLocalId());
                       }).detach();
        });
}

void NativeStreamingSignalImpl::deserializeCustomObjectValues(const SerializedObjectPtr& serializedObject,
                                                              const BaseObjectPtr& context,
                                                              const FunctionPtr& factoryCallback)
{
    Super::deserializeCustomObjectValues(serializedObject, context, factoryCallback);
    checkErrorInfo(setMirroredDataDescriptor(this->dataDescriptor));
}

SignalPtr NativeStreamingSignalImpl::onGetDomainSignal()
{
    MirroredSignalConfigPtr sig;
    checkErrorInfo(getMirroredDomainSignal(&sig));
    return sig;
}

DataDescriptorPtr NativeStreamingSignalImpl::onGetDescriptor()
{
    DataDescriptorPtr desc;
    checkErrorInfo(getMirroredDataDescriptor(&desc));
    return desc;
}


END_NAMESPACE_OPENDAQ_NATIVE_STREAMING_CLIENT_MODULE
