#include <opcuatms_client/tms_attribute_collector.h>
#include <open62541/daqbt_nodeids.h>
#include <open62541/daqdevice_nodeids.h>
#include <open62541/daqbsp_nodeids.h>

BEGIN_NAMESPACE_OPENDAQ_OPCUA_TMS

// Node ids

const OpcUaNodeId TmsAttributeCollector::NodeIdBaseObjectType = OpcUaNodeId(NAMESPACE_DAQBT, UA_DAQBTID_DAQBASEOBJECTTYPE);
const OpcUaNodeId TmsAttributeCollector::NodeIdBaseVariableType = OpcUaNodeId(NAMESPACE_DAQBT, UA_DAQBTID_DAQBASEVARIABLETYPE);
const OpcUaNodeId TmsAttributeCollector::NodeIdDeviceType = OpcUaNodeId(NAMESPACE_DAQDEVICE, UA_DAQDEVICEID_DAQDEVICETYPE);
const OpcUaNodeId TmsAttributeCollector::NodeIdFunctionBlockType = OpcUaNodeId(NAMESPACE_DAQBSP, UA_DAQBSPID_BASEFUNCTIONBLOCKTYPE);
const OpcUaNodeId TmsAttributeCollector::NodeIdComponentType = OpcUaNodeId(NAMESPACE_DAQDEVICE, UA_DAQDEVICEID_DAQCOMPONENTTYPE);
const OpcUaNodeId TmsAttributeCollector::NodeIdSignalType = OpcUaNodeId(NAMESPACE_DAQBSP, UA_DAQBSPID_SIGNALTYPE);
const OpcUaNodeId TmsAttributeCollector::NodeIdInputPortType = OpcUaNodeId(NAMESPACE_DAQBSP, UA_DAQBSPID_INPUTPORTTYPE);
const OpcUaNodeId TmsAttributeCollector::NodeIdEvaluationVariableType = OpcUaNodeId(NAMESPACE_DAQBT, UA_DAQBTID_EVALUATIONVARIABLETYPE);
const OpcUaNodeId TmsAttributeCollector::NodeIdVariableBlockType = OpcUaNodeId(NAMESPACE_DAQBT, UA_DAQBTID_VARIABLEBLOCKTYPE);

// TmsAttributeCollector

TmsAttributeCollector::TmsAttributeCollector(const CachedReferenceBrowserPtr& browser)
    : browser(browser)
{
}


tsl::ordered_set<OpcUaAttribute> TmsAttributeCollector::collectAttributes(const OpcUaNodeId& nodeId)
{
    attributes.clear();

    const auto typeDefinition = browser->getTypeDefinition(nodeId);

    if (typeDefinition.isNull())
        return attributes;

    if (typeEquals(typeDefinition, NodeIdDeviceType))
        collectDeviceAttributes(nodeId);
    else if (isSubtypeOf(typeDefinition, NodeIdFunctionBlockType))
        collectFunctionBlockAttributes(nodeId);
    else if (typeEquals(typeDefinition, NodeIdSignalType))
        collectSignalAttributes(nodeId);
    else if (typeEquals(typeDefinition, NodeIdInputPortType))
        collectInputPortAttributes(nodeId);
    else if (isSubtypeOf(typeDefinition, NodeIdComponentType))
        collectComponentAttributes(nodeId);
    else if (isSubtypeOf(typeDefinition, NodeIdBaseObjectType))
        collectPropertyObjectAttributes(nodeId);
    else if (isSubtypeOf(typeDefinition, NodeIdBaseVariableType))
        collectPropertyAttributes(nodeId);

    return attributes;
}

void TmsAttributeCollector::collectDeviceAttributes(const OpcUaNodeId& nodeId)
{
    collectPropertyObjectAttributes(nodeId);

    const auto& references = browser->browse(nodeId);

    for (const auto& [refNodeId, ref] : references.byNodeId)
    {
        if (typeEquals(ref->typeDefinition.nodeId, NodeIdDeviceType))
            collectDeviceAttributes(refNodeId);
        else if (isSubtypeOf(ref->typeDefinition.nodeId, NodeIdComponentType))
            collectComponentAttributes(refNodeId);
    }

    const auto ioNodeId = browser->getChildNodeId(nodeId, "IO");
    collectIoNode(ioNodeId);

    const auto fbNodeId = browser->getChildNodeId(nodeId, "FB");
    collectFunctionBlockNode(fbNodeId);

    const auto signalsNodeId = browser->getChildNodeId(nodeId, "Sig");
    collectSignalsNode(signalsNodeId);

    const auto inputPortsNodeId = browser->getChildNodeId(nodeId, "IP");
    collectInputPortNode(inputPortsNodeId);

    const auto streamingOptionsNodeId = browser->getChildNodeId(nodeId, "StreamingOptions");
    collectStreamingOptionsNode(streamingOptionsNodeId);

    const auto methodSetId = browser->getChildNodeId(nodeId, "MethodSet");
    collectMethodSetNode(methodSetId);
}

