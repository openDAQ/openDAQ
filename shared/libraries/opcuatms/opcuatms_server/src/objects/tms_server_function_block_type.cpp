#include <opcuatms_server/objects/tms_server_function_block_type.h>
#include <opcuatms/converters/variant_converter.h>
#include <opcuatms_server/objects/tms_server_property_object.h>
#include <open62541/daqbsp_nodeids.h>

using namespace daq::opcua;

BEGIN_NAMESPACE_OPENDAQ_OPCUA_TMS

TmsServerFunctionBlockType::TmsServerFunctionBlockType(const FunctionBlockTypePtr& object,
                                                       const OpcUaServerPtr& server,
                                                       const ContextPtr& context,
                                                       const TmsServerContextPtr& tmsContext)
    : Super(object, server, context, tmsContext)
{
}

std::string TmsServerFunctionBlockType::getBrowseName()
{
    return object.getId();
}

std::string TmsServerFunctionBlockType::getDisplayName()
{
    return object.getId();
}

std::string TmsServerFunctionBlockType::getDescription()
{
    return object.getDescription();
}

OpcUaNodeId TmsServerFunctionBlockType::getTmsTypeId()
{
    return OpcUaNodeId(UA_NS0ID_BASEDATAVARIABLETYPE);
}

void TmsServerFunctionBlockType::addChildNodes()
{
    Super::addChildNodes();
    addDefaultConfigNode();
}

void TmsServerFunctionBlockType::configureVariableNodeAttributes(OpcUaObject<UA_VariableAttributes>& attr)
{
    Super::configureVariableNodeAttributes(attr);

    attr->dataType = UA_TYPES_DAQBSP[UA_TYPES_DAQBSP_FUNCTIONBLOCKINFOSTRUCTURE].typeId;
    attr->accessLevel = UA_ACCESSLEVELMASK_READ;
    attr->writeMask = 0;

    const auto defaultValue = VariantConverter<IFunctionBlockType>::ToVariant(object);
    attr->value = defaultValue.copyAndGetDetachedValue();
}

void TmsServerFunctionBlockType::addDefaultConfigNode()
{
    auto defaultConfig = object.createDefaultConfig();

    if (!defaultConfig.assigned())
        return;

    if (!defaultConfig.isFrozen())
        defaultConfig.freeze();

    tmsDefaultConfig = std::make_shared<TmsServerPropertyObject>(defaultConfig, server, daqContext, tmsContext, "DefaultConfig");
    tmsDefaultConfig->registerOpcUaNode(nodeId);
}


END_NAMESPACE_OPENDAQ_OPCUA_TMS
