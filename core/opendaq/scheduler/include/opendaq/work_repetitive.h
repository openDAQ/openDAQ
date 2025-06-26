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
#include <coretypes/baseobject.h>

BEGIN_NAMESPACE_OPENDAQ

/*!
 * @ingroup opendaq_scheduler_components
 * @addtogroup opendaq_work RepetitiveWork
 * @{
 */

/*!
 * @brief The interface used for repetitive work that needs to be executed in the main loop.
 * The callback is expected to return a boolean value indicating whether the work should be repeated.
 * If the callback does return nothing, it is assumed that the work should not be repeated.
 */
DECLARE_OPENDAQ_INTERFACE(IWorkRepetitive, IBaseObject)
{
    /*!
     * @brief Executes the repetitive task logic and determines whether it should be repeated.
     * @param[out] repeatAfter Set to `true` to continue repeating the task, or `false` to stop.
     */
    virtual ErrCode INTERFACE_FUNC executeRepetitively(Bool* repeatAfter) = 0;
};

/*!@}*/

END_NAMESPACE_OPENDAQ
