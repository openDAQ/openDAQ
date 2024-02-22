#include <opcuatms_client/objects/tms_client_function_block_type_impl.h>
#include <opcuatms/converters/struct_converter.h>
#include <opcuatms_client/objects/tms_client_property_object_factory.h>
#include <opcuatms/converters/property_object_conversion_utils.h>

BEGIN_NAMESPACE_OPENDAQ_OPCUA_TMS

TmsClientFunctionBlockTypeImpl::TmsClientFunctionBlockTypeImpl(const ContextPtr& context,
                                                               const TmsClientContextPtr& tmsContext,
                                                               const opcua::OpcUaNodeId& nodeId)
    : TmsClientObjectImpl(context, tmsContext, nodeId)
    , FunctionBlockTypeImpl("", "", "", nullptr)
{
    readAttributes();
}

ErrCode TmsClientFunctionBlockTypeImpl::getId(IString** id)
{
    OPENDAQ_PARAM_NOT_NULL(id);

    *id = type.getId().detach();
    return OPENDAQ_SUCCESS;
}

ErrCode TmsClientFunctionBlockTypeImpl::getName(IString** name)
{
    OPENDAQ_PARAM_NOT_NULL(name);

    *name = type.getName().detach();
    return OPENDAQ_SUCCESS;
}

ErrCode TmsClientFunctionBlockTypeImpl::getDescription(IString** description)
{
    OPENDAQ_PARAM_NOT_NULL(description);

    *description = type.getDescription().detach();
    return OPENDAQ_SUCCESS;
}

ErrCode TmsClientFunctionBlockTypeImpl::createDefaultConfig(IPropertyObject** defaultConfig)
{
    OPENDAQ_PARAM_NOT_NULL(defaultConfig);

    auto clone = PropertyObjectConversionUtils::ClonePropertyObject(this->defaultConfig);
    *defaultConfig = clone.detach();
    return OPENDAQ_SUCCESS;
}

void TmsClientFunctionBlockTypeImpl::readAttributes()
{
    const auto value = client->readValue(nodeId);
    this->type = VariantConverter<IFunctionBlockType>::ToDaqObject(value);

    const auto defaultConfigId = getNodeId("DefaultConfig");
    this->defaultConfig = TmsClientPropertyObject(daqContext, clientContext, defaultConfigId);
}

END_NAMESPACE_OPENDAQ_OPCUA_TMS
