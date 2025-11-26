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

#include <opendaq/opendaq.h>

#include <ws-streaming/local_signal.hpp>

#include <websocket_streaming/common.h>

BEGIN_NAMESPACE_OPENDAQ_WEBSOCKET_STREAMING

class WsStreamingListener
    : public ImplementationOfWeak<IInputPortNotifications>
{
    public:

        WsStreamingListener(
            IContext *context,
            ISignal *signal,
            wss::local_signal *localSignal);

        ~WsStreamingListener() override;

        void start();

        virtual ErrCode INTERFACE_FUNC acceptsSignal(IInputPort *port, ISignal *signal, Bool *accept) override;
        virtual ErrCode INTERFACE_FUNC connected(IInputPort *port) override;
        virtual ErrCode INTERFACE_FUNC disconnected(IInputPort *port) override;
        virtual ErrCode INTERFACE_FUNC packetReceived(IInputPort *port) override;

    private:

        void onDataPacketReceived(DataPacketPtr packet);
        void onEventPacketReceived(EventPacketPtr packet);

    private:

        SignalPtr           _signal;
        InputPortConfigPtr  _port;
        DataDescriptorPtr   _lastDescriptor;
        wss::local_signal&  _localSignal;
        bool                _ruleType;
};

END_NAMESPACE_OPENDAQ_WEBSOCKET_STREAMING
