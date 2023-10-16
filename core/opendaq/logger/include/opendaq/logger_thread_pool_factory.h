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
#include <coretypes/common.h>
#include <opendaq/logger_thread_pool_ptr.h>

BEGIN_NAMESPACE_OPENDAQ

/*!
 * @ingroup opendaq_logger_thread_pool
 * @addtogroup opendaq_logger_thread_pool_factories Factories
 * @{
 */

/*!
 * @brief Creates a Logger Thread Pool object.
 */
inline LoggerThreadPoolPtr LoggerThreadPool()
{
    return LoggerThreadPoolPtr(LoggerThreadPool_Create());
}

/*!@}*/

END_NAMESPACE_OPENDAQ
