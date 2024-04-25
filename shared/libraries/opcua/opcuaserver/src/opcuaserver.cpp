#include <opcuaserver/opcuaserver.h>
#include <opcuaserver/opcuatmstypes.h>
#include <open62541/plugin/accesscontrol_default.h>
#include <open62541/plugin/nodestore_default.h>
#include <open62541/server_config_default.h>
#include <open62541/plugin/log_stdout.h>
#include <cassert>
#include <coreobjects/authentication_provider_factory.h>
#include <coreobjects/exceptions.h>

BEGIN_NAMESPACE_OPENDAQ_OPCUA

OpcUaServer::OpcUaServer()
    : eventManager(std::make_shared<ServerEventManager>(this))
{
    setPort(OPCUA_DEFAULT_PORT);
    createSessionContextCallback = [this](const OpcUaNodeId& sessionId) { return createSessionContextCallbackImp(sessionId); };
    deleteSessionContextCallback = [this](void* context) { deleteSessionContextCallbackImp(context); };
    authenticationProvider = AuthenticationProvider();
}

void* OpcUaServer::createSessionContextCallbackImp(const OpcUaNodeId& sessionId)
{
    return new OpcUaSession(sessionId, &serverLock);
}

void OpcUaServer::deleteSessionContextCallbackImp(void* context)
{
    delete static_cast<OpcUaSession*>(context);
}

std::unordered_set<void*>& OpcUaServer::getSessions()
{
    return sessionContext;
}

OpcUaServer::~OpcUaServer()
{
    OpcUaServer::stop();
}

uint16_t& OpcUaServer::getPort()
{
    return port;
}

void OpcUaServer::setPort(uint16_t port)
{
    this->port = port;
}

void OpcUaServer::setAuthenticationProvider(const AuthenticationProviderPtr& authenticationProvider)
{
    this->authenticationProvider = authenticationProvider;
}

void OpcUaServer::setSecurityConfig(OpcUaServerSecurityConfig* config)
{
    throw std::exception("method setSecurityConfig() is deprecated");
}

const OpcUaServerSecurityConfig* OpcUaServer::getSecurityConfig() const
{
    throw std::exception("method getSecurityConfig() is deprecated");
}

void OpcUaServer::start()
{
    if (getStarted())
        throw OpcUaException(UA_STATUSCODE_BADINVALIDSTATE, "Thread is already started.");

    if (!isPrepared())
        prepare();

    UA_StatusCode retval = UA_Server_run_startup(server);
    CheckStatusCodeException(retval, "Failed to start server");

    ThreadEx::start();
}

void OpcUaServer::stop()
{
    ThreadEx::stop();
    shutdownServer();
}

void OpcUaServer::prepare()
{
    try
    {
        if (getStarted())
            throw OpcUaException(UA_STATUSCODE_BADINVALIDSTATE, "Server is running");

        if (isPrepared())
            shutdownServer();

        prepareServer();
    }
    catch (const OpcUaException&)
    {
        shutdownServer();
        throw;
    }
}

bool OpcUaServer::isPrepared()
{
    return server != nullptr;
}

UA_Server* OpcUaServer::createServer()
{
    UA_ServerConfig config;
    memset(&config, 0, sizeof(UA_ServerConfig));
    config.logger = UA_Log_Stdout_withLevel(UA_LOGLEVEL_WARNING);
    UA_Nodestore_HashMap(&config.nodestore);
    return UA_Server_newWithConfig(&config);
}

void OpcUaServer::prepareServer()
{
    server = createServer();
    UA_ServerConfig* config = UA_Server_getConfig(server);

    prepareServerMinimal(config);
    config->context = this;
    config->nodeLifecycle.generateChildNodeId = generateChildId;

    prepareAccessControl(config);
    addTmsTypes(server);

    eventManager->registerEvents();
}

