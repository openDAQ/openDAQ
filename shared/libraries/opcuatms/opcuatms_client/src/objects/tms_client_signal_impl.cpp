#include <list>
#include "open62541/tmsbsp_nodeids.h"
#include "opcuashared/opcuacommon.h"
#include "opcuatms/exceptions.h"
#include "opcuatms_client/objects/tms_client_signal_impl.h"
#include "opcuatms/converters/variant_converter.h"
#include <opendaq/event_packet_params.h>
#include <opendaq/data_descriptor_ptr.h>


BEGIN_NAMESPACE_OPENDAQ_OPCUA_TMS

using namespace opcua;

TmsClientSignalImpl::TmsClientSignalImpl(
    const ContextPtr& ctx,
    const ComponentPtr& parent,
    const StringPtr& localId,
    const TmsClientContextPtr& clientContext,
    const OpcUaNodeId& nodeId
)
    : TmsClientComponentBaseImpl(ctx, parent, localId, clientContext, nodeId)
{
    deviceSignalId = nodeId.getIdentifier();

    if (referenceUtils.hasReference(nodeId, "Value"))
        descriptorNodeId = std::make_unique<OpcUaNodeId>(referenceUtils.getChildNodeId(referenceUtils.getChildNodeId(nodeId, "Value"), "DataDescriptor"));

    registerObject(this->borrowPtr<BaseObjectPtr>());
}

ErrCode TmsClientSignalImpl::getPublic(Bool* valPublic)
{
    *valPublic = isPublic;
    return OPENDAQ_SUCCESS;
}

ErrCode TmsClientSignalImpl::setPublic(Bool valPublic)
{
    isPublic = valPublic;
    return OPENDAQ_SUCCESS;
}

ErrCode TmsClientSignalImpl::getDescriptor(IDataDescriptor** descriptor)
{
    return daqTry([&]() {
        *descriptor = nullptr;

        if (descriptorNodeId)
        {
            OpcUaVariant opcUaVariant = client->readValue(*descriptorNodeId);
            if (!opcUaVariant.isNull())
            {
                DataDescriptorPtr descriptorPtr = VariantConverter<IDataDescriptor, DataDescriptorPtr>::ToDaqObject(opcUaVariant);
                *descriptor = descriptorPtr.addRefAndReturn();
            }
        }

        return OPENDAQ_SUCCESS;
    });
}

ErrCode TmsClientSignalImpl::setDescriptor(IDataDescriptor* /*descriptor*/)
{
    return OPENDAQ_ERR_OPCUA_CLIENT_CALL_NOT_AVAILABLE;
}

ErrCode TmsClientSignalImpl::getDomainSignal(ISignal** signal)
{
    SignalPtr signalPtr;
    ErrCode errCode = wrapHandlerReturn(this, &TmsClientSignalImpl::onGetDomainSignal, signalPtr);

    *signal = signalPtr.detach();

    return errCode;
}

SignalPtr TmsClientSignalImpl::onGetDomainSignal()
{
    OpcUaNodeId referenceTypeId(NAMESPACE_TMSBSP, UA_TMSBSPID_HASDOMAINSIGNAL);
    auto nodeIds = referenceUtils.getReferencedNodes(nodeId, referenceTypeId, true);
    assert(nodeIds.size() <= 1);

    if (!nodeIds.empty())
    {
        auto domainSignalNodeId = *nodeIds.begin();
        return findSignal(domainSignalNodeId);
    }

    return nullptr;
}

ErrCode TmsClientSignalImpl::setDomainSignal(ISignal* signal)
{
    return OPENDAQ_ERR_OPCUA_CLIENT_CALL_NOT_AVAILABLE;
}

ErrCode TmsClientSignalImpl::getRelatedSignals(IList** signals)
{
    ListPtr<ISignal> signalsPtr;
    ErrCode errCode = wrapHandlerReturn(this, &TmsClientSignalImpl::onGetRelatedSignals, signalsPtr);
    *signals = signalsPtr.detach();
    return errCode;
}


ListPtr<ISignal> TmsClientSignalImpl::onGetRelatedSignals()
{
    OpcUaNodeId referenceTypeId(NAMESPACE_TMSBSP, UA_TMSBSPID_RELATESTOSIGNAL);
    auto nodeIds = referenceUtils.getReferencedNodes(nodeId, referenceTypeId, true);
    ListPtr<ISignal> resultList = List<ISignal>();
    for (auto signalNodeId : nodeIds)
    {
        auto signal = findSignal(signalNodeId);
        resultList.pushBack(signal);
    }
    return resultList.detach();
}

ErrCode TmsClientSignalImpl::setRelatedSignals(IList* signals)
{
    return OPENDAQ_ERR_OPCUA_CLIENT_CALL_NOT_AVAILABLE;
}

ErrCode TmsClientSignalImpl::addRelatedSignal(ISignal* signal)
{
    return OPENDAQ_ERR_OPCUA_CLIENT_CALL_NOT_AVAILABLE;
}

ErrCode TmsClientSignalImpl::removeRelatedSignal(ISignal* signal)
{
    return OPENDAQ_ERR_OPCUA_CLIENT_CALL_NOT_AVAILABLE;
}

ErrCode TmsClientSignalImpl::clearRelatedSignals()
{
    return OPENDAQ_ERR_OPCUA_CLIENT_CALL_NOT_AVAILABLE;
}

ErrCode TmsClientSignalImpl::getName(IString** name)
{
    OPENDAQ_PARAM_NOT_NULL(name);

    auto objPtr = this->borrowPtr<ComponentPtr>();

    return daqTry(
        [&name, &objPtr]()
        {
            *name = objPtr.getPropertyValue("Name").asPtr<IString>().detach();
            return OPENDAQ_SUCCESS;
        });
}

ErrCode TmsClientSignalImpl::setName(IString* name)
{
    return OPENDAQ_ERR_OPCUA_CLIENT_CALL_NOT_AVAILABLE;
}

Bool TmsClientSignalImpl::onTriggerEvent(EventPacketPtr eventPacket)
{
    if (eventPacket.assigned() && eventPacket.getEventId() == event_packet_id::DATA_DESCRIPTOR_CHANGED)
        triggerDataDescriptorChanged(eventPacket);

    // No new duplicated event packets have been created so returns true to forward original packet
    return True;
}

StringPtr TmsClientSignalImpl::onGetRemoteId() const
{
    return String(deviceSignalId);
}

EventPacketPtr TmsClientSignalImpl::createDataDescriptorChangedEventPacket()
{
    const std::lock_guard<std::mutex> lock(signalMutex);
    return DataDescriptorChangedEventPacket(lastSignalDescriptor, lastDomainDescriptor);
}

void TmsClientSignalImpl::triggerDataDescriptorChanged(const EventPacketPtr& eventPacket)
{
    const auto params = eventPacket.getParameters();
    DataDescriptorPtr newSignalDescriptor = params[event_packet_param::DATA_DESCRIPTOR];
    DataDescriptorPtr newDomainDescriptor = params[event_packet_param::DOMAIN_DATA_DESCRIPTOR];

    const std::lock_guard<std::mutex> lock(signalMutex);
    if (newSignalDescriptor.assigned())
    {
        lastSignalDescriptor = newSignalDescriptor;
    }
    if (newDomainDescriptor.assigned())
    {
        lastDomainDescriptor = newDomainDescriptor;
    }
}

END_NAMESPACE_OPENDAQ_OPCUA_TMS
