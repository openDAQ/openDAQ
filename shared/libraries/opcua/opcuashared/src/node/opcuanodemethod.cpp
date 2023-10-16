#include "opcuashared/node/opcuanodemethod.h"

BEGIN_NAMESPACE_OPENDAQ_OPCUA

OpcUaNodeMethod::OpcUaNodeMethod(const OpcUaNodeId& uaNode, const OpcUaNodeId& parentNodeId)
    : OpcUaNode(uaNode, OpcUaNodeClass::Method)
    , parentNodeId(parentNodeId)
{
}

OpcUaNodeMethod::OpcUaNodeMethod(const UA_ReferenceDescription& uaNodeDescription, const OpcUaNodeId& parentNodeId)
    : OpcUaNode(uaNodeDescription, OpcUaNodeClass::Method)
    , parentNodeId(parentNodeId)
{
}

OpcUaNodeMethod::~OpcUaNodeMethod()
{
}

std::string OpcUaNodeMethod::getTypeDescription()
{
    return typeDescription;
}

void OpcUaNodeMethod::initTypeDescription()
{
    bool inputEnabled = inputParameters.size();
    bool outputEnabled = outputParameters.size();

    std::string type;

    if (inputEnabled && outputEnabled)
        type = " (in/out)";
    else if (inputEnabled && !outputEnabled)
        type = " (in)";
    else if (!inputEnabled && outputEnabled)
        type = " (out)";

    typeDescription = "Method" + type;
}

const OpcUaNodeId& OpcUaNodeMethod::getParentNodeId() const
{
    return parentNodeId;
}

void OpcUaNodeMethod::addInputParameter(const std::string& name, const OpcUaNodeId& dataTypeId)
{
    OpcUaChannelMethodParameter param;
    param.setName(name);
    param.setDataTypeId(dataTypeId);
    inputParameters.push_back(param);
}

void OpcUaNodeMethod::addOutputParameter(const std::string& name, const OpcUaNodeId& dataTypeId)
{
    OpcUaChannelMethodParameter param;
    param.setName(name);
    param.setDataTypeId(dataTypeId);
    outputParameters.push_back(param);
}

END_NAMESPACE_OPENDAQ_OPCUA
