#pragma once

#include <opendaq/opendaq.h>

#include <ws-streaming/local_signal.hpp>

#include <websocket_streaming_server_module/common.h>

BEGIN_NAMESPACE_OPENDAQ_NEWER_WEBSOCKET_STREAMING_SERVER_MODULE

class WsStreamingListener
    : public ImplementationOfWeak<IInputPortNotifications>
{
    public:

        WsStreamingListener(
            IContext *context,
            ISignal *signal,
            wss::local_signal *localSignal);

        void start();

        virtual ErrCode INTERFACE_FUNC acceptsSignal(IInputPort *port, ISignal *signal, Bool *accept) override;
        virtual ErrCode INTERFACE_FUNC connected(IInputPort *port) override;
        virtual ErrCode INTERFACE_FUNC disconnected(IInputPort *port) override;
        virtual ErrCode INTERFACE_FUNC packetReceived(IInputPort *port) override;

    private:

        void onDataPacketReceived(DataPacketPtr packet);

    private:

        SignalPtr _signal;
        InputPortConfigPtr _port;
        DataDescriptorPtr _lastDescriptor;
        wss::local_signal& _localSignal;
        bool _ruleType;
};

END_NAMESPACE_OPENDAQ_NEWER_WEBSOCKET_STREAMING_SERVER_MODULE