void OpcUaServer::prepareServerMinimal(UA_ServerConfig* config)
{
    UA_StatusCode retval = UA_ServerConfig_setMinimal(config, getPort(), nullptr);
    CheckStatusCodeException(retval, "Failed to configure server minimal.");
}

void OpcUaServer::prepareAccessControl(UA_ServerConfig* config)
{
    config->accessControl.clear(&config->accessControl);

    auto status =
        UA_AccessControl_default(config, false, NULL, &config->securityPolicies[config->securityPoliciesSize - 1].policyUri, 0, nullptr);

    CheckStatusCodeException(status, "Failed to configure access control.");

    activateSession_default = config->accessControl.activateSession;
    config->accessControl.activateSession = activateSession;
    config->accessControl.closeSession = closeSession;
}

void OpcUaServer::shutdownServer()
{
    if (getStarted())
    {
        UA_StatusCode status = UA_Server_run_shutdown(server);
        CheckStatusCodeException(status);
    }

    if (isPrepared())
    {
        UA_Server_delete(server);
        server = nullptr;
    }

    assert(sessionContext.size() == 0);
}

UA_StatusCode OpcUaServer::validateIdentityToken(const UA_ExtensionObject* token)
{
    if (token->content.decoded.type == &UA_TYPES[UA_TYPES_USERNAMEIDENTITYTOKEN])
    {
        if (isUsernameIdentityTokenValid((UA_UserNameIdentityToken*) token->content.decoded.data))
            return UA_STATUSCODE_GOOD;
    }
    else if (token->content.decoded.type == &UA_TYPES[UA_TYPES_ANONYMOUSIDENTITYTOKEN])
    {
        if (isAnonymousIdentityTokenValid((UA_AnonymousIdentityToken*) token->content.decoded.data))
            return UA_STATUSCODE_GOOD;
    }
    else
    {
        return UA_STATUSCODE_BADIDENTITYTOKENINVALID;
    }

    return UA_STATUSCODE_BADUSERACCESSDENIED;
}

bool OpcUaServer::isUsernameIdentityTokenValid(const UA_UserNameIdentityToken* token)
{
    const auto username = utils::ToStdString(token->userName);
    const auto password = utils::ToStdString(token->password);

    try
    {
        authenticationProvider.authenticate(username, password);
    }
    catch (const DaqException&)
    {
        return false;
    }

    return true;
}

bool OpcUaServer::isAnonymousIdentityTokenValid(const UA_AnonymousIdentityToken* /*token*/)
{
    return authenticationProvider.isAnonymousAllowed();
}

void OpcUaServer::createSession(const OpcUaNodeId& sessionId, void** sessionContext)
{
    bool sessionContextAlreadyCreated = *sessionContext != nullptr;

    if (createSessionContextCallback && !sessionContextAlreadyCreated)
    {
        *sessionContext = createSessionContextCallback(sessionId);
        if (*sessionContext != nullptr)
            this->sessionContext.insert(*sessionContext);
    }
}

void OpcUaServer::execute()
{
    setThreadName("OpcUaServer");
    while (!terminated)
    {
        UA_Server_run_iterate(server, true);
    }
    shutdownServer();
}

