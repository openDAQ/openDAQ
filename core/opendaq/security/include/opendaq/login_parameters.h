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
#include <opendaq/authentication_method.h>

BEGIN_NAMESPACE_OPENDAQ

/*#
 * [interfaceLibrary(IAuthenticationMethod, "opendaq")]
 * [interfaceSmartPtr(IAuthenticationMethod, AuthenticationMethodPtr, "<opendaq/authentication_method_ptr.h>")]
 */

struct ILoginParametersBuilder;

/*!
 * @brief Carries the parameters needed to authenticate a connection attempt - such as the selected
 * authentication method and the resolved credentials. Built via `ILoginParametersBuilder`, created by core
 * and read by the module implementation. Immutable.
 */
DECLARE_OPENDAQ_INTERFACE(ILoginParameters, IBaseObject)
{
    /*!
     * @brief Gets the authentication method the credentials are shaped for - the one selected by
     * `IAuthenticationConfig` among the component type's own supported methods.
     * @param[out] method The selected authentication method.
     */
    virtual ErrCode INTERFACE_FUNC getAuthenticationMethod(IAuthenticationMethod** method) = 0;

    // [templateType(credentials, IString, IString)]
    /*!
     * @brief Gets the resolved credentials - a value for each of the authentication method's mandatory
     * fields. Always assigned - empty for a method with none (e.g. "Anonymous").
     * @param[out] credentials The resolved credentials.
     */
    virtual ErrCode INTERFACE_FUNC getCredentials(IDict** credentials) = 0;

    // TODO add identity fingerprint
};

/*!
 * @brief Creates a `LoginParameters` from a `ILoginParametersBuilder`'s currently configured values.
 */
//[factory(Hide)]
OPENDAQ_DECLARE_CLASS_FACTORY_WITH_INTERFACE(
    LIBRARY_FACTORY, LoginParametersFromBuilder, ILoginParameters,
    ILoginParametersBuilder*, builder
)

END_NAMESPACE_OPENDAQ
