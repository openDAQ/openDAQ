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
#include "opendaq/module_info.h"
#include <coretypes/dictobject.h>

BEGIN_NAMESPACE_OPENDAQ

struct IAuthenticationMethod;

/*#
 * [interfaceLibrary(IAuthenticationMethod, "opendaq")]
 * [interfaceSmartPtr(IAuthenticationMethod, AuthenticationMethodPtr, "<opendaq/authentication_method_ptr.h>")]
 */

/*!
 * @brief Private interface to component type. Allows for setting the module information, and reading the
 * raw authentication method dict a public `IComponentType::createAuthenticationConfig()` call builds its
 * config from.
 */
DECLARE_OPENDAQ_INTERFACE(IComponentTypePrivate, IBaseObject)
{
    // [returnSelf]
    /*!
     * @brief Sets the module information.
     * @param info The module information.
     */
    virtual ErrCode INTERFACE_FUNC setModuleInfo(IModuleInfo* info) = 0;

    /*!
     * @brief Gets the authentication methods this component type supports authenticating with, keyed by their
     * own authentication method id.
     * @param[out] authenticationMethods The supported authentication methods, keyed by their own id.
     */
    // [templateType(authenticationMethods, IString, IAuthenticationMethod)]
    virtual ErrCode INTERFACE_FUNC getSupportedAuthenticationMethods(IDict** authenticationMethods) = 0;
};

/*!@}*/

END_NAMESPACE_OPENDAQ
