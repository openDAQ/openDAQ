#pragma once

#include <cstddef>
#include <cstdint>
#include <optional>
#include <ostream>
#include <string>

namespace daq::ws_streaming
{
    /**
     * Contains constants and other definitions related to the WebSocket protocol.
     */
    namespace websocket_protocol
    {
#pragma pack(push, 1)
        /**
         * The structure of a WebSocket streaming protocol constant-value data signal packet on
         * the wire. Such a packet contains the index of the sample where the change occurred, and
         * the signal's new constant value.
         *
         * @todo What exactly is a sample index?
         */
        struct constant_value_packet
        {
            std::uint64_t index;    /**< The index of the sample where the change occurred
                                         (XXX TODO: what does this mean exactly?) */
            std::int64_t value;     /**< The signal's new constant value. */
        };
#pragma pack(pop)

        /**
         * The maximum possible frame header size, in bytes.
         */
        constexpr std::size_t MAX_HEADER_SIZE = 10;

        /**
         * The magic key string to be used for calculating the value of the Sec-WebSocket-Accept
         * HTTP negotiation header.
         */
        constexpr const char MAGIC_KEY[] = "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";

        /**
         * Constants used in the WebSocket frame header to specify flags.
         */
        namespace flags
        {
            constexpr unsigned FIN = 0x80;  /**< Identifies the last fragment in a sequence. */
        }

        /**
         * Constants used to specify WebSocket operations (frame types).
         */
        namespace opcodes
        {
            constexpr unsigned TEXT = 1;    /**< Identifies a frame containing UTF-8 text. */
            constexpr unsigned BINARY = 2;  /**< Identifies a frame containing opaque binary data. */
            constexpr unsigned CLOSE = 8;   /**< Identifies a close frame. */
            constexpr unsigned PING = 9;    /**< Identifies a ping frame. */
            constexpr unsigned PONG = 10;   /**< Identifies a pong frame. */
        }

        /**
         * Writes a WebSocket HTTP upgrade negotiation response to the specified output stream.
         *
         * @param os The output stream to write to.
         * @param key The Sec-WebSocket-Accept key string from the HTTP request.
         */
        std::ostream& generate_upgrade_response(std::ostream& os, const std::string& key);

        /**
         * Populates a WebSocket frame header.
         *
         * @perfcrit This function is called once for every transmitted WebSocket frame.
         *
         * @param header A pointer to memory to populate with the header. The pointed-to area must
         *     be large enough to hold the largest possible header (MAX_HEADER_SIZE).
         * @param opcode The WebSocket opcode.
         * @param flags A combination of WebSocket flag values.
         * @param payload_size The size of the payload in bytes.
         *
         * @return The size of the generated header in bytes.
         */
        inline std::size_t generate_header(std::uint8_t *header,
            unsigned opcode, unsigned flags, std::size_t payload_size)
        {
            header[0] = opcode | flags;
            std::uint64_t payload_size_64 = payload_size;

            if (payload_size <= 125)
            {
                header[1] = payload_size_64;
                return 2;
            }

            else if (payload_size_64 <= 65535)
            {
                header[1] = 126;
                header[2] = payload_size_64 >> 8;
                header[3] = payload_size_64;
                return 4;
            }

            else
            {
                header[1] = 127;
                header[2] = payload_size_64 >> 56;
                header[3] = payload_size_64 >> 48;
                header[4] = payload_size_64 >> 40;
                header[5] = payload_size_64 >> 32;
                header[6] = payload_size_64 >> 24;
                header[7] = payload_size_64 >> 16;
                header[8] = payload_size_64 >> 8;
                header[9] = payload_size_64;
                return 10;
            }
        }

        /**
         * A structure containing values from a decoded WebSocket frame header. The
         * decode_header() function populates and returns an instance of this structure.
         */
        struct decoded_header
        {
            std::size_t header_size;                    /**< The size of the header in bytes. */
            unsigned flags;                             /**< The value of the header's flags vield. */
            unsigned opcode;                            /**< The value of the header's opcode field. */
            std::size_t payload_size;                   /**< The claimed payload size in bytes. */
            std::optional<std::uint32_t> masking_key;   /**< The 32-bit masking key, or std::nullopt if the header indicates the payload is not masked. */
        };

        /**
         * Decodes a WebSocket frame header.
         *
         * @param data A pointer to the WebSocket frame data. The data may be truncated; i.e., it
         *     is safe to call this function even if it's not known whether the data contains a
         *     complete and valid frame. In this case the returned decoded_header::header_size
         *     member is set to 0 (see the Returns description).
         * @param size The size of the data pointed to by @p data in bytes.
         *
         * @return A decoded_header structure containing the values of the header's fields. If the
         *     pointed-to data contains a complete frame (including payload), the returned
         *     decoded_header::header_size member is set to the actual size of the header. If the
         *     data is truncated, the returned decoded_header::header_size member is set to 0.
         */
        decoded_header decode_header(const std::uint8_t *data, std::size_t size) noexcept;
    }
}
