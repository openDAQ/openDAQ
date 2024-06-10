#include <ping/icmp_header.h>

unsigned char ICMPHeader::getType() const
{
    return header[0];
}

unsigned char ICMPHeader::getCode() const
{
    return header[1];
}

uint16_t ICMPHeader::getChecksum() const
{
    return decode(2, 3);
}

uint16_t ICMPHeader::getIdentifier() const
{
    return decode(4, 5);
}

uint16_t ICMPHeader::getSequenceNumber() const
{
    return decode(6, 7);
}

void ICMPHeader::setType(unsigned char n)
{
    header[0] = n;
}

void ICMPHeader::setCode(unsigned char n)
{
    header[1] = n;
}

void ICMPHeader::setChecksum(uint16_t n)
{
    encode(2, 3, n);
}

void ICMPHeader::setIdentifier(uint16_t n)
{
    encode(4, 5, n);
}

void ICMPHeader::setSequenceNumber(uint16_t n)
{
    encode(6, 7, n);
}

uint16_t ICMPHeader::decode(int a, int b) const
{
    return (header[a] << 8) + header[b];
}

void ICMPHeader::encode(int a, int b, uint16_t n)
{
    header[a] = static_cast<unsigned char>(n >> 8);
    header[b] = static_cast<unsigned char>(n & 0xFF);
}

std::istream& operator>>(std::istream& is, ICMPHeader& header)
{
    return is.read(reinterpret_cast<char*>(header.header), 8);
}

std::ostream& operator<<(std::ostream& os, const ICMPHeader& header)
{
    return os.write(reinterpret_cast<const char*>(header.header), 8);
}