void TmsAttributeCollector::collectFunctionBlockAttributes(const OpcUaNodeId& nodeId)
{
    collectPropertyObjectAttributes(nodeId);

    const auto fbInfoId = browser->getChildNodeId(nodeId, "FunctionBlockInfo");
    attributes.insert({fbInfoId, UA_ATTRIBUTEID_VALUE});

    const auto& references = browser->browse(nodeId);

    for (const auto& [refNodeId, ref] : references.byNodeId)
    {
        if (isSubtypeOf(ref->typeDefinition.nodeId, NodeIdFunctionBlockType))
            collectFunctionBlockAttributes(refNodeId);
    }

    const auto signalsNodeId = browser->getChildNodeId(nodeId, "Sig");
    collectSignalsNode(signalsNodeId);

    const auto inputPortsNodeId = browser->getChildNodeId(nodeId, "IP");
    collectInputPortNode(inputPortsNodeId);
}

void TmsAttributeCollector::collectInputPortAttributes(const OpcUaNodeId& nodeId)
{
    collectBaseObjectAttributes(nodeId);
}

void TmsAttributeCollector::collectSignalAttributes(const OpcUaNodeId& nodeId)
{
    collectBaseObjectAttributes(nodeId);
    attributes.insert({nodeId, UA_ATTRIBUTEID_BROWSENAME});
}

void TmsAttributeCollector::collectComponentAttributes(const OpcUaNodeId& nodeId)
{
    collectPropertyObjectAttributes(nodeId);
    
    const auto& references = browser->browse(nodeId);

    for (const auto& [refNodeId, ref] : references.byNodeId)
    {
        if (isSubtypeOf(ref->typeDefinition.nodeId, NodeIdFunctionBlockType))
            collectFunctionBlockAttributes(refNodeId);
        else if (isSubtypeOf(ref->typeDefinition.nodeId, NodeIdComponentType))
            collectComponentAttributes(refNodeId);
    }
}

void TmsAttributeCollector::collectPropertyObjectAttributes(const OpcUaNodeId& nodeId)
{
    collectBaseObjectAttributes(nodeId);

    attributes.insert({nodeId, UA_ATTRIBUTEID_DESCRIPTION});
    
    const auto& references = browser->browse(nodeId);

    for (const auto& [refNodeId, ref] : references.byNodeId)
    {
        if (ref->nodeClass == UA_NODECLASS_METHOD)
            collectMethodAttributes(refNodeId);
        else if (typeEquals(ref->typeDefinition.nodeId, NodeIdEvaluationVariableType))
            collectEvaluationPropertyAttributes(refNodeId);
        else if (isSubtypeOf(ref->typeDefinition.nodeId, NodeIdBaseObjectType))
            collectPropertyObjectAttributes(refNodeId);
        else if (isSubtypeOf(ref->typeDefinition.nodeId, NodeIdBaseVariableType))
            collectPropertyAttributes(refNodeId);
    }
}

void TmsAttributeCollector::collectPropertyAttributes(const OpcUaNodeId& nodeId)
{
    collectBaseObjectAttributes(nodeId);

    attributes.insert({nodeId, UA_ATTRIBUTEID_VALUE});
    attributes.insert({nodeId, UA_ATTRIBUTEID_DISPLAYNAME});
    attributes.insert({nodeId, UA_ATTRIBUTEID_DESCRIPTION});

    if (browser->hasReference(nodeId, "ValidationExpression"))
        attributes.insert({browser->getChildNodeId(nodeId, "ValidationExpression"), UA_ATTRIBUTEID_VALUE});
    if (browser->hasReference(nodeId, "CoercionExpression"))
        attributes.insert({browser->getChildNodeId(nodeId, "CoercionExpression"), UA_ATTRIBUTEID_VALUE});

    const auto& references = browser->browse(nodeId);

    for (const auto& [refNodeId, ref] : references.byNodeId)
    {
        if (typeEquals(ref->typeDefinition.nodeId, NodeIdEvaluationVariableType))
            collectEvaluationPropertyAttributes(refNodeId);
        else if (isSubtypeOf(ref->typeDefinition.nodeId, NodeIdBaseVariableType))
            collectPropertyAttributes(refNodeId);
    }
}

