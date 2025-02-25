#pragma once

#include <opendaq/opendaq.h>

#include "signal_writer.hpp"
#include "websocket_client_established.hpp"

namespace daq::ws_streaming
{
    /**
     * A signal_writer implementation for explicit signals. For explicit signals, the raw contents
     * of each received data packet are transmitted directly as a single WebSocket Streaming
     * Protocol signal data packet.
     */
    struct explicit_signal_writer : signal_writer
    {
        /**
         * @copydoc signal_writer::signal_writer()
         */
        explicit_signal_writer(unsigned signo, websocket_client_established& client)
            : signal_writer(signo, client)
        {
        }

        /**
         * @copydoc signal_writer::write()
         *
         * @perfcrit This function is called once per subscribed client for every openDAQ packet
         *     received from explicit signals.
         */
        bool write(daq::DataPacketPtr packet) override
        {
            return client.send_data(signo, packet.getRawData(), packet.getDataSize());
        }
    };
}
