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

#include <native_streaming_protocol/server_session_handler.h>

#include <opendaq/context_ptr.h>
#include <opendaq/logger_ptr.h>
#include <opendaq/logger_component_ptr.h>
#include <opendaq/signal_ptr.h>

BEGIN_NAMESPACE_OPENDAQ_NATIVE_STREAMING_PROTOCOL

using SendToClientCallback = std::function<void(std::shared_ptr<ServerSessionHandler>& sessionHandler)>;

class SubscribersRegistry
{
public:
    explicit SubscribersRegistry(const ContextPtr& context);

    void sendToClients(SendToClientCallback sendCallback);
    void sendToSubscribers(const SignalPtr& signal, SendToClientCallback sendCallback);
    void sendToClient(SessionPtr session, SendToClientCallback sendCallback);

    void registerSignal(const SignalPtr& signal);
    bool removeSignal(const SignalPtr& signal);

    void registerClient(std::shared_ptr<ServerSessionHandler> sessionHandler);
    std::vector<std::string> unregisterClient(SessionPtr session);

    bool registerSignalSubscriber(const std::string& signalStringId, SessionPtr session);
    bool removeSignalSubscriber(const std::string& signalStringId, SessionPtr session);

private:
    ContextPtr context;
    LoggerPtr logger;
    LoggerComponentPtr loggerComponent;

    std::unordered_map<std::string, std::vector<std::shared_ptr<ServerSessionHandler>>> signalsSubscribers;
    std::vector<std::shared_ptr<ServerSessionHandler>> sessionHandlers;
    std::mutex sync;
};

END_NAMESPACE_OPENDAQ_NATIVE_STREAMING_PROTOCOL
