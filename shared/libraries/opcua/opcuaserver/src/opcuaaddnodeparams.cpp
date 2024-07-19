#include <opcuaserver/opcuaaddnodeparams.h>

BEGIN_NAMESPACE_OPENDAQ_OPCUA

/*RequestedNodeIdBaseOnName*/

static daq::opcua::OpcUaNodeId RequestedNodeIdBaseOnName(const std::string& name, const OpcUaNodeId& parentNodeId)
{
    if (parentNodeId.getValue().identifierType == UA_NODEIDTYPE_STRING)
    {
        std::string newNodeIdStr = utils::ToStdString(parentNodeId.getValue().identifier.string) + "/" + name;
        return OpcUaNodeId(parentNodeId.getNamespaceIndex(), newNodeIdStr);
    }
    return UA_NODEID_NULL;
}


/*AddNodeParams*/

AddNodeParams::AddNodeParams(const OpcUaNodeId& requestedNewNodeId, const OpcUaNodeId& parentNodeId, const OpcUaNodeId& referenceTypeId)
    : requestedNewNodeId(requestedNewNodeId)
    , parentNodeId(parentNodeId)
    , referenceTypeId(referenceTypeId)
{
}

void AddNodeParams::setBrowseName(const std::string& browseName)
{
    this->browseName = UA_QUALIFIEDNAME_ALLOC(0, browseName.c_str());
}

/*GenericAddNodeParams*/

template <typename T>
GenericAddNodeParams<T>::GenericAddNodeParams(const OpcUaNodeId& requestedNewNodeId,
                                              const OpcUaNodeId& parentNodeId,
                                              const OpcUaNodeId& referenceTypeId,
                                              const T& defaultAttributes)
    : AddNodeParams(requestedNewNodeId, parentNodeId, referenceTypeId)
    , attr(defaultAttributes)
{
}

/*AddObjectNodeParams*/

AddObjectNodeParams::AddObjectNodeParams(const OpcUaNodeId& requestedNewNodeId)
    : AddObjectNodeParams(requestedNewNodeId, OpcUaNodeId())
{
}

AddObjectNodeParams::AddObjectNodeParams(const OpcUaNodeId& requestedNewNodeId, const OpcUaNodeId& parentNodeId)
    : GenericAddNodeParams<UA_ObjectAttributes>(
          requestedNewNodeId, parentNodeId, OpcUaNodeId(UA_NS0ID_HASCOMPONENT), UA_ObjectAttributes_default)
{
}

AddObjectNodeParams::AddObjectNodeParams(const std::string& name, const OpcUaNodeId& parentNodeId)
    : GenericAddNodeParams<UA_ObjectAttributes>(
          RequestedNodeIdBaseOnName(name, parentNodeId), parentNodeId, OpcUaNodeId(UA_NS0ID_HASCOMPONENT), UA_ObjectAttributes_default)
{
}

/*AddVariableNodeParams*/

AddVariableNodeParams::AddVariableNodeParams(const std::string& name, const OpcUaNodeId& parentNodeId)
    : GenericAddNodeParams<UA_VariableAttributes>(
                  RequestedNodeIdBaseOnName(name, parentNodeId), parentNodeId, OpcUaNodeId(UA_NS0ID_HASPROPERTY), UA_VariableAttributes_default)
{
}

AddVariableNodeParams::AddVariableNodeParams(const OpcUaNodeId& requestedNewNodeId)
    : AddVariableNodeParams(requestedNewNodeId, OpcUaNodeId())
{
}

AddVariableNodeParams::AddVariableNodeParams(const OpcUaNodeId& requestedNewNodeId, const OpcUaNodeId& parentNodeId)
    : GenericAddNodeParams<UA_VariableAttributes>(
          requestedNewNodeId, parentNodeId, OpcUaNodeId(UA_NS0ID_HASPROPERTY), UA_VariableAttributes_default)
{
}

void AddVariableNodeParams::setDataType(const OpcUaNodeId& dataTypeId)
{
    attr->dataType = *dataTypeId;
}

/*AddMethodNodeParams*/

AddMethodNodeParams::AddMethodNodeParams(const OpcUaNodeId& requestedNewNodeId)
    : AddMethodNodeParams(requestedNewNodeId, OpcUaNodeId())
{
}

AddMethodNodeParams::AddMethodNodeParams(const OpcUaNodeId& requestedNewNodeId, const OpcUaNodeId& parentNodeId)
    : GenericAddNodeParams<UA_MethodAttributes>(
          requestedNewNodeId, parentNodeId, OpcUaNodeId(UA_NS0ID_HASPROPERTY), UA_MethodAttributes_default)
{
}

AddMethodNodeParams::AddMethodNodeParams(const std::string& name, const OpcUaNodeId& parentNodeId)
    : GenericAddNodeParams<UA_MethodAttributes>(
                  RequestedNodeIdBaseOnName(name, parentNodeId), parentNodeId, OpcUaNodeId(UA_NS0ID_HASPROPERTY), UA_MethodAttributes_default)
{
}

AddMethodNodeParams::~AddMethodNodeParams()
{
    if(outputArguments)
        UA_Array_delete(outputArguments, outputArgumentsSize, GetUaDataType<UA_Argument>());
    if(inputArguments)
        UA_Array_delete(inputArguments, inputArgumentsSize, GetUaDataType<UA_Argument>());
}

/*AddVariableTypeNodeParams*/

AddVariableTypeNodeParams::AddVariableTypeNodeParams(const OpcUaNodeId& requestedNewNodeId)
    : AddVariableTypeNodeParams(requestedNewNodeId, OpcUaNodeId())
{
}

AddVariableTypeNodeParams::AddVariableTypeNodeParams(const OpcUaNodeId& requestedNewNodeId, const OpcUaNodeId& parentNodeId)
    : GenericAddNodeParams<UA_VariableTypeAttributes>(
          requestedNewNodeId, parentNodeId, OpcUaNodeId(UA_NS0ID_HASSUBTYPE), UA_VariableTypeAttributes_default)
{
}

/*AddObjectTypeNodeParams*/

AddObjectTypeNodeParams::AddObjectTypeNodeParams(const OpcUaNodeId& requestedNewNodeId)
    : AddObjectTypeNodeParams(requestedNewNodeId, OpcUaNodeId())
{
}

AddObjectTypeNodeParams::AddObjectTypeNodeParams(const OpcUaNodeId& requestedNewNodeId, const OpcUaNodeId& parentNodeId)
    : GenericAddNodeParams<UA_ObjectTypeAttributes>(
          requestedNewNodeId, parentNodeId, OpcUaNodeId(UA_NS0ID_HASSUBTYPE), UA_ObjectTypeAttributes_default)
{
}

END_NAMESPACE_OPENDAQ_OPCUA
