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

#include <opcuaclient/opcuaclient.h>

#include <opendaq/utils/thread_ex.h>

#include <future>
#include <set>
#include <queue>

BEGIN_NAMESPACE_OPENDAQ_OPCUA


class OpcUaTaskProcessor;
using OpcUaTaskProcessorPtr = std::shared_ptr<OpcUaTaskProcessor>;

class OpcUaTaskProcessor : protected daq::utils::ThreadEx
{
public:
    explicit OpcUaTaskProcessor(const OpcUaClientPtr& client);
    virtual ~OpcUaTaskProcessor();

    void start() override;
    void stop() override;

    bool connect();
    bool isConnected();
    void setConnectionTimeout(uint32_t timeoutMs);
    const OpcUaClientPtr& getClient() const noexcept;

    void executeTask(const OpcUaTaskType& task);

    std::future<void> executeTaskAwait(const OpcUaTaskType& task);

protected:
    using QueueItemType = std::pair<std::promise<void>, OpcUaTaskType>;

    void executeTask(std::promise<void>& promise, const OpcUaTaskType& task) const;

    void execute() override;

    QueueItemType getNextTask();

    OpcUaClientPtr client;

    std::atomic<std::thread::id> executingThreadId;

    std::queue<QueueItemType> queue;
    std::mutex mutex;

    std::atomic<bool> connected;
    UA_StatusCode prevStatus;

    std::condition_variable cvWait;
    std::mutex waitMutex;
};

END_NAMESPACE_OPENDAQ_OPCUA
