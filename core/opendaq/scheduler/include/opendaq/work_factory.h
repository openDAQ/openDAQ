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
#include <opendaq/work_ptr.h>
#include <opendaq/work_repetitive_ptr.h>
#include <opendaq/work_impl.h>

BEGIN_NAMESPACE_OPENDAQ

/*!
 * @ingroup opendaq_work
 * @addtogroup opendaq_work_factories Factories
 * @{
 */

/*!
 *  @brief creates a work object, a lightweight implementation of callback used in scheduler for worker tasks.
 */
template <class Callback>
WorkPtr Work(Callback&& callback)
{
    return createWithImplementation<IWork, WorkImpl<Callback>>(std::forward<Callback>(callback));
}

/*!
 *  @brief creates a work object, a lightweight implementation of callback used in scheduler for worker tasks.
 */
template <class Callback>
WorkPtr WorkRepetitive(Callback&& callback)
{
    return createWithImplementation<IWork, WorkRepetitiveImpl<Callback>>(std::forward<Callback>(callback));
}

/*!@}*/

END_NAMESPACE_OPENDAQ
