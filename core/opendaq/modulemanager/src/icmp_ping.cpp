#include <opendaq/icmp_header.h>
#include <opendaq/icmp_ping.h>
#include <opendaq/ipv4_header.h>
#include <opendaq/format.h>
#include <opendaq/custom_log.h>
#include <thread>

using namespace boost;
using namespace boost::asio;
using namespace std::chrono_literals;

BEGIN_NAMESPACE_OPENDAQ

IcmpPing::IcmpPing(boost::asio::io_context& ioContext, const daq::LoggerPtr& logger, int maxHops)
    : loggerComponent(logger.getOrAddComponent("IcmpPing"))
    , stopReceive(false)
    , found(false)
    , maxHops(maxHops)
    , numRemotes(0)
    , numSent(0)
    , identifier(GetIdentifier())
    , work(ioContext.get_executor())
    , socket(ioContext)
    , sequenceNumber(0)
    , numReplies(0)
{
}

std::shared_ptr<IcmpPing> IcmpPing::Create(io_context& ioContext, const daq::LoggerPtr& logger, int maxHops)
{
    return std::shared_ptr<IcmpPing>(new IcmpPing(ioContext, logger, maxHops));
}

void IcmpPing::setMaxHops(uint32_t hops)
{
    this->maxHops = static_cast<int32_t>(hops);
}

std::size_t IcmpPing::getNumReplies() const noexcept
{
    return numReplies;
}

void IcmpPing::start(const ip::address_v4& remote, const ip::address_v4& network)
{
    start(std::vector{remote}, network);
}

void IcmpPing::start(const std::vector<boost::asio::ip::address_v4>& remotes, const boost::asio::ip::address_v4& network)
{
    if (socket.is_open())
        throw std::runtime_error("Socket already open, please close before trying to ping again.");

    stopReceive = false;

    socket.open(ip::icmp::v4());
    socket.set_option(ip::udp::socket::reuse_address(true));

    socket.get_option(multicastHopsDefault);
    socket.get_option(unicastHopsDefault);
    // LOG("Socket ping TTL default M: {} U: {}\n", mulitcastHopsDefault.value(), unicastHopsDefault.value());

    found = false;
    numReplies = 0;
    sequenceNumber = 0;

    if (maxHops == -1)
    {
        maxHops = 64;
    }

    socket.set_option(ip::multicast::hops(maxHops));
    socket.set_option(ip::unicast::hops(maxHops));

    socket.bind(ip::icmp::endpoint(network, 0));

    // socket.get_option(mulitcastHopsDefault);
    // socket.get_option(unicastHopsDefault);
    // LOG("Socket ping TTL set M: {} U: {}\n", mulitcastHopsDefault.value(), unicastHopsDefault.value());

    startReceive();
    startSend(remotes);
}

void IcmpPing::stop()
{
    if (stopReceive)
        return;

    stopReceive = true;

    system::error_code ec;
    socket.shutdown(boost::asio::socket_base::shutdown_both, ec);
    if (ec && ec != boost::asio::error::bad_descriptor)
    {
        LOG_E("Error shutting ICMP socket [{}] \n", ec.message());
    }

    // LOG("Closing down ICMP socket\n");

    socket.close(ec);
    if (ec && ec != boost::asio::error::bad_descriptor)
    {
        LOG_E("Error closing down ICMP socket for [{}] \n", ec.message());
    }

    cv.notify_one();
    // LOG("Closed ICMP socket\n");
}

void IcmpPing::startSend(const std::vector<boost::asio::ip::address_v4>& remotes)
{
    std::string body("\"Hello!\" from openDAQ ping.");

    // Create an ICMP header for an echo request.
    ICMPHeader echoRequest;
    echoRequest.setType(ICMPHeader::EchoRequest);
    echoRequest.setCode(0);
    echoRequest.setIdentifier(identifier);
    echoRequest.setSequenceNumber(++sequenceNumber);
    computeChecksum(echoRequest, body.begin(), body.end());

    // Encode the request packet.
    boost::asio::streambuf requestBuffer;
    std::ostream os(&requestBuffer);
    os << echoRequest << body;

    // Send the request.
    numReplies = 0;
    numRemotes = remotes.size();
    numSent = 0;

    // LOG("Ping remotes: {}\n", numRemotes);

    timeStart = std::chrono::steady_clock::now();

    for (std::size_t i = 0; i < numRemotes; ++i)
    {
        if (found)
        {
            // LOG("Notifying CV on found\n");
            numSent = numRemotes;
            cv.notify_one();
            break;
        }

        if (i != 0 && i % 1000 == 0)
            std::this_thread::sleep_for(50ms);

        auto ip = remotes[i];

        socket.async_send_to(
            requestBuffer.data(),
            ip::icmp::endpoint(ip, 0),
            [i, ip, this, ptr = shared_from_this()](boost::system::error_code ec, std::size_t)
        {
            if (++numSent == numRemotes)
            {
                cv.notify_one();
            }

            // auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(steady_timer::clock_type::now() - timeStart).count();
            if (ec)
            {
                cv.notify_one();
                // LOG("[{}] Error sending ping to: {} [{}] <{} ms> [{}]\n", i, ip, ec.message(), ms, numSent);
                LOG_T("[{}] Error sending ping to: {} [{}] [{}]\n", i, ip, ec.message(), numSent.load());
                return;
            }

            // auto str2 = ip.to_string();
            // LOG("[{}] Sent ping to {} on thread id: {} <{}ms>\n", i, ip, std::this_thread::get_id(), ms);
            LOG_T("[{}] Sent ping to {} on thread id: {}\n", i, ip, fmt::streamed(std::this_thread::get_id()));
        });
    }

    timeSent = steady_timer::clock_type::now();
}

