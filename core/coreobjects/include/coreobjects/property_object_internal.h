/*
 * Copyright 2022-2025 openDAQ d.o.o.
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
#include <coreobjects/property.h>
#include <coreobjects/mutex.h>
#include <coreobjects/object_lock_guard.h>

BEGIN_NAMESPACE_OPENDAQ

/*!
 * @ingroup objects_property_object
 * @addtogroup objects_property_object_obj PropertyObjectInternal
 * @{
 */

enum class LockingStrategy : EnumType
{
    OwnLock = 0,        // Object locks its own mutex.
    InheritLock,        // Object locks the mutex of the nearest ancestor with the OwnLock strategy.
    ForwardOwnerLockOwn // Object locks its own mutex, but forwards the `getMutexOwner` request to its owner/parent.
};

/*#
 * [interfaceSmartPtr(ILockGuard, LockGuardPtr, "<coreobjects/object_lock_guard_ptr.h>")]
 */

DECLARE_OPENDAQ_INTERFACE(IPropertyObjectInternal, IBaseObject)
{
    virtual ErrCode INTERFACE_FUNC checkForReferences(IProperty* property, Bool* isReferenced) = 0;
    virtual ErrCode INTERFACE_FUNC checkForReferencesNoLock(IProperty* property, Bool* isReferenced) = 0;
    virtual ErrCode INTERFACE_FUNC enableCoreEventTrigger() = 0;
    virtual ErrCode INTERFACE_FUNC disableCoreEventTrigger() = 0;
    virtual ErrCode INTERFACE_FUNC getCoreEventTrigger(IProcedure** trigger) = 0;
    virtual ErrCode INTERFACE_FUNC setCoreEventTrigger(IProcedure* trigger) = 0;
    virtual ErrCode INTERFACE_FUNC clone(IPropertyObject** cloned) = 0;
    virtual ErrCode INTERFACE_FUNC setPath(IString* path) = 0;
    virtual ErrCode INTERFACE_FUNC getPath(IString** path) = 0;
    virtual ErrCode INTERFACE_FUNC isUpdating(Bool* updating) = 0;
    virtual ErrCode INTERFACE_FUNC hasUserReadAccess(IBaseObject* userContext, Bool * hasAccessOut) = 0;

    virtual ErrCode INTERFACE_FUNC getPropertyValueNoLock(IString* name, IBaseObject** value) = 0;
    virtual ErrCode INTERFACE_FUNC getPropertySelectionValueNoLock(IString* name, IBaseObject** value) = 0;
    virtual ErrCode INTERFACE_FUNC setPropertyValueNoLock(IString* name, IBaseObject* value) = 0;
    virtual ErrCode INTERFACE_FUNC setProtectedPropertyValueNoLock(IString* name, IBaseObject* value) = 0;
    virtual ErrCode INTERFACE_FUNC clearPropertyValueNoLock(IString* name) = 0;

    /*!
     *  @brief Gets a lock guard that locks the object's mutex.
     */
    virtual ErrCode INTERFACE_FUNC getLockGuard(ILockGuard** lockGuard) = 0;

    /*!
     *  @brief Gets a lock guard that locks the object's mutex. The mutex is wrapped to be recursive,
     *  allowing for multiple locks to be instantiated on the same thread. Fails if a non-recursive lock
     *  was already created on this thread.
     */
    virtual ErrCode INTERFACE_FUNC getRecursiveLockGuard(ILockGuard** lockGuard) = 0;

    /*!
     * @brief Sets the locking strategy of the object. The locking strategy must be set before the
     * object gets and owner is added to a parent folder.
     */
    virtual ErrCode INTERFACE_FUNC setLockingStrategy(LockingStrategy strategy) = 0;
    /*!
     * @brief Gets the locking strategy of the object. The locking strategy must be set before the
     * object gets and owner is added to a parent folder.
     */
    virtual ErrCode INTERFACE_FUNC getLockingStrategy(LockingStrategy* strategy) = 0;
    /*!
     * @brief Gets the object's mutex. Returns the mutex of the closest `OwnLock` strategy ancestor
     * if using the `InheritLock` strategy.
     */
    virtual ErrCode INTERFACE_FUNC getMutex(IMutex** mutex) = 0;
    /*
     * @brief Gets a reference to the owner of the mutex locked by the object. If the object's locking
     * strategy is not `OwnLock`, returns the closest ancestor with the `OwnLock` strategy.
     */
    virtual ErrCode INTERFACE_FUNC getMutexOwner(IPropertyObjectInternal** owner) = 0;
};

/*!@}*/

END_NAMESPACE_OPENDAQ
