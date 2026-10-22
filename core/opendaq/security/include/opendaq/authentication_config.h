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
#include <coretypes/dictobject.h>
#include <coreobjects/property_object.h>
#include <opendaq/authentication_method.h>

BEGIN_NAMESPACE_OPENDAQ

/*#
 * [interfaceLibrary(IPropertyObject, "coreobjects")]
 * [interfaceSmartPtr(IPropertyObject, GenericPropertyObjectPtr, "<coreobjects/property_object_ptr.h>", true)]
 * [interfaceLibrary(IAuthenticationMethod, "opendaq")]
 * [interfaceSmartPtr(IAuthenticationMethod, AuthenticationMethodPtr, "<opendaq/authentication_method_ptr.h>")]
 */

/*!
 * @brief Carries the authentication settings used for a single connection attempt to a component.
 *
 * A component created with authentication keeps the config it was authenticated with (see
 * `IComponentPrivate::setAuthenticationConfig`), so that reloading it later goes through the same
 * credential-request process again.
 *
 * Is itself a Property object - `"AuthenticationMethod"` is a Selection property whose candidates are the
 * supported methods' own ids, as plain strings (the real `IAuthenticationMethod` objects themselves - fields
 * and all - are reachable via `getAuthenticationMethods()`, keyed by that same id).
 * A directly-supplied credential is carried as a `"SuppliedCredential"` property - a dictionary of field
 * value(s), keyed by their own field id (see `ICredentialField::getId`); its default value is an empty
 * dictionary, which means "none supplied" (the module asks the registered credential provider instead). A
 * non-empty value counts as supplied, and must carry a non-empty value for every field the currently selected
 * `"AuthenticationMethod"` marks required (see `ICredentialField::isRequired`) - a write that doesn't is
 * rejected. Changing `"AuthenticationMethod"` unconditionally resets `"SuppliedCredential"` back to empty.
 * The typed getters/setters below are an equal, typed alternative to tuning the
 * config through these properties directly - a caller can customize it either way, entirely through
 * `IAuthenticationConfig` itself or entirely through the generic `IPropertyObject` interface this object also
 * implements; `"SuppliedCredential"` and `"AuthenticationMethod"` can both equally be read and set through
 * either.
 */
DECLARE_OPENDAQ_INTERFACE(IAuthenticationConfig, IPropertyObject)
{
    /*!
     * @brief Gets the authentication method currently selected - the one of `getAuthenticationMethods()`
     * whose own id matches the selected `"AuthenticationMethod"` property value.
     * @param[out] authenticationMethod The currently selected authentication method.
     */
    virtual ErrCode INTERFACE_FUNC getSelectedAuthenticationMethod(IAuthenticationMethod** authenticationMethod) = 0;

    /*!
     * @brief Selects the authentication method to use, by its own id - the typed equivalent of
     * `setPropertySelectionValue("AuthenticationMethod", authenticationMethodId)`. Selecting a new method always
     * resets `"SuppliedCredential"` back to empty.
     * @param authenticationMethodId The id of one of `getAuthenticationMethods()`'s own keys.
     * @throws NotFoundException if `authenticationMethodId` doesn't match any of this config's supported
     * authentication methods.
     */
    virtual ErrCode INTERFACE_FUNC setAuthenticationMethod(IString* authenticationMethodId) = 0;

    /*!
     * @brief Gets every authentication method this config supports, keyed by their own id - the full set of
     * `"AuthenticationMethod"` selection candidates, i.e. the same shape `IComponentTypePrivate::getSupportedAuthenticationMethods()`
     * has, since this config was built from exactly that set. An equal, typed alternative to reading the
     * candidates generically off the `"AuthenticationMethod"` property.
     * @param[out] authenticationMethods The supported authentication methods, keyed by their own id.
     */
    // [templateType(authenticationMethods, IString, IAuthenticationMethod)]
    virtual ErrCode INTERFACE_FUNC getAuthenticationMethods(IDict** authenticationMethods) = 0;

    // [templateType(credential, IString, IString)]
    /*!
     * @brief Sets the credential supplied directly by the caller - the typed equivalent of
     * `setPropertyValue("SuppliedCredential", credential)`. Must carry a non-empty value for every field the
     * currently selected `"AuthenticationMethod"` marks required (see `ICredentialField::isRequired`), unless
     * `credential` itself is empty, which clears it back to the default "none supplied" state.
     * @param credential The supplied credential - pass empty to clear it back to "none supplied".
     * @throws InvalidParameterException if `credential` is missing one or more fields required by the
     * currently selected authentication method.
     */
    virtual ErrCode INTERFACE_FUNC setSuppliedCredential(IDict* credential) = 0;

    // [templateType(credential, IString, IString)]
    /*!
     * @brief Gets the credential supplied directly by the caller - the value of the `"SuppliedCredential"`
     * property, a dictionary of field value(s) keyed by their own field id. Always assigned - empty (the
     * property's default value) means none was supplied, in which case the module obtains one from the
     * registered credential provider instead.
     * @param[out] credential The supplied credential - empty if none was supplied.
     */
    virtual ErrCode INTERFACE_FUNC getSuppliedCredential(IDict** credential) = 0;
};

/*!
 * @brief Builds an `AuthenticationConfig` supporting every authentication method described in
 * `authenticationMethods`. Each entry becomes one candidate value of the resulting config's
 * `"AuthenticationMethod"` Selection property (see `IAuthenticationConfig`); its first entry,
 * in dict iteration order, starts out selected.
 *
 * A config that only ever supports one method is simply the one-entry case of this: pass a
 * `authenticationMethods` dict with a single key/value pair.
 */
OPENDAQ_DECLARE_CLASS_FACTORY_WITH_INTERFACE(
    LIBRARY_FACTORY, AuthenticationConfig, IAuthenticationConfig,
    IDict*, authenticationMethods
)

END_NAMESPACE_OPENDAQ
