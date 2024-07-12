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
#include <coretypes/common.h>
#include <coretypes/baseobject.h>
#include <coretypes/stringobject.h>
#include <coreobjects/user.h>

BEGIN_NAMESPACE_OPENDAQ

/*!
 * @ingroup objects_security
 * @addtogroup objects_security_authentication_provider AuthenticationProvider
 * @{
 */

/*!
 * @brief A class which is responsible for authenticating a user. The authentication is usually done by verifying
 * the username and password. An authenticator implementation might use external services for achieving that. It might
 * make a call to an external databse, do a lookup to a json file with defined users or it might simply check the
 * password against a hardcoded one.
 */
DECLARE_OPENDAQ_INTERFACE(IAuthenticationProvider, IBaseObject)
{
    /*!
     * @brief Authenticate user using username and password. If authentication is successful, a User instance is returned.
     * Otherwise an exception is thrown.
     * 
     * @param username The username.
     * @param password The password in plain text.
     * @param user[out] And instance of successfully authenticated user. If authentication is not successful, an exception is thrown.
     */
    virtual ErrCode INTERFACE_FUNC authenticate(IString* username, IString* password, IUser** userOut) = 0;

    /*!
     * @brief Returns true if anonymous authentication is allowed. When anonymous authentication is enabled, user can connect
     * to the server without providing username or password.
     * @param allowedOut[out] True if anonymous authentication is allowed.
     */
    virtual ErrCode INTERFACE_FUNC isAnonymousAllowed(Bool* allowedOut) = 0;
};

/*!@}*/

/*!
 * @brief Creates an authentication provider out of static list of users.
 * @param userList List of User objects.
 */
OPENDAQ_DECLARE_CLASS_FACTORY_WITH_INTERFACE_AND_CREATEFUNC(LIBRARY_FACTORY,
                                                            StaticAuthenticationProvider,
                                                            IAuthenticationProvider,
                                                            createStaticAuthenticationProvider,
                                                            Bool,
                                                            allowAnonymous,
                                                            IList*,
                                                            userList)

/*!
 * @brief Creates an authentication provider out of json string.
 * @param jsonString Json string containg a list of serialized User objects.
 */
OPENDAQ_DECLARE_CLASS_FACTORY_WITH_INTERFACE_AND_CREATEFUNC(LIBRARY_FACTORY,
                                                            JsonStringAuthenticationProvider,
                                                            IAuthenticationProvider,
                                                            createJsonStringAuthenticationProvider,
                                                            IString*,
                                                            jsonString)

/*!
 * @brief Creates an authentication provider out of json file.
 * @param filename File path to a json file containing a list of serialized User objects.
 */
OPENDAQ_DECLARE_CLASS_FACTORY_WITH_INTERFACE_AND_CREATEFUNC(LIBRARY_FACTORY,
                                                            JsonFileAuthenticationProvider,
                                                            IAuthenticationProvider,
                                                            createJsonFileAuthenticationProvider,
                                                            IString*,
                                                            filename)

END_NAMESPACE_OPENDAQ
