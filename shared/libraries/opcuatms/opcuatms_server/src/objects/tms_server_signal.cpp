#include "opcuatms_server/objects/tms_server_signal.h"
#include "opcuatms/converters/variant_converter.h"
#include "open62541/daqbsp_nodeids.h"
#include "open62541/statuscodes.h"

BEGIN_NAMESPACE_OPENDAQ_OPCUA_TMS

using namespace opcua;

TmsServerSignal::TmsServerSignal(const SignalPtr& object,
                                 const OpcUaServerPtr& server,
                                 const ContextPtr& context,
                                 const TmsServerContextPtr& tmsContext)
    : Super(object, server, context, tmsContext)
{
}

OpcUaNodeId TmsServerSignal::getReferenceType()
{
    // TODO UA_DAQBSPID_HASSTATUSSIGNAL
    return OpcUaNodeId(NAMESPACE_DAQBSP, UA_DAQBSPID_HASVALUESIGNAL);
}

OpcUaNodeId TmsServerSignal::getTmsTypeId()
{
    return OpcUaNodeId(NAMESPACE_DAQBSP, UA_DAQBSPID_SIGNALTYPE);
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
            addReadCallback(descriptorId,
                            [this]()
                            {
                                DataDescriptorPtr descriptor = object.getDescriptor();
                                if (descriptor != nullptr)
                                    return VariantConverter<IBaseObject>::ToVariant(descriptor, nullptr, daqContext);
                                else
                                    return OpcUaVariant();
                            });
        }
    }

    addReadCallback(valueId,
                    [this]()
                    {
                        ObjectPtr lastValue = object.getLastValue();
                        if (lastValue != nullptr)
                            return VariantConverter<IBaseObject>::ToVariant(lastValue, nullptr, daqContext);

                        return OpcUaVariant();
                    });

    auto analogValueId = getChildNodeId("AnalogValue");
    addReadCallback(analogValueId,
                    [this]()
                    {
                        SampleType type = object.getDescriptor().getSampleType();
                        if (type != SampleType::Float32 && type != SampleType::Float64 && type != SampleType::Int8 &&
                            type != SampleType::Int16 && type != SampleType::Int32 && type != SampleType::Int64 &&
                            type != SampleType::UInt8 && type != SampleType::UInt16 && type != SampleType::UInt32 &&
                            type != SampleType::UInt64 && type != SampleType::RangeInt64 && type != SampleType::ComplexFloat32 &&
                            type != SampleType::ComplexFloat64)
                            return OpcUaVariant();

                        ObjectPtr lastValue = object.getLastValue();
                        if (lastValue != nullptr)
                            return VariantConverter<IBaseObject>::ToVariant(lastValue, nullptr, daqContext);

                        return OpcUaVariant();
                    });

    // TODO: Value, AnalogValue, Status
    Super::bindCallbacks();
}

bool TmsServerSignal::createOptionalNode(const opcua::OpcUaNodeId& nodeId)
{
    const auto name = server->readBrowseNameString(nodeId);
    if (name == "Value")
        return true;
    if (name == "AnalogValue")
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
                addReference(domainSignalNodeId, OpcUaNodeId(NAMESPACE_DAQBSP, UA_DAQBSPID_HASDOMAINSIGNAL));
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
