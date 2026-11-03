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
#include <coretypes/baseobject.h>
#include <coretypes/string_ptr.h>
#include <opendaq/authentication_method.h>

BEGIN_NAMESPACE_OPENDAQ

/*#
 * [interfaceLibrary(IAuthenticationMethod, "opendaq")]
 * [interfaceSmartPtr(IAuthenticationMethod, AuthenticationMethodPtr, "<opendaq/authentication_method_ptr.h>")]
 * [interfaceLibrary(IComponentType, "opendaq")]
 * [interfaceSmartPtr(IComponentType, GenericComponentTypePtr, "<opendaq/component_type_ptr.h>")]
 */

struct ICredentialRequestBuilder;
struct IComponentType;

/*!
 * @brief Carries the details of a credential request handed to `ICredentialProvider::requestCredentials`
 * when authentication is required for a connection attempt.
 *
 * Built via `ICredentialRequestBuilder`. Never carries the actual credentials - only
 * enough context (the component type, connection details, and the negotiated authentication method - whose
 * own id names the authentication method) for the provider to determine how to provide the credentials.
 */
DECLARE_OPENDAQ_INTERFACE(ICredentialRequest, IBaseObject)
{
    /*!
     * @brief Gets the type of the component the request is for.
     * @param[out] componentType The component type.
     */
    // [templateType(componentType, IComponentType)]
    virtual ErrCode INTERFACE_FUNC getComponentType(IComponentType** componentType) = 0;

    /*!
     * @brief Gets the canonical connection string of the connection attempt this request was formed for -
     * already resolved via the owning module's own `onGetCanonicalConnectionString` (routing prefix
     * trimmed, every parameter made explicit), not necessarily the raw string the caller originally supplied.
     * @param[out] connectionString The canonical connection string.
     */
    virtual ErrCode INTERFACE_FUNC getConnectionString(IString** connectionString) = 0;

    /*!
     * @brief Gets the device's name (e.g. obtained from its info or the add config), is a context for the provider, to show
     * the user. Optional - unassigned when unknown.
     * @param[out] displayName The device's display name.
     */
    virtual ErrCode INTERFACE_FUNC getDisplayName(IString** displayName) = 0;

    /*!
     * @brief Gets the manufacturer of the device the connection is being established to or for - a request
     * can be for a direct connection to that device, or for a streaming connection attached to it. Optional -
     * unassigned if the manufacturer isn't known for this connection.
     * @param[out] manufacturer The device manufacturer.
     */
    virtual ErrCode INTERFACE_FUNC getManufacturer(IString** manufacturer) = 0;

    /*!
     * @brief Gets the serial number of the device the connection is being established to or for - a
     * request can be for a direct connection to that device, or for a streaming connection attached to it.
     * Optional - unassigned if the serial number isn't known for this connection.
     * @param[out] serialNumber The device serial number.
     */
    virtual ErrCode INTERFACE_FUNC getSerialNumber(IString** serialNumber) = 0;

    /*!
     * @brief Gets the model of the device the connection is being established to or for - a request can be
     * for a direct connection to that device, or for a streaming connection attached to it. Optional -
     * unassigned if the model isn't known for this connection.
     * @param[out] model The device model.
     */
    virtual ErrCode INTERFACE_FUNC getModel(IString** model) = 0;

    /*!
     * @brief Gets the authentication method the provider must provide a credential for, read from
     * `IAuthenticationConfig` when the request was built.
     * @param[out] authenticationMethod The authentication method.
     */
    virtual ErrCode INTERFACE_FUNC getAuthenticationMethod(IAuthenticationMethod** authenticationMethod) = 0;
};

/*!
 * @brief Creates a `CredentialRequest` from a `ICredentialRequestBuilder`.
 */
//[factory(Hide)]
OPENDAQ_DECLARE_CLASS_FACTORY_WITH_INTERFACE(
    LIBRARY_FACTORY, CredentialRequestFromBuilder, ICredentialRequest,
    ICredentialRequestBuilder*, builder
)

END_NAMESPACE_OPENDAQ
