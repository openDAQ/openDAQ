#include "opcuatms_server/objects/tms_server_signal.h"
#include "opcuatms/converters/variant_converter.h"
#include "open62541/statuscodes.h"
#include "open62541/tmsbsp_nodeids.h"

BEGIN_NAMESPACE_OPENDAQ_OPCUA_TMS

using namespace opcua;

TmsServerSignal::TmsServerSignal(const SignalPtr& object, const OpcUaServerPtr& server, const ContextPtr& context)
    : Super(object, server, context)
{
}

OpcUaNodeId TmsServerSignal::getReferenceType()
{
    //TODO UA_TMSBSPID_HASSTATUSSIGNAL 
    return OpcUaNodeId(NAMESPACE_TMSBSP, UA_TMSBSPID_HASVALUESIGNAL);
}

OpcUaNodeId TmsServerSignal::getTmsTypeId()
{
    return OpcUaNodeId(NAMESPACE_TMSBSP, UA_TMSBSPID_SIGNALTYPE);
}

void TmsServerSignal::bindCallbacks()
{
    auto valueId = getChildNodeId("Value");
    OpcUaObject<UA_BrowseDescription> bd;
    bd->nodeId = valueId.copyAndGetDetachedValue();
    bd->resultMask = UA_BROWSERESULTMASK_ALL;
    auto result = server->browse(bd);

    for (size_t i = 0; i < result->referencesSize; i++)
    {
        auto reference = result->references[i];
        std::string browseName = opcua::utils::ToStdString(reference.browseName.name);
        if (browseName == "DataDescriptor")
        {
            OpcUaNodeId descriptorId{reference.nodeId.nodeId};
            addReadCallback(descriptorId, [this]() {
                DataDescriptorPtr descriptor = object.getDescriptor();
                if (descriptor != nullptr)
                    return VariantConverter<IBaseObject>::ToVariant(descriptor, nullptr, daqContext);
                else
                    return OpcUaVariant();
            });
        }
    }

    // TODO: Value, AnalogValue, Status
    Super::bindCallbacks();
}

bool TmsServerSignal::createOptionalNode(const opcua::OpcUaNodeId& nodeId)
{
    const auto name = server->readBrowseNameString(nodeId);
    if (name == "Value")
        return true;

    return Super::createOptionalNode(nodeId);
}

void TmsServerSignal::createNonhierarchicalReferences()
{
    auto domainSignal = object.getDomainSignal();
    if (domainSignal.assigned())
    {
        auto domainSignalNodeId = findSignalNodeId(domainSignal);
        if (!domainSignalNodeId.isNull())
        {
            try
            {
                addReference(domainSignalNodeId, OpcUaNodeId(NAMESPACE_TMSBSP, UA_TMSBSPID_HASDOMAINSIGNAL));
            }
            catch (const OpcUaException& ex)
            {
                if (ex.getStatusCode() != UA_STATUSCODE_BADDUPLICATEREFERENCENOTALLOWED)
                    throw;
            }
            
        }
    }
}

END_NAMESPACE_OPENDAQ_OPCUA_TMS
