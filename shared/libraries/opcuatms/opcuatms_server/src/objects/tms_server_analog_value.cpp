#include <opcuatms_server/objects/tms_server_analog_value.h>
#include <opcuatms/converters/variant_converter.h>
#include <open62541/daqbsp_nodeids.h>

BEGIN_NAMESPACE_OPENDAQ_OPCUA_TMS

using namespace opcua;

TmsServerAnalogValue::TmsServerAnalogValue(const SignalPtr& signal, const opcua::OpcUaServerPtr& server, const ContextPtr& context, const TmsServerContextPtr& tmsContext)
    : Super(BaseObjectPtr(), server, context, tmsContext)
    , signal(signal)
{
}

std::string TmsServerAnalogValue::getBrowseName()
{
    return "AnalogValue";
}

opcua::OpcUaNodeId TmsServerAnalogValue::getTmsTypeId()
{
    // Return the base data variable type definition
    // The actual concrete data type is set via getDataTypeId()
    return OpcUaNodeId(0, UA_NS0ID_BASEDATAVARIABLETYPE);
}

opcua::OpcUaNodeId TmsServerAnalogValue::getDataTypeId()
{
    try
    {
        const auto descriptor = signal.getDescriptor();
        if (descriptor.assigned())
        {
            SampleType sampleType = descriptor.getSampleType();
            return sampleTypeToOpcUaDataType(sampleType);
        }
    }
    catch (...)
    {
        // If descriptor is not available or any error occurs, return null
        // which will use the default type from the type definition
    }
    
    return {};
}

opcua::OpcUaNodeId TmsServerAnalogValue::sampleTypeToOpcUaDataType(SampleType sampleType)
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
        default:
            return OpcUaNodeId();
    }
}

void TmsServerAnalogValue::bindCallbacks()
{
    // The data type is already set correctly during node creation via getDataTypeId()
    // in configureVariableNodeAttributes()
    
    addReadCallback(nodeId, [this]()
    {
        const auto descriptor = signal.getDescriptor();
        SampleType type = descriptor.assigned() ? descriptor.getSampleType() : SampleType::Undefined;

        if (type != SampleType::Float32 && type != SampleType::Float64 && type != SampleType::Int8 &&
            type != SampleType::Int16 && type != SampleType::Int32 && type != SampleType::Int64 &&
            type != SampleType::UInt8 && type != SampleType::UInt16 && type != SampleType::UInt32 &&
            type != SampleType::UInt64 && type != SampleType::RangeInt64 && type != SampleType::ComplexFloat32 &&
            type != SampleType::ComplexFloat64)
            return OpcUaVariant();

        ObjectPtr lastValue = signal.getLastValue();
        if (lastValue != nullptr)
            return VariantConverter<IBaseObject>::ToVariant(lastValue, nullptr, daqContext);

        return OpcUaVariant();
    });
    
    Super::bindCallbacks();
}

END_NAMESPACE_OPENDAQ_OPCUA_TMS

