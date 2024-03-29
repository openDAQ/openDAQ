/*
 * Copyright 2022-2023 Blueberry d.o.o.
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
#include <opendaq/authentication_provider.h>
#include <coretypes/intfs.h>
#include <coretypes/string_ptr.h>
#include <opendaq/user_ptr.h>

BEGIN_NAMESPACE_OPENDAQ

// AuthenticationProviderImpl

class AuthenticationProviderImpl : public ImplementationOf<IAuthenticationProvider>
{
public:
    explicit AuthenticationProviderImpl();

    ErrCode INTERFACE_FUNC authenticate(IString* username, IString* password, IUser** userOut) override;

protected:
    void loadUserList(const ListPtr<IUser>& userList);
    virtual UserPtr findUser(const StringPtr& username);
    bool isPasswordValid(const StringPtr& hash, const StringPtr& password);

    DictPtr<IString, IUser> users;
};

// StaticAuthenticationProviderImpl

class StaticAuthenticationProviderImpl : public AuthenticationProviderImpl
{
public:
    StaticAuthenticationProviderImpl(const ListPtr<IUser>& users);
};

// JsonStringAuthenticationProviderImpl

class JsonStringAuthenticationProviderImpl : public AuthenticationProviderImpl
{
public:
    JsonStringAuthenticationProviderImpl(const StringPtr& jsonString);

protected:
    void loadJsonString(const StringPtr& jsonString);
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
