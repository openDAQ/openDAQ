#pragma once
#include <fmt/ostream.h>

#include <boost/asio/buffer.hpp>
#include <boost/asio/ip/address.hpp>
#include <boost/asio/ip/address_v4.hpp>
#include <boost/asio/ip/network_v4.hpp>
#include <boost/asio/ip/udp.hpp>

#if FMT_VERSION >= 100100

template <>
struct fmt::formatter<boost::asio::ip::address_v4> : ostream_formatter
{
};

template <>
struct fmt::formatter<boost::asio::ip::network_v4> : ostream_formatter
{
};

template <>
struct fmt::formatter<boost::asio::ip::address> : ostream_formatter
{
};

template <>
struct fmt::formatter<boost::asio::ip::udp::endpoint> : ostream_formatter
{
};
#else

namespace fmt
{
    template <typename T>
    auto streamed(T&& argument)
    {
        return argument;
    }

    template <typename Enum>
    constexpr auto underlying(Enum enumValue) -> std::underlying_type_t<Enum>
    {
        return static_cast<std::underlying_type_t<Enum>>(enumValue);
    }

    template <>
    struct formatter<boost::asio::const_buffer> : formatter<std::string_view, std::string_view::value_type>
    {
        auto format(const boost::asio::const_buffer& c, format_context& ctx)
        {
            return formatter<std::string_view, std::string_view::value_type>::format(
                std::string_view((const char*) c.data(), c.size()),
                ctx);
        }
    };
}

#endif

namespace boost::asio
{
    inline auto format_as(const const_buffer& c)
    {
        return std::string_view((const char*) c.data(), c.size());
    }
}  // namespace boost::asio
