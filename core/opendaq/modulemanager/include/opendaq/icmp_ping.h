/*
 * Copyright 2022-2025 openDAQ d.o.o.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#pragma once
#include <boost/asio.hpp>
#include <chrono>
#include <mutex>
#include <opendaq/logger_ptr.h>
#include <opendaq/logger_component_ptr.h>

BEGIN_NAMESPACE_OPENDAQ

class IcmpPing final : public std::enable_shared_from_this<IcmpPing>
{
public:
    using TimeoutDuration = std::chrono::steady_clock::duration;

    static std::shared_ptr<IcmpPing> Create(boost::asio::io_context& ioContext, const daq::LoggerPtr& logger, int maxHops = -1);

    IcmpPing(const IcmpPing&) = delete;
    IcmpPing(IcmpPing&& ping) noexcept = delete;

    ~IcmpPing() = default;

    void start(const boost::asio::ip::address_v4& remote,
               const boost::asio::ip::address_v4& network = boost::asio::ip::address_v4::any());

    void start(const std::vector<boost::asio::ip::address_v4>& remotes,
               const boost::asio::ip::address_v4& network = boost::asio::ip::address_v4::any());

    void stop();
    void setMaxHops(uint32_t hops);

    void waitSendAndReply();

    std::size_t getNumReplies() const noexcept;
    std::unordered_set<std::string> getReplyAddresses() const;

    void clearReplies();

    IcmpPing& operator=(IcmpPing&& other) noexcept = delete;
    IcmpPing& operator=(const IcmpPing& other) = delete;

private:
    explicit IcmpPing(boost::asio::io_context& ioContext, const daq::LoggerPtr& logger, int maxHops = -1);

    void startSend(const std::vector<boost::asio::ip::address_v4>& remotes);

    void startReceive();
    void handleReceive(std::size_t length);

    static uint16_t GetIdentifier();

    daq::LoggerComponentPtr loggerComponent;

    std::atomic<bool> stopReceive;
    int32_t maxHops;

    std::size_t numRemotes;
    std::atomic<std::size_t> numSent;
    std::mutex mutexRequests;
    std::condition_variable allRequestsSent;
    std::mutex mutexReplies;
    std::condition_variable allRepliesReceived;

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
    std::unordered_set<std::string> responseAddresses;
};

END_NAMESPACE_OPENDAQ
