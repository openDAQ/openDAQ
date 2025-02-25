#pragma once

#include <cstdint>
#include <optional>

#include <opendaq/opendaq.h>

#include "signal_writer.hpp"
#include "websocket_client_established.hpp"
#include "websocket_protocol.hpp"

namespace daq::ws_streaming
{
    /**
     * A signal_writer implementation for constant-value signals. For such signals, a WebSocket
     * Streaming Protocol signal data packet is generated each time the value is observed to
     * change (and once for the first received value). The generated data packet contains the
     * sample index where the change occurred and the new constant value.
     */
    class constant_signal_writer : public signal_writer
    {
        public:

            /**
             * @copydoc signal_writer::write()
             */
            constant_signal_writer(unsigned signo, websocket_client_established& client)
                : signal_writer(signo, client)
            {
            }

            /**
             * @copydoc signal_writer::write()
             *
             * @perfcrit This function is called once per subscribed client for every openDAQ packet
             *     received from explicit signals.
             *
             * @todo The sample index is set to zero. How is the sample index determined in openDAQ?
             */
            bool write(daq::DataPacketPtr packet) override
            {
                if (packet.getRawDataSize() < sizeof(std::uint64_t))
                    return true;

                std::int64_t new_value = *reinterpret_cast<const std::int64_t *>(packet.getRawData());

                if (new_value != last_value.value_or(~new_value))
                {
                    last_value = new_value;
                    websocket_protocol::constant_value_packet data { .index = 0, .value = new_value };
                    if (!client.send_data(signo, &data, sizeof(data)))
                        return false;
                }

                return true;
            }

        private:

            /**
             * The last-sent value, or std::nullopt if no value has been sent yet.
             */
            std::optional<std::int64_t> last_value;
    };
}
