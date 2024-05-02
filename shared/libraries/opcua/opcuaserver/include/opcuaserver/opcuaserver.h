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

#include <opendaq/utils/thread_ex.h>
#include <unordered_set>

#include <opcuashared/node/opcuanodeobject.h>
#include <opcuashared/opcuasecurity_config.h>

#include <opcuaserver/opcuaaddnodeparams.h>
#include <opcuaserver/event_attributes.h>
#include <opcuaserver/opcuaservernode.h>
#include <opcuaserver/opcuasession.h>
#include <opcuaserver/server_event_manager.h>
#include <opcuaserver/common.h>

#include <open62541/server.h>

BEGIN_NAMESPACE_OPENDAQ_OPCUA

class OpcUaServer final : public daq::utils::ThreadEx
{
public:
    OpcUaServer();
    ~OpcUaServer();

    static constexpr uint16_t OPCUA_DEFAULT_PORT = 4840;

    uint16_t& getPort();
    void setPort(uint16_t port);

    void setSecurityConfig(OpcUaServerSecurityConfig* config);
    const OpcUaServerSecurityConfig* getSecurityConfig() const;
    void prepare();
    bool isPrepared();

    void start() override;
    void stop() override;

    OpcUaServerNode getNode(const OpcUaNodeId& nodeId);
    OpcUaServerObjectNode getRootNode();
    OpcUaServerObjectNode getObjectsNode();
    OpcUaServerObjectNode getTypesNode();
    OpcUaServerObjectNode getViewsNode();
    OpcUaServerObjectNode getObjectTypesNode();
    OpcUaServerObjectNode getVariableTypesNode();
    OpcUaServerObjectNode getDataTypesNode();
    OpcUaServerObjectNode getReferenceTypesNode();
    ServerEventManagerPtr getEventManager();

    bool nodeExists(const OpcUaNodeId& nodeId);
    bool nodeExists(const UA_NodeId& nodeId);

    OpcUaObject<UA_BrowseResult> browse(const OpcUaObject<UA_BrowseDescription>& browseDescription);

    OpcUaNodeId addObjectNode(const AddObjectNodeParams& params);
    OpcUaNodeId addVariableNode(const AddVariableNodeParams& params);
    OpcUaNodeId addMethodNode(const AddMethodNodeParams& params);
    OpcUaNodeId addObjectTypeNode(const AddObjectTypeNodeParams& params);
    OpcUaNodeId addVariableTypeNode(const AddVariableTypeNodeParams& params);

    void triggerEvent(const OpcUaNodeId& eventType, const OpcUaNodeId& originNodeId, const EventAttributes& eventAttributes);

    void deleteNode(const OpcUaNode& node);
    void deleteNode(const OpcUaNodeId& nodeId);

    OpcUaNodeClass readNodeClass(const OpcUaNodeId& nodeId) const;
    OpcUaObject<UA_QualifiedName> readBrowseName(const OpcUaNodeId& nodeId) const;
    std::string readBrowseNameString(const OpcUaNodeId& nodeId) const;

    void setDisplayName(const OpcUaNodeId& nodeId, const OpcUaObject<UA_LocalizedText>& localizedText);
    void setDisplayName(const OpcUaNodeId& nodeId, const std::string& text);
    OpcUaObject<UA_LocalizedText> readDisplayName(const OpcUaNodeId& nodeId) const;

    void setDescription(const OpcUaNodeId& nodeId, const OpcUaObject<UA_LocalizedText>& localizedText);
    void setDescription(const OpcUaNodeId& nodeId, const std::string& text);

    void writeValue(const OpcUaNodeId& nodeId, const OpcUaVariant& var);
    OpcUaVariant readValue(const OpcUaNodeId& nodeId);
    OpcUaNodeId readDataType(const OpcUaNodeId& typeNodeId);

    void addReference(const OpcUaNodeId& sourceId, const OpcUaNodeId& refTypeId, const OpcUaNodeId& targetId, bool isForward = true);
    void deleteReference(const OpcUaNodeId& sourceId, const OpcUaNodeId& refTypeId, const OpcUaNodeId& targetId, bool isForward = true);
    bool referenceExists(const OpcUaNodeId& sourceId, const OpcUaNodeId& refTypeId, const OpcUaNodeId& targetId, bool isForward = true);

    // session context
    typedef std::function<void*(const OpcUaNodeId& sessionId)> CreateSessionContextCallbackType;
    CreateSessionContextCallbackType createSessionContextCallback;
    typedef std::function<void(void*)> DeleteSessionContextCallbackType;
    DeleteSessionContextCallbackType deleteSessionContextCallback;

    std::unordered_set<void*>& getSessions();  // use only in server thread

    void* createSessionContextCallbackImp(const OpcUaNodeId& sessionId);
    void deleteSessionContextCallbackImp(void* context);

    // TODO move locking to model
    bool passwordLock(const std::string& password);
    bool passwordUnlock(const std::string& password);
    bool isPasswordLocked();

    UA_Server* getUaServer() const noexcept;

protected:
    void execute() override;

private:
    UA_Server* createServer();
    void prepareServer();
    void prepareServerMinimal(UA_ServerConfig* config);
    void prepareServerSecured(UA_ServerConfig* config);
    void prepareEncryption(UA_ServerConfig* config);
    void prepareAccessControl(UA_ServerConfig* config);
    void configureAppUri(UA_ServerConfig* config);
    void shutdownServer();

    static UA_StatusCode activateSession(UA_Server* server,
                                         UA_AccessControl* ac,
                                         const UA_EndpointDescription* endpointDescription,
                                         const UA_ByteString* secureChannelRemoteCertificate,
                                         const UA_NodeId* sessionId,
                                         const UA_ExtensionObject* userIdentityToken,
                                         void** sessionContext);
    static void closeSession(UA_Server* server, UA_AccessControl* ac, const UA_NodeId* sessionId, void* sessionContext);
    static UA_StatusCode generateChildId(UA_Server *server, const UA_NodeId *sessionId, void *sessionContext, const UA_NodeId *sourceNodeId, const UA_NodeId *targetParentNodeId, const UA_NodeId *referenceTypeId, UA_NodeId *targetNodeId);
    static UA_StatusCode authenticateUser(OpcUaServer* serverInstance, const UA_ExtensionObject* userIdentityToken);

    // missing UA_Server void* member workaround...
    static OpcUaServer* getServer(UA_Server* server);

    UA_StatusCode (*activateSession_default)(UA_Server* server,
                                             UA_AccessControl* ac,
                                             const UA_EndpointDescription* endpointDescription,
                                             const UA_ByteString* secureChannelRemoteCertificate,
                                             const UA_NodeId* sessionId,
                                             const UA_ExtensionObject* userIdentityToken,
                                             void** sessionContext){};

    OpcUaServerLock serverLock;
    uint16_t port{OPCUA_DEFAULT_PORT};
    UA_Server* server{};
    std::optional<OpcUaServerSecurityConfig> securityConfig;
    std::unordered_set<void*> sessionContext;
    ServerEventManagerPtr eventManager;
    static std::mutex serverMappingMutex;
    static std::map<UA_Server*, OpcUaServer*> serverMapping;
};

END_NAMESPACE_OPENDAQ_OPCUA
