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

/*
Helper class for OpcUaTaskProcessor
*/

#pragma once

#include <opcuashared/opcua.h>

#include <unordered_map>
#include <cassert>
#include <functional>

BEGIN_NAMESPACE_OPENDAQ_OPCUA

class OpcUaClient;

struct TimerTaskControl
{
    bool terminate{false};
    void terminateTimerTask()
    {
        terminate = true;
    }
};

using OpcUaTaskType = std::function<void(OpcUaClient&)>;
using OpcUaTimerTaskType = std::function<void(OpcUaClient&, TimerTaskControl&)>;
using OpcUaCallbackIdent = UA_UInt64;

class TimerTaskContextCollection
{
public:
    explicit TimerTaskContextCollection();

    void* createContext();
    void deleteContext(void* context);

    void insertTimerTask(void* context, OpcUaCallbackIdent ident, const OpcUaTimerTaskType& task);
    void removeTimerTask(OpcUaCallbackIdent ident);

    bool timerTaskExists(OpcUaCallbackIdent ident) const;

    static void getTaskExecData(void* context, OpcUaCallbackIdent* callbackIdent, OpcUaTimerTaskType** task);

protected:
    struct KeyType
    {
        KeyType()
            : owner(nullptr)
        {
        }

        explicit KeyType(TimerTaskContextCollection* owner)
            : owner(owner)
        {
        }
        OpcUaCallbackIdent ident{};
        TimerTaskContextCollection* owner;
    };

    struct KeyTypeHash
    {
        std::size_t operator()(const KeyType* k) const
        {
            return std::hash<OpcUaCallbackIdent>()(k->ident);
        }
    };

    struct KeyTypeEqualTo
    {
        bool operator()(const KeyType* a, const KeyType* b) const
        {
            assert(a != nullptr && b != nullptr);
            return a->ident == b->ident;
        }
    };

    std::unordered_map<KeyType*, OpcUaTimerTaskType, KeyTypeHash, KeyTypeEqualTo> taskCallbacks;
};

END_NAMESPACE_OPENDAQ_OPCUA
