/*
 * Copyright 2022-2025 openDAQ d.o.o.
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

#include <opcuaserver/opcuaserver.h>
#include <open62541/client_config_default.h>
#include <opcuaclient/opcuaclient.h>

BEGIN_NAMESPACE_OPENDAQ_OPCUA

static constexpr const char SERVER_URL[] = "opc.tcp://localhost";

inline OpcUaServer createServer()
{
    return OpcUaServer();
}

inline UA_Client* CreateClient()
{
    UA_Client* client = UA_Client_new();
    UA_ClientConfig_setDefault(UA_Client_getConfig(client));
    return client;
}

inline std::shared_ptr<OpcUaClient> CreateClientAndConnect()
{
    auto client = std::make_shared<OpcUaClient>(SERVER_URL);
    client->connect();
    assert(client->isConnected());
    return client;
}

END_NAMESPACE_OPENDAQ_OPCUA
