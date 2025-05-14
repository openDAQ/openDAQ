#include "opcuaclient/opcuaclient.h"
#include <opcuashared/opcuaobject.h>
#include <open62541/client_config_default.h>

#include <open62541/client.h>
#include <open62541/client_highlevel.h>
#include <open62541/client_subscriptions.h>
#include "opcuashared/opcuasecuritycommon.h"

#include <opcuashared/opcualog.h>

BEGIN_NAMESPACE_OPENDAQ_OPCUA

using namespace daq::utils;

// LockedClient
ClientLockGuard::ClientLockGuard(OpcUaClient* client) noexcept
    : client(client)
{
    if (client != nullptr)
        client->getLock().lock();
}

ClientLockGuard::ClientLockGuard(ClientLockGuard&& other) noexcept
{
    this->client = other.client;
    other.client = nullptr;
}

ClientLockGuard& ClientLockGuard::operator=(ClientLockGuard&& other) noexcept
{
    this->client = other.client;
    other.client = nullptr;
    return *this;
}

ClientLockGuard::~ClientLockGuard() noexcept
{
    if (client != nullptr)
        client->getLock().unlock();
}

ClientLockGuard::operator UA_Client*()
{
    return client != nullptr ? client->getUaClient() : nullptr;
}

// OpcUaClient
OpcUaClient::OpcUaClient(const OpcUaEndpoint& endpoint)
    : endpoint(endpoint)
    , timerTasks()
    , iterateThread("OpcUaClient")
{
    iterateThread.setCallback(std::bind(&OpcUaClient::executeIterateCallback, this));
    initialize();
}

OpcUaClient::OpcUaClient(const std::string& url)
    : OpcUaClient(OpcUaEndpoint(url))
{
}

OpcUaClient::~OpcUaClient()
{
    disconnect();
}

void OpcUaClient::initialize()
{
    std::lock_guard guard(getLock());

    uaclient = UaClientFactory::Create(nullptr, UA_LOGLEVEL_WARNING, endpoint.getCustomDataTypes());

    UA_Client_getConfig(uaclient)->clientContext = this;

    setTimeout(timeoutMs);

    setConnectivityCheckInterval(connectivityCheckInterval);
}

UaClientFactory::UaClientFactory()
{
}

UA_Client* UaClientFactory::Create(const OpcUaClientSecurityConfig* securityConfig,
                                   UA_LogLevel logLevel,
                                   const UA_DataTypeArray* customDataTypes)
{
    UaClientFactory factory;
    factory.setSecurityConfig(securityConfig);
    factory.setLogLevel(logLevel);
    factory.setCustomDataTypes(customDataTypes);
    return factory.build();
}

void UaClientFactory::setSecurityConfig(const OpcUaClientSecurityConfig* securityConfig)
{
    this->securityConfig = securityConfig;
}

void UaClientFactory::setLogLevel(const UA_LogLevel logLevel)
{
    this->logLevel = logLevel;
}

void UaClientFactory::setCustomDataTypes(const UA_DataTypeArray* customDataTypes)
{
    this->customDataTypes = customDataTypes;
}

void UaClientFactory::configureClient()
{
    // config->logger = UA_Log_Plog_withLevel(logLevel);

    if (securityConfig == NULL)
    {
        UA_StatusCode retval = UA_ClientConfig_setDefault(config);
        if (retval != UA_STATUSCODE_GOOD)
            throw OpcUaException(retval, "Failed to configure client defaults.");
    }
    else
        configureClientSecurity();

    config->customDataTypes = customDataTypes;

    configureClientConfigDefaults();
}

void UaClientFactory::configureClientSecurity()
{
    securityConfig->validate();

    if (!securityConfig->hasCertificate())
    {
        UA_StatusCode retval = UA_ClientConfig_setDefault(config);
        if (retval != UA_STATUSCODE_GOOD)
            throw OpcUaException(retval, "Failed to configure client defaults.");
    }
    else
    {
#ifdef OPCUA_ENABLE_ENCRYPTION
        UA_StatusCode retval = UA_ClientConfig_setDefaultEncryption(config,
                                                                    securityConfig->certificate.getValue(),
                                                                    securityConfig->privateKey.getValue(),
                                                                    (securityConfig->trustAll) ? NULL : securityConfig->trustList.data(),
                                                                    (securityConfig->trustAll) ? 0 : securityConfig->trustList.size(),
                                                                    securityConfig->revocationList.data(),
                                                                    securityConfig->revocationList.size());

        if (retval != UA_STATUSCODE_GOOD)
            throw OpcUaException(retval, "Failed to configure client encryption.");

        if (!securityConfig->trustAll && securityConfig->trustList.size() == 0)
            config->certificateVerification.verifyCertificate = OpcUaSecurityCommon::verifyCertificateRejectAll;

        config->securityMode = securityConfig->securityMode;
        configureClientAppUri();
#else
        throw OpcUaException(UA_STATUSCODE_BADINTERNALERROR, "Encryption was not enabled when building the project.");
#endif
    }
}