OpcUaServerNode OpcUaServer::getNode(const OpcUaNodeId& nodeId)
{
    return OpcUaServerNode(*this, nodeId);
}
OpcUaServerObjectNode OpcUaServer::getRootNode()
{
    return OpcUaServerObjectNode(*this, OpcUaNodeId(UA_NS0ID_ROOTFOLDER));
}
OpcUaServerObjectNode OpcUaServer::getObjectsNode()
{
    return OpcUaServerObjectNode(*this, OpcUaNodeId(UA_NS0ID_OBJECTSFOLDER));
}
OpcUaServerObjectNode OpcUaServer::getTypesNode()
{
    return OpcUaServerObjectNode(*this, OpcUaNodeId(UA_NS0ID_TYPESFOLDER));
}
OpcUaServerObjectNode OpcUaServer::getViewsNode()
{
    return OpcUaServerObjectNode(*this, OpcUaNodeId(UA_NS0ID_VIEWSFOLDER));
}
OpcUaServerObjectNode OpcUaServer::getObjectTypesNode()
{
    return OpcUaServerObjectNode(*this, OpcUaNodeId(UA_NS0ID_OBJECTTYPESFOLDER));
}
OpcUaServerObjectNode OpcUaServer::getVariableTypesNode()
{
    return OpcUaServerObjectNode(*this, OpcUaNodeId(UA_NS0ID_VARIABLETYPESFOLDER));
}
OpcUaServerObjectNode OpcUaServer::getDataTypesNode()
{
    return OpcUaServerObjectNode(*this, OpcUaNodeId(UA_NS0ID_DATATYPESFOLDER));
}
OpcUaServerObjectNode OpcUaServer::getReferenceTypesNode()
{
    return OpcUaServerObjectNode(*this, OpcUaNodeId(UA_NS0ID_REFERENCETYPESFOLDER));
}

ServerEventManagerPtr OpcUaServer::getEventManager()
{
    return eventManager;
}

bool OpcUaServer::nodeExists(const OpcUaNodeId& nodeId)
{
    return nodeExists(*nodeId);
}

bool OpcUaServer::nodeExists(const UA_NodeId& nodeId)
{
    UA_NodeClass nodeClass;
    return UA_Server_readNodeClass(server, nodeId, &nodeClass) == UA_STATUSCODE_GOOD;
}

OpcUaObject<UA_BrowseResult> OpcUaServer::browse(const OpcUaObject<UA_BrowseDescription>& browseDescription)
{
    const size_t MAX_REFERENCES = 0;  // All
    return UA_Server_browse(server, MAX_REFERENCES, browseDescription.get());
}

OpcUaNodeId OpcUaServer::addObjectNode(const AddObjectNodeParams& params)
{
    auto lock = std::lock_guard(getLock());
    eventManager->onCreateOptionalNode(params.addOptionalNodeCallback);
    UA_NodeId outNodeId;

    auto status = UA_Server_addObjectNode(server,
                                          *params.requestedNewNodeId,
                                          *params.parentNodeId,
                                          *params.referenceTypeId,
                                          *params.browseName,
                                          *params.typeDefinition,
                                          *params.attr,
                                          params.nodeContext,
                                          &outNodeId);

    CheckStatusCodeException(status);

    return outNodeId;
}

OpcUaNodeId OpcUaServer::addVariableNode(const AddVariableNodeParams& params)
{
    auto lock = std::lock_guard(getLock());
    eventManager->onCreateOptionalNode(params.addOptionalNodeCallback);
    UA_NodeId outNodeId;

    auto status = UA_Server_addVariableNode(server,
                                            *params.requestedNewNodeId,
                                            *params.parentNodeId,
                                            *params.referenceTypeId,
                                            *params.browseName,
                                            *params.typeDefinition,
                                            *params.attr,
                                            params.nodeContext,
                                            &outNodeId);

    CheckStatusCodeException(status);
    return outNodeId;
}

OpcUaNodeId OpcUaServer::addMethodNode(const AddMethodNodeParams& params)
{
    UA_NodeId outNodeId;
    CheckStatusCodeException(UA_Server_addMethodNode(server,
                                                     *params.requestedNewNodeId,
                                                     *params.parentNodeId,
                                                     *params.referenceTypeId,
                                                     *params.browseName,
                                                     *params.attr,
                                                     params.method,
                                                     params.inputArgumentsSize,
                                                     params.inputArguments,
                                                     params.outputArgumentsSize,
                                                     params.outputArguments,
                                                     params.nodeContext,
                                                     &outNodeId));

    return outNodeId;
}

