#include <opcuatms_server/objects/tms_server_object.h>
#include <opendaq/instance_ptr.h>
#include <opendaq/signal_ptr.h>
#include <coreobjects/eval_value_ptr.h>
#include "opcuatms/converters/variant_converter.h"
#include "open62541/server.h"
#include "open62541/daqbsp_nodeids.h"

BEGIN_NAMESPACE_OPENDAQ_OPCUA_TMS

using namespace opcua;
namespace opcua_utils = opcua::utils;

TmsServerObject::TmsServerObject(const OpcUaServerPtr& server, const ContextPtr& context, const TmsServerContextPtr& tmsContext)
    : server(server)
    , daqContext(context)
    , numberInList(0)
    , tmsContext(tmsContext)
{
}

TmsServerObject::~TmsServerObject()
{
}

std::string TmsServerObject::getBrowseName()
{
    return typeBrowseName;
}

OpcUaNodeId TmsServerObject::getRequestedNodeId()
{
    return {};
}

std::string TmsServerObject::getDisplayName()
{
    return "";
}

std::string TmsServerObject::getDescription()
{
    return "";
}

OpcUaNodeId TmsServerObject::getReferenceType()
{
    return OpcUaNodeId(UA_NS0ID_HASCOMPONENT);
}

OpcUaNodeId TmsServerObject::registerOpcUaNode(const OpcUaNodeId& parentNodeId)
{
    validate();
    this->nodeId = createNode(parentNodeId);
    return registerToExistingOpcUaNode(this->nodeId);
}

OpcUaNodeId TmsServerObject::registerToExistingOpcUaNode(const OpcUaNodeId& nodeId)
{
    if (this->nodeId.isNull())
        validate();

    this->nodeId = nodeId;
    browseReferences();
    addChildNodes();
    browseReferences();
    bindCallbacksInternal();
    bindCallbacks();
    registerToTmsServerContext();
    bindReadWriteCallbacks();
    return this->nodeId;
}

OpcUaNodeId TmsServerObject::getNodeId()
{
    return nodeId;
}

void TmsServerObject::setNumberInList(uint32_t numberInList)
{
    this->numberInList = numberInList;
}

void TmsServerObject::addHierarchicalReference(const OpcUaNodeId& parent)
{
    server->addReference(parent, getReferenceType(), getNodeId(), true);
}

void TmsServerObject::createNonhierarchicalReferences()
{
}

void TmsServerObject::onCoreEvent(const CoreEventArgsPtr& /*eventArgs*/)
{
}

NodeEventManagerPtr TmsServerObject::addEvent(const StringPtr& nodeName)
{
    auto nodeId = getChildNodeId(nodeName);
    return addEvent(nodeId);
}

NodeEventManagerPtr TmsServerObject::addEvent(const OpcUaNodeId& nodeId)
{
    if (eventManagers.count(nodeId) > 0)
        return eventManagers[nodeId];

    auto eventManager = std::make_shared<NodeEventManager>(nodeId, server);
    eventManagers.insert({nodeId, eventManager});
    return eventManager;
}

void TmsServerObject::validate()
{
}

OpcUaNodeId TmsServerObject::createNode(const OpcUaNodeId& parentNodeId)
{
    OpcUaNodeId newNodeId;
    auto typeNodeId = getTmsTypeId();

    std::string browseName;
    auto blueberryComponent = getObject().asPtrOrNull<IComponent>(true);
    if (blueberryComponent.assigned())
    {
        browseName = blueberryComponent.getLocalId().toStdString();
    }
    else
    {
        typeBrowseName = readTypeBrowseName();
        browseName = getBrowseName();
        if (browseName.empty())
            browseName = typeBrowseName;
    }

    auto params = AddObjectNodeParams(getRequestedNodeId(), parentNodeId);
    configureNodeAttributes(params.attr);
    params.referenceTypeId = getReferenceType();
    params.setBrowseName(browseName);
    params.typeDefinition = typeNodeId;
    params.nodeContext = this;
    params.addOptionalNodeCallback = [this](const OpcUaNodeId& nodeId) { return this->createOptionalNode(nodeId); };
    newNodeId = server->addObjectNode(params);

    return OpcUaNodeId(newNodeId);
}

void TmsServerObject::addChildNodes()
{
}

bool TmsServerObject::hasChildNode(const std::string& nodeName) const
{
    return references.count(nodeName) != 0;
}

OpcUaNodeId TmsServerObject::getChildNodeId(const std::string& nodeName)
{
    return references[nodeName]->nodeId.nodeId;
}

OpcUaNodeId TmsServerObject::findSignalNodeId(const SignalPtr& signal) const
{
    return findTmsObjectNodeId(signal);
}

void TmsServerObject::bindCallbacksInternal()
{
    if (hasChildNode("NumberInList"))
        this->addReadCallback("NumberInList", [this]() { return VariantConverter<IInteger>::ToVariant(numberInList); });
}

