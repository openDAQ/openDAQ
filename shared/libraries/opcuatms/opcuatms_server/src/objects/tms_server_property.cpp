#include <opcuatms_server/objects/tms_server_property.h>
#include <coreobjects/eval_value_factory.h>
#include <opcuatms/converters/list_conversion_utils.h>
#include <opcuatms_server/objects/tms_server_eval_value.h>
#include <open62541/daqbt_nodeids.h>
#include <open62541/types_daqbt_generated.h>
#include <opcuatms/core_types_utils.h>
#include <opcuatms/converters/variant_converter.h>
#include <open62541/daqbsp_nodeids.h>
#include <coreobjects/unit_factory.h>

BEGIN_NAMESPACE_OPENDAQ_OPCUA_TMS

using namespace opcua;

TmsServerProperty::TmsServerProperty(const PropertyPtr& object,
                                     const opcua::OpcUaServerPtr& server,
                                     const ContextPtr& context,
                                     const TmsServerContextPtr& tmsContext,
                                     const std::string& browseName)
    : Super(object, server, context, tmsContext)
    , browseName(browseName)
{
    objectInternal = object.asPtr<IPropertyInternal>(false);

    if (isReferenceType())
        hideReferenceTypeChildren();
    if (isNumericType())
        hideNumericTypeChildren();
    if (isIntrospectionType())
        hideIntrospectionTypeChildren();
    if (isSelectionType())
        hideSelectionTypeChildren();
    if (isStructureType())
        hideStructureTypeChildren();
}

TmsServerProperty::TmsServerProperty(const PropertyPtr& object,
                                     const opcua::OpcUaServerPtr& server,
                                     const ContextPtr& context,
                                     const TmsServerContextPtr& tmsContext,
                                     const std::unordered_map<std::string, uint32_t>& propOrder,
                                     const std::string& browseName)
    : TmsServerProperty(object, server, context, tmsContext, browseName)
{
    this->propOrder = propOrder;
    this->numberInList = propOrder.at(object.getName());
}

TmsServerProperty::TmsServerProperty(const PropertyPtr& object,
                                     const opcua::OpcUaServerPtr& server,
                                     const ContextPtr& context,
                                     const TmsServerContextPtr& tmsContext,
                                     const PropertyObjectPtr& parent,
                                     const std::unordered_map<std::string, uint32_t>& propOrder,
                                     const std::string& browseName)
    : TmsServerProperty(object, server, context, tmsContext, propOrder, browseName)
{
    this->parent = parent;
}

std::string TmsServerProperty::getPropertyName()
{
    return this->object.getName();
}

std::string TmsServerProperty::getBrowseName()
{
    return this->browseName.empty() ? this->object.getName().toStdString() : this->browseName;
}

void TmsServerProperty::bindCallbacks()
{
    if (!HiddenNodes.count("CoercionExpression"))
    {
        addReadCallback("CoercionExpression", [this] { return VariantConverter<IString>::ToVariant(object.getCoercer().getEval()); });
    }

    if (!HiddenNodes.count("ValidationExpression"))
    {
        addReadCallback("ValidationExpression", [this] { return VariantConverter<IString>::ToVariant(object.getValidator().getEval()); });
    }

    for (auto childProp : childProperties)
    {
        auto name = childProp.second->getBrowseName();
        auto parentObj = this->parent.getRef();
        if (!parentObj.getProperty(name).asPtr<IPropertyInternal>().getReferencedPropertyUnresolved().assigned())
        {
            addReadCallback(name, [this, name]
                {
                    const auto value = this->parent.getRef().getPropertyValue(name);
                    return VariantConverter<IBaseObject>::ToVariant(value, nullptr, daqContext);
                });

            if (!parentObj.supportsInterface<IFreezable>() || !parentObj.isFrozen())
            {
                addWriteCallback(name, [this, name](const OpcUaVariant& variant) 
                    {
                        const auto value = VariantConverter<IBaseObject>::ToDaqObject(variant, daqContext);
                        this->parent.getRef().setPropertyValue(name, value);
                        return UA_STATUSCODE_GOOD;
                    });
            }
        }
        else
        {
                addReadCallback(name, [this, name]
                {
                    const auto refProp = this->parent.getRef().getProperty(name).asPtr<IPropertyInternal>().getReferencedPropertyUnresolved();
                    return VariantConverter<IBaseObject>::ToVariant(refProp.getEval(), nullptr, daqContext);
                });
        }

    }
}

