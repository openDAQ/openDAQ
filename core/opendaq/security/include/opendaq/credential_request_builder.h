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
#include <opendaq/credential_request.h>

BEGIN_NAMESPACE_OPENDAQ

/*#
 * [interfaceLibrary(IAuthenticationMethod, "opendaq")]
 * [interfaceSmartPtr(IAuthenticationMethod, AuthenticationMethodPtr, "<opendaq/authentication_method_ptr.h>")]
 * [interfaceLibrary(IComponentType, "opendaq")]
 * [interfaceSmartPtr(IComponentType, GenericComponentTypePtr, "<opendaq/component_type_ptr.h>")]
 */

/*!
 * @brief Builds `ICredentialRequest` objects - used by a device/module to form the request it hands to
 * `ICredentialProvider::requestCredentials` when authentication is required for a connection attempt.
 */
DECLARE_OPENDAQ_INTERFACE(ICredentialRequestBuilder, IBaseObject)
{
/*!
 * @brief Builds and returns a `CredentialRequest` using the currently configured values. Fails if
 * `componentType`, `connectionString`, or `authenticationMethod` was never set.
 * @param[out] request The built credential request.
 */
virtual ErrCode INTERFACE_FUNC build(ICredentialRequest** request) = 0;

/*!
 * @brief Sets the type of the component the request is being built for. Required - `build()` fails if
 * never set.
 * @param componentType The component type.
 */
// [returnSelf]
// [templateType(componentType, IComponentType)]
virtual ErrCode INTERFACE_FUNC setComponentType(IComponentType* componentType) = 0;

/*!
 * @brief Gets the type of the component the request is being built for.
 * @param[out] componentType The component type.
 */
// [templateType(componentType, IComponentType)]
virtual ErrCode INTERFACE_FUNC getComponentType(IComponentType** componentType) = 0;

/*!
 * @brief Sets the canonical connection string of the connection attempt the request is being built for -
 * expected to already be resolved via the owning module's `onGetCanonicalConnectionString` (routing prefix
 * trimmed, every parameter made explicit), not the raw string the caller originally supplied. Required -
 * `build()` fails if never set.
 * @param connectionString The canonical connection string.
 */
// [returnSelf]
virtual ErrCode INTERFACE_FUNC setConnectionString(IString* connectionString) = 0;

/*!
 * @brief Gets the canonical connection string currently set on the builder.
 * @param[out] connectionString The canonical connection string.
 */
virtual ErrCode INTERFACE_FUNC getConnectionString(IString** connectionString) = 0;

/*!
 * @brief Sets the manufacturer of the device the connection is being established to or for - a request
 * can be for a direct connection to that device, or for a streaming connection attached to it. Optional -
 * leave unset if the manufacturer isn't known for this connection.
 * @param manufacturer The device manufacturer.
 */
// [returnSelf]
virtual ErrCode INTERFACE_FUNC setManufacturer(IString* manufacturer) = 0;

/*!
 * @brief Gets the manufacturer currently set on the builder.
 * @param[out] manufacturer The device manufacturer.
 */
virtual ErrCode INTERFACE_FUNC getManufacturer(IString** manufacturer) = 0;

/*!
 * @brief Sets the serial number of the device the connection is being established to or for - a request
 * can be for a direct connection to that device, or for a streaming connection attached to it. Optional -
 * leave unset if the serial number isn't known for this connection.
 * @param serialNumber The device serial number.
 */
// [returnSelf]
virtual ErrCode INTERFACE_FUNC setSerialNumber(IString* serialNumber) = 0;

/*!
 * @brief Gets the serial number currently set on the builder.
 * @param[out] serialNumber The device serial number.
 */
virtual ErrCode INTERFACE_FUNC getSerialNumber(IString** serialNumber) = 0;

/*!
 * @brief Sets the model of the device the connection is being established to or for - a request can be
 * for a direct connection to that device, or for a streaming connection attached to it. Optional - leave
 * unset if the model isn't known for this connection.
 * @param model The device model.
 */
// [returnSelf]
virtual ErrCode INTERFACE_FUNC setModel(IString* model) = 0;

/*!
 * @brief Gets the model currently set on the builder.
 * @param[out] model The device model.
 */
virtual ErrCode INTERFACE_FUNC getModel(IString** model) = 0;

/*!
 * @brief Sets the device's name to show the user. Optional - leave unset when unknown.
 * @param displayName The device's display name.
 */
// [returnSelf]
virtual ErrCode INTERFACE_FUNC setDisplayName(IString* displayName) = 0;

/*!
 * @brief Gets the display name currently set on the builder.
 * @param[out] displayName The device's display name.
 */
virtual ErrCode INTERFACE_FUNC getDisplayName(IString** displayName) = 0;

/*!
 * @brief Sets the authentication method the provider must provide a credential for - typically read from
 * `IAuthenticationConfig` when the request is being built. Required - `build()` fails if never set.
 * @param authenticationMethod The authentication method.
 */
// [returnSelf]
virtual ErrCode INTERFACE_FUNC setAuthenticationMethod(IAuthenticationMethod* authenticationMethod) = 0;

/*!
 * @brief Gets the authentication method currently set on the builder.
 * @param[out] authenticationMethod The authentication method.
 */
virtual ErrCode INTERFACE_FUNC getAuthenticationMethod(IAuthenticationMethod** authenticationMethod) = 0;
};

/*!
 * @brief Creates a `CredentialRequestBuilder` with no values set.
 */
OPENDAQ_DECLARE_CLASS_FACTORY_WITH_INTERFACE(
    LIBRARY_FACTORY, CredentialRequestBuilder, ICredentialRequestBuilder
)

END_NAMESPACE_OPENDAQ
