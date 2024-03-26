/*
 * Copyright 2022-2024 Blueberry d.o.o.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#pragma once

#include <opcuashared/opcuanodeid.h>
#include <open62541/server.h>
#include "opcuaserver/opcuaserver.h"
#include "opcuaserver/server_event_manager.h"

BEGIN_NAMESPACE_OPENDAQ_OPCUA

class NodeEventManager;
using NodeEventManagerPtr = std::shared_ptr<NodeEventManager>;

struct NodeValueCallbackArgs;
using NodeValueCallback = std::function<void(NodeValueCallbackArgs args)>;

class NodeEventManager
{
public:
    struct ReadArgs;
    struct WriteArgs;
    struct DataSourceReadArgs;
    struct DataSourceWriteArgs;
    struct MethodArgs;

    using ReadCallback = std::function<UA_StatusCode(ReadArgs args)>;
    using WriteCallback = std::function<UA_StatusCode(WriteArgs args)>;
    using DataSourceReadCallback = std::function<UA_StatusCode(DataSourceReadArgs args)>;
    using DataSourceWriteCallback = std::function<UA_StatusCode(DataSourceWriteArgs args)>;
    using MethodCallback = std::function<UA_StatusCode(MethodArgs args)>;

    NodeEventManager(const OpcUaNodeId& nodeId, OpcUaServerPtr& server);

    void onRead(ReadCallback callback);
    void onWrite(WriteCallback callback);
    void onDataSourceRead(DataSourceReadCallback callback);
    void onDataSourceWrite(DataSourceWriteCallback callback);
    void onMethodCall(MethodCallback callback);
    void onDisplayNameChanged(DisplayNameChangedCallback callback);
    void onDescriptionChanged(DescriptionChangedCallback callback);

protected:
    OpcUaNodeId nodeId;
    OpcUaServerPtr server;

    WriteCallback writeCallback;
    ReadCallback readCallback;
    DataSourceWriteCallback dataSourceWriteCallback;
    DataSourceReadCallback dataSourceReadCallback;
    MethodCallback methodCallback;

private:
    static void OnWrite(UA_Server* server,
                        const UA_NodeId* sessionId,
                        void* sessionContext,
                        const UA_NodeId* nodeId,
                        void* nodeContext,
                        const UA_NumericRange* range,
                        const UA_DataValue* value);

    static void OnRead(UA_Server* server,
                       const UA_NodeId* sessionId,
                       void* sessionContext,
                       const UA_NodeId* nodeid,
                       void* nodeContext,
                       const UA_NumericRange* range,
                       const UA_DataValue* value);

    static UA_StatusCode OnDataSourceRead(UA_Server* server,
                                          const UA_NodeId* sessionId,
                                          void* sessionContext,
                                          const UA_NodeId* nodeId,
                                          void* nodeContext,
                                          UA_Boolean includeSourceTimeStamp,
                                          const UA_NumericRange* range,
                                          UA_DataValue* value);

    static UA_StatusCode OnDataSourceWrite(UA_Server* server,
                                           const UA_NodeId* sessionId,
                                           void* sessionContext,
                                           const UA_NodeId* nodeId,
                                           void* nodeContext,
                                           const UA_NumericRange* range,
                                           const UA_DataValue* value);

    static UA_StatusCode OnMethod(UA_Server* server,
                                  const UA_NodeId* sessionId,
                                  void* sessionContext,
                                  const UA_NodeId* methodId,
                                  void* methodContext,
                                  const UA_NodeId* objectId,
                                  void* objectContext,
                                  size_t inputSize,
                                  const UA_Variant* input,
                                  size_t outputSize,
                                  UA_Variant* output);
};

struct NodeEventManager::ReadArgs
{
    UA_Server* server;
    const UA_NodeId* sessionId;
    void* sessionContext;
    const UA_NodeId* nodeId;
    void* nodeContext;
    const UA_NumericRange* range;
    const UA_DataValue* value;
};

struct NodeEventManager::WriteArgs
{
    UA_Server* server;
    const UA_NodeId* sessionId;
    void* sessionContext;
    const UA_NodeId* nodeId;
    void* nodeContext;
    const UA_NumericRange* range;
    const UA_DataValue* value;
};

struct NodeEventManager::DataSourceReadArgs
{
    UA_Server* server;
    const UA_NodeId* sessionId;
    void* sessionContext;
    const UA_NodeId* nodeId;
    void* nodeContext;
    UA_Boolean includeSourceTimeStamp;
    const UA_NumericRange* range;
    UA_DataValue* value;
};

struct NodeEventManager::DataSourceWriteArgs
{
    UA_Server* server;
    const UA_NodeId* sessionId;
    void* sessionContext;
    const UA_NodeId* nodeId;
    void* nodeContext;
    const UA_NumericRange* range;
    const UA_DataValue* value;
};

struct NodeEventManager::MethodArgs
{
    UA_Server* server;
    const UA_NodeId* sessionId;
    void* sessionContext;
    const UA_NodeId* methodId;
    void* methodContext;
    const UA_NodeId* objectId;
    void* objectContext;
    size_t inputSize;
    const UA_Variant* input;
    size_t outputSize;
    UA_Variant* output;
};

END_NAMESPACE_OPENDAQ_OPCUA
