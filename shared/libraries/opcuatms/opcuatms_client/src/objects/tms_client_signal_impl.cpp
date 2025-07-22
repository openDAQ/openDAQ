#include <list>
#include <open62541/daqbsp_nodeids.h>
#include <opcuashared/opcuacommon.h>
#include <opcuatms/exceptions.h>
#include <opcuatms_client/objects/tms_client_signal_impl.h>
#include <opcuatms/converters/variant_converter.h>
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

ErrCode TmsClientSignalImpl::setDescriptor(IDataDescriptor* /*descriptor*/)
{
    return DAQ_MAKE_ERROR_INFO(OPENDAQ_ERR_OPCUA_CLIENT_CALL_NOT_AVAILABLE);
}

SignalPtr TmsClientSignalImpl::onGetDomainSignal()
{
    try
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
    }
    catch (...)
    {
        LOG_W("Failed to get domain signal on OpcUA client signal \"{}\"", this->globalId);
    }

    return nullptr;
}

DataDescriptorPtr TmsClientSignalImpl::onGetDescriptor()
{
    try
    {
        if (descriptorNodeId)
        {
            OpcUaVariant opcUaVariant = client->readValue(*descriptorNodeId);
            if (!opcUaVariant.isNull())
            {
                DataDescriptorPtr descriptorPtr = VariantConverter<IDataDescriptor, DataDescriptorPtr>::ToDaqObject(opcUaVariant);
                return descriptorPtr.addRefAndReturn();
            }
        }
    }
    catch (...)
    {
        LOG_W("Failed to get descriptor on OpcUA client signal \"{}\"", this->globalId);
    }

    return nullptr;
}

bool TmsClientSignalImpl::clearDescriptorOnUnsubscribe()
{
    return true;
}

ErrCode TmsClientSignalImpl::setDomainSignal(ISignal* signal)
{
    return DAQ_MAKE_ERROR_INFO(OPENDAQ_ERR_OPCUA_CLIENT_CALL_NOT_AVAILABLE);
}

ErrCode TmsClientSignalImpl::getRelatedSignals(IList** signals)
{
    ListPtr<ISignal> signalsPtr;
    const ErrCode errCode = wrapHandlerReturn(this, &TmsClientSignalImpl::onGetRelatedSignals, signalsPtr);
    if (OPENDAQ_FAILED(errCode))
    {
        daqClearErrorInfo();
        LOG_W("Failed to get related signals on OpcUA client signal \"{}\"", this->globalId);
    }
    *signals = signalsPtr.detach();
    return OPENDAQ_SUCCESS;
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
    return DAQ_MAKE_ERROR_INFO(OPENDAQ_ERR_OPCUA_CLIENT_CALL_NOT_AVAILABLE);
}

ErrCode TmsClientSignalImpl::addRelatedSignal(ISignal* signal)
{
    return DAQ_MAKE_ERROR_INFO(OPENDAQ_ERR_OPCUA_CLIENT_CALL_NOT_AVAILABLE);
}

ErrCode TmsClientSignalImpl::removeRelatedSignal(ISignal* signal)
{
    return DAQ_MAKE_ERROR_INFO(OPENDAQ_ERR_OPCUA_CLIENT_CALL_NOT_AVAILABLE);
}

ErrCode TmsClientSignalImpl::clearRelatedSignals()
{
    return DAQ_MAKE_ERROR_INFO(OPENDAQ_ERR_OPCUA_CLIENT_CALL_NOT_AVAILABLE);
}

Bool TmsClientSignalImpl::onTriggerEvent(const EventPacketPtr& eventPacket)
{
    return Self::onTriggerEvent(eventPacket);
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
            try
            {
                const auto valueNodeId = clientContext->getReferenceBrowser()->getChildNodeId(nodeId, nodeName);
                OpcUaVariant opcUaVariant = client->readValue(*valueNodeId);
                if (!opcUaVariant.isNull())
                {
                    BaseObjectPtr valuePtr = VariantConverter<IBaseObject, BaseObjectPtr>::ToDaqObject(opcUaVariant);
                    *value = valuePtr.addRefAndReturn();
                }
            }
            catch (...)
            {
                LOG_W("Failed to get last value on OpcUA client signal \"{}\"", this->globalId);
            }
            return OPENDAQ_SUCCESS;
        };

    if (descriptorNodeId && *value != nullptr)
        return OPENDAQ_SUCCESS;
    
    if (hasReference("AnalogValue"))
        return readValueFunction(value, "AnalogValue");

    return OPENDAQ_SUCCESS;
}

END_NAMESPACE_OPENDAQ_OPCUA_TMS
