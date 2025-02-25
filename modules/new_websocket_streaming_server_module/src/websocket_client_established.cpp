#include <array>
#include <cerrno>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <utility>

#include <boost/asio.hpp>

#include "streaming_protocol.hpp"
#include "websocket_client.hpp"
#include "websocket_client_established.hpp"
#include "websocket_protocol.hpp"

static std::string calculate_stream_id(boost::asio::ip::tcp::socket& socket)
{
    // Return a string of the form A.B.C.D:PORT where A, B, C and D are the four IP address octets.
    auto endpoint = socket.remote_endpoint();
    return endpoint.address().to_string() + ":" + std::to_string(endpoint.port());
}

daq::ws_streaming::websocket_client_established::websocket_client_established(boost::asio::ip::tcp::socket&& socket)
    : websocket_client(std::move(socket))
    , stream_id(calculate_stream_id(this->socket))
{
    // Request a TCP send buffer large enough to hold 5
    // seconds worth of data for 6 real64 channels at 131072 Hz.
    // NOTE: The kernel doubles this size, reserving half of it for bookkeeping!
    // XXX TODO
    int requested_size = 131072 * 6 * 8 * 5;
    boost::system::error_code ec;
    boost::asio::socket_base::send_buffer_size option(requested_size);
    this->socket.set_option(option, ec);
    this->socket.get_option(option, ec);
    if (ec)
        std::cerr << "[ws-streaming] failed to get/set TCP send buffer size: " << ec << std::endl;
    else if (option.value() < requested_size)
    {
        std::cerr << "[ws-streaming] TCP send buffer size is less than requested: " << option.value() << " < " << requested_size << std::endl;
        std::cerr << "[ws-streaming] This may result in unreliable streaming connections!" << std::endl;
    }
}

bool daq::ws_streaming::websocket_client_established::service()
{
    boost::system::error_code ec;

    // Read any available data from the socket into the read buffer.
    std::size_t bytes_read = socket.read_some(boost::asio::buffer(&read_buffer[buffered_bytes], read_buffer.size() - buffered_bytes), ec);

    // If a socket error occurred...
    if (ec)
    {
        // If there's no data ready after all (this should only happen if the caller called service() wrongly)...
        if (ec == boost::asio::error::would_block)
            return true;

        // Otherwise a genuine error.
        std::cerr << "[ws-streaming] client (established): receive error, errno " << ec << std::endl;
        return false;
    }

    // If the socket was closed...
    else if (bytes_read == 0)
    {
        std::cerr << "[ws-streaming] client (established): client disconnected (recv 0)" << std::endl;
        return false;
    }

    // Otherwise valid data was read.
    buffered_bytes += bytes_read;

    // Process as many WebSocket frames as possible.
    while (true)
    {
        // Try to decode the WebSocket header.
        auto header = websocket_protocol::decode_header(read_buffer.data(), buffered_bytes);

        // If there's not enough data to form a complete frame, we can't process any more.
        if (!header.header_size)
            break;

        // We have a valid and complete WebSocket frame.
        switch (header.opcode)
        {
            // React to close frames by sending our own close frame and then signaling the caller to disconnect.
            case websocket_protocol::opcodes::CLOSE:
            {
                std::array<std::uint8_t, 2> close_frame = { 0x88, 0x00 };
                socket.write_some(boost::asio::buffer(close_frame), ec);
                return false;
            }

            // React to any other frames by ignoring them.
            default:
                break;
        }

        // Consume the handled frame by sliding the remaining data in the read buffer over
        // to the left. (Can't use std::memcpy() for this because the ranges overlap.)
        std::memmove(
            &read_buffer[0],
            &read_buffer[header.header_size + header.payload_size],
            buffered_bytes - header.header_size + header.payload_size);
        buffered_bytes -= header.header_size + header.payload_size;
    }

    // If the read buffer is still full after processing, it is an error condition
    // (the client must be sending a frame larger than our fixed-size read buffer).
    if (buffered_bytes == read_buffer.size())
        return false;

    return true;
}

bool daq::ws_streaming::websocket_client_established::send_metadata(unsigned signo, const nlohmann::json& metadata)
{
    std::array<std::uint8_t, streaming_protocol::MAX_HEADER_SIZE + sizeof(std::uint32_t)> streaming_header;
    std::array<std::uint8_t, websocket_protocol::MAX_HEADER_SIZE> websocket_header;

    auto encoded_metadata = nlohmann::json::to_msgpack(metadata);

    auto streaming_header_size = streaming_protocol::generate_header(
        streaming_header.data(),
        signo,
        streaming_protocol::packet_type::METADATA,
        encoded_metadata.size() + sizeof(std::uint32_t));
    *reinterpret_cast<std::uint32_t *>(&streaming_header[streaming_header_size])
        = htole32(streaming_protocol::metadata_encoding::MSGPACK);
    streaming_header_size += sizeof(std::uint32_t);

    auto websocket_header_size = websocket_protocol::generate_header(
        websocket_header.data(),
        websocket_protocol::opcodes::BINARY,
        websocket_protocol::flags::FIN,
        streaming_header_size + encoded_metadata.size());

    std::array<boost::asio::const_buffer, 3> buffers =
    {
        boost::asio::buffer(websocket_header.data(), websocket_header_size),
        boost::asio::buffer(streaming_header.data(), streaming_header_size),
        boost::asio::buffer(encoded_metadata),
    };

    std::size_t total_size = buffers[0].size() + buffers[1].size() + buffers[2].size();

    boost::system::error_code ec;
    std::size_t sent = socket.write_some(buffers, ec);

    return !ec && sent == total_size;
}

bool daq::ws_streaming::websocket_client_established::send_data(
    unsigned signo, const void *data, std::size_t size) noexcept
{
    std::array<std::uint8_t, streaming_protocol::MAX_HEADER_SIZE> streaming_header;
    std::array<std::uint8_t, websocket_protocol::MAX_HEADER_SIZE> websocket_header;

    auto streaming_header_size = streaming_protocol::generate_header(
        streaming_header.data(),
        signo,
        streaming_protocol::packet_type::DATA,
        size);

    auto websocket_header_size = websocket_protocol::generate_header(
        websocket_header.data(),
        websocket_protocol::opcodes::BINARY,
        websocket_protocol::flags::FIN,
        streaming_header_size + size);

    std::array<boost::asio::const_buffer, 3> buffers =
    {
        boost::asio::buffer(websocket_header.data(), websocket_header_size),
        boost::asio::buffer(streaming_header.data(), streaming_header_size),
        boost::asio::buffer(data, size),
    };

    std::size_t total_size = buffers[0].size() + buffers[1].size() + buffers[2].size();

    boost::system::error_code ec;
    std::size_t sent = socket.write_some(buffers, ec);

    return !ec && sent == total_size;
}
