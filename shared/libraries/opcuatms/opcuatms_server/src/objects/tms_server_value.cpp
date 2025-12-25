#include <opcuatms_server/objects/tms_server_value.h>
#include <opcuatms/converters/variant_converter.h>
#include <open62541/daqbsp_nodeids.h>
#include "opendaq/custom_log.h"

BEGIN_NAMESPACE_OPENDAQ_OPCUA_TMS

using namespace opcua;

TmsServerValue::TmsServerValue(const SignalPtr& signal, const opcua::OpcUaServerPtr& server, const ContextPtr& context, const TmsServerContextPtr& tmsContext)
    : Super(BaseObjectPtr(), server, context, tmsContext)
    , signal(signal)
{
}

std::string TmsServerValue::getBrowseName()
{
    return "Value";
}

opcua::OpcUaNodeId TmsServerValue::getTmsTypeId()
{
    // Return the base data variable type definition
    // The actual concrete data type is set via getDataTypeId()
    return OpcUaNodeId(0, UA_NS0ID_BASEDATAVARIABLETYPE);
}

opcua::OpcUaNodeId TmsServerValue::getDataTypeId()
{
    try
    {
        const auto descriptor = signal.getDescriptor();
        if (descriptor.assigned())
        {
            SampleType sampleType = descriptor.getSampleType();
            return SampleTypeToOpcUaDataType(sampleType);
        }
    }
    catch (...)
    {
        // If descriptor is not available or any error occurs, return null
        // which will use the default type from the type definition
    }
    
    return {};
}

opcua::OpcUaNodeId TmsServerValue::SampleTypeToOpcUaDataType(SampleType sampleType)
{
    switch (sampleType)
    {
        case SampleType::Float32:
            return OpcUaNodeId(0, UA_NS0ID_FLOAT);
        case SampleType::Float64:
            return OpcUaNodeId(0, UA_NS0ID_DOUBLE);
        case SampleType::Int8:
            return OpcUaNodeId(0, UA_NS0ID_SBYTE);
        case SampleType::UInt8:
            return OpcUaNodeId(0, UA_NS0ID_BYTE);
        case SampleType::Int16:
            return OpcUaNodeId(0, UA_NS0ID_INT16);
        case SampleType::UInt16:
            return OpcUaNodeId(0, UA_NS0ID_UINT16);
        case SampleType::Int32:
            return OpcUaNodeId(0, UA_NS0ID_INT32);
        case SampleType::UInt32:
            return OpcUaNodeId(0, UA_NS0ID_UINT32);
        case SampleType::Int64:
            return OpcUaNodeId(0, UA_NS0ID_INT64);
        case SampleType::UInt64:
            return OpcUaNodeId(0, UA_NS0ID_UINT64);
        case SampleType::RangeInt64:
            return OpcUaNodeId(0, UA_NS0ID_RANGE);
        case SampleType::ComplexFloat32:
            return OpcUaNodeId(0, UA_NS0ID_COMPLEXNUMBERTYPE);
        case SampleType::ComplexFloat64:
            return OpcUaNodeId(0, UA_NS0ID_DOUBLECOMPLEXNUMBERTYPE);
        case SampleType::String:
            return OpcUaNodeId(0, UA_NS0ID_STRING);
        case SampleType::Binary:
            return OpcUaNodeId(0, UA_NS0ID_BYTESTRING);
        default:
            return OpcUaNodeId();
    }
}
void TmsServerValue::addChildNodes()
{
    try
    {
        auto params = AddVariableNodeParams("DataDescriptor", nodeId);
        params.setBrowseName("DataDescriptor");
        params.setDataType(OpcUaNodeId(NAMESPACE_DAQBSP, UA_DAQBSPID_DATADESCRIPTORSTRUCTURE));
        params.typeDefinition = OpcUaNodeId(0, UA_NS0ID_BASEDATAVARIABLETYPE);
        params.referenceTypeId = OpcUaNodeId(UA_NS0ID_HASPROPERTY);

        server->addVariableNode(params);
    }
    catch (const std::exception&)
    {
        const auto loggerComponent = this->daqContext.getLogger().getOrAddComponent("OpenDAQOPCUAServerModule");
        LOG_D("OPC UA Value {} failed create data descriptor node.", this->signal.getGlobalId());
    }

    Super::addChildNodes();
}

void TmsServerValue::bindCallbacks()
{
    addReadCallback(nodeId, [this]()
    {
        ObjectPtr lastValue = signal.getLastValue();
        if (lastValue != nullptr)
            return VariantConverter<IBaseObject>::ToVariant(lastValue, nullptr, daqContext);

        return OpcUaVariant();
    });

    addReadCallback("DataDescriptor", [this]()
    {
        DataDescriptorPtr descriptor = signal.getDescriptor();
        if (descriptor.assigned())
            return VariantConverter<IBaseObject>::ToVariant(descriptor, nullptr, daqContext);
        else
            return OpcUaVariant();
    });

    Super::bindCallbacks();
}

END_NAMESPACE_OPENDAQ_OPCUA_TMS

