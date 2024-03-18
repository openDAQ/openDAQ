#include "opcuatms_client/objects/tms_client_property_impl.h"
#include <opcuatms/core_types_utils.h>
#include "coreobjects/coercer_factory.h"
#include "coreobjects/eval_value_factory.h"
#include "coreobjects/validator_factory.h"
#include "opcuatms/converters/variant_converter.h"
#include "opcuatms/converters/selection_converter.h"
#include "open62541/daqbt_nodeids.h"
#include "opendaq/custom_log.h"

BEGIN_NAMESPACE_OPENDAQ_OPCUA_TMS

using namespace daq::opcua;

namespace details
{
    enum class PropertyField
    {
        CoercionExpression = 0,
        ValidationExpression,
        DefaultValue,
        IsReadOnly,
        IsVisible,
        Unit,
        MaxValue,
        MinValue,
        SuggestedValues,
        SelectionValues
    };

    static std::unordered_map<std::string, PropertyField> stringToPropertyFieldEnum{
        {"CoercionExpression", PropertyField::CoercionExpression},
        {"ValidationExpression", PropertyField::ValidationExpression},
        {"DefaultValue", PropertyField::DefaultValue},
        {"IsReadOnly", PropertyField::IsReadOnly},
        {"IsVisible", PropertyField::IsVisible},
        {"Unit", PropertyField::Unit},
        {"MaxValue", PropertyField::MaxValue},
        {"MinValue", PropertyField::MinValue},
        {"SuggestedValues", PropertyField::SuggestedValues},
        {"SelectionValues", PropertyField::SelectionValues},
    };
}

TmsClientPropertyImpl::TmsClientPropertyImpl(const ContextPtr& daqContext, const TmsClientContextPtr& ctx, const opcua::OpcUaNodeId& nodeId)
    : TmsClientObjectImpl(daqContext, ctx, nodeId)
{
    if (!this->daqContext.getLogger().assigned())
        throw ArgumentNullException("Logger must not be null");

    this->loggerComponent = this->daqContext.getLogger().getOrAddComponent("TmsClientPropertyImpl");
    
    clientContext->readObjectAttributes(nodeId);

    readBasicInfo();
    configurePropertyFields();
}

void TmsClientPropertyImpl::readBasicInfo()
{
    auto reader = clientContext->getAttributeReader();
    this->name = String(reader->getValue(nodeId, UA_ATTRIBUTEID_DISPLAYNAME).toString());
    this->description = String(reader->getValue(nodeId, UA_ATTRIBUTEID_DESCRIPTION).toString());

    const auto dataType = reader->getValue(nodeId, UA_ATTRIBUTEID_DATATYPE).toNodeId();
    const auto enumerationTypeId = OpcUaNodeId(0, UA_NS0ID_ENUMERATION);

    if (clientContext->getReferenceBrowser()->isSubtypeOf(dataType, enumerationTypeId))
    {
        this->valueType = ctEnumeration;
    }
    else
    {
        const auto variant = reader->getValue(nodeId, UA_ATTRIBUTEID_VALUE);
        const auto object = VariantConverter<IBaseObject>::ToDaqObject(variant, daqContext);
        this->valueType = object.getCoreType();
    }
}

