//
// ICMPHeader.hpp
// ~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2021 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef ICMP_HEADER_HPP
#define ICMP_HEADER_HPP

#include <algorithm>
#include <istream>
#include <ostream>

// ICMP header for both IPv4 and IPv6.
//
// The wire format of an ICMP header is:
//
// 0               8               16                             31
// +---------------+---------------+------------------------------+      ---
// |               |               |                              |       ^
// |     setType   |     setCode   |          getChecksum         |       |
// |               |               |                              |       |
// +---------------+---------------+------------------------------+    8 bytes
// |                               |                              |       |
// |          setIdentifier        |       sequence number        |       |
// |                               |                              |       v
// +-------------------------------+------------------------------+      ---

class ICMPHeader
{
public:
    enum
    {
        EchoReply = 0,
        DestinationUnreachable = 3,
        SourceQuench = 4,
        Redirect = 5,
        EchoRequest = 8,
        TimeExceeded = 11,
        ParameterProblem = 12,
        TimestampRequest = 13,
        TimestampReply = 14,
        InfoRequest = 15,
        InfoReply = 16,
        AddressRequest = 17,
        AddressReply = 18
    };

    [[nodiscard]] unsigned char getType() const;
    [[nodiscard]] unsigned char getCode() const;
    [[nodiscard]] uint16_t getChecksum() const;
    [[nodiscard]] uint16_t getIdentifier() const;
    [[nodiscard]] uint16_t getSequenceNumber() const;
    void setType(unsigned char n);
    void setCode(unsigned char n);
    void setChecksum(uint16_t n);
    void setIdentifier(uint16_t n);
    void setSequenceNumber(uint16_t n);

    friend std::istream& operator>>(std::istream& is, ICMPHeader& header);
    friend std::ostream& operator<<(std::ostream& os, const ICMPHeader& header);

private:
    [[nodiscard]] uint16_t decode(int a, int b) const;
    void encode(int a, int b, uint16_t n);

    unsigned char header[8]{};
};

template<typename Iterator>
void computeChecksum(ICMPHeader& header, Iterator bodyBegin, Iterator bodyEnd)
{
    unsigned int sum = (header.getType() << 8)
                       + header.getCode()
                       + header.getIdentifier()
                       + header.getSequenceNumber();

    Iterator bodyIter = bodyBegin;
    while (bodyIter != bodyEnd)
    {
        sum += (static_cast<unsigned char>(*bodyIter++) << 8);
        if (bodyIter != bodyEnd)
            sum += static_cast<unsigned char>(*bodyIter++);
    }

    sum = (sum >> 16) + (sum & 0xFFFF);
    sum += (sum >> 16);
    header.setChecksum(static_cast<uint16_t>(~sum));
}

#endif// ICMP_HEADER_HPP