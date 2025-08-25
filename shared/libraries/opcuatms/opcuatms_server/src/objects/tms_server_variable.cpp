#include <opcuatms_server/objects/tms_server_variable.h>
#include <coreobjects/eval_value_ptr.h>
#include <open62541/server.h>
#include <opendaq/function_block_type_ptr.h>

BEGIN_NAMESPACE_OPENDAQ_OPCUA_TMS

using namespace daq::opcua;

template <class CoreType>
TmsServerVariable<CoreType>::TmsServerVariable(const CoreType& object, const opcua::OpcUaServerPtr& server, const ContextPtr& context, const TmsServerContextPtr& tmsContext)
    : Super(object, server, context, tmsContext)
{
}

template <class CoreType>
opcua::OpcUaNodeId TmsServerVariable<CoreType>::createNode(const opcua::OpcUaNodeId& parentNodeId)
{
    OpcUaNodeId newNodeId;

    this->typeBrowseName = this->readTypeBrowseName();
    std::string name = this->getBrowseName();

    auto params = AddVariableNodeParams(name, parentNodeId);
    configureVariableNodeAttributes(params.attr);
    params.setBrowseName(name);
    params.typeDefinition = this->getTmsTypeId();
    params.nodeContext = this;
    params.addOptionalNodeCallback = [this](const OpcUaNodeId& nodeId) { return this->createOptionalNode(nodeId); };
    newNodeId = this->server->addVariableNode(params);

    return OpcUaNodeId(newNodeId);
}

template <class CoreType>
opcua::OpcUaNodeId TmsServerVariable<CoreType>::getDataTypeId()
{
    return {};
}

template <class CoreType>
void TmsServerVariable<CoreType>::configureVariableNodeAttributes(opcua::OpcUaObject<UA_VariableAttributes>& attr)
{
    const auto dataType = this->getDataTypeId();
    if (!dataType.isNull())
    {
        attr->dataType = dataType.getValue();
        if (attr->dataType.identifier.numeric == UA_NS0ID_INT64)
        {
            UA_Int64 var = 0;
            UA_Variant_setScalarCopy(&attr->value, &var, &UA_TYPES[UA_TYPES_INT64]);
        }
        else if (attr->dataType.identifier.numeric == UA_NS0ID_DOUBLE)
        {
            UA_Double var = 0;
            UA_Variant_setScalarCopy(&attr->value, &var, &UA_TYPES[UA_TYPES_DOUBLE]);
        }
    }
    else
    {
        const auto tmsTypeId = this->getTmsTypeId();
        const auto dataTypeId = this->server->readDataType(tmsTypeId);
        attr->dataType = *dataTypeId;
    }
    attr->accessLevel = UA_ACCESSLEVELMASK_READ | UA_ACCESSLEVELMASK_WRITE;
    attr->writeMask |= UA_WRITEMASK_DISPLAYNAME | UA_WRITEMASK_DESCRIPTION;
}

template class TmsServerVariable<ListPtr<IFloat>>;
template class TmsServerVariable<EvalValuePtr>;
template class TmsServerVariable<PropertyPtr>;
template class TmsServerVariable<FunctionBlockTypePtr>;

END_NAMESPACE_OPENDAQ_OPCUA_TMS
