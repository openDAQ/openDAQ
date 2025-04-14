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

#ifdef __cplusplus
extern "C"
{
#endif

#define OPENDAQ_LOG_LEVEL_TRACE 0
#define OPENDAQ_LOG_LEVEL_DEBUG 1
#define OPENDAQ_LOG_LEVEL_INFO 2
#define OPENDAQ_LOG_LEVEL_WARN 3
#define OPENDAQ_LOG_LEVEL_ERROR 4
#define OPENDAQ_LOG_LEVEL_CRITICAL 5
#define OPENDAQ_LOG_LEVEL_OFF 6
#define OPENDAQ_LOG_LEVEL_DEFAULT 7

    enum LogLevel
    {
        LogLevelTrace = OPENDAQ_LOG_LEVEL_TRACE,        ///< used to log any details about the execution for diagnostic
        LogLevelDebug = OPENDAQ_LOG_LEVEL_DEBUG,        ///< used to log information which useful during software debugging
        LogLevelInfo = OPENDAQ_LOG_LEVEL_INFO,          ///< used to log significant point-of-interest of the normal execution
        LogLevelWarn = OPENDAQ_LOG_LEVEL_WARN,          ///< used to log expected problems that do not abort the execution
        LogLevelError = OPENDAQ_LOG_LEVEL_ERROR,        ///< used to log unexpected failures
        LogLevelCritical = OPENDAQ_LOG_LEVEL_CRITICAL,  ///< used to log unrecoverable problems or corruptions
        LogLevelOff = OPENDAQ_LOG_LEVEL_OFF,            ///< used to turn off logging
        LogLevelDefault = OPENDAQ_LOG_LEVEL_DEFAULT     ///< choose default logging level either from environment or compile-time setting
    };

#ifdef __cplusplus
}
#endif