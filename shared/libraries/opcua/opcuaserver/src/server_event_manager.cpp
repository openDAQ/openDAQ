#include "opcuaserver/server_event_manager.h"
#include "opcuaserver/opcuaserver.h"

BEGIN_NAMESPACE_OPENDAQ_OPCUA

ServerEventManager::ServerEventManager(const OpcUaServerPtr& server)
    : server(server.get())
{
}

ServerEventManager::ServerEventManager(OpcUaServer* server)
    : server(server)
{
}

void ServerEventManager::registerEvents()
{
    auto config = UA_Server_getConfig(server->getUaServer());

    config->nodeLifecycle.context = this;
    config->displayNameChanged = DisplayNameChanged;
    config->nodeLifecycle.createOptionalChild = CreateOptionalNode;
}

UA_Boolean ServerEventManager::triggerCreateOptionalNode(const UA_NodeId* nodeId)
{
    const auto nodeIdObj = OpcUaNodeId(*nodeId);
    if (createOptionalNodeCallback == nullptr)
        return false;

    return createOptionalNodeCallback(nodeIdObj);
}

void ServerEventManager::triggerDisplayNameChanged(const UA_NodeId* nodeId, UA_LocalizedText* name)
{
    const auto nodeIdObj = OpcUaNodeId(*nodeId);
    if (displayNameCallbacks.count(nodeIdObj) == 0)
        return;

    auto callback = displayNameCallbacks[nodeIdObj];
    callback(nodeIdObj, OpcUaObject<UA_LocalizedText>(*name));
}

void ServerEventManager::onCreateOptionalNode(const CreatOptionalNodeCallback& callback)
{
    createOptionalNodeCallback = callback;
}

void ServerEventManager::onDisplayNameChanged(const OpcUaNodeId& nodeId, const DisplayNameChangedCallback& callback)
{
    displayNameCallbacks.insert({nodeId, callback});
}

void ServerEventManager::removeOnDisplayNameChanged(const OpcUaNodeId& nodeId)
{
    displayNameCallbacks.erase(nodeId);
}

// Static callbacks

UA_Boolean ServerEventManager::CreateOptionalNode(UA_Server* server,
                                                  const UA_NodeId* sessionId,
                                                  void* sessionContext,
                                                  const UA_NodeId* sourceNodeId,
                                                  const UA_NodeId* targetParentNodeId,
                                                  const UA_NodeId* referenceTypeId)
{
    auto& lifecycle = UA_Server_getConfig(server)->nodeLifecycle;
    auto eventManager = (ServerEventManager*) lifecycle.context;
    return eventManager->triggerCreateOptionalNode(sourceNodeId);
}

void ServerEventManager::DisplayNameChanged(UA_Server* server, UA_NodeId* nodeId, UA_LocalizedText* newDisplayName)
{
    auto& lifecycle = UA_Server_getConfig(server)->nodeLifecycle;
    auto eventManager = (ServerEventManager*) lifecycle.context;
    eventManager->triggerDisplayNameChanged(nodeId, newDisplayName);
}

END_NAMESPACE_OPENDAQ_OPCUA