opcua::OpcUaNodeId TmsServerProperty::getTmsTypeId()
{
    if (objectInternal.getSelectionValuesUnresolved().assigned())
        return OpcUaNodeId(NAMESPACE_DAQBT, UA_DAQBTID_SELECTIONVARIABLETYPE);

    if (objectInternal.getReferencedPropertyUnresolved().assigned())
        return OpcUaNodeId(NAMESPACE_DAQBT, UA_DAQBTID_REFERENCEVARIABLETYPE);

    const auto type = object.getValueType();

    switch (type)
    {
        case CoreType::ctInt:
        case CoreType::ctFloat:
            return OpcUaNodeId(NAMESPACE_DAQBT, UA_DAQBTID_NUMERICVARIABLETYPE);
        case CoreType::ctStruct:
            return OpcUaNodeId(NAMESPACE_DAQBT, UA_DAQBTID_STRUCTUREVARIABLETYPE);
        case CoreType::ctEnumeration:
        default:
            break;
    }

    return OpcUaNodeId(NAMESPACE_DAQBT, UA_DAQBTID_INTROSPECTIONVARIABLETYPE);
}

bool TmsServerProperty::createOptionalNode(const opcua::OpcUaNodeId& nodeId)
{
    const auto name = server->readBrowseNameString(nodeId);

    return HiddenNodes.count(name) == 0;
}

void TmsServerProperty::addChildNodes()
{
    if (isReferenceType())
    {
        addReferenceTypeChildNodes();
        return;
    }

    addIntrospectionTypeChildNodes();
    if (isNumericType())
        addNumericTypeChildNodes();
    else if (isSelectionType())
        addSelectionTypeChildNodes();
}

void TmsServerProperty::configureVariableNodeAttributes(opcua::OpcUaObject<UA_VariableAttributes>& attr)
{
    Super::configureVariableNodeAttributes(attr);

    attr->writeMask = attr->writeMask & ~UA_WRITEMASK_DISPLAYNAME;
    attr->writeMask = attr->writeMask & ~UA_WRITEMASK_DESCRIPTION;

    if (object.getDescription().assigned())
        attr->description = UA_LOCALIZEDTEXT_ALLOC("", object.getDescription().getCharPtr());
}

opcua::OpcUaNodeId TmsServerProperty::getDataTypeId()
{
    const auto objInternal = object.asPtr<IPropertyInternal>();
    if (objInternal.getReferencedPropertyUnresolved().assigned())
        return OpcUaNodeId(0, UA_NS0ID_STRING);

    const auto type = objInternal.getValueTypeUnresolved();
    switch (type)
    {
        case CoreType::ctBool:
            return OpcUaNodeId(0, UA_NS0ID_BOOLEAN);
        case CoreType::ctInt:
            return OpcUaNodeId(0, UA_NS0ID_INT64);
        case CoreType::ctFloat:
            return OpcUaNodeId(0, UA_NS0ID_DOUBLE);
        case CoreType::ctString:
            return OpcUaNodeId(0, UA_NS0ID_STRING);
        case CoreType::ctEnumeration:
        {
            EnumerationPtr enumPtr = this->parent.getRef().getPropertyValue(object.getName());
            std::string enumTypeName =  enumPtr.getEnumerationType().getName();
            const auto DataType = GetUAEnumerationDataTypeByName(enumTypeName);
            if (DataType != nullptr)
                return DataType->typeId;
            else
                break;
        }
        case CoreType::ctStruct:
        {
            StructPtr structPtr = this->parent.getRef().getPropertyValue(object.getName());
            std::string structTypeName =  structPtr.getStructType().getName();
            const auto DataType = GetUAStructureDataTypeByName(structTypeName);
            if(DataType != nullptr)
                return DataType->typeId;
            else
                break;
        }
        default:
            break;
    }

    return {};
}

void TmsServerProperty::validate()
{
}

void TmsServerProperty::registerEvalValueNode(const std::string& nodeName, TmsServerEvalValue::ReadCallback readCallback, bool isSelection)
{
    auto nodeId = getChildNodeId(nodeName);
    auto serverObject = std::make_shared<TmsServerEvalValue>(server, daqContext, tmsContext);
    serverObject->setReadCallback(std::move(readCallback));
    serverObject->setIsSelectionType(isSelection);
    auto childNodeId = serverObject->registerToExistingOpcUaNode(nodeId);

    childObjects.insert({childNodeId, serverObject});
}

bool TmsServerProperty::isSelectionType()
{
    return getTmsTypeId() == OpcUaNodeId(NAMESPACE_DAQBT, UA_DAQBTID_SELECTIONVARIABLETYPE);
}

bool TmsServerProperty::isNumericType()
{
    return getTmsTypeId() == OpcUaNodeId(NAMESPACE_DAQBT, UA_DAQBTID_NUMERICVARIABLETYPE);
}

bool TmsServerProperty::isIntrospectionType()
{
    return getTmsTypeId() == OpcUaNodeId(NAMESPACE_DAQBT, UA_DAQBTID_INTROSPECTIONVARIABLETYPE);
}

bool TmsServerProperty::isReferenceType()
{
    return getTmsTypeId() == OpcUaNodeId(NAMESPACE_DAQBT, UA_DAQBTID_REFERENCEVARIABLETYPE);
}

bool TmsServerProperty::isStructureType()
{
    return getTmsTypeId() == OpcUaNodeId(NAMESPACE_DAQBT, UA_DAQBTID_STRUCTUREVARIABLETYPE);
}

