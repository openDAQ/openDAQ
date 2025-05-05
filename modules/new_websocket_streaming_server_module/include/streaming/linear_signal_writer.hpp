#pragma once

#include <opendaq/opendaq.h>

#include "signal_writer.hpp"
#include "websocket_client_established.hpp"
#include "websocket_protocol.hpp"

namespace daq::ws_streaming
{
    /**
     * A signal_writer implementation for linear-value signals.
     *
     * @todo This is not implemented! No packets are
     */
    class linear_signal_writer : public signal_writer
    {
        public:

            /**
             * @copydoc signal_writer::write()
             *
             * @param signal A reference to the openDAQ signal object.
             */
            linear_signal_writer(unsigned signo, websocket_client_established& client)
                : signal_writer(signo, client)
            {
            }

            /**
             * @copydoc signal_writer::write(daq::DataPacketPtr)
             *
             * @perfcrit This function is called once per subscribed client for every openDAQ packet
             *     received from explicit signals.
             */
            bool write(daq::DataPacketPtr packet) override
            {
                return true;
            }
    };
}
