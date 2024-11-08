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
#include <coretypes/baseobject.h>
#include <coreobjects/user.h>

BEGIN_NAMESPACE_OPENDAQ

/*#
 * [interfaceSmartPtr(IUser, UserPtr, "<coreobjects/user_ptr.h>")]
 */

/*!
 * @ingroup objects_security
 * @addtogroup objects_security_user UserLock
 * @{
 */

/*!
 * @brief This class manages the lock state of an object, usually a device. A device can be locked either with a specific user or without one.
 * If the device is locked with a specific user, only that user can unlock it. If the device is locked without a user, any user can unlock it.
 */
DECLARE_OPENDAQ_INTERFACE(IUserLock, IBaseObject)
{
    /*!
     * @brief Lock the object. Only the user who locked the object can unlock it. If the object was locked without a specific user,
     * or with an anonymous user, any user can unlock it.
     *
     * @param user User performing the lock action.
     */
    virtual ErrCode INTERFACE_FUNC lock(IUser* user = nullptr) = 0;

    /*!
     * @brief Unlock the object. Only the user who locked the object can unlock it. If the object was locked without a specific user,
     * or with an anonymous user, any user can unlock it.
     *
     * @param user User performing the unlock action.
     */
    virtual ErrCode INTERFACE_FUNC unlock(IUser* user = nullptr) = 0;

    /*!
     * @brief Forcefully unlock the object. A force unlock will always succeed, regardless of which user initially locked the object.
     */
    virtual ErrCode INTERFACE_FUNC forceUnlock() = 0;

    /*!
     * @brief Returns true if the object is locked.
     *
     * @param isLockedOut[out] True if the object is locked.
     */
    virtual ErrCode INTERFACE_FUNC isLocked(Bool* isLockedOut) = 0;
};

/*!@}*/

/*!
 * @brief Creates an unlocked UserLock object.
 */
OPENDAQ_DECLARE_CLASS_FACTORY(LIBRARY_FACTORY, UserLock)

END_NAMESPACE_OPENDAQ
