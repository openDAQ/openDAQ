#pragma once

#include <cstdint>
#include <list>
#include <map>
#include <memory>
#include <string>
#include <thread>

#include <boost/asio.hpp>

#include <nlohmann/json.hpp>

#include <opendaq/opendaq.h>

#include "websocket_client.hpp"
#include "websocket_client_established.hpp"
#include "websocket_client_negotiating.hpp"

namespace daq::ws_streaming
{
    /**
     * The root class for the WebSocket Streaming Protocol server. The server object enumerates
     * the publicly-exposed openDAQ signals and attaches a WebSocketSignalListenerImpl object to
     * each one. It also creates a single worker thread which handles incoming connections in an
     * `epoll()`-based event loop. This worker thread is responsible for creating websocket_client
     * objects to manage both control and WebSocket connections, and for detecting and destroying
     * disconnected clients.
     */
    class server
    {
        public:

            /**
             * Creates a new server object listening on the specified ports. First, the
             * device's publicly-exposed signals are enumerated, and listener objects are attached
             * to each signal. Next, the worker thread described in the class overview is created,
             * which opens the specified TCP ports and begins waiting for connections.
             *
             * @param device A reference to the openDAQ device hosting the server. This device's
             *     publicly-exposed signals are enumerated.
             * @param ws_port The TCP port number on which to listen for WebSocket connections.
             * @param control_port The TCP port number on which to listen for control connections.
             *
             * @throws std::exception An error occurred.
             */
            server(daq::DevicePtr device, std::uint16_t ws_port, std::uint16_t control_port);

            /**
             * Gracefully stops and joins the worker thread.
             *
             * @todo All listeners should be detached to prevent zombie listeners referenced by
             *     the public output signals. This should not cause a crash, because the listeners
             *     do not reference the server object, and only hold weak references to the
             *     clients, which will therefore be properly destroyed.
             */
            ~server();

            server(const server&) = delete;
            server& operator=(const server&) = delete;

            /**
             * Gracefully stops and joins the worker thread.
             *
             * @todo All listeners should be detached to prevent zombie listeners referenced by
             *     the public output signals. This should not cause a crash, because the listeners
             *     do not reference the server object, and only hold weak references to the
             *     clients, which will therefore be properly destroyed.
             */
            void stop();

        private:

            void async_accept_connection(boost::asio::ip::tcp::acceptor& acceptor);
            void thread_main();

            bool subscribe(
                std::weak_ptr<websocket_client_established> weak_client,
                const std::string& signal_id,
                bool is_implicit = false);

            bool unsubscribe(
                std::weak_ptr<websocket_client_established> weak_client,
                const std::string& signal_id,
                bool is_implicit = false);

            bool on_establish(
                std::weak_ptr<websocket_client_negotiating> weak_client,
                std::list<std::shared_ptr<websocket_client>>::iterator client_it);

            bool on_request(const nlohmann::json& content);

            std::uint16_t ws_port;
            std::uint16_t control_port;

            boost::asio::io_context ioc;

            std::thread thread;

            std::list<std::shared_ptr<websocket_client>> clients;
            nlohmann::json available;

            std::map<std::string, daq::InputPortNotificationsPtr> listeners;
    };
}
