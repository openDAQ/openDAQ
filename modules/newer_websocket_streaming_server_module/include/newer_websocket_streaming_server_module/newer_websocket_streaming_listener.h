#pragma once

#include <opendaq/opendaq.h>

#include <ws-streaming/ws-streaming.hpp>

#include <newer_websocket_streaming_server_module/common.h>

BEGIN_NAMESPACE_OPENDAQ_NEWER_WEBSOCKET_STREAMING_SERVER_MODULE

class NewerWebsocketStreamingListenerImpl
    : public ImplementationOfWeak<IInputPortNotifications>
{
    public:

        NewerWebsocketStreamingListenerImpl(
            IContext *context,
            ISignal *signal,
            wss::local_signal *localSignal);

        ~NewerWebsocketStreamingListenerImpl();

        void start();

        virtual ErrCode INTERFACE_FUNC acceptsSignal(IInputPort *port, ISignal *signal, Bool *accept) override;
        virtual ErrCode INTERFACE_FUNC connected(IInputPort *port) override;
        virtual ErrCode INTERFACE_FUNC disconnected(IInputPort *port) override;
        virtual ErrCode INTERFACE_FUNC packetReceived(IInputPort *port) override;

    private:

        wss::metadata_builder buildMetadata(const DataDescriptorPtr& descriptor);

        void onDataPacketReceived(DataPacketPtr packet);

    private:

        SignalPtr signal;
        InputPortConfigPtr port;
        DataDescriptorPtr lastDescriptor;
        wss::local_signal& localSignal;
};

OPENDAQ_DECLARE_CLASS_FACTORY_WITH_INTERFACE(
    INTERNAL_FACTORY, NewerWebsocketStreamingListener, IInputPortNotifications,
    IContext *, context,
    ISignal *, signal,
    wss::local_signal *, localSignal
)

END_NAMESPACE_OPENDAQ_NEWER_WEBSOCKET_STREAMING_SERVER_MODULE
