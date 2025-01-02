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
#include <opendaq/logger_component_ptr.h>
#include <opendaq/logger_thread_pool_ptr.h>
#include <opendaq/logger_thread_pool_factory.h>
#include <opendaq/logger_sink_factory.h>
#include <opendaq/log.h>

BEGIN_NAMESPACE_OPENDAQ

/*!
 * @ingroup opendaq_logger_component
 * @addtogroup opendaq_logger_component_factories Factories
 * @{
 */

/*!
 * @brief Creates a Logger component object with a given name, sinks, thread pool and log severity level.
 * @param name The name of the component.
 * @param sinks The list of Sink objects. List members are of type `ILoggerSink`.
 * @param threadPool The Thread pool object.
 * @param level The minimal severity level of message to log by the component.
 */
inline LoggerComponentPtr LoggerComponent(const StringPtr& name, ListPtr<ILoggerSink> sinks = DefaultSinks(),
                                          const LoggerThreadPoolPtr& threadPool = LoggerThreadPool(),
                                          LogLevel level = LogLevel(OPENDAQ_LOG_LEVEL))
{
    return LoggerComponentPtr(LoggerComponent_Create(name, sinks, threadPool, level));
}

/*!@}*/

END_NAMESPACE_OPENDAQ
