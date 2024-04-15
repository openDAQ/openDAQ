//
// IPv4Header.hpp
// ~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2021 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef IPV4_HEADER_HPP
#define IPV4_HEADER_HPP

#include <boost/asio/ip/address_v4.hpp>

// Packet header for IPv4.
//
// The wire format of an IPv4 header is:
//
// 0               8               16                             31
// +-------+-------+---------------+------------------------------+      ---
// |       |       |               |                              |       ^
// |version|header |    type of    |    total length in bytes     |       |
// |  (4)  | length|    service    |                              |       |
// +-------+-------+---------------+-+-+-+------------------------+       |
// |                               | | | |                        |       |
// |        getIdentification      |0|D|M|    fragment offset     |       |
// |                               | |F|F|                        |       |
// +---------------+---------------+-+-+-+------------------------+       |
// |               |               |                              |       |
// | time to live  |   protocol    |       header checksum        |   20 bytes
// |               |               |                              |       |
// +---------------+---------------+------------------------------+       |
// |                                                              |       |
// |                      source IPv4 address                     |       |
// |                                                              |       |
// +--------------------------------------------------------------+       |
// |                                                              |       |
// |                   destination IPv4 address                   |       |
// |                                                              |       v
// +--------------------------------------------------------------+      ---
// |                                                              |       ^
// |                                                              |       |
// /                        options (if any)                      /    0 - 40
// /                                                              /     bytes
// |                                                              |       |
// |                                                              |       v
// +--------------------------------------------------------------+      ---

class IPv4Header
{
public:
    unsigned char getVersion() const;
    unsigned short getHeaderLength() const;
    unsigned char getTypeOfService() const;
    unsigned short getTotalLength() const;
    unsigned short getIdentification() const;
    bool getDontFragment() const;
    bool getMoreFragments() const;
    unsigned short getFragmentOffset() const;
    unsigned int getTimeToLive() const;
    unsigned char getProtocol() const;
    unsigned short getHeaderChecksum() const;

    boost::asio::ip::address_v4 getSourceAddress() const;
    boost::asio::ip::address_v4 getDestinationAddress() const;

    friend std::istream& operator>>(std::istream& is, IPv4Header& header);

private:
    unsigned short decode(int a, int b) const;

    unsigned char header[60]{};
};

#endif// IPV4_HEADER_HPP