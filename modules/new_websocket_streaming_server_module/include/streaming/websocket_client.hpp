#pragma once

#include <functional>
#include <utility>

#include <boost/asio.hpp>

#include <fcntl.h>

namespace daq::ws_streaming
{
    /**
     * An abstract base class for streaming client objects in some phase of their lifecycle. One
     * derived class exists to handle clients which are in the HTTP negotiation phase. Another
     * derived class exists to handle clients which have completed the HTTP negotiation and are
     * able to send and receive WebSocket frames. The base class covers the phase-independent
     * functionality used by the websocket_server. When a client completes HTTP negotiation, the
     * server replaces the client object with a new object of the correct derived type.
     *
     * Clients place their sockets in nonblocking mode. The server should notify the client when
     * data is available to be read by calling service().
     */
    class websocket_client
    {
        public:

            /**
             * Constructs a new client object.
             *
             * @param socket The client's socket. Ownership of the socket is transferred to the
             *     new object. After the call, @p socket no longer refers to a socket.
             */
            websocket_client(boost::asio::ip::tcp::socket&& socket)
                : socket(std::move(socket))
            {
                this->socket.non_blocking(true);
                async_service();
            }

            /**
             * Client objects are not copyable (or moveable).
             */
            websocket_client(const websocket_client&) = delete;

            /**
             * Client objects are not copyable (or moveable).
             */
            websocket_client& operator=(const websocket_client&) = delete;

            /**
             * Client objects must be deleteable from references to the base class.
             */
            virtual ~websocket_client()
            {
                close_socket();
            }

            /**
             * Notifies the client that data is available to be read. The derived class
             * implementation should read and process all available data, and return a flag
             * indicating whether an error has occurred.
             *
             * Derived class implementations should avoid throwing exceptions. However, any
             * exception thrown should be considered equivalent to a false return value.
             *
             * @return false if an error occurred. In that case the caller should destroy the
             *     client object (thereby disconnecting the client). Otherwise, true.
             */
            virtual bool service() = 0;

            /**
             * Gets a reference to the client's socket without releasing or transferring
             * ownership.
             *
             * @return A reference to the client's socket. If release() was previously called,
             *     the returned socket is not connected.
             */
            boost::asio::ip::tcp::socket& get_socket() noexcept
            {
                return socket;
            }

            /**
             * Releases ownership of the client's socket and returns it. Future calls to service()
             * will fail (and return false) and get_socket() will return -1. This function can be
             * used when replacing a client object with a new object of a different derived type.
             *
             * @return The client's socket.
             */
            boost::asio::ip::tcp::socket release() noexcept
            {
                boost::asio::ip::tcp::socket released_socket = std::move(socket);
                return released_socket;
            }

            /**
             * Explicitly closes the socket if it is open.
             */
            void close_socket() noexcept
            {
                if (socket.is_open())
                {
                    boost::system::error_code ec;
                    socket.shutdown(boost::asio::ip::tcp::socket::shutdown_both, ec);
                    socket.close(ec);
                }
            }

            std::function<void()> on_finish;

        protected:

            void async_service()
            {
                socket.async_wait(boost::asio::socket_base::wait_read, [this](boost::system::error_code ec)
                {
                    if (!socket.is_open())
                        return;
                    else if (!ec && service() && socket.is_open())
                        async_service();
                    else if (on_finish)
                        on_finish();
                });
            }

            /**
             * A client object owns its socket.
             */
            boost::asio::ip::tcp::socket socket;
    };
}
