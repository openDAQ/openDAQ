#pragma once

#include <opendaq/opendaq.h>

#include "websocket_client_established.hpp"

namespace daq::ws_streaming
{
    /**
     * An abstract base class for signal writers. Signal writers handle the details of
     * transmitting WebSocket Streaming Protocol signal data frame(s) for a received openDAQ data
     * packet object. Signal writer implementations exist for each "flavor" of supported data
     * packets, such as explicit signals and constant-value signals.
     *
     * All signal writer objects are configured with the Streaming Protocol "signo" number and a
     * reference to the client object. The openDAQ packet listener then invokes write() for each
     * received data packet. The implementation is then expected to synchronously transmit zero or
     * more WebSocket data frames to the referenced client based on the contents of the packet.
     * This is usually done by calling websocket_client_established::send_data().
     *
     * Some signal writers may need to hold additional state, such as the last-transmitted value
     * for a constant-value signal where new values are transmitted only when a change is
     * detected.
     */
    class signal_writer
    {
        public:

            /**
             * Constructs a new signal writer object.
             *
             * @param signo The Streaming Protocol "signo" number for this subscribed signal.
             * @param client A reference to the WebSocket client to write to.
             */
            signal_writer(unsigned signo, websocket_client_established& client)
                : signo(signo)
                , client(client)
            {
            }

            /**
             * Destroys a signal writer object.
             */
            virtual ~signal_writer()
            {
            }

            /**
             * Transmits zero or more WebSocket data frames based on the contents of a received
             * openDAQ data packet. The implementation is expected to synchronously transmit frames,
             * but without blocking, typically by calling websocket_client_established::send_data().
             *
             * @perfcrit This function is called once per subscribed client for every openDAQ
             *     packet received.
             *
             * @param packet A smart pointer holding a received openDAQ data packet.
             *
             * @return true if all necessary WebSocket frame(s) were successfully transmitted, or
             *     false if an error occurred, such as a full TCP send buffer or disconnected
             *     client.
             */
            virtual bool write(daq::DataPacketPtr packet) = 0;

        protected:

            /**
             * The Streaming Protocol "signo" number for this subscribed signal.
             */
            unsigned signo;

            /**
             * A reference to the WebSocket client to write to.
             */
            websocket_client_established& client;

    };
}
