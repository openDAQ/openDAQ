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

/*
 *
 * Handles client / server communication. Separated implementation for two reasons: 1) it can be instantiated by client or browser and
 * shared 2) connection issues are isolated from OPC UA services implementation.
 */

#pragma once

#include <atomic>
#include <mutex>
#include <set>
#include <thread>

#include <opcuaclient/opcuacallmethodrequest.h>
#include <opcuaclient/opcuareadvalueid.h>
#include <opcuashared/opcuacallmethodresult.h>
#include <opcuashared/opcuaendpoint.h>
#include <opcuashared/opcuanodeid.h>
#include <opcuashared/opcuavariant.h>
#include <open62541/client.h>
#include <open62541/util.h>
#include <opendaq/utils/timer_thread.h>

#include <opcuaclient/opcuatimertaskcontextcollection.h>
#include "opcuashared/node/opcuanodemethod.h"

#include <opcuaclient/subscriptions.h>

BEGIN_NAMESPACE_OPENDAQ_OPCUA

class OpcUaClient;
using OpcUaClientPtr = std::shared_ptr<OpcUaClient>;

class ClientLockGuard
{
public:
    ClientLockGuard(OpcUaClient* client = nullptr) noexcept;
    virtual ~ClientLockGuard() noexcept;
    operator UA_Client*();

    ClientLockGuard(ClientLockGuard&& other) noexcept;
    ClientLockGuard& operator=(ClientLockGuard&& other) noexcept;

    ClientLockGuard(const ClientLockGuard&) = delete;
    ClientLockGuard& operator=(const ClientLockGuard&) = delete;

protected:
    OpcUaClient* client;
};

class UaClientFactory
{
public:
    UaClientFactory();
    static UA_Client* Create(const OpcUaClientSecurityConfig* securityConfig = nullptr,
                             UA_LogLevel logLevel = UA_LOGLEVEL_WARNING,
                             const UA_DataTypeArray* customDataTypes = nullptr);

    void setSecurityConfig(const OpcUaClientSecurityConfig* securityConfig);
    void setCustomDataTypes(const UA_DataTypeArray* customDataTypes);
    void setLogLevel(const UA_LogLevel logLevel);

    UA_Client* build();

private:
    void configureClient();
    void configureClientSecurity();
    void configureClientAppUri();
    void configureClientConfigDefaults();

    UA_ClientConfig* config;
    const OpcUaClientSecurityConfig* securityConfig = nullptr;
    const UA_DataTypeArray* customDataTypes = nullptr;
    UA_Client* client;
    UA_LogLevel logLevel;
};

class OpcUaClient
{
public:
    explicit OpcUaClient(const std::string& url);
    explicit OpcUaClient(const OpcUaEndpoint& endpoint);
    ~OpcUaClient();

    static constexpr size_t CONNECTION_TIMEOUT_SECONDS = 10;

    void initialize();
    void connect();
    void disconnect(bool doClear = true);
    void clear();
    bool isConnected();
    UA_Client* getUaClient();
    ClientLockGuard getLockedUaClient();
    const OpcUaEndpoint& getEndpoint() const;
    void setTimeout(uint32_t timoutMs);
    uint32_t getTimeout() const;
    void setConnectivityCheckInterval(uint32_t connectivityCheckInterval);

    std::recursive_mutex& getLock();

    void runIterate(std::chrono::milliseconds period = std::chrono::milliseconds(20), std::chrono::milliseconds iterateTimeout = std::chrono::milliseconds(0));
    void stopIterate();

    UA_StatusCode iterate(std::chrono::milliseconds timeout = std::chrono::milliseconds(0));

    OpcUaCallbackIdent scheduleTimerTask(double intervalMs, const OpcUaTimerTaskType& task);
    void removeTimerTask(OpcUaCallbackIdent ident);
    bool timerTaskExists(OpcUaCallbackIdent ident);

    bool nodeExists(const OpcUaNodeId& nodeId);
    OpcUaVariant readValue(const OpcUaNodeId& node);
    UA_NodeClass readNodeClass(const OpcUaNodeId& nodeId);
    std::string readBrowseName(const OpcUaNodeId& nodeId);
    std::string readDisplayName(const OpcUaNodeId& nodeId);
    size_t readDimension(const OpcUaNodeId& nodeId);
    void writeDisplayName(const OpcUaNodeId& nodeId, const std::string& displayName);
    void writeDisplayName(const OpcUaNodeId& nodeId, const OpcUaObject<UA_LocalizedText>& displayName);
    std::string readDescription(const OpcUaNodeId& nodeId);
    void writeDescription(const OpcUaNodeId& nodeId, const std::string& description);
    void writeDescription(const OpcUaNodeId& nodeId, const OpcUaObject<UA_LocalizedText>& description);
    OpcUaNodeId readDataType(const OpcUaNodeId& nodeId);

    OpcUaObject<UA_CallResponse> callMethods(const OpcUaObject<UA_CallRequest>& request);

    OpcUaObject<UA_CallMethodResult> callMethod(const OpcUaCallMethodRequest& callRequest);

    void callMethods(const std::vector<OpcUaCallMethodRequestWithCallback>& container);

    void writeValue(const OpcUaNodeId& nodeId, const OpcUaVariant& value);

    OpcUaObject<UA_ReadResponse> readNodeAttributes(const OpcUaObject<UA_ReadRequest>& request);

    void readNodeAttributes(const std::vector<OpcUaReadValueIdWithCallback>& request);

    Subscription* createSubscription(const OpcUaObject<UA_CreateSubscriptionRequest>& request,
                                     const StatusChangeNotificationCallbackType& statusChangeCallback = nullptr);

protected:
    static void timerTaskCallback(UA_Client* client, void* data);

    void executeIterateCallback();

    UA_Client* uaclient{};
    OpcUaEndpoint endpoint;
    uint32_t timeoutMs{CONNECTION_TIMEOUT_SECONDS * 1000};
    uint32_t connectivityCheckInterval{CONNECTION_TIMEOUT_SECONDS * 1000};

    std::recursive_mutex lock;

    TimerTaskContextCollection timerTasks;

    std::chrono::milliseconds iterateTimeout;

    daq::utils::TimerThread iterateThread;
};

END_NAMESPACE_OPENDAQ_OPCUA
