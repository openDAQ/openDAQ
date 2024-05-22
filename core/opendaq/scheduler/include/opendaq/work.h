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
#include <coretypes/baseobject.h>

BEGIN_NAMESPACE_OPENDAQ

/*!
 * @ingroup opendaq_scheduler_components
 * @addtogroup opendaq_work Work
 * @{
 */

/*!
 *  @brief A lightweight implementation of callback used in scheduler for worker tasks.
 */
DECLARE_OPENDAQ_INTERFACE(IWork, IBaseObject)
{
    /*!
     * @brief Executes the callback.
     */
    virtual ErrCode INTERFACE_FUNC execute() = 0;
};

/*!@}*/

END_NAMESPACE_OPENDAQ
