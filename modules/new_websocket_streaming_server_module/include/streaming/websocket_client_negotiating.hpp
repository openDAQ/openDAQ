#pragma once

#include <array>
#include <cstddef>
#include <functional>

#include <boost/asio.hpp>

#include <httpparser/httprequestparser.h>
#include <httpparser/request.h>

#include <nlohmann/json.hpp>

#include "websocket_client.hpp"

namespace daq::ws_streaming
{
    /**
     * A websocket_client class for handling WebSocket clients that have connected and are in the
     * HTTP negotiation phase.
     *
     * completed the
     * WebSocket upgrade negotiation and are able to send and receive WebSocket frames. Such
     * clients are capable of being added to WebSocketSignalListenerImpl's client list.
     *
     * Established clients also provide functions for transmitting WebSocket Streaming Protocol
     * data and metadata packets. These functions can be called by the WebSocketSignalListenerImpl
     * (possibly via a signal_writer) to transmit streaming data.
     */
    class websocket_client_negotiating : public websocket_client
    {
        public:

            websocket_client_negotiating(boost::asio::ip::tcp::socket&& socket);

            bool service() override;

            std::function<bool()> on_establish;
            std::function<bool(const nlohmann::json&)> on_request;

        protected:

            static constexpr std::size_t read_buffer_size = 16 * 1024;

            std::array<char, read_buffer_size> read_buffer;
            httpparser::HttpRequestParser http_request_parser;
            httpparser::Request http_request;
    };
}
