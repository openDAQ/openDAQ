#include <opendaq/ipv4_header.h>
#include <istream>

unsigned char IPv4Header::getVersion() const
{
    return (header[0] >> 4) & 0xF;
}

unsigned short IPv4Header::getHeaderLength() const
{
    return (header[0] & 0xF) * 4;
}

unsigned char IPv4Header::getTypeOfService() const
{
    return header[1];
}

unsigned short IPv4Header::getTotalLength() const
{
    return decode(2, 3);
}

unsigned short IPv4Header::getIdentification() const
{
    return decode(4, 5);
}

bool IPv4Header::getDontFragment() const
{
    return (header[6] & 0x40) != 0;
}

bool IPv4Header::getMoreFragments() const
{
    return (header[6] & 0x20) != 0;
}

unsigned short IPv4Header::getFragmentOffset() const
{
    return decode(6, 7) & 0x1FFF;
}

unsigned int IPv4Header::getTimeToLive() const
{
    return header[8];
}

unsigned char IPv4Header::getProtocol() const
{
    return header[9];
}

unsigned short IPv4Header::getHeaderChecksum() const
{
    return decode(10, 11);
}

unsigned short IPv4Header::decode(int a, int b) const
{
    return (header[a] << 8) + header[b];
}

boost::asio::ip::address_v4 IPv4Header::getSourceAddress() const
{
    boost::asio::ip::address_v4::bytes_type bytes = {{header[12], header[13], header[14], header[15]}};
    return boost::asio::ip::address_v4(bytes);
}

boost::asio::ip::address_v4 IPv4Header::getDestinationAddress() const
{
    boost::asio::ip::address_v4::bytes_type bytes = {{header[16], header[17], header[18], header[19]}};
    return boost::asio::ip::address_v4(bytes);
}

std::istream& operator>>(std::istream& is, IPv4Header& header)
{
    is.read(reinterpret_cast<char*>(header.header), 20);
    if (header.getVersion() != 4)
        is.setstate(std::ios::failbit);

    std::streamsize options_length = header.getHeaderLength() - 20;
    if (options_length < 0 || options_length > 40)
    {
        is.setstate(std::ios::failbit);
    }
    else
    {
        is.read(reinterpret_cast<char*>(header.header) + 20, options_length);
    }
    return is;
}
