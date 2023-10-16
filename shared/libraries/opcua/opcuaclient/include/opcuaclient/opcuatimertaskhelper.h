/*
 * Copyright 2022-2023 Blueberry d.o.o.
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

BEGIN_NAMESPACE_OPENDAQ_OPCUA

class OpcUaTimerTaskHelper;
using OpcUaTimerTaskHelperPtr = std::shared_ptr<OpcUaTimerTaskHelper>;

class OpcUaTimerTaskHelper  // Similar API as CommonLib::TimerThread
{
public:
    using CallbackFunction = std::function<void(OpcUaClient&)>;

    explicit OpcUaTimerTaskHelper(OpcUaClient& client, int intervalMs, const CallbackFunction& callback);
    virtual ~OpcUaTimerTaskHelper();

    virtual void start();
    virtual void stop();

    double getIntervalMs() const;
    void setIntervalMs(const double value);

    void terminate();
    const std::atomic<bool>& getTerminated() const;

    bool getStarted() const;

protected:
    virtual void execute(OpcUaClient& client, TimerTaskControl& control);

    double intervalMs;
    std::atomic<bool> terminated;
    std::optional<OpcUaCallbackIdent> callbackIdent;
    OpcUaClient& client;
    CallbackFunction callback;
};

END_NAMESPACE_OPENDAQ_OPCUA
