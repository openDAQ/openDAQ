#pragma once
#include <boost/asio.hpp>
#include <chrono>
#include <mutex>

class IcmpPing final : public std::enable_shared_from_this<IcmpPing>
{
public:
    using TimeoutDuration = std::chrono::steady_clock::duration;

    static std::shared_ptr<IcmpPing> Create(boost::asio::io_context& ioContext, int maxHops = -1);

    IcmpPing(const IcmpPing&) = delete;
    IcmpPing(IcmpPing&& ping) noexcept = delete;

    ~IcmpPing() = default;

    void start(const boost::asio::ip::address_v4& remote,
               const boost::asio::ip::address_v4& network = boost::asio::ip::address_v4::any());

    void start(const std::vector<boost::asio::ip::address_v4>& remotes,
               const boost::asio::ip::address_v4& network = boost::asio::ip::address_v4::any());

    void stop();
    void setMaxHops(uint32_t hops);

    bool waitSend();

    std::size_t getNumReplies() const noexcept;

    IcmpPing& operator=(IcmpPing&& other) noexcept = delete;
    IcmpPing& operator=(const IcmpPing& other) = delete;

private:
    explicit IcmpPing(boost::asio::io_context& ioContext, int maxHops = -1);

    void startSend(const std::vector<boost::asio::ip::address_v4>& remotes);

    void startReceive();
    void handleReceive(std::size_t length);

    static uint16_t GetIdentifier();

    std::atomic<bool> stopReceive;
    std::atomic<bool> found;
    int32_t maxHops;

    std::size_t numRemotes;
    std::atomic<std::size_t> numSent;
    std::mutex mutex;
    std::condition_variable cv;

    uint16_t identifier;
    boost::asio::executor_work_guard<boost::asio::io_context::executor_type> work;
    boost::asio::ip::icmp::socket socket;

    boost::asio::ip::multicast::hops multicastHopsDefault;
    boost::asio::ip::unicast::hops unicastHopsDefault;

    std::chrono::steady_clock::time_point timeStart;
    std::chrono::steady_clock::time_point timeSent;
    boost::asio::streambuf replyBuffer{};
    uint16_t sequenceNumber;
    std::size_t numReplies;
};
