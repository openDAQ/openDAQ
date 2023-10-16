#include <opcuaclient/event_filter.h>
#include <opcuashared/opcuaexception.h>
#include <open62541/nodeids.h>

BEGIN_NAMESPACE_OPENDAQ_OPCUA

EventFilter::EventFilter(size_t selectClausesSize)
    : OpcUaObject<UA_EventFilter>()
{
    resizeSelectClauses(selectClausesSize);
}

void EventFilter::resizeSelectClauses(size_t selectClausesSize)
{
    CheckStatusCodeException(UA_Array_resize((void**) &value.selectClauses, &value.selectClausesSize, selectClausesSize, &UA_TYPES[UA_TYPES_SIMPLEATTRIBUTEOPERAND]));
}

void EventFilter::setSelectClause(size_t index, const OpcUaObject<UA_SimpleAttributeOperand>& simpleAttributeOperand)
{
    value.selectClauses[index] = simpleAttributeOperand.copyAndGetDetachedValue();
}
void EventFilter::setSelectClause(size_t index, OpcUaObject<UA_SimpleAttributeOperand>&& simpleAttributeOperand)
{
    value.selectClauses[index] = simpleAttributeOperand.getDetachedValue();
}

/*SimpleAttributeOperand*/

OpcUaObject<UA_SimpleAttributeOperand> SimpleAttributeOperand::CreateStandardEventValue(const char* attributeName)
{
    OpcUaObject<UA_SimpleAttributeOperand> simpleAttributeOperand;

    simpleAttributeOperand->typeDefinitionId = UA_NODEID_NUMERIC(0, UA_NS0ID_BASEEVENTTYPE);
    simpleAttributeOperand->browsePathSize = 1;
    simpleAttributeOperand->browsePath =
        (UA_QualifiedName*)UA_Array_new(simpleAttributeOperand->browsePathSize, &UA_TYPES[UA_TYPES_QUALIFIEDNAME]);

    simpleAttributeOperand->attributeId = UA_ATTRIBUTEID_VALUE;
    simpleAttributeOperand->browsePath[0] = UA_QUALIFIEDNAME_ALLOC(0, attributeName);

    return simpleAttributeOperand;
}

END_NAMESPACE_OPENDAQ_OPCUA