OpcUaNodeId OpcUaServer::addObjectTypeNode(const AddObjectTypeNodeParams& params)
{
    UA_NodeId outNodeId;
    CheckStatusCodeException(UA_Server_addObjectTypeNode(server,
                                                         *params.requestedNewNodeId,
                                                         *params.parentNodeId,
                                                         *params.referenceTypeId,
                                                         *params.browseName,
                                                         *params.attr,
                                                         params.nodeContext,
                                                         &outNodeId));

    return outNodeId;
}

OpcUaNodeId OpcUaServer::addVariableTypeNode(const AddVariableTypeNodeParams& params)
{
    UA_NodeId outNodeId;
    CheckStatusCodeException(UA_Server_addVariableTypeNode(server,
                                                           *params.requestedNewNodeId,
                                                           *params.parentNodeId,
                                                           *params.referenceTypeId,
                                                           *params.browseName,
                                                           *params.typeDefinition,
                                                           *params.attr,
                                                           params.nodeContext,
                                                           &outNodeId));

    return outNodeId;
}

void OpcUaServer::triggerEvent(const OpcUaNodeId& eventType, const OpcUaNodeId& originNodeId, const EventAttributes& eventAttributes)
{
    OpcUaNodeId eventNodeId;
    CheckStatusCodeException(UA_Server_createEvent(server, *eventType, eventNodeId.get()), "createEvent failed");

    const auto& attributes = eventAttributes.getAttributes();
    for (const auto& attribute : attributes)
    {
        const OpcUaObject<UA_QualifiedName>& propertyName = attribute.first;
        const OpcUaObject<UA_Variant>& value = attribute.second;
        CheckStatusCodeException(UA_Server_writeObjectProperty(server, *eventNodeId, *propertyName, *value),
                                 "setting event attribute fails");
    }

    CheckStatusCodeException(UA_Server_triggerEvent(server, *eventNodeId, *originNodeId, NULL, UA_TRUE), "triggerEvent failed");
}

OpcUaObject<UA_QualifiedName> OpcUaServer::readBrowseName(const OpcUaNodeId& nodeId) const
{
    OpcUaObject<UA_QualifiedName> qualifiedName;
    CheckStatusCodeException(UA_Server_readBrowseName(server, *nodeId, qualifiedName.get()));
    return qualifiedName;
}

std::string OpcUaServer::readBrowseNameString(const OpcUaNodeId& nodeId) const
{
    auto browseName = readBrowseName(nodeId);
    return utils::ToStdString(browseName->name);
}

OpcUaNodeClass OpcUaServer::readNodeClass(const OpcUaNodeId& nodeId) const
{
    UA_NodeClass nodeClass;
    CheckStatusCodeException(UA_Server_readNodeClass(server, *nodeId, &nodeClass));
    return static_cast<OpcUaNodeClass>(nodeClass);
}

void OpcUaServer::setDisplayName(const OpcUaNodeId& nodeId, const OpcUaObject<UA_LocalizedText>& localizedText)
{
    const auto status = UA_Server_writeDisplayName(server, *nodeId, *localizedText);
    CheckStatusCodeException(status);
}

void OpcUaServer::setDisplayName(const OpcUaNodeId& nodeId, const std::string& text)
{
    OpcUaObject<UA_LocalizedText> localizedText = UA_LOCALIZEDTEXT_ALLOC("", text.c_str());
    setDisplayName(nodeId, localizedText);
}

OpcUaObject<UA_LocalizedText> OpcUaServer::readDisplayName(const OpcUaNodeId& nodeId) const
{
    OpcUaObject<UA_LocalizedText> localizedText;
    CheckStatusCodeException(UA_Server_readDisplayName(server, *nodeId, localizedText.get()));
    return localizedText;
}

void OpcUaServer::setDescription(const OpcUaNodeId& nodeId, const OpcUaObject<UA_LocalizedText>& localizedText)
{
    const auto status = UA_Server_writeDescription(server, *nodeId, *localizedText);
    CheckStatusCodeException(status);
}

