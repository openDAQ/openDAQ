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
#include <coretypes/common.h>
#include <coretypes/baseobject.h>
#include <coretypes/stringobject.h>
#include <opendaq/log_level.h>

BEGIN_NAMESPACE_OPENDAQ

/*!
 * @ingroup opendaq_logger
 * @{
 */

/*!
 * @brief Enumeration of available log severity levels. `Trace` is the lowest level.
 * `Critical` is the highest level.
 */
enum class LogLevel
{
    Trace = OPENDAQ_LOG_LEVEL_TRACE,        ///< used to log any details about the execution for diagnostic
    Debug = OPENDAQ_LOG_LEVEL_DEBUG,        ///< used to log information which useful during software debugging
    Info = OPENDAQ_LOG_LEVEL_INFO,          ///< used to log significant point-of-interest of the normal execution
    Warn = OPENDAQ_LOG_LEVEL_WARN,          ///< used to log expected problems that do not abort the execution
    Error = OPENDAQ_LOG_LEVEL_ERROR,        ///< used to log unexpected failures
    Critical = OPENDAQ_LOG_LEVEL_CRITICAL,  ///< used to log unrecoverable problems or corruptions
    Off = OPENDAQ_LOG_LEVEL_OFF,            ///< used to turn off logging
    Default = OPENDAQ_LOG_LEVEL_DEFAULT     ///< choose default logging level either from environment or compile-time setting
};

/*!@}*/

/*!
 * @ingroup opendaq_logger
 * @addtogroup opendaq_logger_sink Logger sink
 * @{
 */

/*!
 * @brief Represents the object that actually writes the log messages to the target.
 * Each Logger Sink is responsible for only single target: file, console etc.
 *
 * Logger Sink allows to set up a threshold log severity level, so the messages with lower level
 * will not be written to the target.
 *
 * Additionally, it provides the ability to customize format for messages written to the target,
 * see `setPattern` method.
 */
DECLARE_OPENDAQ_INTERFACE(ILoggerSink, IBaseObject)
{
    /*!
     * @brief Sets the minimal severity level of messages to be written to the target.
     * @param level The log severity level of the sink.
     */
    virtual ErrCode INTERFACE_FUNC setLevel(LogLevel level) = 0;

    /*!
     * @brief Gets the minimal severity level of messages to be written to the target.
     * @param[out] level The log severity level of the sink.
     */
    virtual ErrCode INTERFACE_FUNC getLevel(LogLevel* level) = 0;

    /*!
     * @brief Checks whether the messages with given log severity level will be written to the target or not.
     * @param level The severity level of messages.
     * @param[out] willLog True if the messages with @p level will be written to the target; false otherwise.
     */
    virtual ErrCode INTERFACE_FUNC shouldLog(LogLevel level, Bool* willLog) = 0;

    /*!
     * @brief Sets the custom formatter pattern for the sink.
     * @param pattern The formatter pattern string.
     */
    virtual ErrCode INTERFACE_FUNC setPattern(IString* pattern) = 0;

    /*!
     * @brief Triggers writing out the messages from temporary buffers to the target.
     */
    virtual ErrCode INTERFACE_FUNC flush() = 0;
};

/*!@}*/

OPENDAQ_DECLARE_CLASS_FACTORY_WITH_INTERFACE_AND_CREATEFUNC(LIBRARY_FACTORY, StdErrLoggerSink, ILoggerSink, createStdErrLoggerSink)
OPENDAQ_DECLARE_CLASS_FACTORY_WITH_INTERFACE_AND_CREATEFUNC(LIBRARY_FACTORY, StdOutLoggerSink, ILoggerSink, createStdOutLoggerSink)

OPENDAQ_DECLARE_CLASS_FACTORY_WITH_INTERFACE_AND_CREATEFUNC(LIBRARY_FACTORY,
                                                         RotatingFileLoggerSink,
                                                         ILoggerSink,
                                                         createRotatingFileLoggerSink,
                                                         IString*,
                                                         fileName,
                                                         SizeT,
                                                         maxFileByteSize,
                                                         SizeT,
                                                         maxFiles)

OPENDAQ_DECLARE_CLASS_FACTORY_WITH_INTERFACE_AND_CREATEFUNC(LIBRARY_FACTORY,
                                                         BasicFileLoggerSink,
                                                         ILoggerSink,
                                                         createBasicFileLoggerSink,
                                                         IString*,
                                                         fileName)

#ifdef _WIN32
    OPENDAQ_DECLARE_CLASS_FACTORY_WITH_INTERFACE_AND_CREATEFUNC(LIBRARY_FACTORY, WinDebugLoggerSink, ILoggerSink, createWinDebugLoggerSink)
#endif

OPENDAQ_DECLARE_CLASS_FACTORY_WITH_INTERFACE_AND_CREATEFUNC(LIBRARY_FACTORY, LastMessageLoggerSink, ILoggerSink, createLastMessageLoggerSink)


END_NAMESPACE_OPENDAQ
