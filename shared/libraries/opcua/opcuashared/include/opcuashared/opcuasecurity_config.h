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
#include <optional>
#include <string>
#include <functional>
#include <opcuashared/opcuacommon.h>
#include <opcuashared/opcuaobject.h>
#include <opcuashared/opcuavector.h>

BEGIN_NAMESPACE_OPENDAQ_OPCUA

typedef std::function<UA_StatusCode(bool isAnonymous, const std::string& username, const std::string& password)> AuthenticateUserCallback;

class OpcUaSecurityConfig
{
public:
    UA_MessageSecurityMode securityMode = UA_MESSAGESECURITYMODE_NONE;
    std::optional<std::string> appUri;
    OpcUaObject<UA_ByteString> certificate;
    OpcUaObject<UA_ByteString> privateKey;
    OpcUaVector<UA_ByteString> trustList;
    OpcUaVector<UA_ByteString> revocationList;
    bool trustAll = false;

    void validate() const;
    bool hasCertificate() const;
    bool hasPrivateKey() const;
    std::optional<std::string> getAppUriOrParseFromCertificate() const;

protected:
    OpcUaSecurityConfig();
    OpcUaSecurityConfig(const OpcUaSecurityConfig& config);
    OpcUaSecurityConfig& operator=(const OpcUaSecurityConfig& config);

};

class OpcUaServerSecurityConfig : public OpcUaSecurityConfig
{
public:
    AuthenticateUserCallback authenticateUser;

    OpcUaServerSecurityConfig();
};

class OpcUaClientSecurityConfig : public OpcUaSecurityConfig
{
public:
    std::optional<std::string> username;
    std::optional<std::string> password;

    bool isAnonymous() const;
};

END_NAMESPACE_OPENDAQ_OPCUA
