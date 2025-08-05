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

#include <atomic>
#include <thread>

#include <opendaq/function_block_impl.h>
#include <opendaq/opendaq.h>

BEGIN_NAMESPACE_OPENDAQ_WEBSOCKET_STREAMING_SERVER_MODULE

class WsStreamTestFb : public FunctionBlock
{
    public:

        static constexpr const char *ID = "WsStreamTestFb";

        static FunctionBlockTypePtr createType();

        explicit WsStreamTestFb(
            const ContextPtr& ctx,
            const ComponentPtr& parent,
            const StringPtr& localId);

    private:

        void reconfigure();
        void threadFunc();

        unsigned _sampleRate = 1000;

        DataDescriptorPtr _domainDescriptor;

        SignalConfigPtr _linearDomainSignal;
        SignalConfigPtr _constantValueSignal;
        SignalConfigPtr _explicitValueSignal1;
        SignalConfigPtr _explicitValueSignal2;

        std::atomic<bool> _exit = false;
        std::thread _thread;
};

END_NAMESPACE_OPENDAQ_WEBSOCKET_STREAMING_SERVER_MODULE
