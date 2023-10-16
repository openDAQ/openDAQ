#include <native_streaming_client_module/native_streaming_signal_impl.h>

#include <opendaq/event_packet_params.h>
#include <opendaq/signal_remote_ptr.h>

#include <coreobjects/property_object_protected_ptr.h>

BEGIN_NAMESPACE_OPENDAQ_NATIVE_STREAMING_CLIENT_MODULE

static constexpr char delimeter = '*';

NativeStreamingSignalImpl::NativeStreamingSignalImpl(const ContextPtr& ctx,
                                                     const ComponentPtr& parent,
                                                     const DataDescriptorPtr& descriptor,
                                                     const StringPtr& streamingId)
    : SignalRemoteBase(ctx, parent, createLocalId(streamingId))
    , streamingId(streamingId)
    , mirroredDataDescriptor(descriptor)
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

ErrCode NativeStreamingSignalImpl::getDescriptor(IDataDescriptor** descriptor)
{
    OPENDAQ_PARAM_NOT_NULL(descriptor);

    std::scoped_lock lock(this->sync);

    *descriptor = mirroredDataDescriptor.addRefAndReturn();
    return OPENDAQ_SUCCESS;
}

ErrCode NativeStreamingSignalImpl::getDomainSignal(ISignal** signal)
{
    OPENDAQ_PARAM_NOT_NULL(signal);

    std::scoped_lock lock(this->sync);

    *signal = mirroredDomainSignal.addRefAndReturn();
    return OPENDAQ_SUCCESS;
}

Bool NativeStreamingSignalImpl::onTriggerEvent(EventPacketPtr eventPacket)
{
    if (!eventPacket.assigned())
        return False;

    if (eventPacket.getEventId() == event_packet_id::DATA_DESCRIPTOR_CHANGED)
    {
        const auto params = eventPacket.getParameters();
        DataDescriptorPtr newSignalDescriptor = params[event_packet_param::DATA_DESCRIPTOR];
        DataDescriptorPtr newDomainDescriptor = params[event_packet_param::DOMAIN_DATA_DESCRIPTOR];

        if (newSignalDescriptor.assigned())
        {
            mirroredDataDescriptor = newSignalDescriptor;
        }

        if (mirroredDomainSignal.assigned() && newDomainDescriptor.assigned())
        {
            auto domainSignalEventPacket = DataDescriptorChangedEventPacket(newDomainDescriptor, nullptr);
            mirroredDomainSignal.asPtr<ISignalRemote>().triggerEvent(domainSignalEventPacket);
        }
        return True;
    }
    else if (eventPacket.getEventId() == event_packet_id::PROPERTY_CHANGED)
    {
        const auto params = eventPacket.getParameters();
        StringPtr propName = params.get(event_packet_param::NAME);
        BaseObjectPtr propValue = params.get(event_packet_param::VALUE);

        auto protectedObject = this->template borrowPtr<PropertyObjectProtectedPtr>();
        protectedObject.setProtectedPropertyValue(propName, propValue);

        // setProtectedPropertyValue will create new eventPacket, so returns false to disable
        // forwarding of original packet
        return False;
    }

    // packet was not handled so returns True to forward the original packet
    return True;
}

void NativeStreamingSignalImpl::assignDomainSignal(const SignalPtr& domainSignal)
{
    if (domainSignal.asPtrOrNull<ISignalRemote>() == nullptr)
    {
        throw NoInterfaceException(
            fmt::format(R"(Domain signal "{}" does not implement ISignalRemote interface.)",
                        domainSignal.getGlobalId()));
    }

    std::scoped_lock lock(this->sync);

    mirroredDomainSignal = domainSignal;
}

void NativeStreamingSignalImpl::removeDomainSignal()
{
    std::scoped_lock lock(this->sync);

    mirroredDomainSignal = nullptr;
}

EventPacketPtr NativeStreamingSignalImpl::createDataDescriptorChangedEventPacket()
{
    DataDescriptorPtr domainDescriptor;
    if (mirroredDomainSignal.assigned())
        domainDescriptor = mirroredDomainSignal.getDescriptor();
    return DataDescriptorChangedEventPacket(mirroredDataDescriptor, domainDescriptor);
}

END_NAMESPACE_OPENDAQ_NATIVE_STREAMING_CLIENT_MODULE
