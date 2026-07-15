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
#include <coretypes/baseobject.h>
#include <opendaq/work.h>

BEGIN_NAMESPACE_OPENDAQ

/*!
 * @brief Internal interface for repetitive work lifecycle used by the scheduler.
 */
DECLARE_OPENDAQ_INTERFACE(IWorkRepetitiveInternal, IBaseObject)
{
    /*!
     * @brief Returns and clears the callback passed to cancel(), if any.
     * @param[out] onStopCallback Set to the callback passed to cancel(), or nullptr.
     */
    virtual ErrCode INTERFACE_FUNC getOnStopCallback(IWork** onStopCallback) = 0;
};

END_NAMESPACE_OPENDAQ
