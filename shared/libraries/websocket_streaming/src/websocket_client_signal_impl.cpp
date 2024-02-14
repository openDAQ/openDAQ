#include <opendaq/event_packet_params.h>
#include <opendaq/data_descriptor_ptr.h>
#include <opendaq/signal_factory.h>
#include <coreobjects/property_factory.h>
#include "websocket_streaming/websocket_client_signal_impl.h"

BEGIN_NAMESPACE_OPENDAQ_WEBSOCKET_STREAMING

static constexpr char delimeter = '#';

WebsocketClientSignalImpl::WebsocketClientSignalImpl(const ContextPtr& ctx,
                                                     const ComponentPtr& parent,
                                                     const StringPtr& streamingId)
    : MirroredSignalBase(ctx, parent, CreateLocalId(streamingId), nullptr)
    , streamingId(streamingId)
{
}

StringPtr WebsocketClientSignalImpl::CreateLocalId(const StringPtr& streamingId)
{
    std::string localId = streamingId;

    const char slash = '/';
    std::replace(localId.begin(), localId.end(), slash, delimeter);

    return String(localId);
}

StringPtr WebsocketClientSignalImpl::onGetRemoteId() const
{
    return streamingId;
}

ErrCode WebsocketClientSignalImpl::getDescriptor(IDataDescriptor** descriptor)
{
    OPENDAQ_PARAM_NOT_NULL(descriptor);

    std::scoped_lock lock(signalMutex);

    *descriptor = mirroredDataDescriptor.addRefAndReturn();
    return OPENDAQ_SUCCESS;
}

Bool WebsocketClientSignalImpl::onTriggerEvent(EventPacketPtr eventPacket)
{
    if (eventPacket.assigned() && eventPacket.getEventId() == event_packet_id::DATA_DESCRIPTOR_CHANGED)
    {
        const auto params = eventPacket.getParameters();
        DataDescriptorPtr newSignalDescriptor = params[event_packet_param::DATA_DESCRIPTOR];
        DataDescriptorPtr newDomainDescriptor = params[event_packet_param::DOMAIN_DATA_DESCRIPTOR];

        std::scoped_lock lock(signalMutex);
        if (newSignalDescriptor.assigned())
        {
            mirroredDataDescriptor = newSignalDescriptor;
        }

        if (domainSignalArtificial.assigned() && newDomainDescriptor.assigned())
        {
            domainSignalArtificial.asPtr<IWebsocketStreamingSignalPrivate>()->assignDescriptor(newDomainDescriptor);
        }
    }

    // No new duplicated event packets have been created so returns true to forward original packet
    return True;
}

void WebsocketClientSignalImpl::createAndAssignDomainSignal(const DataDescriptorPtr& domainDescriptor)
{
    std::scoped_lock lock(signalMutex);

    domainSignalArtificial = createWithImplementation<ISignal, WebsocketClientSignalImpl>(
        this->context,
        this->parent.getRef(),
        CreateLocalId(streamingId+"_time_artificial"));
    domainSignalArtificial.asPtr<IWebsocketStreamingSignalPrivate>()->assignDescriptor(domainDescriptor);
}

void WebsocketClientSignalImpl::assignDescriptor(const DataDescriptorPtr& descriptor)
{
    std::scoped_lock lock(signalMutex);

    mirroredDataDescriptor = descriptor;
}

SignalPtr WebsocketClientSignalImpl::onGetDomainSignal()
{
    std::scoped_lock lock(signalMutex);

    return domainSignalArtificial;
}

END_NAMESPACE_OPENDAQ_WEBSOCKET_STREAMING
