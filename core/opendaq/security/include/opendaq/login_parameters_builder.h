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
#include <opendaq/login_parameters.h>

BEGIN_NAMESPACE_OPENDAQ

/*#
 * [interfaceLibrary(IAuthenticationMethod, "opendaq")]
 * [interfaceSmartPtr(IAuthenticationMethod, AuthenticationMethodPtr, "<opendaq/authentication_method_ptr.h>")]
 */

/*!
 * @brief Builds `ILoginParameters` objects with the parameters needed to authenticate
 * a connection attempt, such as the selected authentication method and the resolved credentials.
 * The `credentials` defaults to empty if never set.
 */
DECLARE_OPENDAQ_INTERFACE(ILoginParametersBuilder, IBaseObject)
{
    /*!
     * @brief Builds and returns a `LoginParameters` using the currently configured values. Fails unless
     * `authenticationMethod` was set, and `credentials` satisfies its required fields.
     * @param[out] parameters The built login parameters.
     */
    virtual ErrCode INTERFACE_FUNC build(ILoginParameters** parameters) = 0;

    /*!
     * @brief Sets the resolved credentials - a value for each of the authentication method's mandatory
     * fields; empty for a method with none (e.g. "Anonymous"). Defaults to empty if never set.
     * @param credentials The resolved credentials.
     */
    // [returnSelf]
    // [templateType(credentials, IString, IString)]
    virtual ErrCode INTERFACE_FUNC setCredentials(IDict* credentials) = 0;

    // [templateType(credentials, IString, IString)]
    /*!
     * @brief Gets the credentials currently set on the builder.
     * @param[out] credentials The resolved credentials.
     */
    virtual ErrCode INTERFACE_FUNC getCredentials(IDict** credentials) = 0;

    /*!
     * @brief Sets the authentication method the credentials are shaped for. Required - `build()` fails if
     * never set.
     * @param method The authentication method.
     */
    // [returnSelf]
    virtual ErrCode INTERFACE_FUNC setAuthenticationMethod(IAuthenticationMethod* method) = 0;

    /*!
     * @brief Gets the authentication method currently set on the builder.
     * @param[out] method The authentication method.
     */
    virtual ErrCode INTERFACE_FUNC getAuthenticationMethod(IAuthenticationMethod** method) = 0;
};

/*!
 * @brief Creates a `LoginParametersBuilder` with no values set.
 */
OPENDAQ_DECLARE_CLASS_FACTORY_WITH_INTERFACE(
    LIBRARY_FACTORY, LoginParametersBuilder, ILoginParametersBuilder
)

END_NAMESPACE_OPENDAQ
