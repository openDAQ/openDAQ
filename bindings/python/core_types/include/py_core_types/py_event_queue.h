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
#include <coretypes/objectptr.h>
#include <coretypes/common.h>
#include <pybind11/pybind11.h>
#include "py_core_types/py_queued_event_handler.h"

#include <queue>
#include <mutex>
#include <functional>

class PyEventQueue
{
public:
    static std::shared_ptr<PyEventQueue> Create();
    static std::weak_ptr<PyEventQueue> GetWeak();

    void enqueueEvent(daq::IPythonQueuedEventHandler* eventHandler, daq::IBaseObject* sender, daq::IEventArgs* eventArgs);
    void processEvents();
    void clearQueue();

private:
    PyEventQueue() = default;
    std::queue<std::function<void()>> callbackQueue;
    std::mutex callbackQueueMutex;
};

pybind11::class_<PyEventQueue, std::shared_ptr<PyEventQueue>> declarePyEventQueue(pybind11::module_ m);
void definePyEventQueue(pybind11::module_ m, pybind11::class_<PyEventQueue, std::shared_ptr<PyEventQueue>> cls);