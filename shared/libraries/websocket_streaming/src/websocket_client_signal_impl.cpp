#include <opendaq/event_packet_params.h>
#include <opendaq/data_descriptor_ptr.h>
#include <opendaq/signal_factory.h>
#include <opendaq/mirrored_signal_private.h>
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

Bool WebsocketClientSignalImpl::onTriggerEvent(const EventPacketPtr& eventPacket)
{
    return Self::onTriggerEvent(eventPacket);
}

void WebsocketClientSignalImpl::createAndAssignDomainSignal(const DataDescriptorPtr& domainDescriptor)
{
    const auto domainSig = createWithImplementation<IMirroredSignalPrivate, WebsocketClientSignalImpl>(
        this->context, this->parent.getRef(), CreateLocalId(streamingId + "_time_artificial"));
    domainSig->setMirroredDataDescriptor(domainDescriptor);
    setMirroredDomainSignal(domainSig);
}

SignalPtr WebsocketClientSignalImpl::onGetDomainSignal()
{
    return getMirroredDomainSignal();
}

DataDescriptorPtr WebsocketClientSignalImpl::onGetDescriptor()
{
    return getMirroredDataDescriptor();
}

END_NAMESPACE_OPENDAQ_WEBSOCKET_STREAMING