void UaClientFactory::configureClientAppUri()
{
    std::optional<std::string> appUri = securityConfig->getAppUriOrParseFromCertificate();
    if (appUri.has_value())
    {
        UA_String_clear(&config->clientDescription.applicationUri);
        config->clientDescription.applicationUri = UA_STRING_ALLOC(appUri.value().c_str());
    }
}

void UaClientFactory::configureClientConfigDefaults()
{
    config->timeout = OpcUaClient::CONNECTION_TIMEOUT_SECONDS * 1000;
    config->connectivityCheckInterval = OpcUaClient::CONNECTION_TIMEOUT_SECONDS;
}

UA_Client* UaClientFactory::build()
{
    client = UA_Client_new();
    config = UA_Client_getConfig(client);

    try
    {
        configureClient();
    }
    catch (const OpcUaException&)
    {
        UA_Client_delete(client);
        throw;
    }

    return client;
}

void OpcUaClient::connect()
{
    std::lock_guard guard(getLock());

    if (!uaclient)
        initialize();

    UA_StatusCode status = UA_STATUSCODE_GOOD;

    if (endpoint.isAnonymous())
        status = UA_Client_connect(uaclient, endpoint.getUrl().c_str());
    else
        status =
            UA_Client_connectUsername(uaclient, endpoint.getUrl().c_str(), endpoint.getUsername().c_str(), endpoint.getPassword().c_str());

    if (!OPCUA_STATUSCODE_SUCCEEDED(status))
        throw OpcUaException(status, "Failed to connect to OpcUa server");
}

void OpcUaClient::disconnect(bool doClear)
{
    stopIterate();
    std::lock_guard guard(getLock());

    if (!uaclient)
        return;

    UA_Client_disconnect(uaclient);

    if (doClear)
        clear();  // to clear all internal states
}

void OpcUaClient::clear()
{
    if (uaclient)
    {
        UA_Client_delete(uaclient);
        uaclient = nullptr;
    }
}

UA_Client* OpcUaClient::getUaClient()
{
    return uaclient;
}

ClientLockGuard OpcUaClient::getLockedUaClient()
{
    return ClientLockGuard(this);
}

bool OpcUaClient::isConnected()
{
    std::lock_guard guard(getLock());
    if (!uaclient)
        return false;

    UA_SecureChannelState channelState;
    UA_SessionState sessionState;
    UA_StatusCode connectStatus;
    UA_Client_getState(uaclient, &channelState, &sessionState, &connectStatus);

    return OPCUA_STATUSCODE_SUCCEEDED(connectStatus) && channelState == UA_SECURECHANNELSTATE_OPEN;
}

const OpcUaEndpoint& OpcUaClient::getEndpoint() const
{
    return endpoint;
}

void OpcUaClient::setTimeout(uint32_t timeoutMs)
{
    std::lock_guard guard(getLock());
    this->timeoutMs = timeoutMs;
    if (uaclient)
        UA_Client_getConfig(uaclient)->timeout = timeoutMs;
}

uint32_t OpcUaClient::getTimeout() const
{
    return this->timeoutMs;
}

void OpcUaClient::setConnectivityCheckInterval(uint32_t connectivityCheckInterval)
{
    std::lock_guard guard(getLock());
    this->connectivityCheckInterval = connectivityCheckInterval;
    if (uaclient)
    {
        auto* config = UA_Client_getConfig(uaclient);

        if (connectivityCheckInterval > config->secureChannelLifeTime)
            THROW_RUNTIME_ERROR("Connectivity check interval [" << connectivityCheckInterval << "] exceeds secure channel life time ["
                                                                << config->secureChannelLifeTime << "]");

        UA_Client_getConfig(uaclient)->connectivityCheckInterval = connectivityCheckInterval;
    }
}

std::recursive_mutex& OpcUaClient::getLock()
{
    return lock;
}

void OpcUaClient::runIterate(std::chrono::milliseconds period, std::chrono::milliseconds iterateTimeout)
{
    assert(period.count() >= 0);
    assert(iterateTimeout.count() >= 0);

    if (iterateThread.getStarted())
        throw std::runtime_error("Iterate already started.");

    iterateThread.setIntervalMs(period.count());
    this->iterateTimeout = iterateTimeout;
    iterateThread.start();
}

void OpcUaClient::stopIterate()
{
    iterateThread.stop();
}

void OpcUaClient::executeIterateCallback()
{
    auto statusCode = iterate(iterateTimeout);
    if (OPCUA_STATUSCODE_FAILED(statusCode))
        iterateThread.terminate();
}

