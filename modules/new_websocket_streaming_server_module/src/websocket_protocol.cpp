#include <array>
#include <cstddef>
#include <cstdint>
#include <ostream>
#include <string>

#include <boost/uuid/detail/sha1.hpp>
#include <iostream>

#include "base64.hpp"
#include "websocket_protocol.hpp"

std::ostream& daq::ws_streaming::websocket_protocol::generate_upgrade_response(std::ostream& os, const std::string& key)
{
    boost::uuids::detail::sha1 sha1;
    sha1.process_bytes(key.data(), key.length());
    sha1.process_bytes(MAGIC_KEY, sizeof(MAGIC_KEY) - 1);

    boost::uuids::detail::sha1::digest_type sha1_value;
    sha1.get_digest(sha1_value);

    std::array<std::uint8_t, 20> sha1_bytes;
    for (unsigned i = 0; i < 5; ++i)
    {
        sha1_bytes[4 * i + 0] = sha1_value[i] >> 24;
        sha1_bytes[4 * i + 1] = sha1_value[i] >> 16;
        sha1_bytes[4 * i + 2] = sha1_value[i] >> 8;
        sha1_bytes[4 * i + 3] = sha1_value[i];
    }

    auto response_key = base64::encode_into<std::string>(
        sha1_bytes.data(),
        sha1_bytes.data() + sha1_bytes.size());

    os << "HTTP/1.1 101 Switching Protocols\r\n";
    os << "Upgrade: websocket\r\n";
    os << "Connection: Upgrade\r\n";
    os << "Sec-WebSocket-Accept: " << response_key << "\r\n";
    os << "\r\n";

    return os;
}

daq::ws_streaming::websocket_protocol::decoded_header
daq::ws_streaming::websocket_protocol::decode_header(const std::uint8_t *data, std::size_t size) noexcept
{
    decoded_header header { };
    const std::uint8_t *data_begin = data;

    if (size < 2)
        return header;

    header.opcode = data[0] & 0xF;
    header.flags = data[0] & 0xF0;
    bool is_masked = 0 != (data[1] & 0x80);
    header.payload_size = data[1] & 0x7F;

    data += 2;
    size -= 2;

    if (header.payload_size == 126)
    {
        if (size < sizeof(std::uint16_t))
            return header;

        header.payload_size =
            (static_cast<std::uint16_t>(data[0]) << 8) |
            data[1];

        data += sizeof(std::uint16_t);
        size -= sizeof(std::uint16_t);
    }

    else if (header.payload_size == 127)
    {
        if (size < sizeof(std::uint64_t))
            return header;

        header.payload_size =
            (static_cast<std::uint64_t>(data[0]) << 56) |
            (static_cast<std::uint64_t>(data[1]) << 48) |
            (static_cast<std::uint64_t>(data[2]) << 40) |
            (static_cast<std::uint64_t>(data[3]) << 32) |
            (static_cast<std::uint64_t>(data[4]) << 24) |
            (static_cast<std::uint64_t>(data[5]) << 16) |
            (static_cast<std::uint64_t>(data[6]) << 8) |
            data[7];

        data += sizeof(std::uint64_t);
        size -= sizeof(std::uint64_t);
    }

    if (is_masked)
    {
        if (size < sizeof(decltype(header.masking_key)::value_type))
            return header;

        header.masking_key =
            (static_cast<std::uint32_t>(data[0]) << 24) |
            (static_cast<std::uint32_t>(data[1]) << 16) |
            (static_cast<std::uint32_t>(data[2]) << 8) |
            data[3];

        data += sizeof(decltype(header.masking_key)::value_type);
        size -= sizeof(decltype(header.masking_key)::value_type);
    }

    if (size >= header.payload_size)
        header.header_size = data - data_begin;

    return header;
}
