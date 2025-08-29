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
#include <coretypes/common.h>
#include <coretypes/baseobject.h>

BEGIN_NAMESPACE_OPENDAQ


/*!
 * @ingroup objects_property_object
 * @addtogroup objects_mutex Mutex
 * @{
 */

/*!
 * @brief Mutex wrapper interface. Wraps a std::mutex object.
 */
DECLARE_OPENDAQ_INTERFACE(IMutex, IBaseObject)
{
    /*!
     * @brief Locks the mutex; blocking call.
     */
    virtual ErrCode INTERFACE_FUNC lock() = 0;

    /*!
     * @brief Tries to lock the mutex and returns `true` if successful; non-blocking call.
     */
    virtual ErrCode INTERFACE_FUNC tryLock(Bool* succeeded) = 0;

    /*!
     * @brief Unlocks the mutex.
     */
    virtual ErrCode INTERFACE_FUNC unlock() = 0;
};

/*!@}*/

OPENDAQ_DECLARE_CLASS_FACTORY(LIBRARY_FACTORY, Mutex)

END_NAMESPACE_OPENDAQ