void TmsAttributeCollector::collectEvaluationPropertyAttributes(const OpcUaNodeId& nodeId)
{
    attributes.insert({nodeId, UA_ATTRIBUTEID_VALUE});

    const auto evalValueId = browser->getChildNodeId(nodeId, "EvaluationExpression");
    attributes.insert({evalValueId, UA_ATTRIBUTEID_VALUE});
}

void TmsAttributeCollector::collectBaseObjectAttributes(const OpcUaNodeId& nodeId)
{
    attributes.insert({nodeId, UA_ATTRIBUTEID_NODECLASS});

    const auto numberInListId = browser->getChildNodeId(nodeId, "NumberInList");
    attributes.insert({numberInListId, UA_ATTRIBUTEID_VALUE});
}

void TmsAttributeCollector::collectMethodAttributes(const OpcUaNodeId& nodeId)
{
    if (browser->hasReference(nodeId, "InputArguments"))
        attributes.insert({browser->getChildNodeId(nodeId, "InputArguments"), UA_ATTRIBUTEID_VALUE});
    if (browser->hasReference(nodeId, "OutputArguments"))
        attributes.insert({browser->getChildNodeId(nodeId, "OutputArguments"), UA_ATTRIBUTEID_VALUE});
    if (browser->hasReference(nodeId, "NumberInList"))
        attributes.insert({browser->getChildNodeId(nodeId, "NumberInList"), UA_ATTRIBUTEID_VALUE});
}

void TmsAttributeCollector::collectVariableBlockAttributes(const OpcUaNodeId& nodeId)
{
    collectPropertyObjectAttributes(nodeId);
}

void TmsAttributeCollector::collectIoNode(const OpcUaNodeId& nodeId)
{
    const auto& references = browser->browse(nodeId);

    for (const auto& [refNodeId, ref] : references.byNodeId)
    {
        if (isSubtypeOf(ref->typeDefinition.nodeId, NodeIdFunctionBlockType))
            collectFunctionBlockAttributes(refNodeId);
        else if (isSubtypeOf(ref->typeDefinition.nodeId, NodeIdComponentType))
            collectComponentAttributes(refNodeId);
    }
}

void TmsAttributeCollector::collectInputPortNode(const OpcUaNodeId& nodeId)
{
    const auto& references = browser->browse(nodeId);

    for (const auto& [refNodeId, ref] : references.byNodeId)
    {
        if (typeEquals(ref->typeDefinition.nodeId, NodeIdInputPortType))
            collectInputPortAttributes(refNodeId);
    }
}

void TmsAttributeCollector::collectFunctionBlockNode(const OpcUaNodeId& nodeId)
{
    const auto& references = browser->browse(nodeId);

    for (const auto& [refNodeId, ref] : references.byNodeId)
    {
        if (isSubtypeOf(ref->typeDefinition.nodeId, NodeIdFunctionBlockType))
            collectFunctionBlockAttributes(refNodeId);
    }
}

void TmsAttributeCollector::collectSignalsNode(const OpcUaNodeId& nodeId)
{
    const auto& signalReferences = browser->browse(nodeId);

    for (const auto& [refNodeId, ref] : signalReferences.byNodeId)
    {
        if (typeEquals(ref->typeDefinition.nodeId, NodeIdSignalType))
            collectSignalAttributes(refNodeId);
    }
}

void TmsAttributeCollector::collectStreamingOptionsNode(const OpcUaNodeId& nodeId)
{
    const auto& references = browser->browse(nodeId);

    for (const auto& [refNodeId, ref] : references.byNodeId)
    {
        if (typeEquals(ref->typeDefinition.nodeId, NodeIdVariableBlockType))
            collectVariableBlockAttributes(refNodeId);
    }
}

void TmsAttributeCollector::collectMethodSetNode(const OpcUaNodeId& nodeId)
{
    const auto& references = browser->browse(nodeId);

    for (const auto& [refNodeId, ref] : references.byNodeId)
    {
        if (ref->nodeClass == UA_NODECLASS_METHOD)
            collectMethodAttributes(refNodeId);
    }
}

bool TmsAttributeCollector::isSubtypeOf(const OpcUaNodeId& typeId, const OpcUaNodeId& baseType)
{
    return browser->isSubtypeOf(typeId, baseType);
}

bool TmsAttributeCollector::typeEquals(const OpcUaNodeId& typeId, const OpcUaNodeId& baseType)
{
    return typeId == baseType;
}

END_NAMESPACE_OPENDAQ_OPCUA_TMS