void TmsClientPropertyImpl::configurePropertyFields()
{
    const auto evaluationVariableTypeId = OpcUaNodeId(NAMESPACE_DAQBT, UA_DAQBTID_EVALUATIONVARIABLETYPE);
    const auto& references = clientContext->getReferenceBrowser()->browse(nodeId);
    const auto reader = clientContext->getAttributeReader();

    for (auto [browseName, ref] : references.byBrowseName)
    {
        const auto childNodeId = OpcUaNodeId(ref->nodeId.nodeId);

        if (browseName == "CoercionExpression")
        {
            this->coercer = Coercer(VariantConverter<IString>::ToDaqObject(reader->getValue(childNodeId, UA_ATTRIBUTEID_VALUE)));
        }
        else if (browseName == "ValidationExpression")
        {
            this->validator = Validator(VariantConverter<IString>::ToDaqObject(reader->getValue(childNodeId, UA_ATTRIBUTEID_VALUE)));
        }
        else if (clientContext->getReferenceBrowser()->isSubtypeOf(ref->typeDefinition.nodeId, evaluationVariableTypeId))
        {
            auto evalId = clientContext->getReferenceBrowser()->getChildNodeId(childNodeId, "EvaluationExpression");

            StringPtr evalStr = VariantConverter<IString>::ToDaqObject(reader->getValue(evalId, UA_ATTRIBUTEID_VALUE));
            if (details::stringToPropertyFieldEnum.count(browseName))
            {
                bool strHasValue = false;
                const auto propertyField = details::stringToPropertyFieldEnum[browseName];
                if (evalStr.assigned())
                {
                    if (evalStr.getLength() > 0)
                        strHasValue = true;
                }
                if (strHasValue)
                {
                    switch (propertyField)
                    {
                        case details::PropertyField::DefaultValue:

                            this->defaultValue = EvalValue(evalStr);
                            break;

                        case details::PropertyField::IsReadOnly:
                            this->readOnly = EvalValue(evalStr).asPtr<IBoolean>();
                            break;

                        case details::PropertyField::IsVisible:
                            this->visible = EvalValue(evalStr).asPtr<IBoolean>();
                            break;

                        case details::PropertyField::Unit:
                            this->unit = EvalValue(evalStr).asPtr<IUnit>();
                            break;

                        case details::PropertyField::MaxValue:
                            this->maxValue = EvalValue(evalStr).asPtr<INumber>();
                            break;

                        case details::PropertyField::MinValue:
                            this->minValue = EvalValue(evalStr).asPtr<INumber>();
                            break;

                        case details::PropertyField::SuggestedValues:
                            this->suggestedValues = EvalValue(evalStr).asPtr<IList>();
                            break;

                        case details::PropertyField::SelectionValues:
                            this->selectionValues = EvalValue(evalStr);
                            break;
                        case details::PropertyField::CoercionExpression:
                        case details::PropertyField::ValidationExpression:
                            break;
                    }
                }
                else
                {
                    switch (propertyField)
                    {
                        case details::PropertyField::DefaultValue:
                        {
                            // ToDo: This is a workarround for devices which are delivering not a default value,
                            // even if this is a mandatory property in the openDAQ Standard.
                            // However, the SDK creates too strong a requirement, which cannot be
                            // met by all the standards or devices to be embraced.
                            // In this case the actual value from the first connect is set to it.
                            // But, this creates a weak point:
                            // SDK stores only values of variables which are != to the device default value.
                            // The choosen default value could be not the true default value from the device.
                            // So, all in all we aligned on that in future the SDK will also support properties
                            // which have not a default value as the device for which the workaround is needed.
                            // But this is feature request and is covered with
                            // https://blueberrydaq.atlassian.net/browse/TBBAS-1216.
                            // But as long as the feature is not implemented this is a valid workarround to get
                            // devices working which are deliviering not a default value via the opc-ua interface.
                            // Afterwards, the workaround needs to be rolled back.

                            auto value = reader->getValue(childNodeId, UA_ATTRIBUTEID_VALUE);
                            if(value.isNull())
                            {
                                value = reader->getValue(nodeId, UA_ATTRIBUTEID_VALUE);
                                this->defaultValue = VariantConverter<IBaseObject>::ToDaqObject(value, daqContext);
                                LOG_W(
                                    "Failed to read default value of property {} on OpcUa client. Detault value is set to the value at connection time.",
                                    this->name);
                            }

                            //Special handling for enumerations as this data type is encoded as Int32 in OPCUA
                            const auto dataType = reader->getValue(nodeId, UA_ATTRIBUTEID_DATATYPE).toNodeId();
                            const auto enumerationTypeId = OpcUaNodeId(0, UA_NS0ID_ENUMERATION);

                            if (clientContext->getReferenceBrowser()->isSubtypeOf(dataType, enumerationTypeId))
                            {
                                if (value->type != &UA_TYPES[UA_TYPES_INT32])
                                    throw ConversionFailedException{"Enumeration node data type is not uint32_t"};

                                const auto enumBrowseName = client->readBrowseName(dataType);
                                const auto enumType = GetUAEnumerationDataTypeByName(enumBrowseName);
                                OpcUaVariant variant{};
                                UA_Variant_setScalarCopy(&variant.getValue(), value->data, enumType);
                                this->defaultValue = VariantConverter<IEnumeration>::ToDaqObject(variant, daqContext);
                            }
                            else
                                this->defaultValue = VariantConverter<IBaseObject>::ToDaqObject(value, daqContext);

                            if (this->defaultValue.assigned() && this->defaultValue.asPtrOrNull<IFreezable>().assigned())
                                this->defaultValue.freeze();

                            break;
                        }
                        case details::PropertyField::IsReadOnly:
                            this->readOnly = VariantConverter<IBoolean>::ToDaqObject(reader->getValue(childNodeId, UA_ATTRIBUTEID_VALUE));
                            break;
                        case details::PropertyField::IsVisible:
                            this->visible = VariantConverter<IBoolean>::ToDaqObject(reader->getValue(childNodeId, UA_ATTRIBUTEID_VALUE));
                            break;
                        case details::PropertyField::Unit:
                            this->unit = VariantConverter<IUnit>::ToDaqObject(reader->getValue(childNodeId, UA_ATTRIBUTEID_VALUE));
                            break;
                        case details::PropertyField::MaxValue:
                            this->maxValue = VariantConverter<INumber>::ToDaqObject(reader->getValue(childNodeId, UA_ATTRIBUTEID_VALUE));
                            break;
                        case details::PropertyField::MinValue:
                            this->minValue = VariantConverter<INumber>::ToDaqObject(reader->getValue(childNodeId, UA_ATTRIBUTEID_VALUE));
                            break;
                        case details::PropertyField::SuggestedValues:
                        {
                            this->suggestedValues =
                                VariantConverter<IBaseObject>::ToDaqList(reader->getValue(childNodeId, UA_ATTRIBUTEID_VALUE), daqContext);
                            if (this->suggestedValues.assigned() && this->suggestedValues.asPtrOrNull<IFreezable>().assigned())
                                this->suggestedValues.freeze();
                            break;
                        }
                        case details::PropertyField::SelectionValues:
                        {
                            this->selectionValues =
                                SelectionVariantConverter::ToDaqObject(reader->getValue(childNodeId, UA_ATTRIBUTEID_VALUE));
                            if (this->selectionValues.assigned() && this->selectionValues.asPtrOrNull<IFreezable>().assigned())
                                this->selectionValues.freeze();
                            break;
                        }
                        case details::PropertyField::CoercionExpression:
                        case details::PropertyField::ValidationExpression:
                            break;
                    }
                }
            }
        }
    }
}

END_NAMESPACE_OPENDAQ_OPCUA_TMS
