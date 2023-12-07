#include "opcuatms_client/objects/tms_client_property_impl.h"
#include "coreobjects/coercer_factory.h"
#include "coreobjects/eval_value_factory.h"
#include "coreobjects/validator_factory.h"
#include "opcuatms/converters/variant_converter.h"
#include "opcuatms/converters/selection_converter.h"
#include "open62541/daqbt_nodeids.h"

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
    readBasicInfo();
    configurePropertyFields();
}

void TmsClientPropertyImpl::readBasicInfo()
{
    const auto variant = client->readValue(nodeId);
    const auto object = VariantConverter<IBaseObject>::ToDaqObject(variant, daqContext);
    this->valueType = object.getCoreType();

    this->name = String(client->readDisplayName(nodeId));
    this->description = String(client->readDescription(nodeId));
}

void TmsClientPropertyImpl::configurePropertyFields()
{
    const auto evaluationVariableTypeId = OpcUaNodeId(NAMESPACE_TMSBT, UA_TMSBTID_EVALUATIONVARIABLETYPE);
    const auto& references = clientContext->getReferenceBrowser()->browse(nodeId);

    for (auto [browseName, ref] : references.byBrowseName)
    {
        const auto childNodeId = OpcUaNodeId(ref->nodeId.nodeId);

        if (browseName == "CoercionExpression")
        {
            this->coercer = Coercer(VariantConverter<IString>::ToDaqObject(client->readValue(childNodeId)));
        }
        else if (browseName == "ValidationExpression")
        {
            this->validator = Validator(VariantConverter<IString>::ToDaqObject(client->readValue(childNodeId)));
        }
        else if (clientContext->getReferenceBrowser()->isSubtypeOf(ref->typeDefinition.nodeId, evaluationVariableTypeId))
        {
            auto evalId = clientContext->getReferenceBrowser()->getChildNodeId(childNodeId, "EvaluationExpression");

            StringPtr evalStr = VariantConverter<IString>::ToDaqObject(client->readValue(evalId));
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
                            this->defaultValue = VariantConverter<IBaseObject>::ToDaqObject(client->readValue(childNodeId), daqContext);
                            if (this->defaultValue.assigned() && this->defaultValue.asPtrOrNull<IFreezable>().assigned())
                                this->defaultValue.freeze();
                            break;

                        case details::PropertyField::IsReadOnly:
                            this->readOnly = VariantConverter<IBoolean>::ToDaqObject(client->readValue(childNodeId));
                            break;

                        case details::PropertyField::IsVisible:
                            this->visible = VariantConverter<IBoolean>::ToDaqObject(client->readValue(childNodeId));
                            break;

                        case details::PropertyField::Unit:
                            this->unit = VariantConverter<IUnit>::ToDaqObject(client->readValue(childNodeId));
                            break;

                        case details::PropertyField::MaxValue:
                            this->maxValue = VariantConverter<INumber>::ToDaqObject(client->readValue(childNodeId));
                            break;

                        case details::PropertyField::MinValue:
                            this->minValue = VariantConverter<INumber>::ToDaqObject(client->readValue(childNodeId));
                            break;

                        case details::PropertyField::SuggestedValues:
                            this->suggestedValues = VariantConverter<IBaseObject>::ToDaqList(client->readValue(childNodeId), daqContext);
                            if (this->suggestedValues.assigned() && this->suggestedValues.asPtrOrNull<IFreezable>().assigned())
                                this->suggestedValues.freeze();
                            break;

                        case details::PropertyField::SelectionValues:
                            this->selectionValues = SelectionVariantConverter::ToDaqObject(client->readValue(childNodeId));
                            if (this->selectionValues.assigned() && this->selectionValues.asPtrOrNull<IFreezable>().assigned())
                                this->selectionValues.freeze();
                            break;
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
