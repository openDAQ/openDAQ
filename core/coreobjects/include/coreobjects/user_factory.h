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
#include <coreobjects/user_ptr.h>

BEGIN_NAMESPACE_OPENDAQ

/*!
 * @ingroup objects_security
 * @addtogroup objects_security_user Factories
 * @{
 */

/*!
 * @brief Creates a immutable user object with provided arguments.
 * @param username Username of a user.
 * @param passwordHash Hashed password as a string in Modular Crypt Format.
 * @param groups The list of group IDs which the user belongs to.
 */
inline UserPtr User(const StringPtr& username, const StringPtr& passwordHash, const ListPtr<IString> groups = nullptr)
{
    UserPtr obj(User_Create(username, passwordHash, groups));
    return obj;
}

/*!@}*/

END_NAMESPACE_OPENDAQ
