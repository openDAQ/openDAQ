#include "opcuaserver/node_event_manager.h"

BEGIN_NAMESPACE_OPENDAQ_OPCUA

NodeEventManager::NodeEventManager(const OpcUaNodeId& nodeId, OpcUaServerPtr& server)
    : nodeId(nodeId)
    , server(server)
{
    UA_Server_setNodeContext(server->getUaServer(), *nodeId, this);
}

void NodeEventManager::onRead(ReadCallback callback)
{
    readCallback = std::move(callback);

    UA_ValueCallback valueCallback;
    UA_Server_getVariableNode_valueCallback(server->getUaServer(), *nodeId, &valueCallback);
    valueCallback.onRead = OnRead;
    UA_Server_setVariableNode_valueCallback(server->getUaServer(), *nodeId, valueCallback);
}

void NodeEventManager::onWrite(WriteCallback callback)
{
    writeCallback = std::move(callback);

    UA_ValueCallback valueCallback;
    
    UA_Server_getVariableNode_valueCallback(server->getUaServer(), *nodeId, &valueCallback);
    valueCallback.onWrite = OnWrite;
    UA_Server_setVariableNode_valueCallback(server->getUaServer(), *nodeId, valueCallback);
}

void NodeEventManager::onDataSourceRead(DataSourceReadCallback callback)
{
    dataSourceReadCallback = std::move(callback);

    UA_DataSource dataSource;
    UA_Server_getVariableNode_dataSource(server->getUaServer(), *nodeId, &dataSource);
    dataSource.read = OnDataSourceRead;
    UA_Server_setVariableNode_dataSource(server->getUaServer(), *nodeId, dataSource);
}

void NodeEventManager::onDataSourceWrite(DataSourceWriteCallback callback)
{
    dataSourceWriteCallback = std::move(callback);

    UA_DataSource dataSource;
    UA_Server_getVariableNode_dataSource(server->getUaServer(), *nodeId, &dataSource);
    dataSource.write = OnDataSourceWrite;
    UA_Server_setVariableNode_dataSource(server->getUaServer(), *nodeId, dataSource);
}

void NodeEventManager::onMethodCall(MethodCallback callback)
{
    methodCallback = std::move(callback);

    UA_Server_setMethodNodeCallback(server->getUaServer(), *nodeId, OnMethod);
}

void NodeEventManager::onDisplayNameChanged(DisplayNameChangedCallback callback)
{
    server->getEventManager()->onDisplayNameChanged(nodeId, callback);
}

// c-style callback, required by open62541 interface

void NodeEventManager::OnWrite(UA_Server* server,
                                    const UA_NodeId* sessionId,
                                    void* sessionContext,
                                    const UA_NodeId* nodeId,
                                    void* nodeContext,
                                    const UA_NumericRange* range,
                                    const UA_DataValue* value)
{
    auto manager = (NodeEventManager*) nodeContext;

    WriteArgs args;
    args.server = server;
    args.sessionId = sessionId;
    args.sessionContext = sessionContext;
    args.nodeId = nodeId;
    args.nodeContext = nodeContext;
    args.range = range;
    args.value = value;

    manager->writeCallback(args);
}

void NodeEventManager::OnRead(UA_Server* server,
                                   const UA_NodeId* sessionId,
                                   void* sessionContext,
                                   const UA_NodeId* nodeId,
                                   void* nodeContext,
                                   const UA_NumericRange* range,
                                   const UA_DataValue* value)
{
    auto manager = (NodeEventManager*) nodeContext;

    ReadArgs args;
    args.server = server;
    args.sessionId = sessionId;
    args.sessionContext = sessionContext;
    args.nodeId = nodeId;
    args.nodeContext = nodeContext;
    args.range = range;
    args.value = value;

    manager->readCallback(args);
}

UA_StatusCode NodeEventManager::OnDataSourceRead(UA_Server* server,
                                                      const UA_NodeId* sessionId,
                                                      void* sessionContext,
                                                      const UA_NodeId* nodeId,
                                                      void* nodeContext,
                                                      UA_Boolean includeSourceTimeStamp,
                                                      const UA_NumericRange* range,
                                                      UA_DataValue* value)
{
    auto manager = (NodeEventManager*) nodeContext;

    DataSourceReadArgs args;
    args.server = server;
    args.sessionId = sessionId;
    args.sessionContext = sessionContext;
    args.nodeId = nodeId;
    args.nodeContext = nodeContext;
    args.includeSourceTimeStamp = includeSourceTimeStamp;
    args.range = range;
    args.value = value;

    return manager->dataSourceReadCallback(args);
}

UA_StatusCode NodeEventManager::OnDataSourceWrite(UA_Server* server,
                                                       const UA_NodeId* sessionId,
                                                       void* sessionContext,
                                                       const UA_NodeId* nodeId,
                                                       void* nodeContext,
                                                       const UA_NumericRange* range,
                                                       const UA_DataValue* value)
{
    auto manager = (NodeEventManager*) nodeContext;

    DataSourceWriteArgs args;
    args.server = server;
    args.sessionId = sessionId;
    args.sessionContext = sessionContext;
    args.nodeId = nodeId;
    args.nodeContext = nodeContext;
    args.range = range;
    args.value = value;

    return manager->dataSourceWriteCallback(args);
}

UA_StatusCode NodeEventManager::OnMethod(UA_Server* server,
                                              const UA_NodeId* sessionId,
                                              void* sessionContext,
                                              const UA_NodeId* methodId,
                                              void* methodContext,
                                              const UA_NodeId* objectId,
                                              void* objectContext,
                                              size_t inputSize,
                                              const UA_Variant* input,
                                              size_t outputSize,
                                              UA_Variant* output)
{
    auto manager = (NodeEventManager*) methodContext;

    MethodArgs args;
    args.server = server;
    args.sessionId = sessionId;
    args.sessionContext = sessionContext;
    args.methodId = methodId;
    args.methodContext = methodContext;
    args.objectId = objectId;
    args.objectContext = objectContext;
    args.inputSize = inputSize;
    args.input = input;
    args.outputSize = outputSize;
    args.output = output;

    return manager->methodCallback(args);
}

END_NAMESPACE_OPENDAQ_OPCUA
