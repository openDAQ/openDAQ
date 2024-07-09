/*
 * Copyright 2022-2024 openDAQ d. o. o.
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
#include <coretypes/intfs.h>
#include <memory>
#include <opendaq/logger_sink.h>
#include <spdlog/spdlog.h>
#include <spdlog/sinks/rotating_file_sink.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <opendaq/logger_sink_base_private.h>
#include <opendaq/logger_sink_last_message_private.h>
#include <opendaq/logger_sink_last_message_impl.h>

BEGIN_NAMESPACE_OPENDAQ

template <typename... Interfaces>
class LoggerSinkBase : public ImplementationOf<ILoggerSink, ILoggerSinkBasePrivate, Interfaces...>
{
public:
    using Sink = spdlog::sinks::sink;
    using SinkPtr = std::shared_ptr<Sink>;

    LoggerSinkBase(SinkPtr&& sink);

    ErrCode INTERFACE_FUNC setLevel(LogLevel level) override;
    ErrCode INTERFACE_FUNC getLevel(LogLevel* level) override;

    ErrCode INTERFACE_FUNC shouldLog(LogLevel level, Bool* willLog) override;

    ErrCode INTERFACE_FUNC setPattern(IString* pattern) override;
    ErrCode INTERFACE_FUNC flush() override;

    ErrCode INTERFACE_FUNC equals(IBaseObject* other, Bool* equals) const override;

    ErrCode INTERFACE_FUNC getSinkImpl(SinkPtr* sinkImp) override;

protected:
    SinkPtr sink;
};

template <typename TSinkType, typename... Interfaces>
class LoggerSinkImpl : public LoggerSinkBase<Interfaces...>
{
public:
    using Super = LoggerSinkBase<Interfaces...>;
    using SinkType = TSinkType;

    LoggerSinkImpl();
    explicit LoggerSinkImpl(typename Super::SinkPtr&& sink);
};

template <>
class LoggerSinkImpl<spdlog::sinks::rotating_file_sink_mt>
    : public LoggerSinkImpl<void>
{
public:
    LoggerSinkImpl(IString* fileName, SizeT maxFileSize, SizeT maxFiles);
};

template <>
class LoggerSinkImpl<spdlog::sinks::basic_file_sink_mt>
    : public LoggerSinkImpl<void>
{
public:
    explicit LoggerSinkImpl(IString* fileName);
};

class LoggerSinkLastMessageImpl
    : public LoggerSinkImpl<spdlog::sinks::LoggerSinkLastMessageMt, ILastMessageLoggerSinkPrivate>
{
public:
    ErrCode INTERFACE_FUNC getLastMessage(IString** lastMessage) override;
    ErrCode INTERFACE_FUNC waitForMessage(SizeT timeoutMs, Bool* success) override;
};


END_NAMESPACE_OPENDAQ