void OpcUaServer::setDescription(const OpcUaNodeId& nodeId, const std::string& text)
{
    OpcUaObject<UA_LocalizedText> localizedText = UA_LOCALIZEDTEXT_ALLOC("", text.c_str());
    setDescription(nodeId, localizedText);
}

void OpcUaServer::writeValue(const OpcUaNodeId& nodeId, const OpcUaVariant& value)
{
    CheckStatusCodeException(UA_Server_writeValue(server, *nodeId, *value));
}

OpcUaVariant OpcUaServer::readValue(const OpcUaNodeId& nodeId)
{
    OpcUaVariant value;
    CheckStatusCodeException(UA_Server_readValue(server, *nodeId, value.get()));
    return value;
}

OpcUaNodeId OpcUaServer::readDataType(const OpcUaNodeId& typeNodeId)
{
    OpcUaNodeId dataTypeId;
    const auto status = UA_Server_readDataType(getUaServer(), *typeNodeId, dataTypeId.get());
    CheckStatusCodeException(status);
    return dataTypeId;
}

void OpcUaServer::deleteNode(const OpcUaNode& node)
{
    deleteNode(node.getNodeId());
}

void OpcUaServer::deleteNode(const OpcUaNodeId& nodeId)
{
    UA_StatusCode status = UA_Server_deleteNode(server, *nodeId, true);
    CheckStatusCodeException(status);
}

void OpcUaServer::addReference(const OpcUaNodeId& sourceId, const OpcUaNodeId& refTypeId, const OpcUaNodeId& targetId, bool isForward)
{
    OpcUaObject<UA_ExpandedNodeId> extendedTargetId;
    extendedTargetId->nodeId = targetId.copyAndGetDetachedValue();

    UA_StatusCode status = UA_Server_addReference(server, *sourceId, *refTypeId, *extendedTargetId, isForward);
    CheckStatusCodeException(status);
}

void OpcUaServer::deleteReference(const OpcUaNodeId& sourceId, const OpcUaNodeId& refTypeId, const OpcUaNodeId& targetId, bool isForward)
{
    OpcUaObject<UA_ExpandedNodeId> extendedTargetId;
    extendedTargetId->nodeId = targetId.copyAndGetDetachedValue();

    UA_StatusCode status = UA_Server_deleteReference(server, *sourceId, *refTypeId, isForward, *extendedTargetId, true);
    CheckStatusCodeException(status);
}

bool OpcUaServer::referenceExists(const OpcUaNodeId& sourceId, const OpcUaNodeId& refTypeId, const OpcUaNodeId& targetId, bool isForward)
{
    OpcUaObject<UA_BrowseDescription> browseDesc;
    browseDesc->browseDirection = UA_BROWSEDIRECTION_BOTH;
    browseDesc->nodeClassMask = UA_NODECLASS_OBJECT | UA_NODECLASS_VARIABLE | UA_NODECLASS_METHOD;
    browseDesc->includeSubtypes = false;
    browseDesc->nodeId = sourceId.copyAndGetDetachedValue();
    browseDesc->referenceTypeId = refTypeId.copyAndGetDetachedValue();
    browseDesc->resultMask = UA_BROWSERESULTMASK_ISFORWARD;

    OpcUaObject<UA_BrowseResult> browseResult = UA_Server_browse(server, 0, browseDesc.get());

    UA_NodeId_init(&browseDesc->nodeId);
    UA_NodeId_init(&browseDesc->referenceTypeId);

    for (size_t i = 0; i < browseResult->referencesSize; i++)
    {
        const auto& referenceDesc = browseResult->references[i];
        if (referenceDesc.isForward == isForward && targetId == referenceDesc.nodeId.nodeId)
            return true;
    }
    return false;
}

OpcUaServer* OpcUaServer::getServer(UA_Server* server)
{
    auto config = UA_Server_getConfig(server);
    assert(config != nullptr && config->context != nullptr);
    return (OpcUaServer*)  config->context;
}