void TmsServerObject::bindCallbacks()
{
}

void TmsServerObject::registerToTmsServerContext()
{
}

void TmsServerObject::bindReadWriteCallbacks()
{
    for (const auto& entry : readCallbacks)
    {
        const OpcUaNodeId nodeId = entry.first;
        auto readCallback = entry.second;

        addEvent(nodeId)->onDataSourceRead([this, readCallback](NodeEventManager::DataSourceReadArgs args) -> UA_StatusCode {
            std::lock_guard<std::mutex> lock(this->valueMutex);
            try
            {
                auto& dataVelue = args.value;
                dataVelue->hasServerTimestamp = UA_TRUE;
                dataVelue->sourceTimestamp = getCurrentClock();
                OpcUaVariant variant = readCallback();
                dataVelue->value = variant.getDetachedValue();
                return UA_STATUSCODE_GOOD;
            }
            catch (...)
            {
                return UA_STATUSCODE_BADINTERNALERROR;
            }
        });
    }

    for (const auto& entry : writeCallbacks)
    {
        const OpcUaNodeId nodeId = entry.first;
        auto writeCallback = entry.second;

        addEvent(nodeId)->onDataSourceWrite([this, writeCallback](NodeEventManager::DataSourceWriteArgs args) -> UA_StatusCode {
            std::lock_guard<std::mutex> lock(this->valueMutex);
            try
            {
                auto variant = OpcUaVariant(std::move(args.value->value));
                return writeCallback(variant);
            }
            catch (...)
            {
                return UA_STATUSCODE_BADINTERNALERROR;
            }
        });
    }
}

int64_t TmsServerObject::getCurrentClock()
{
    // later we will probably have a getClockProvider() method or something similar
    return -1;
}

std::string TmsServerObject::readTypeBrowseName()
{
    auto typeNodeId = getTmsTypeId();
    OpcUaObject<UA_QualifiedName> browseName;
    UA_Server_readBrowseName(server->getUaServer(), *typeNodeId, browseName.get());
    return opcua_utils::ToStdString(browseName->name);
}

bool TmsServerObject::createOptionalNode(const OpcUaNodeId& nodeId)
{
    return true;
}

void TmsServerObject::configureNodeAttributes(OpcUaObject<UA_ObjectAttributes>& attr)
{
    attr->eventNotifier = UA_EVENTNOTIFIER_SUBSCRIBE_TO_EVENT;
    auto displayName = getDisplayName();
    if (!displayName.empty())
        attr->displayName = UA_LOCALIZEDTEXT_ALLOC("", displayName.c_str());
    auto description = getDescription();
    if (!description.empty())
        attr->description = UA_LOCALIZEDTEXT_ALLOC("", description.c_str());
}

void TmsServerObject::addReadCallback(const std::string& nodeName, ReadVariantCallback readFunc)
{
    auto nodeId = getChildNodeId(nodeName);
    readCallbacks.insert({nodeId, std::move(readFunc)});
}

void TmsServerObject::addWriteCallback(const std::string& nodeName, WriteVariantCallback writeFunc)
{
    auto nodeId = getChildNodeId(nodeName);
    writeCallbacks.insert({nodeId, std::move(writeFunc)});
}

void TmsServerObject::addReadCallback(const OpcUaNodeId& nodeId, ReadVariantCallback readFunc)
{
    readCallbacks.insert({nodeId, std::move(readFunc)});
}

void TmsServerObject::addWriteCallback(const OpcUaNodeId& nodeId, WriteVariantCallback writeFunc)
{
    writeCallbacks.insert({nodeId, std::move(writeFunc)});
}

void TmsServerObject::addReference(const OpcUaNodeId& targetNodeId, const OpcUaNodeId& referenceTypeId)
{
    server->addReference(nodeId, referenceTypeId, targetNodeId, true);
}

void TmsServerObject::deleteReferencesOfType(const opcua::OpcUaNodeId& referenceTypeId)
{
    browseReferences();

    for (const auto& [browseName, ref] : references)
    {
        if (OpcUaNodeId(ref->referenceTypeId) == referenceTypeId)
            server->deleteReference(nodeId, referenceTypeId, ref->nodeId.nodeId, ref->isForward);
    }

    browseReferences();
}

void TmsServerObject::browseReferences()
{
    OpcUaObject<UA_BrowseDescription> bd;
    bd->nodeId = nodeId.copyAndGetDetachedValue();
    bd->resultMask = UA_BROWSERESULTMASK_ALL;
    auto result = server->browse(bd);

    for (size_t i = 0; i < result->referencesSize; i++)
    {
        auto reference = result->references[i];
        std::string browseName = opcua_utils::ToStdString(reference.browseName.name);
        references.insert({browseName, OpcUaObject<UA_ReferenceDescription>(reference)});
    }
}

END_NAMESPACE_OPENDAQ_OPCUA_TMS
