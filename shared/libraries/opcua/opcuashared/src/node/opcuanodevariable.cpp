#include "opcuashared/node/opcuanodevariable.h"

BEGIN_NAMESPACE_OPENDAQ_OPCUA

OpcUaNodeVariable::OpcUaNodeVariable(const OpcUaNodeId& uaNode, const UA_DataType& uaDataType, size_t dimension)
    : OpcUaNodeVariable(uaNode, uaDataType.typeId, dimension)
{
}

OpcUaNodeVariable::OpcUaNodeVariable(const OpcUaNodeId& uaNode, const OpcUaNodeId& dataTypeNodeId, size_t dimension)
    : OpcUaNode(uaNode, OpcUaNodeClass::Variable)
    , dataTypeNodeId(dataTypeNodeId)
    , dimension(dimension)
{
}

OpcUaNodeVariable::OpcUaNodeVariable(const UA_ReferenceDescription& uaNodeDescription, const UA_DataType& uaDataType, size_t dimension)
    : OpcUaNodeVariable(uaNodeDescription, uaDataType.typeId, dimension)
{
}

OpcUaNodeVariable::OpcUaNodeVariable(const UA_ReferenceDescription& uaNodeDescription, const OpcUaNodeId& dataTypeNodeId, size_t dimension)
    : OpcUaNode(uaNodeDescription, OpcUaNodeClass::Variable)
    , dataTypeNodeId(dataTypeNodeId)
    , dimension(dimension)
{
}

OpcUaNodeVariable::~OpcUaNodeVariable()
{
}

const OpcUaNodeId OpcUaNodeVariable::getDataTypeNodeId() const
{
    return dataTypeNodeId;
}

OpcUaVariantPtr OpcUaNodeVariable::getVariant()
{
    return variant;
}

void OpcUaNodeVariable::setVariant(const UA_Variant& value)
{
    if (!variant)
        variant = std::make_shared<OpcUaVariant>(value);
    else
        variant->setValue(value);
}

size_t OpcUaNodeVariable::getDimension()
{
    return dimension;
}

void OpcUaNodeVariable::setDimension(size_t dimension)
{
    this->dimension = dimension;
}

END_NAMESPACE_OPENDAQ_OPCUA
