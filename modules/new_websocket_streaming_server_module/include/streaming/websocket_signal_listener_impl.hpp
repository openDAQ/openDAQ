#pragma once

#include <cstddef>
#include <cstdint>
#include <functional>
#include <list>
#include <memory>
#include <mutex>
#include <utility>

#include <opendaq/opendaq.h>

#include "signal_writer_factory.hpp"
#include "subscribed_client.hpp"
#include "websocket_client_established.hpp"

namespace daq::ws_streaming
{
    class WebSocketSignalListenerImpl : public daq::ImplementationOfWeak<daq::IInputPortNotifications>
    {
        public:

            WebSocketSignalListenerImpl(daq::IDevice *device_ptr, daq::ISignal *signal_ptr, unsigned signo);

            virtual daq::ErrCode acceptsSignal(daq::IInputPort *port, daq::ISignal *signal, daq::Bool *accept) override;
            virtual daq::ErrCode connected(daq::IInputPort *port) override;
            virtual daq::ErrCode disconnected(daq::IInputPort *port) override;
            virtual daq::ErrCode packetReceived(daq::IInputPort *port) override;

            daq::SignalPtr getSignal() { return signal; }
            unsigned getSigno() const noexcept { return signo; }
            bool addClient(std::shared_ptr<websocket_client_established> client, bool isImplicit);
            void removeClient(boost::asio::detail::socket_type socket, bool onlyIfImplicit);

            void linkDomainSignal(const std::function<WebSocketSignalListenerImpl *(const std::string& name)>& findListener);
            void start();

        private:

            std::pair<std::int64_t, std::int64_t> checkDomainPacket(daq::DataPacketPtr packet);

            daq::SignalPtr signal;
            daq::InputPortConfigPtr port;

            unsigned signo;
            std::mutex mutex;
            signal_writer_factory writer_factory;
            std::list<subscribed_client> clients;

            WebSocketSignalListenerImpl *domain_listener = nullptr;

            daq::DataPacketPtr last_packet;
            daq::DataDescriptorPtr last_descriptor;
            std::int64_t linear_start = 0;
            std::int64_t linear_delta = 1;
            std::int64_t expected_value = std::numeric_limits<std::int64_t>::min();
    };

    OPENDAQ_DECLARE_CLASS_FACTORY_WITH_INTERFACE(
        INTERNAL_FACTORY, WebSocketSignalListener, daq::IInputPortNotifications,
        daq::IDevice *, device,
        daq::ISignal *, signal,
        unsigned, signo
    )
}
