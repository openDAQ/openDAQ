#include <opendaq/icmp_ping.h>
#include <array>
#include <boost/asio.hpp>
#include <functional>
#include <list>

#if defined(_WIN32)
// iphlpapi.h declares the types icmpapi.h uses
#include <iphlpapi.h>

#include <icmpapi.h>
#else
#include <sys/socket.h>
#include <unistd.h>
#endif

BEGIN_NAMESPACE_OPENDAQ

using namespace boost::asio;
using boost::system::error_code;

static ip::address parseAddress(const std::string& text, error_code& ec)
{
    const bool bracketed = text.size() > 2 && text.front() == '[' && text.back() == ']';
    return ip::make_address(bracketed ? text.substr(1, text.size() - 2) : text, ec);
}

#if !defined(_WIN32)

// Opens a raw ICMP socket (root, CAP_NET_RAW), else a datagram one (macOS, Linux within net.ipv4.ping_group_range).
static error_code openIcmpSocket(ip::icmp::socket& socket, const ip::icmp& protocol, int hopLimit)
{
    error_code ec;
    socket.open(protocol, ec);
    if (ec)
    {
        const int fd = ::socket(protocol.family(), SOCK_DGRAM, protocol.protocol());
        if (fd < 0)
            return ec;
        if (socket.assign(protocol, fd, ec); ec)
        {
            ::close(fd);
            return ec;
        }
    }

    if (socket.set_option(ip::unicast::hops(hopLimit), ec); ec)
    {
        error_code ignored;
        socket.close(ignored);
    }
    return ec;
}

std::map<std::string, AddressReachabilityStatus> icmpPing(const std::vector<std::string>& addresses,
                                                          int hopLimit,
                                                          std::chrono::milliseconds timeout)
{
    // Index 0 is IPv4, 1 is IPv6. Identifier and sequence are 0; the kernel computes the ICMPv6 checksum.
    static constexpr uint8_t echoRequests[2][8] = {{8, 0, 0xF7, 0xFF}, {128}};
    static constexpr uint8_t echoReplyTypes[2] = {0, 129};

    io_context ioContext;
    std::array<ip::icmp::socket, 2> sockets{ip::icmp::socket(ioContext), ip::icmp::socket(ioContext)};
    std::array<error_code, 2> openErrors;
    std::array<std::array<uint8_t, 1500>, 2> buffers{};
    std::array<ip::icmp::endpoint, 2> senders;
    std::multimap<ip::address, std::string> awaiting;
    std::map<std::string, AddressReachabilityStatus> result;

    // IPv4 raw sockets, and datagram sockets on macOS, put the IPv4 header in front of the ICMP message.
    const auto onDatagram = [&](int family, std::size_t size)
    {
        const auto& data = buffers[family];
        const std::size_t offset = family == 0 && size > 0 && data[0] >> 4 == 4 ? (data[0] & 0x0F) * 4u : 0u;
        if (offset >= size || data[offset] != echoReplyTypes[family])
            return;
        const auto [first, last] = awaiting.equal_range(senders[family].address());
        for (auto it = first; it != last; ++it)
            result[it->second] = AddressReachabilityStatus::Reachable;
        awaiting.erase(first, last);
    };

    std::function<void(int)> receive = [&](int family)
    {
        const auto onReceived = [&, family](const error_code& ec, std::size_t size)
        {
            if (ec)
                return;
            onDatagram(family, size);
            if (awaiting.empty())
                ioContext.stop();
            else
                receive(family);
        };
        sockets[family].async_receive_from(buffer(buffers[family]), senders[family], onReceived);
    };

    for (const auto& text : addresses)
    {
        error_code ec;
        const auto address = parseAddress(text, ec);
        const auto [entry, inserted] = result.try_emplace(text, AddressReachabilityStatus::Unknown);
        if (!inserted || ec)
            continue;

        const int family = address.is_v6() ? 1 : 0;
        auto& socket = sockets[family];
        if (!socket.is_open() && !openErrors[family])
        {
            openErrors[family] = openIcmpSocket(socket, family ? ip::icmp::v6() : ip::icmp::v4(), hopLimit);
            if (!openErrors[family])
                receive(family);
        }

        if (openErrors[family])
        {
            if (openErrors[family] == error::address_family_not_supported)
                entry->second = AddressReachabilityStatus::Unreachable;
            continue;
        }

        socket.send_to(buffer(echoRequests[family]), ip::icmp::endpoint(address, 0), 0, ec);
        if (!ec)
            awaiting.emplace(address, text);
        // Other send errors (such as EPERM from a firewall rule) leave the address untested.
        if (!ec || ec == error::network_unreachable || ec == error::host_unreachable || ec == error::network_down)
            entry->second = AddressReachabilityStatus::Unreachable;
    }

    if (!awaiting.empty())
        ioContext.run_for(timeout);
    return result;
}

