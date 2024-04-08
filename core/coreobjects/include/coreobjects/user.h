/*
 * Copyright 2022-2024 Blueberry d.o.o.
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
#include <coretypes/listobject.h>

BEGIN_NAMESPACE_OPENDAQ

/*!
 * @ingroup objects_security
 * @addtogroup objects_security_user User
 * @{
 */

/*!
 * @brief An immutable structure which describes an openDAQ user. It holds username, password as a hash string
 * and a list of groups assigned to the user.
 */
DECLARE_OPENDAQ_INTERFACE(IUser, IBaseObject)
{
    /*!
     * @brief Returns the username as a string.
     * @param username[out] The username.
     */
    virtual ErrCode INTERFACE_FUNC getUsername(IString * *username) = 0;

    // [templateType(groups, IString)]
    /*!
     * @brief Returns a list of group IDs which the user belongs to.
     * @param password[out] groups The list of group IDs which the user belongs to.
     */
    virtual ErrCode INTERFACE_FUNC getGroups(IList** groups) = 0;
};

/*!@}*/

/*!
 * @brief Creates an immutable user object
 * @param username The username.
 * @param passwordHash The hashed password as a string in Modular Crypt Format.
 * @param groups The list of group IDs which the user belongs to.
 */
OPENDAQ_DECLARE_CLASS_FACTORY(LIBRARY_FACTORY, User, IString*, username, IString*, passwordHash, IList*, groups)

END_NAMESPACE_OPENDAQ
