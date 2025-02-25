#pragma once

#include <cstdint>
#include <memory>
#include <optional>
#include <utility>

#include "signal_writer.hpp"
#include "websocket_client_established.hpp"

namespace daq::ws_streaming
{
    /**
     * A bookkeeping data structure used by WebSocketSignalListenerImpl to keep track of clients
     * which have been added to the listener's client list. Each subscribed_client object owns a
     * shared pointer to the WebSocket client object and a unique pointer to a signal_writer
     * object appropriate for the type of signal handled by the listener.
     */
    struct subscribed_client
    {
        /**
         * Constructs a new subscribed_client object.
         *
         * @param client A shared pointer to the WebSocket client object.
         * @param writer An rvalue reference to a unique pointer to a signal_writer object
         *     appropriate for the type of signal handled by the listener. The subscribed_client
         *     object takes ownership of this pointer.
         */
        subscribed_client(
                const std::shared_ptr<websocket_client_established>& client,
                std::unique_ptr<signal_writer>&& writer)
            : client(client)
            , writer(std::move(writer))
        {
        }

        /**
         * A shared pointer to the WebSocket client object.
         */
        std::weak_ptr<websocket_client_established> client;

        /**
         * An rvalue reference to a unique pointer to a signal_writer object appropriate for the
         * type of signal handled by the listener.
         */
        std::unique_ptr<signal_writer> writer;

        /**
         * The metadata object describing the signal to which this client is subscribed. This is
         * assigned by the constructor, and can be later updated if a descriptor-change event is
         * received. We must save this data because some properties (specifically those attached
         * to the signal itself rather than the descriptor) cannot be re-retrieved inside the
         * change event itself (the change event occurs while holding the openDAQ sync lock).
         */
        std::string id;
        std::string description;
        std::string domain_signal_id;
        bool is_implicit = false;

        /**
         * The number of data packets transmitted to this client since the last "signal" metadata
         * update. We need to track this value to be able to figure out what timestamp the client
         * will see at any given time, so we can back-calculate the domain signal value the client
         * needs to see to arrive at the correct timestamp.
         */
        std::int64_t samples_since_signal_update = 0;

        /**
         * The last domain value that was written to this client.
         */
        std::optional<std::int64_t> domain_value = std::nullopt;
    };
}
