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
#include <opendaq/credential_request.h>
#include <opendaq/credential_request_builder.h>
#include <opendaq/component_type_ptr.h>
#include <opendaq/authentication_method_ptr.h>

BEGIN_NAMESPACE_OPENDAQ

class CredentialRequestImpl : public ImplementationOf<ICredentialRequest>
{
public:
    explicit CredentialRequestImpl(ICredentialRequestBuilder* credentialRequestBuilder);

    ErrCode INTERFACE_FUNC getComponentType(IComponentType** componentType) override;
    ErrCode INTERFACE_FUNC getConnectionString(IString** connectionString) override;
    ErrCode INTERFACE_FUNC getDisplayName(IString** displayName) override;
    ErrCode INTERFACE_FUNC getManufacturer(IString** manufacturer) override;
    ErrCode INTERFACE_FUNC getSerialNumber(IString** serialNumber) override;
    ErrCode INTERFACE_FUNC getModel(IString** model) override;
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
