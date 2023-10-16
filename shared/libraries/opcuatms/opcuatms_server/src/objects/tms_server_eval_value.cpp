#include "opcuatms_server/objects/tms_server_eval_value.h"
#include "opcuatms/converters/variant_converter.h"
#include "opcuatms/converters/selection_converter.h"
#include "open62541/tmsbt_nodeids.h"

BEGIN_NAMESPACE_OPENDAQ_OPCUA_TMS

using namespace opcua;

// TmsServerEvalValue

TmsServerEvalValue::TmsServerEvalValue(const EvalValuePtr& object, const opcua::OpcUaServerPtr& server, const ContextPtr& context)
    : Super(nullptr, server, context)
{
    this->readCallback = [this, object]() { return object; };
    this->writeCallback = [](const BaseObjectPtr& object) { return UA_STATUSCODE_BADNOTWRITABLE; };
}

TmsServerEvalValue::TmsServerEvalValue(const opcua::OpcUaServerPtr& server, const ContextPtr& context)
    : TmsServerEvalValue(nullptr, server, context)
{
}

std::string TmsServerEvalValue::getBrowseName()
{
    return "EvalValue";
}

void TmsServerEvalValue::setReadCallback(ReadCallback readCallback)
{
    this->readCallback = std::move(readCallback);
}

void TmsServerEvalValue::setWriteCallback(WriteCallback writeCallback)
{
    this->writeCallback = writeCallback;
}

void TmsServerEvalValue::setIsSelectionType(bool isSelection)
{
    this->isSelection = isSelection;
}

opcua::OpcUaNodeId TmsServerEvalValue::getTmsTypeId()
{
    return OpcUaNodeId(NAMESPACE_TMSBT, UA_TMSBTID_EVALUATIONVARIABLETYPE);
}

void TmsServerEvalValue::configureVariableNodeAttributes(opcua::OpcUaObject<UA_VariableAttributes>& attr)
{
    attr->accessLevel = UA_ACCESSLEVELMASK_READ | UA_ACCESSLEVELMASK_WRITE;
}

void TmsServerEvalValue::bindCallbacks()
{
    addReadCallback(nodeId, [this]() { return readRoot(); });
    addWriteCallback(nodeId, [this](const OpcUaVariant& variant) -> UA_StatusCode { return writeRoot(variant); });
    addReadCallback("EvaluationExpression", [this]() { return readEvaluationExpression(); });
}

opcua::OpcUaVariant TmsServerEvalValue::readEvaluationExpression()
{
    const auto object = this->readCallback();
    if (!object.assigned())
        return OpcUaVariant();

    try
    {
        const auto eval = object.asPtrOrNull<IEvalValue>();
        if (eval.assigned())
            return VariantConverter<IString>::ToVariant(eval.getEval());
    }
    catch (const ConversionFailedException&)
    {
    }

    return OpcUaVariant();
}

opcua::OpcUaVariant TmsServerEvalValue::readRoot()
{
    const auto object = this->readCallback();
    if (!object.assigned())
        return OpcUaVariant();

    try
    {
        if (isSelection)
        {
            const auto eval = object.asPtrOrNull<IEvalValue>();
            if (eval.assigned())
                return SelectionVariantConverter::ToVariant(eval.getResult());

            return SelectionVariantConverter::ToVariant(object);
        }

        const auto eval = object.asPtrOrNull<IEvalValue>();
        if (eval.assigned())
            return VariantConverter<IBaseObject>::ToVariant(eval.getResult(), nullptr, daqContext);

        return VariantConverter<IBaseObject>::ToVariant(object, nullptr, daqContext);
    }
    catch (const ConversionFailedException&)
    {
        return OpcUaVariant();
    }
}

UA_StatusCode TmsServerEvalValue::writeRoot(const OpcUaVariant& variant)
{
    try
    {
        const auto object = VariantConverter<IBaseObject>::ToDaqObject(variant, daqContext);
        return writeCallback(object);
    }
    catch (const ConversionFailedException&)
    {
        return UA_STATUSCODE_BADINVALIDARGUMENT;
    }

    return UA_STATUSCODE_GOOD;
}

END_NAMESPACE_OPENDAQ_OPCUA_TMS
