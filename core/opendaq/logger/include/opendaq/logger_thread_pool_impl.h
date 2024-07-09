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
#include <opendaq/logger_thread_pool.h>
#include <opendaq/logger_thread_pool_private.h>
#include <coretypes/intfs.h>

#include <memory>

BEGIN_NAMESPACE_OPENDAQ

class LoggerThreadPoolImpl final : public ImplementationOf<ILoggerThreadPool, ILoggerThreadPoolPrivate>
{
public:
    LoggerThreadPoolImpl();

    // ILoggerThreadPoolPrivate
    ErrCode INTERFACE_FUNC getThreadPoolImpl(ThreadPoolPtr *impl) override;

private:
    ThreadPoolPtr spdlogThreadPool;
};

END_NAMESPACE_OPENDAQ
