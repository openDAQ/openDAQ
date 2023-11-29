#include <list>
#include "open62541/daqbsp_nodeids.h"
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

    if (hasReference("Value"))
    {
        const auto valueNodeId = clientContext->getReferenceBrowser()->getChildNodeId(nodeId, "Value");
        const auto dataDescriptorNodeId = clientContext->getReferenceBrowser()->getChildNodeId(valueNodeId, "DataDescriptor");
        descriptorNodeId = std::make_unique<OpcUaNodeId>(dataDescriptorNodeId);
    }

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
    auto filter = BrowseFilter();
    filter.referenceTypeId = OpcUaNodeId(NAMESPACE_DAQBSP, UA_DAQBSPID_HASDOMAINSIGNAL);
    filter.direction = UA_BROWSEDIRECTION_FORWARD;

    const auto& references = clientContext->getReferenceBrowser()->browseFiltered(nodeId, filter);
    assert(references.byNodeId.size() <= 1);

    if (!references.byNodeId.empty())
    {
        auto domainSignalNodeId = references.byNodeId.begin().key();
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
    auto filter = BrowseFilter();
    filter.referenceTypeId = OpcUaNodeId(NAMESPACE_DAQBSP, UA_DAQBSPID_RELATESTOSIGNAL);
    filter.direction = UA_BROWSEDIRECTION_FORWARD;

    const auto& references = clientContext->getReferenceBrowser()->browseFiltered(nodeId, filter);

    ListPtr<ISignal> resultList = List<ISignal>();

    for (const auto& [signalNodeId, ref] : references.byNodeId)
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

Bool TmsClientSignalImpl::onTriggerEvent(EventPacketPtr eventPacket)
{
    // No new duplicated event packets have been created so returns true to forward original packet
    return True;
}


StringPtr TmsClientSignalImpl::onGetRemoteId() const
{
    return String(deviceSignalId);
}

ErrCode TmsClientSignalImpl::getLastValue(IBaseObject** value)
{
    *value = nullptr;

    auto readValueFunction = [this](IBaseObject** value, const std::string& nodeName)
        {
            const auto valueNodeId = clientContext->getReferenceBrowser()->getChildNodeId(nodeId, nodeName);
            OpcUaVariant opcUaVariant = client->readValue(*valueNodeId);
            if (!opcUaVariant.isNull())
            {
                BaseObjectPtr valuePtr = VariantConverter<IBaseObject, BaseObjectPtr>::ToDaqObject(opcUaVariant);
                *value = valuePtr.addRefAndReturn();
                return OPENDAQ_SUCCESS;
            }
            return OPENDAQ_IGNORED;
        };

    if (descriptorNodeId && readValueFunction(value, "Value") == OPENDAQ_SUCCESS)
        return OPENDAQ_SUCCESS;
    
    if (hasReference("AnalogValue"))
        return readValueFunction(value, "AnalogValue");

    return OPENDAQ_IGNORED;
}

END_NAMESPACE_OPENDAQ_OPCUA_TMS
