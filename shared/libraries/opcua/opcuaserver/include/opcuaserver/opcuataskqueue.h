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
#include <opcuashared/opcua.h>
#include <mutex>
#include <queue>
#include <condition_variable>
#include <functional>

BEGIN_NAMESPACE_OPENDAQ_OPCUA

class OpcUaTaskQueue
{
public:
    static constexpr unsigned int infinity = 0;
    using Function = std::function<void()>;

    void push(Function&& func);
    bool pop(Function& func);
    bool pop(Function& func, unsigned int timeout_ms);

    void processTaskQueue();

private:
    std::mutex mutex_;
    std::condition_variable cv_;
    std::deque<Function> q_;
};

END_NAMESPACE_OPENDAQ_OPCUA
