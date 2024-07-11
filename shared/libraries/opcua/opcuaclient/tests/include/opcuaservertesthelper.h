/*
 * Copyright 2022-2024 openDAQ d.o.o.
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

#include <gtest/gtest.h>
#include <future>
#include "opcuaclient/opcuaclient.h"
#include "opcuashared/opcua.h"
#include "opcuashared/opcuacommon.h"
#include <open62541/server_config_default.h>
#include <thread>

BEGIN_NAMESPACE_OPENDAQ_OPCUA

#define ASSERT_EQ_STATUS(status, expectedStatus) ASSERT_EQ(status, (UA_StatusCode) expectedStatus)

class OpcUaServerTestHelper final
{
public:
    using OnConfigureCallback = std::function<void(UA_ServerConfig* config)>;

    OpcUaServerTestHelper();
    ~OpcUaServerTestHelper();

    void setSessionTimeout(double sessionTimeoutMs);

    void onConfigure(const OnConfigureCallback& callback);
    void startServer();
    void stop();

    std::string getServerUrl() const;

    void publishVariable(std::string identifier,
                         const void* value,
                         const UA_DataType* type,
                         UA_NodeId* parentNodeId,
                         const char* locale = "en_US",
                         int nodeIndex = 1,
                         size_t dimension = 1);

private:
    void runServer();
    void createModel();
    void publishFolder(const char* identifier, UA_NodeId* parentNodeId, const char* locale = "en_US", int nodeIndex = 1);
    void publishMethod(std::string identifier, UA_NodeId* parentNodeId, const char* locale = "en_US", int nodeIndex = 1);
    static UA_StatusCode helloMethodCallback(UA_Server* server,
                                             const UA_NodeId* sessionId,
                                             void* sessionHandle,
                                             const UA_NodeId* methodId,
                                             void* methodContext,
                                             const UA_NodeId* objectId,
                                             void* objectContext,
                                             size_t inputSize,
                                             const UA_Variant* input,
                                             size_t outputSize,
                                             UA_Variant* output);

    double sessionTimeoutMs;
    UA_Server* server{};
    std::unique_ptr<std::thread> serverThreadPtr;
    std::atomic<UA_Boolean> serverRunning = false;

    UA_UInt16 port = 4842u;
    OnConfigureCallback onConfigureCallback;
};

class BaseClientTest : public testing::Test
{
protected:
    void SetUp() override;
    void TearDown() override;
    OpcUaServerTestHelper testHelper;

    std::string getServerUrl() const;

    static void IterateAndWaitForPromise(OpcUaClient& client, const std::future<void>& future)
    {
        using namespace std::chrono;
        while (client.iterate(milliseconds(10)) == UA_STATUSCODE_GOOD &&
               future.wait_for(milliseconds(1)) != std::future_status::ready)
        {
        };
    }

    OpcUaClientPtr prepareAndConnectClient(int timeout = -1);
};

END_NAMESPACE_OPENDAQ_OPCUA
