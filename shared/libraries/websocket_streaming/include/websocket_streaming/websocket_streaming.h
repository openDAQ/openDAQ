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


#define BEGIN_NAMESPACE_OPENDAQ_WEBSOCKET_STREAMING \
    namespace daq::websocket_streaming              \
    {
#define END_NAMESPACE_OPENDAQ_WEBSOCKET_STREAMING }

#include <memory>

namespace daq::streaming_protocol
{
    static const uint16_t WEBSOCKET_LISTENING_PORT = 7414;
    static const uint16_t HTTP_CONTROL_PORT = 7438;
    class SubscribedSignal;
    using SubscribedSignalPtr = std::shared_ptr<SubscribedSignal>;

    class ProtocolHandler;
    using ProtocolHanlderPtr = std::shared_ptr<ProtocolHandler>;

    class StreamWriter;
    using StreamWriterPtr = std::shared_ptr<StreamWriter>;

    class BaseSignal;
    using BaseSignalPtr = std::shared_ptr<BaseSignal>;

    class BaseValueSignal;
    using BaseValueSignalPtr = std::shared_ptr<BaseValueSignal>;

    class BaseSynchronousSignal;
    using BaseSynchronousSignalPtr = std::shared_ptr<BaseSynchronousSignal>;

    class BaseDomainSignal;
    using BaseDomainSignalPtr = std::shared_ptr<BaseDomainSignal>;

    class LinearTimeSignal;
    using LinearTimeSignalPtr = std::shared_ptr<LinearTimeSignal>;

    class BaseConstantSignal;
    using BaseConstantSignalPtr = std::shared_ptr<BaseConstantSignal>;
}

namespace daq::stream
{
    class Stream;
    using StreamPtr = std::shared_ptr<Stream>;

    class WebsocketServer;
    using WebsocketServerPtr = std::shared_ptr<WebsocketServer>;
    using WebsocketServerUniquePtr = std::unique_ptr<WebsocketServer>;

    class WebsocketClientStream;
    using WebsocketClientStreamPtr = std::shared_ptr<WebsocketClientStream>;
}
