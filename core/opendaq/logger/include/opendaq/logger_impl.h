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
#include <opendaq/logger.h>
#include <opendaq/logger_sink_ptr.h>
#include <opendaq/logger_component_ptr.h>
#include <opendaq/logger_thread_pool_ptr.h>
#include <coretypes/intfs.h>
#include <coretypes/listobject_factory.h>
#include <coretypes/string_ptr.h>

#include <memory>
#include <unordered_map>
#include <vector>

#include <spdlog/spdlog.h>
#include <spdlog/details/periodic_worker.h>

BEGIN_NAMESPACE_OPENDAQ

class LoggerImpl final : public ImplementationOf<ILogger>
{
public:
    using LoggerFlushWorker = spdlog::details::periodic_worker;
    using LoggerFlushWorkerPtr = std::unique_ptr<LoggerFlushWorker>;

    explicit LoggerImpl(const ListPtr<ILoggerSink>& sinksList, LogLevel level = LogLevel::Default);

    ErrCode INTERFACE_FUNC setLevel(LogLevel level) override;
    ErrCode INTERFACE_FUNC getLevel(LogLevel* level) override;

    ErrCode INTERFACE_FUNC getOrAddComponent(IString* name, ILoggerComponent** component) override;
    ErrCode INTERFACE_FUNC addComponent(IString* name, ILoggerComponent** component) override;
    ErrCode INTERFACE_FUNC removeComponent(IString* name) override;

    ErrCode INTERFACE_FUNC getComponents(IList** components) override;
    ErrCode INTERFACE_FUNC getComponent(IString* name, ILoggerComponent** component) override;

    ErrCode INTERFACE_FUNC flush() override;
    ErrCode INTERFACE_FUNC flushOnLevel(LogLevel level) override;

private:
    void flushComponents();
    void flushSinks();
private:
    std::mutex addComponentMutex;
    std::vector<LoggerSinkPtr> sinks;
    std::unordered_map<std::string, LoggerComponentPtr> components;
    LoggerThreadPoolPtr threadPool;
    LogLevel level;
    LoggerFlushWorkerPtr periodicFlushWorker;
    LogLevel flushLevel;
};

END_NAMESPACE_OPENDAQ