UA_StatusCode OpcUaClient::iterate(std::chrono::milliseconds timeout)
{
    UA_Int32 timeoutMs = timeout.count();
    assert(timeoutMs >= 0);
    return UA_Client_run_iterate_timer_tasks(getLockedUaClient(), timeoutMs, true);
}

OpcUaCallbackIdent OpcUaClient::scheduleTimerTask(double intervalMs, const OpcUaTimerTaskType& task)
{
    OpcUaCallbackIdent callbackId;

    auto client = getLockedUaClient();

    void* context = timerTasks.createContext();

    UA_StatusCode status = UA_Client_addRepeatedCallback(client, OpcUaClient::timerTaskCallback, context, intervalMs, &callbackId);

    if (OPCUA_STATUSCODE_FAILED(status))
    {
        timerTasks.deleteContext(context);
        throw OpcUaException(status, "Failed to add repeated callback");
    }

    timerTasks.insertTimerTask(context, callbackId, task);

    return callbackId;
}

void OpcUaClient::removeTimerTask(OpcUaCallbackIdent ident)
{
    auto client = getLockedUaClient();
    UA_Client_removeCallback(client, ident);
    timerTasks.removeTimerTask(ident);
}

bool OpcUaClient::timerTaskExists(OpcUaCallbackIdent ident)
{
    std::lock_guard guard(lock);
    return timerTasks.timerTaskExists(ident);
}

void OpcUaClient::timerTaskCallback(UA_Client* client, void* data)
{
    OpcUaCallbackIdent callbackIdent;
    OpcUaTimerTaskType* task;
    TimerTaskContextCollection::getTaskExecData(data, &callbackIdent, &task);

    OpcUaClient* opcUaClient = static_cast<OpcUaClient*>(UA_Client_getConfig(client)->clientContext);
    TimerTaskControl control;
    try
    {
        (*task)(*opcUaClient, control);
    }
    catch (const std::exception& e)
    {
        LOGE << "Error occured during executing user task. " << e.what();
    }
    if (control.terminate)
        opcUaClient->removeTimerTask(callbackIdent);
}

OpcUaVariant OpcUaClient::readValue(const OpcUaNodeId& node)
{
    OpcUaVariant val;
    CheckStatusCodeException(UA_Client_readValueAttribute(getLockedUaClient(), *node, val.get()));

    return val;
}

OpcUaObject<UA_ReadResponse> OpcUaClient::readNodeAttributes(const OpcUaObject<UA_ReadRequest>& request)
{
    return UA_Client_Service_read(getLockedUaClient(), *request);
}

void OpcUaClient::readNodeAttributes(const std::vector<OpcUaReadValueIdWithCallback>& requests)
{
    size_t requestCnt = std::size(requests);
    if (requestCnt == 0)
        return;

    OpcUaObject<UA_ReadRequest> readRequest;
    CheckStatusCodeException(
        UA_Array_resize((void**) &readRequest->nodesToRead, &readRequest->nodesToReadSize, requestCnt, &UA_TYPES[UA_TYPES_READVALUEID]));

    for (size_t i = 0; i < requestCnt; i++)
        UA_ReadValueId_copy(requests[i].get(), &readRequest->nodesToRead[i]);

    readRequest->timestampsToReturn = UA_TimestampsToReturn::UA_TIMESTAMPSTORETURN_NEITHER;

    OpcUaObject<UA_ReadResponse> response = readNodeAttributes(readRequest);
    CheckStatusCodeException(response->responseHeader.serviceResult);

    for (size_t i = 0; i < requestCnt; i++)
    {
        UA_DataValue* v = &response->results[i];
        requests[i].processFunction(v);
    }
}

OpcUaObject<UA_CallResponse> OpcUaClient::callMethods(const OpcUaObject<UA_CallRequest>& request)
{
    return UA_Client_Service_call(getLockedUaClient(), *request);
}

UA_NodeClass OpcUaClient::readNodeClass(const OpcUaNodeId& nodeId)
{
    UA_NodeClass nodeClass;
    CheckStatusCodeException(UA_Client_readNodeClassAttribute(getLockedUaClient(), *nodeId, &nodeClass));
    return nodeClass;
}

bool OpcUaClient::nodeExists(const OpcUaNodeId& node)
{
    UA_NodeClass nodeClass;
    auto status = UA_Client_readNodeClassAttribute(getLockedUaClient(), *node, &nodeClass);
    if (status == UA_STATUSCODE_BADNODEIDUNKNOWN)
        return false;

    CheckStatusCodeException(status);

    return true;
}

std::string OpcUaClient::readBrowseName(const OpcUaNodeId& nodeId)
{
    OpcUaObject<UA_QualifiedName> browseName;
    CheckStatusCodeException(UA_Client_readBrowseNameAttribute(getLockedUaClient(), *nodeId, browseName.get()));
    
    return OpcUaNode::GetBrowseName(*browseName);
}

