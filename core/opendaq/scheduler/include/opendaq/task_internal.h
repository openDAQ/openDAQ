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
#include <coretypes/baseobject.h>
#include <opendaq/task_flow.h>

BEGIN_NAMESPACE_OPENDAQ

/*!
 * @ingroup opendaq_scheduler_components
 * @addtogroup opendaq_task TaskInternal
 * @{
 */

/*!
 * @brief Interface for accessing task and graph internals used to schedule them.
 * It should only be used internally inside the scheduler library.
 */
DECLARE_OPENDAQ_INTERFACE(ITaskInternal, IBaseObject)
{
    virtual tf::Task& INTERFACE_FUNC getTask() = 0;
    virtual tf::Taskflow* INTERFACE_FUNC getFlow() = 0;
    //
    virtual void* INTERFACE_FUNC getGraph() = 0;
};
/*!@}*/

END_NAMESPACE_OPENDAQ
