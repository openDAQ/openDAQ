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
#include <opendaq/logger_sink_ptr.h>
#include <algorithm>

BEGIN_NAMESPACE_OPENDAQ

/*!
 * @ingroup opendaq_logger_sink
 * @addtogroup opendaq_logger_sink_factories Factories
 * @{
 */

/*!
 * @brief Creates a Logger Sink object with Stderr as a target.
 */
inline LoggerSinkPtr StdErrLoggerSink()
{
    return LoggerSinkPtr(StdErrLoggerSink_Create());
}

/*!
 * @brief Creates a Logger Sink object with Stdout as a target.
 */
inline LoggerSinkPtr StdOutLoggerSink()
{
    return LoggerSinkPtr(StdOutLoggerSink_Create());
}

/*!
 * @brief Creates a Logger Sink object with rotating files as a target.
 * @param fileName The base name of the files.
 * @param maxFileSize The maximum size of each file.
 * @param maxFiles The maximum count of files.
 */
inline LoggerSinkPtr RotatingFileLoggerSink(const StringPtr& fileName, SizeT maxFileSize, SizeT maxFiles)
{
    return LoggerSinkPtr(RotatingFileLoggerSink_Create(fileName, maxFileSize, maxFiles));
}

/*!
 * @brief Creates a Logger Sink object with basic file as a target.
 * @param fileName The name of the file.
 */
inline LoggerSinkPtr BasicFileLoggerSink(const StringPtr& fileName)
{
    return LoggerSinkPtr(BasicFileLoggerSink_Create(fileName));
}

#ifdef _WIN32
/*!
 * @brief Creates a Logger Sink object with WinDebug output as a target.
 */
inline LoggerSinkPtr WinDebugLoggerSink()
{
    return LoggerSinkPtr(WinDebugLoggerSink_Create());
}

#endif

inline LogLevel getEnvLogLevel(const std::string& envStr, int defaultLevel)
{
    int level = defaultLevel;
    char* env = std::getenv(envStr.c_str());
    if (env != nullptr)
    {
        try
        {
            level = std::stoi(env);
            if (level < 0 || level >= OPENDAQ_LOG_LEVEL_DEFAULT)
                level = defaultLevel;
        }
        catch (...)
        {
            level = defaultLevel;
        }
    }

    return static_cast<LogLevel>(level);
}

inline void getEnvFileSinkLogLevelAndFileName(LogLevel& level, std::string& fileName)
{
    level = getEnvLogLevel("OPENDAQ_SINK_FILE_LOG_LEVEL", OPENDAQ_LOG_LEVEL_TRACE);
    char* env = std::getenv("OPENDAQ_SINK_FILE_FILE_NAME");
    if (env != nullptr)
        fileName = env;
    else
        fileName.clear();
}

/*!
 * @brief Creates a list of Sink objects.
 * @param fileName The base file name for rotating files sink.
 *
 * The rotating files sink is present in the created list if @p fileName is provided,
 * otherwise only Stdout and WinDebug sinks are present
 */
inline ListPtr<ILoggerSink> DefaultSinks(const StringPtr& fileName = nullptr)
{
    auto sinks = List<ILoggerSink>();

    auto consoleSinkLogLevel = getEnvLogLevel("OPENDAQ_SINK_CONSOLE_LOG_LEVEL", OPENDAQ_LOG_LEVEL_INFO);
    if (consoleSinkLogLevel != LogLevel::Off)
    {
        auto consoleSink = StdOutLoggerSink();
        consoleSink.setLevel(consoleSinkLogLevel);
        sinks.pushBack(consoleSink);
    }

#if defined(_WIN32)
    auto winDebugSinkLogLevel = getEnvLogLevel("OPENDAQ_SINK_WINDEBUG_LOG_LEVEL", OPENDAQ_LOG_LEVEL_INFO);
    if (winDebugSinkLogLevel != LogLevel::Off)
    {
        auto winDebugSink = WinDebugLoggerSink();
        winDebugSink.setLevel(winDebugSinkLogLevel);
        sinks.pushBack(winDebugSink);
    }
#endif

    std::string fileSinkFileName;
    LogLevel fileSinkLogLevel;
    getEnvFileSinkLogLevelAndFileName(fileSinkLogLevel, fileSinkFileName);
    if (fileSinkFileName.empty() && fileName.assigned())
        fileSinkFileName = fileName.toStdString();

    if (!fileSinkFileName.empty())
    {
        auto fileSink = RotatingFileLoggerSink(fileSinkFileName, 1048576, 5);
        fileSink.setLevel(fileSinkLogLevel);
        sinks->pushBack(fileSink);
    }

    return sinks;
}

/*!@}*/

inline LogLevel LogLevelFromString(const StringPtr logLevelName)
{
    std::string name = logLevelName.toStdString();
    std::transform(name.begin(), name.end(), name.begin(), ::tolower);

    if (name == "trace")
        return LogLevel::Trace;
    if (name == "debug")
        return LogLevel::Debug;
    if (name == "info")
        return LogLevel::Info;
    if (name == "warn")
        return LogLevel::Warn;
    if (name == "error")
        return LogLevel::Error;
    if (name == "critical")
        return LogLevel::Critical;
    if (name == "off")
        return LogLevel::Off;

    return LogLevel::Trace;
}

END_NAMESPACE_OPENDAQ
