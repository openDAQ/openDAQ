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
#include <coreobjects/authentication_provider.h>
#include <coretypes/intfs.h>
#include <coretypes/string_ptr.h>
#include <coreobjects/user_ptr.h>
#include <rapidjson/document.h>

BEGIN_NAMESPACE_OPENDAQ

// AuthenticationProviderImpl

class AuthenticationProviderImpl : public ImplementationOf<IAuthenticationProvider>
{
public:
    explicit AuthenticationProviderImpl(bool allowAnonymous);

    ErrCode INTERFACE_FUNC authenticate(IString* username, IString* password, IUser** userOut) override;
    ErrCode INTERFACE_FUNC isAnonymousAllowed(Bool* allowedOut) override;
    ErrCode INTERFACE_FUNC authenticateAnonymous(IUser** userOut) override;
    ErrCode INTERFACE_FUNC findUser(IString* username, IUser** userOut) override;

protected:
    void loadUserList(const ListPtr<IUser>& userList);
    virtual UserPtr findUserInternal(const StringPtr& username);
    bool isPasswordValid(const std::string& hash, const StringPtr& password);

    bool allowAnonymous;
    DictPtr<IString, IUser> users;
    const UserPtr anonymousUser;
};

// StaticAuthenticationProviderImpl

class StaticAuthenticationProviderImpl : public AuthenticationProviderImpl
{
public:
    StaticAuthenticationProviderImpl(bool allowAnonymous, const ListPtr<IUser>& users);
};

// JsonStringAuthenticationProviderImpl

class JsonStringAuthenticationProviderImpl : public AuthenticationProviderImpl
{
public:
    JsonStringAuthenticationProviderImpl(const StringPtr& jsonString);

protected:
    void loadJsonString(const StringPtr& jsonString);
    bool parseAllowAnonymous(rapidjson::Document& document);
    DictPtr<IString, IUser> parseUsers(rapidjson::Document& document);
    UserPtr parseUser(const rapidjson::Value& userObject);
    ListPtr<IString> parseUserGroups(const rapidjson::Value& userObject);
};

// JsonFileAuthenticationProviderImpl

class JsonFileAuthenticationProviderImpl : public JsonStringAuthenticationProviderImpl
{
public:
    JsonFileAuthenticationProviderImpl(const StringPtr& filename);

private:
    std::string readJsonFile(const StringPtr& filename);
};

END_NAMESPACE_OPENDAQ
