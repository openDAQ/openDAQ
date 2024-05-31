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

Endpoint defines the used network protocol and the necessary security
settings to be able to connect to the server.
One server can provide one or more endpoints.

*/

#pragma once

#include <opcuashared/opcua.h>
#include <string>
#include <memory>
#include "opcuacommon.h"
#include <opcuashared/opcuasecurity_config.h>
#include <opcuashared/opcuadatatypearraylist.h>
#include <open62541/plugin/log.h>

BEGIN_NAMESPACE_OPENDAQ_OPCUA

class OpcUaEndpoint;
using OpcUaEndpointPtr = std::shared_ptr<OpcUaEndpoint>;

class OpcUaEndpoint
{
public:
    OpcUaEndpoint(const std::string& url);
    OpcUaEndpoint(const std::string& url, const std::string& username, const std::string& password);

    void setName(const std::string& name);
    const std::string getName() const;

    void setUrl(const std::string& url);
    const std::string getUrl() const;

    void setUsername(const std::string& username);
    const std::string getUsername() const;

    void setPassword(const std::string& password);
    const std::string getPassword() const;

    void registerCustomTypes(const size_t typesSize, const UA_DataType* types);
    const UA_DataTypeArray* getCustomDataTypes() const;

    bool isAnonymous();

private:
    std::string name;
    std::string url;
    std::string username;
    std::string password;
    OpcUaDataTypeArrayList customDataTypeList;
};

END_NAMESPACE_OPENDAQ_OPCUA
