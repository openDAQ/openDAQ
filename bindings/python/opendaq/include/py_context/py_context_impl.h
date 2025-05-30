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

#include <opendaq/context_impl.h>
#include "py_context/py_context.h"
#include "py_core_types/py_queued_event_handler.h"

#include <queue>
#include <mutex>
#include <functional>


BEGIN_NAMESPACE_OPENDAQ

class PythonContextImpl: public GenericContextImpl<IPythonContext>
{
public:
    using Super = GenericContextImpl<IPythonContext>;
    using Super::Super;

    ErrCode INTERFACE_FUNC setEventToQueue(IPythonQueuedEventHandler* eventHandler, IBaseObject* sender, IEventArgs* eventArgs) override;
    ErrCode INTERFACE_FUNC processEventsFromQueue()override;

private:
    using callbackT = std::function<ErrCode()>;
    std::queue<callbackT> callbackQueue;
    std::mutex callbackQueueMutex;
};

END_NAMESPACE_OPENDAQ