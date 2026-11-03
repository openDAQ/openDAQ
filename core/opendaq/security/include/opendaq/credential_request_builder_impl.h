/*
 * Copyright 2022-2026 openDAQ d.o.o.
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

#include <coretypes/impl.h>
#include <opendaq/credential_request_builder.h>
#include <opendaq/component_type_ptr.h>
#include <opendaq/authentication_method_ptr.h>

BEGIN_NAMESPACE_OPENDAQ

class CredentialRequestBuilderImpl : public ImplementationOf<ICredentialRequestBuilder>
{
public:
    explicit CredentialRequestBuilderImpl();

    ErrCode INTERFACE_FUNC build(ICredentialRequest** dimension) override;

    ErrCode INTERFACE_FUNC setComponentType(IComponentType* componentType) override;
    ErrCode INTERFACE_FUNC getComponentType(IComponentType** componentType) override;
    ErrCode INTERFACE_FUNC setConnectionString(IString* connectionString) override;
    ErrCode INTERFACE_FUNC getConnectionString(IString** connectionString) override;

    ErrCode INTERFACE_FUNC setManufacturer(IString* manufacturer) override;
    ErrCode INTERFACE_FUNC getManufacturer(IString** manufacturer) override;
    ErrCode INTERFACE_FUNC setSerialNumber(IString* serialNumber) override;
    ErrCode INTERFACE_FUNC getSerialNumber(IString** serialNumber) override;
    ErrCode INTERFACE_FUNC setModel(IString* model) override;
    ErrCode INTERFACE_FUNC getModel(IString** model) override;

    ErrCode INTERFACE_FUNC setDisplayName(IString* displayName) override;
    ErrCode INTERFACE_FUNC getDisplayName(IString** displayName) override;

    ErrCode INTERFACE_FUNC setAuthenticationMethod(IAuthenticationMethod* authenticationMethod) override;
    ErrCode INTERFACE_FUNC getAuthenticationMethod(IAuthenticationMethod** authenticationMethod) override;

private:
    ComponentTypePtr componentType;
    StringPtr connectionString;
    StringPtr displayName;
    StringPtr manufacturer;
    StringPtr serialNumber;
    StringPtr model;
    AuthenticationMethodPtr authenticationMethod;
};

END_NAMESPACE_OPENDAQ