void TmsServerProperty::hideReferenceTypeChildren()
{
    // TODO: Adjust model to reference type variables not having the IsVisible field
    HiddenNodes.insert("IsVisible");
}

void TmsServerProperty::hideNumericTypeChildren()
{
    hideIntrospectionTypeChildren();

    if (const auto pos = HiddenNodes.find("DefaultValue"); pos != HiddenNodes.cend())
        HiddenNodes.erase(pos);
    if (!objectInternal.getMaxValueUnresolved().assigned())
        HiddenNodes.insert("MaxValue");
    if (!objectInternal.getMinValueUnresolved().assigned())
        HiddenNodes.insert("MinValue");
    if (!objectInternal.getSuggestedValuesUnresolved().assigned())
        HiddenNodes.insert("SuggestedValues");
}

void TmsServerProperty::hideSelectionTypeChildren()
{
    hideIntrospectionTypeChildren();

    if (const auto pos = HiddenNodes.find("DefaultValue"); pos != HiddenNodes.cend())
        HiddenNodes.erase(pos);
}

void TmsServerProperty::hideIntrospectionTypeChildren()
{
    if (!objectInternal.getVisibleUnresolved().assigned())
        HiddenNodes.insert("IsVisible");
    if (!objectInternal.getReadOnlyUnresolved().assigned())
        HiddenNodes.insert("IsReadOnly");
    if (!objectInternal.getDefaultValueUnresolved().assigned())
        HiddenNodes.insert("DefaultValue");
    if (!objectInternal.getUnitUnresolved().assigned())
        HiddenNodes.insert("Unit");
    if (!object.getValidator().assigned())
        HiddenNodes.insert("ValidationExpression");
    if (!object.getCoercer().assigned())
        HiddenNodes.insert("CoercionExpression");
}

void TmsServerProperty::hideStructureTypeChildren()
{
    // TODO: Add support for these
    HiddenNodes.insert("FieldCoercionExpression");
    HiddenNodes.insert("FieldValidationExpression");
}

void TmsServerProperty::addReferenceTypeChildNodes()
{
    const auto type = object.getValueType();
    if (type == CoreType::ctStruct)
    {
        StructPtr structPtr = this->parent.getRef().getPropertyValue(object.getName());
        std::string structTypeName =  structPtr.getStructType().getName();
        if (!nativeStructConversionSupported(structTypeName) && GetUAStructureDataTypeByName(structTypeName) == nullptr)
            return;
    }

    if (type == CoreType::ctEnumeration)
    {
        EnumerationPtr enumPtr = this->parent.getRef().getPropertyValue(object.getName());
        std::string enumTypeName =  enumPtr.getEnumerationType().getName();
        if(GetUAEnumerationDataTypeByName(enumTypeName) == nullptr)
            return;
    }

    const auto refNames = objectInternal.getReferencedPropertyUnresolved().getPropertyReferences();
    for (auto propName : refNames)
    {
        auto prop = parent.getRef().getProperty(propName);
        if (prop.getValueType() != ctObject)
        {
            auto serverInfo = registerTmsObjectOrAddReference<TmsServerProperty>(nodeId, prop, std::numeric_limits<uint32_t>::max(), parent.getRef(), propOrder);
            auto childNodeId = serverInfo->getNodeId();
            childProperties.insert({childNodeId, serverInfo});
        }
    }
}

void TmsServerProperty::addNumericTypeChildNodes()
{
    if (!HiddenNodes.count("MinValue"))
        registerEvalValueNode("MinValue", [this] { return this->objectInternal.getMinValueUnresolved(); });
    if (!HiddenNodes.count("MaxValue"))
        registerEvalValueNode("MaxValue", [this] { return this->objectInternal.getMaxValueUnresolved(); });
    if (!HiddenNodes.count("SuggestedValues"))
        registerEvalValueNode("SuggestedValues", [this] { return this->objectInternal.getSuggestedValuesUnresolved(); });
}

void TmsServerProperty::addSelectionTypeChildNodes()
{
    registerEvalValueNode("SelectionValues", [this] { return this->objectInternal.getSelectionValuesUnresolved(); }, true);
}

void TmsServerProperty::addIntrospectionTypeChildNodes()
{
    if (!HiddenNodes.count("IsReadOnly"))
        registerEvalValueNode("IsReadOnly", [this] { return this->objectInternal.getReadOnlyUnresolved(); });
    if (!HiddenNodes.count("IsVisible"))
        registerEvalValueNode("IsVisible", [this] { return this->objectInternal.getVisibleUnresolved(); });
    if (!HiddenNodes.count("DefaultValue"))
        registerEvalValueNode("DefaultValue", [this] { return this->objectInternal.getDefaultValueUnresolved(); });
    if (!HiddenNodes.count("Unit"))
        registerEvalValueNode("Unit", [this] { return this->objectInternal.getUnitUnresolved(); });
}

END_NAMESPACE_OPENDAQ_OPCUA_TMS
