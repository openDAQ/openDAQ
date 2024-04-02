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
#include <coretypes/objectptr.h>
#include <coreobjects/authentication_provider_ptr.h>

BEGIN_NAMESPACE_OPENDAQ

/*!
 * @ingroup opendaq_security
 * @addtogroup opendaq_security_user Factories
 * @{
 */

/*!
 * @brief Creates an empty authentication provider without any user.
 */
inline AuthenticationProviderPtr AuthenticationProvider()
{
    AuthenticationProviderPtr obj(StaticAuthenticationProvider_Create(nullptr));
    return obj;
}

/*!
 * @brief Creates an authentication provider out of static list of users.
 * @param users List of user objects.
 */
inline AuthenticationProviderPtr StaticAuthenticationProvider(const ListPtr<IUser>& users)
{
    AuthenticationProviderPtr obj(StaticAuthenticationProvider_Create(users));
    return obj;
}

/*!
 * @brief Creates an authentication provider out of static json file.
 * @param filename File path to json file contianung a list of user objects.
 */
inline AuthenticationProviderPtr JsonFileAuthenticationProvider(const StringPtr& filename)
{
    AuthenticationProviderPtr obj(JsonFileAuthenticationProvider_Create(filename));
    return obj;
}

/*!
 * @brief Creates an authentication provider out of static json string.
 * @param jsonString List of users and their groups encoded as json string.
 */
inline AuthenticationProviderPtr JsonStringAuthenticationProvider(const StringPtr& jsonString)
{
    AuthenticationProviderPtr obj(JsonStringAuthenticationProvider_Create(jsonString));
    return obj;
}

/*!@}*/

END_NAMESPACE_OPENDAQ
