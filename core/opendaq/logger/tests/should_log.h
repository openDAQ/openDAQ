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
#include <opendaq/logger_sink.h>
#include <gtest/gtest.h>

using ShouldNotLogTrace = testing::TestWithParam<daq::LogLevel>;
using ShouldNotLogDebug = testing::TestWithParam<daq::LogLevel>;
using ShouldNotLogInfo = testing::TestWithParam<daq::LogLevel>;
using ShouldNotLogWarn = testing::TestWithParam<daq::LogLevel>;
using ShouldNotLogError = testing::TestWithParam<daq::LogLevel>;
using ShouldNotLogCritical = testing::TestWithParam<daq::LogLevel>;
using ShouldNotLogOff = testing::TestWithParam<daq::LogLevel>;

using ShouldLogTrace = testing::TestWithParam<daq::LogLevel>;
using ShouldLogDebug = testing::TestWithParam<daq::LogLevel>;
using ShouldLogInfo = testing::TestWithParam<daq::LogLevel>;
using ShouldLogWarn = testing::TestWithParam<daq::LogLevel>;
using ShouldLogError = testing::TestWithParam<daq::LogLevel>;
using ShouldLogCritical = testing::TestWithParam<daq::LogLevel>;
using ShouldLogOff = testing::TestWithParam<daq::LogLevel>;

namespace daq
{
    inline std::string logLevelToString(LogLevel level)
    {
        switch (level)
        {
            case daq::LogLevel::Trace:
                return "Trace";
            case daq::LogLevel::Debug:
                return "Debug";
            case daq::LogLevel::Info:
                return "Info";
            case daq::LogLevel::Warn:
                return "Warn";
            case daq::LogLevel::Error:
                return "Error";
            case daq::LogLevel::Critical:
                return "Critical";
            case daq::LogLevel::Off:
                return "Off";
            case daq::LogLevel::Default:
                return "Default";
        }
        return "Unknown";
    }

    // ReSharper disable once CppInconsistentNaming
    inline void PrintTo(daq::LogLevel logLevel, std::ostream* os)
    {
        // Needed for proper formatting in GTest (compile issues)
        *os << logLevelToString(logLevel);
    }
}

struct LogLevelToString
{
    std::string operator()(const testing::TestParamInfo<daq::LogLevel>& info) const
    {
        return daq::logLevelToString(info.param);
    }
};