#else

// IcmpSendEcho2 needs no administrator rights, unlike raw ICMP sockets.
std::map<std::string, AddressReachabilityStatus> icmpPing(const std::vector<std::string>& addresses,
                                                          int hopLimit,
                                                          std::chrono::milliseconds timeout)
{
    struct Request
    {
        std::string text;
        bool v6;
        HANDLE event;
        std::array<char, 256> reply;
    };

    const HANDLE handles[2] = {IcmpCreateFile(), Icmp6CreateFile()};
    IP_OPTION_INFORMATION options{};
    options.Ttl = static_cast<UCHAR>(hopLimit);
    char payload[8] = {};
    const auto timeoutMs = static_cast<DWORD>(timeout.count());

    std::map<std::string, AddressReachabilityStatus> result;
    std::list<Request> requests;  // the reply buffers must not move while a request is pending
    for (const auto& text : addresses)
    {
        error_code ec;
        const auto address = parseAddress(text, ec);
        const auto [entry, inserted] = result.try_emplace(text, AddressReachabilityStatus::Unknown);
        if (!inserted || ec)
            continue;
        if (handles[address.is_v6()] == INVALID_HANDLE_VALUE)
        {
            // Icmp6CreateFile fails on a host without IPv6.
            if (address.is_v6())
                entry->second = AddressReachabilityStatus::Unreachable;
            continue;
        }

        auto& request = requests.emplace_back(Request{text, address.is_v6(), CreateEventW(nullptr, TRUE, FALSE, nullptr), {}});
        ip::udp::endpoint target(address, 0);
        ip::udp::endpoint source(address.is_v6() ? ip::udp::v6() : ip::udp::v4(), 0);
        const DWORD sent = request.v6 ? Icmp6SendEcho2(handles[1],
                                                       request.event,
                                                       nullptr,
                                                       nullptr,
                                                       reinterpret_cast<sockaddr_in6*>(source.data()),
                                                       reinterpret_cast<sockaddr_in6*>(target.data()),
                                                       payload,
                                                       sizeof(payload),
                                                       &options,
                                                       request.reply.data(),
                                                       sizeof(request.reply),
                                                       timeoutMs)
                                      : IcmpSendEcho2(handles[0],
                                                      request.event,
                                                      nullptr,
                                                      nullptr,
                                                      reinterpret_cast<sockaddr_in*>(target.data())->sin_addr.s_addr,
                                                      payload,
                                                      sizeof(payload),
                                                      &options,
                                                      request.reply.data(),
                                                      sizeof(request.reply),
                                                      timeoutMs);

        const DWORD lastError = sent != 0 ? ERROR_IO_PENDING : GetLastError();
        // Other errors (an invalid address such as 0.0.0.0) leave the address untested.
        if (lastError == ERROR_IO_PENDING || lastError == ERROR_NETWORK_UNREACHABLE || lastError == ERROR_HOST_UNREACHABLE)
            entry->second = AddressReachabilityStatus::Unreachable;
        if (sent != 0)
            SetEvent(request.event);  // completed without going asynchronous
        else if (lastError != ERROR_IO_PENDING)
        {
            CloseHandle(request.event);
            requests.pop_back();
        }
    }

    // Each request ends by its own timeout, so the reply buffers are not freed while the driver may still write them.
    for (auto& request : requests)
    {
        if (WaitForSingleObject(request.event, INFINITE) == WAIT_OBJECT_0)
        {
            auto* reply = request.reply.data();
            const bool replied = request.v6 ? Icmp6ParseReplies(reply, sizeof(request.reply)) > 0 &&
                                                  reinterpret_cast<ICMPV6_ECHO_REPLY*>(reply)->Status == IP_SUCCESS
                                            : IcmpParseReplies(reply, sizeof(request.reply)) > 0 &&
                                                  reinterpret_cast<ICMP_ECHO_REPLY*>(reply)->Status == IP_SUCCESS;
            if (replied)
                result[request.text] = AddressReachabilityStatus::Reachable;
        }
        CloseHandle(request.event);
    }

    for (const HANDLE handle : handles)
        if (handle != INVALID_HANDLE_VALUE)
            IcmpCloseHandle(handle);
    return result;
}

#endif

END_NAMESPACE_OPENDAQ
