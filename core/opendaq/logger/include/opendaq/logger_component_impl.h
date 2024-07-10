/*
 * Copyright 2022-2024 openDAQ d.o.o.
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
#include <opendaq/logger_component.h>
#include <opendaq/logger_thread_pool_ptr.h>

#include <coretypes/intfs.h>
#include <coretypes/listobject_factory.h>
#include <coretypes/string_ptr.h>

#include <memory>

#include <spdlog/async_logger.h>

BEGIN_NAMESPACE_OPENDAQ

class LoggerComponentImpl final : public ImplementationOf<ILoggerComponent>
{
public:
#ifdef OPENDAQ_LOGGER_SYNC
    using LoggerComponentType = spdlog::logger;
#else
    using LoggerComponentType = spdlog::async_logger;
#endif
    using LoggerComponentTypePtr = std::shared_ptr<LoggerComponentType>;

    LoggerComponentImpl(const StringPtr& name, const ListPtr<ILoggerSink>& sinks, const LoggerThreadPoolPtr& threadPool, LogLevel level = LogLevel::Info);

    ErrCode INTERFACE_FUNC getName(IString** name) override;

    ErrCode INTERFACE_FUNC setLevel(LogLevel level) override;
    ErrCode INTERFACE_FUNC getLevel(LogLevel* level) override;

    ErrCode INTERFACE_FUNC logMessage(SourceLocation location, ConstCharPtr msg, LogLevel level) override;

    ErrCode INTERFACE_FUNC setPattern(IString* pattern) override;
    ErrCode INTERFACE_FUNC shouldLog(LogLevel level, Bool* willLog) override;

    ErrCode INTERFACE_FUNC flush() override;
    ErrCode INTERFACE_FUNC flushOnLevel(LogLevel level) override;

    ErrCode INTERFACE_FUNC toString(CharPtr* str) override;

private:
    LoggerComponentTypePtr spdlogLogger;
    LoggerThreadPoolPtr threadPool;

    LogLevel getDefaultLogLevel();
    LogLevel getLogLevelFromParam(LogLevel logLevel);
};

END_NAMESPACE_OPENDAQ