std::string OpcUaClient::readDisplayName(const OpcUaNodeId& nodeId)
{
    OpcUaObject<UA_LocalizedText> displayName;
    CheckStatusCodeException(UA_Client_readDisplayNameAttribute(getLockedUaClient(), *nodeId, displayName.get()));

    return utils::ToStdString(displayName->text);
}

size_t OpcUaClient::readDimension(const OpcUaNodeId& nodeId)
{
    OpcUaObject<UA_Variant> uaVar;
    CheckStatusCodeException(UA_Client_readValueAttribute(getLockedUaClient(), *nodeId, uaVar.get()));

    if (uaVar->arrayLength > 0)
        return uaVar->arrayLength;
    else
        return 1;
}

void OpcUaClient::writeDisplayName(const OpcUaNodeId& nodeId, const std::string& displayName)
{
    OpcUaObject<UA_LocalizedText> uaDisplayName = UA_LOCALIZEDTEXT_ALLOC("", displayName.c_str());
    writeDisplayName(nodeId, uaDisplayName);
}

void OpcUaClient::writeDisplayName(const OpcUaNodeId& nodeId, const OpcUaObject<UA_LocalizedText>& displayName)
{
    auto status = UA_Client_writeDisplayNameAttribute(getLockedUaClient(), *nodeId, displayName.get());
    CheckStatusCodeException(status);
}

std::string OpcUaClient::readDescription(const OpcUaNodeId& nodeId)
{
    OpcUaObject<UA_LocalizedText> description;
    auto status = UA_Client_readDescriptionAttribute(getLockedUaClient(), *nodeId, description.get());
    CheckStatusCodeException(status);
    return utils::ToStdString(description->text);
}

void OpcUaClient::writeDescription(const OpcUaNodeId& nodeId, const std::string& description)
{
    OpcUaObject<UA_LocalizedText> uaDescription = UA_LOCALIZEDTEXT_ALLOC("", description.c_str());
    writeDescription(nodeId, uaDescription);
}

void OpcUaClient::writeDescription(const OpcUaNodeId& nodeId, const OpcUaObject<UA_LocalizedText>& description)
{
    auto status = UA_Client_writeDescriptionAttribute(getLockedUaClient(), *nodeId, description.get());
    CheckStatusCodeException(status);
}

OpcUaNodeId OpcUaClient::readDataType(const OpcUaNodeId& nodeId)
{
    OpcUaNodeId dataTypeId;
    const auto status = UA_Client_readDataTypeAttribute(getLockedUaClient(), *nodeId, dataTypeId.get());
    CheckStatusCodeException(status);
    return dataTypeId;
}

OpcUaObject<UA_CallMethodResult> OpcUaClient::callMethod(const OpcUaCallMethodRequest& callRequest)
{
    OpcUaObject<UA_CallRequest> request;

    auto copyCallRequest = callRequest.copy();
    request->methodsToCall = copyCallRequest.get();
    request->methodsToCallSize = 1;

    OpcUaObject<UA_CallResponse> response = callMethods(request);

    request->methodsToCall = nullptr;
    request->methodsToCallSize = 0;

    CheckStatusCodeException(response->responseHeader.serviceResult);
    return response->results[0];
}

void OpcUaClient::callMethods(const std::vector<OpcUaCallMethodRequestWithCallback>& container)
{
    size_t nodesCnt = std::size(container);
    if (nodesCnt == 0)
        return;

    UA_CallMethodRequest* items = (UA_CallMethodRequest*) UA_Array_new(nodesCnt, &UA_TYPES[UA_TYPES_CALLMETHODREQUEST]);
    for (size_t i = 0; i < nodesCnt; i++)
        items[i] = *container[i];  // optimization. reinit before delete

    OpcUaObject<UA_CallRequest> request;

    request->methodsToCall = items;
    request->methodsToCallSize = nodesCnt;

    OpcUaObject<UA_CallResponse> response = callMethods(request);

    for (size_t i = 0; i < nodesCnt; i++)
        UA_CallMethodRequest_init(&items[i]);

    CheckStatusCodeException(response->responseHeader.serviceResult);

    for (size_t i = 0; i < nodesCnt; i++)
        container[i].processFunction(response->results[i]);
}

void OpcUaClient::writeValue(const OpcUaNodeId& nodeId, const OpcUaVariant& value)
{
    CheckStatusCodeException(UA_Client_writeValueAttribute(getLockedUaClient(), *nodeId, value.get()));
}

Subscription* OpcUaClient::createSubscription(const OpcUaObject<UA_CreateSubscriptionRequest>& request,
                                              const StatusChangeNotificationCallbackType& statusChangeCallback)
{
    return Subscription::CreateSubscription(this, request, statusChangeCallback);
}

END_NAMESPACE_OPENDAQ_OPCUA