bool IcmpPing::waitSend()
{
    if (numReplies > 0)
        return true;

    if (numRemotes == numSent)
    {
        // LOG("Waiting for pings to be sent\n");
        std::unique_lock lock(mutex);
        cv.wait(lock,
                [this, ptr = shared_from_this()]
                {
                    // LOG("CV Check: {}/{} [{}]\n", numSent, numRemotes, found);
                    return numRemotes == numSent || found || stopReceive;
                });

        // LOG("All pings have sent\n");
    }

    return numReplies > 0;
}

void IcmpPing::startReceive()
{
//    LOG("Start receive\n");

    // Discard any data already in the buffer.
    replyBuffer.consume(65536);

    // Wait for a reply. We prepare the buffer to receive up to 64KB.
    socket.async_receive(replyBuffer.prepare(65536),
                         [ptr = shared_from_this()](boost::system::error_code ec, std::size_t length)
                         {
                             if (ec)
                             {
                                 if (ec != error::operation_aborted)
                                 {
                                     DAQLOGF_E(ptr->loggerComponent, "Error receiving ping: {} [{}]\n", ec.message(), ec.value());
                                 }

                                 ptr->cv.notify_one();
                                 return;
                             }
                             else
                             {
                                 ptr->handleReceive(length);
                             }
                         });
}

void IcmpPing::handleReceive(std::size_t length)
{
    if (stopReceive || found)
    {
        // LOG("Already found: exiting...\n");
        return;
    }

    // The actual number of bytes received is committed to the buffer so that we
    // can extract it using a std::istream object.
    replyBuffer.commit(length);

    // Decode the reply packet.
    std::istream is(&replyBuffer);
    IPv4Header ipv4Header;
    ICMPHeader icmpHeader;
    is >> ipv4Header >> icmpHeader;

    // We can receive all ICMP packets received by the host, so we need to
    // filter out only the echo replies that match our setIdentifier and
    // expected sequence number.
    if (is
        && icmpHeader.getType() == ICMPHeader::EchoReply
        && icmpHeader.getIdentifier() == identifier
        && icmpHeader.getSequenceNumber() == sequenceNumber)
    {
        numReplies++;

        // Print out some information about the reply packet.
        chrono::steady_clock::time_point now = chrono::steady_clock::now();
        chrono::steady_clock::duration elapsed = now - timeSent;

        auto sourceAddr = ipv4Header.getSourceAddress();
        auto sourceStr = sourceAddr.to_string();

        auto destinationAddr = ipv4Header.getDestinationAddress();
        auto destinationStr = destinationAddr.to_string();

        LOG_T("[{}] {} bytes from {}: icmp_seq={}, ttl={}, time={} ms\n",
            fmt::streamed(std::this_thread::get_id()),
            length - ipv4Header.getHeaderLength(),
            sourceAddr,
            icmpHeader.getSequenceNumber(),
            ipv4Header.getTimeToLive(),
            chrono::duration_cast<chrono::milliseconds>(elapsed).count()
        );

        LOG_T("\nReceived reply: canceling all async operations\n\n");
        found = true;
        stop();
    }

    if (!stopReceive)
        startReceive();
}

uint16_t IcmpPing::GetIdentifier()
{
#if defined(BOOST_ASIO_WINDOWS)
    return static_cast<uint16_t>(::GetCurrentProcessId());
#else
    return static_cast<uint16_t>(::getpid());
#endif
}

END_NAMESPACE_OPENDAQ
