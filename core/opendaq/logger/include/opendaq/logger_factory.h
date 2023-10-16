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
#include <opendaq/logger_ptr.h>
#include <opendaq/log.h>
#include <opendaq/logger_sink_factory.h>

BEGIN_NAMESPACE_OPENDAQ

/*!
 * @ingroup opendaq_logger_logger
 * @addtogroup opendaq_logger_logger_factories Factories
 * @{
 */

/*!
 * @brief Creates a Logger object with a given log severity level and default set of sinks.
 * @param fileName The name used for the rotating files Sink. @see RotatingFileLoggerSink.
 * @param level The default minimal severity level of the messages to be logged.
 */
inline LoggerPtr Logger(const StringPtr& fileName = nullptr, LogLevel level = LogLevel(OPENDAQ_LOG_LEVEL))
{
    return LoggerPtr(Logger_Create(DefaultSinks(fileName), level));
}

/*!
 * @brief Creates a Logger object with given sinks, and log severity level.
 * @param sinks The list of Sink objects. List members are of type `ILoggerSink`.
 * @param level The default minimal severity level of the messages to be logged.
 */
inline LoggerPtr LoggerWithSinks(ListPtr<ILoggerSink> sinks, LogLevel level = LogLevel(OPENDAQ_LOG_LEVEL))
{
    return LoggerPtr(Logger_Create(sinks, level));
}

/*!@}*/

END_NAMESPACE_OPENDAQ
