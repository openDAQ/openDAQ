#include <opcuaserver/opcuaserver.h>
#include <opcuaserver/opcuatmstypes.h>
#include <open62541/plugin/accesscontrol_default.h>
#include <open62541/plugin/nodestore_default.h>
#include <open62541/server_config_default.h>
#include <open62541/plugin/log_stdout.h>
#include <cassert>

BEGIN_NAMESPACE_OPENDAQ_OPCUA

OpcUaServer::OpcUaServer()
    : eventManager(std::make_shared<ServerEventManager>(this))
{
    setPort(OPCUA_DEFAULT_PORT);
    createSessionContextCallback = [this](const OpcUaNodeId& sessionId) { return createSessionContextCallbackImp(sessionId); };
    deleteSessionContextCallback = [this](void* context) { deleteSessionContextCallbackImp(context); };
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

void OpcUaServer::setSecurityConfig(OpcUaServerSecurityConfig* config)
{
    if (config == nullptr)
        this->securityConfig.reset();
    else
        this->securityConfig = *config;
}

const OpcUaServerSecurityConfig* OpcUaServer::getSecurityConfig() const
{
    return this->securityConfig.has_value() ? &this->securityConfig.value() : nullptr;
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

        std::lock_guard<std::mutex> guard(serverMappingMutex);
        serverMapping[server] = this;
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

    if (!securityConfig.has_value())
        prepareServerMinimal(config);
    else
        prepareServerSecured(config);

    addTmsTypes(server);
    activateSession_default = config->accessControl.activateSession;
    config->accessControl.activateSession = activateSession;
    config->accessControl.closeSession = closeSession;
    eventManager->registerEvents();
}

void OpcUaServer::prepareServerMinimal(UA_ServerConfig* config)
{
    UA_StatusCode retval = UA_ServerConfig_setMinimal(config, getPort(), nullptr);
    CheckStatusCodeException(retval, "Failed to configure server minimal.");
}

void OpcUaServer::prepareServerSecured(UA_ServerConfig* config)
{
    securityConfig->validate();

    if (!securityConfig->hasCertificate())
        prepareServerMinimal(config);
    else
        prepareEncryption(config);

    configureAppUri(config);
    prepareAccessControl(config);
}

void OpcUaServer::prepareEncryption(UA_ServerConfig* config)
{
#ifdef OPCUA_ENABLE_ENCRYPTION
    UA_StatusCode retval =
        UA_ServerConfig_setDefaultWithSecurityPolicies(config,
                                                       portUsed,
                                                       &securityConfig->certificate.getValue(),
                                                       &securityConfig->privateKey.getValue(),
                                                       (securityConfig->trustAll) ? NULL : securityConfig->trustList.data(),
                                                       (securityConfig->trustAll) ? 0 : securityConfig->trustList.size(),
                                                       NULL,
                                                       0,
                                                       securityConfig->revocationList.data(),
                                                       securityConfig->revocationList.size());

    if (!securityConfig->trustAll && securityConfig->trustList.size() == 0)
        config->certificateVerification.verifyCertificate = OpcUaSecurityCommon::verifyCertificateRejectAll;

    if (retval != UA_STATUSCODE_GOOD)
        throw OpcUaException(retval, "Failed to configure security policies.");
#else
    throw OpcUaException(UA_STATUSCODE_BADINTERNALERROR, "Encryption was not enabled when building the project.");
#endif
}

void OpcUaServer::prepareAccessControl(UA_ServerConfig* config)
{
    config->accessControl.clear(&config->accessControl);
    UA_StatusCode retval =
        UA_AccessControl_default(config, true, NULL, &config->securityPolicies[config->securityPoliciesSize - 1].policyUri, 0, NULL);

    if (retval != UA_STATUSCODE_GOOD)
        throw OpcUaException(retval, "Failed to configure server access control.");
}

void OpcUaServer::configureAppUri(UA_ServerConfig* config)
{
    std::optional<std::string> appUri = securityConfig->getAppUriOrParseFromCertificate();
    if (appUri.has_value())
    {
        UA_String_clear(&config->applicationDescription.applicationUri);
        config->applicationDescription.applicationUri = UA_STRING_ALLOC(appUri.value().c_str());
    }
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
        std::lock_guard<std::mutex> guard(serverMappingMutex);
        serverMapping.erase(server);
        server = nullptr;
    }

    assert(sessionContext.size() == 0);
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

std::mutex OpcUaServer::serverMappingMutex;
std::map<UA_Server*, OpcUaServer*> OpcUaServer::serverMapping;

OpcUaServer* OpcUaServer::getServer(UA_Server* server)
{
    std::lock_guard<std::mutex> guard(serverMappingMutex);
    auto it = serverMapping.find(server);
    if (it != serverMapping.end())
        return (*it).second;
    return nullptr;
}

UA_StatusCode OpcUaServer::authenticateUser(OpcUaServer* serverInstance, const UA_ExtensionObject* userIdentityToken)
{
    UA_StatusCode status = UA_STATUSCODE_BADUSERACCESSDENIED;

    if (userIdentityToken->content.decoded.type == &UA_TYPES[UA_TYPES_ANONYMOUSIDENTITYTOKEN])
        status = serverInstance->securityConfig->authenticateUser(true, "", "");
    else if (userIdentityToken->content.decoded.type == &UA_TYPES[UA_TYPES_USERNAMEIDENTITYTOKEN])
    {
        const UA_UserNameIdentityToken* userToken = (UA_UserNameIdentityToken*) userIdentityToken->content.decoded.data;
        std::string username = utils::ToStdString(userToken->userName);
        std::string password = utils::ToStdString(userToken->password);
        status = serverInstance->securityConfig->authenticateUser(false, username, password);
    }

    return status;
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
    OpcUaSecurityConfig* securityConfig = serverInstance->securityConfig.has_value() ? &serverInstance->securityConfig.value() : nullptr;

    if (securityConfig != nullptr && securityConfig->securityMode != endpointDescription->securityMode)
        return UA_STATUSCODE_BADSECURITYMODEREJECTED;

    void* unusedSessionContext = nullptr;  // activateSession_default resets sessionContext. Subsequent calls should keep the session
                                           // context
    UA_StatusCode status = serverInstance->activateSession_default(
        server, ac, endpointDescription, secureChannelRemoteCertificate, sessionId, userIdentityToken, &unusedSessionContext);
    assert(unusedSessionContext == nullptr);

    if (securityConfig != nullptr && (status == UA_STATUSCODE_GOOD || status == UA_STATUSCODE_BADUSERACCESSDENIED))
        status = authenticateUser(serverInstance, userIdentityToken);

    if (status != UA_STATUSCODE_GOOD)
        return status;

    bool sessionContextAlreadyCreated = *sessionContext != nullptr;
    if (serverInstance->createSessionContextCallback && !sessionContextAlreadyCreated)
    {
        OpcUaNodeId sessionNodeId(*sessionId, true);
        *sessionContext = serverInstance->createSessionContextCallback(sessionNodeId);
        if (*sessionContext != nullptr)
            serverInstance->sessionContext.insert(*sessionContext);
    }

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