UA_StatusCode OpcUaServer::activateSession(UA_Server* server,
                                           UA_AccessControl* ac,
                                           const UA_EndpointDescription* endpointDescription,
                                           const UA_ByteString* secureChannelRemoteCertificate,
                                           const UA_NodeId* sessionId,
                                           const UA_ExtensionObject* userIdentityToken,
                                           void** sessionContext)
{
    OpcUaServer* serverInstance = getServer(server);

    // activateSession_default resets sessionContext. Subsequent calls should keep the context
    void* unusedSessionContext = nullptr;
    UA_StatusCode status = serverInstance->activateSession_default(
        server, ac, endpointDescription, secureChannelRemoteCertificate, sessionId, userIdentityToken, &unusedSessionContext);
    assert(unusedSessionContext == nullptr);

    switch (status)
    {
        case UA_STATUSCODE_GOOD:
        case UA_STATUSCODE_BADUSERACCESSDENIED:
        case UA_STATUSCODE_BADIDENTITYTOKENINVALID:
        {
            status = serverInstance->validateIdentityToken(userIdentityToken);
            break;
        }
    }

    if (status == UA_STATUSCODE_GOOD)
        serverInstance->createSession(*sessionId, sessionContext);

    return status;
}

void OpcUaServer::closeSession(UA_Server* server, UA_AccessControl* ac, const UA_NodeId* sessionId, void* sessionContext)
{
    OpcUaNodeId sessionNodeId(*sessionId, true);
    OpcUaServer* serverInstance = getServer(server);

    serverInstance->serverLock.refuseConfigurationControlLock(sessionNodeId);

    if (sessionContext != nullptr)
        serverInstance->sessionContext.erase(sessionContext);

    if (serverInstance->deleteSessionContextCallback)
        serverInstance->deleteSessionContextCallback(sessionContext);
}


UA_StatusCode OpcUaServer::generateChildId(UA_Server* server,
                                           const UA_NodeId*/*sessionId*/,
                                           void*/*sessionContext*/,
                                           const UA_NodeId* sourceNodeId,
                                           const UA_NodeId* targetParentNodeId,
                                           const UA_NodeId*/*referenceTypeId*/,
                                           UA_NodeId* targetNodeId)
{
    if (targetParentNodeId->identifierType == UA_NODEIDTYPE_STRING) {
        const std::string parentNodeIdStr = utils::ToStdString(targetParentNodeId->identifier.string);
        std::string objectTypeIdStr;

        if (sourceNodeId->identifierType == UA_NODEIDTYPE_STRING)
        {
            objectTypeIdStr = utils::ToStdString(sourceNodeId->identifier.string);
        }
        else if (sourceNodeId->identifierType == UA_NODEIDTYPE_NUMERIC)
        {
            UA_QualifiedName* browseName = UA_QualifiedName_new();
            UA_Server_readBrowseName(server, *sourceNodeId, browseName);
            objectTypeIdStr = utils::ToStdString(browseName->name);
            UA_QualifiedName_delete(browseName);
        }
        else
        {
            return UA_STATUSCODE_GOOD;
        }

        const auto newNodeIdStr = parentNodeIdStr + "/" + objectTypeIdStr;
        *targetNodeId = UA_NODEID_STRING_ALLOC(targetParentNodeId->namespaceIndex, newNodeIdStr.c_str());
    }

    return UA_STATUSCODE_GOOD;
}


bool OpcUaServer::passwordLock(const std::string& password)
{
    return serverLock.passwordLock(password);
}

bool OpcUaServer::passwordUnlock(const std::string& password)
{
    return serverLock.passwordUnlock(password);
}

bool OpcUaServer::isPasswordLocked()
{
    return serverLock.isPasswordLocked();
}

UA_Server* OpcUaServer::getUaServer() const noexcept
{
    return server;
}

END_NAMESPACE_OPENDAQ_OPCUA
