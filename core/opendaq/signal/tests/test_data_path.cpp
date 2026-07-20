#include <opendaq/input_port_factory.h>
#include <gtest/gtest.h>
#include <opendaq/gmock/context.h>
#include <opendaq/gmock/input_port.h>
#include <opendaq/gmock/input_port_notifications.h>
#include <opendaq/gmock/signal.h>
#include <opendaq/context_factory.h>
#include <opendaq/signal_factory.h>
#include <opendaq/signal_private_ptr.h>
#include <opendaq/event_packet.h>
#include <opendaq/packet_factory.h>
#include <opendaq/data_rule_factory.h>
#include <coreobjects/unit_factory.h>
#include <atomic>
#include <thread>

#include "opendaq/reader_factory.h"

using namespace daq;
using namespace testing;

struct CustomInputPortNotifications : ImplementationOfWeak<IInputPortNotifications>
{
    typedef MockPtr<IInputPortNotifications, InputPortNotificationsPtr, CustomInputPortNotifications> Strict;

    MOCK_METHOD(daq::ErrCode, acceptsSignal, (IInputPort* port, daq::ISignal* signal, daq::Bool* accept), (override MOCK_CALL));
    MOCK_METHOD(daq::ErrCode, connected, (IInputPort* port), (override MOCK_CALL));
    MOCK_METHOD(daq::ErrCode, disconnected, (IInputPort* port), (override MOCK_CALL));
    MOCK_METHOD(daq::ErrCode, packetReceived, (IInputPort* port), (override MOCK_CALL));

    void setInputPort(const InputPortPtr& inputPort)
    {
        this->inputPort = inputPort;
    }

    ~CustomInputPortNotifications() override
    {
        inputPort.remove();   
    }

    InputPortPtr inputPort;
};

using DataPathTest = Test;

TEST_F(DataPathTest, DestroyListenerInNotification)
{
    const auto ctx = NullContext();

    const auto sigDesc = DataDescriptorBuilder().setSampleType(SampleType::Int32).build();

    const auto signal = Signal(ctx, nullptr, "sig");
    signal.setDescriptor(sigDesc);

    const auto inputPort = InputPort(ctx, nullptr, "ip");

    CustomInputPortNotifications::Strict notifications;
    inputPort.setNotificationMethod(PacketReadyNotification::SameThread);
    inputPort.setListener(notifications);
    notifications.mock().setInputPort(inputPort);

    EXPECT_CALL(notifications.mock(), connected).WillOnce(Return(OPENDAQ_SUCCESS));

    bool eventPacketReceived;
    EXPECT_CALL(notifications.mock(), packetReceived)
        .WillOnce(
            [&eventPacketReceived](IInputPort* port)
            {
                const auto portPtr = InputPortPtr::Borrow(port);

                const auto conn = portPtr.getConnection();
                const auto packet = conn.dequeue();

                eventPacketReceived = packet.supportsInterface<IEventPacket>();

                return OPENDAQ_SUCCESS;
            });

    inputPort.connect(signal);
    ASSERT_TRUE(eventPacketReceived);

    std::mutex mtx;
    std::condition_variable cv0;
    bool doReleaseFromThread = false;

    std::condition_variable cv1;
    bool releasedFromThread = false;

    bool dataPacketReceived = false;
    EXPECT_CALL(notifications.mock(), packetReceived)
        .WillOnce(
            [&mtx, &cv0, &cv1, &doReleaseFromThread, &releasedFromThread, &dataPacketReceived](IInputPort* port)
            {
                const auto portPtr = InputPortPtr::Borrow(port);

                {
                    std::unique_lock lock(mtx);
                    doReleaseFromThread = true;
                    cv0.notify_one();
                }

                const auto conn = portPtr.getConnection();
                const auto packet = conn.dequeue();

                dataPacketReceived = packet.supportsInterface<IDataPacket>();

                {
                    std::unique_lock lock(mtx);
                    while (!releasedFromThread)
                        cv1.wait(lock);
                }

                return OPENDAQ_SUCCESS;
            });

    std::thread thr(
        [&cv0, &cv1, &mtx, &doReleaseFromThread, &releasedFromThread, inputPortPtr = std::move(inputPort), notificationsPtr = std::move(notifications.ptr)] () mutable
        {
            {
                std::unique_lock lock(mtx);
                while (!doReleaseFromThread)
                    cv0.wait(lock);
            }

            notificationsPtr.release();

            {
                std::unique_lock lock(mtx);
                releasedFromThread = true;
                cv1.notify_one();
            }
        });

    const auto packet = DataPacket(sigDesc, 1, nullptr);
    signal.sendPacket(packet);

    thr.join();

    ASSERT_TRUE(dataPacketReceived);
}

// Lock-free data path stress tests. These encode the concurrency invariants of the
// snapshot/tombstone design and are meant to run under TSAN on platforms that have it.

TEST_F(DataPathTest, StressSendVsConnectDisconnect)
{
    const auto ctx = NullContext();

    const auto domainDesc = DataDescriptorBuilder()
                                .setSampleType(SampleType::Int64)
                                .setRule(LinearDataRule(10, 0))
                                .setTickResolution(Ratio(1, 1000))
                                .setUnit(Unit("s", -1, "second", "time"))
                                .setOrigin("1970-01-01T00:00:00Z")
                                .build();
    const auto sigDesc = DataDescriptorBuilder().setSampleType(SampleType::Float64).build();

    const auto domainSignal = Signal(ctx, nullptr, "domain");
    domainSignal.setDescriptor(domainDesc);
    const auto signal = Signal(ctx, nullptr, "sig");
    signal.setDescriptor(sigDesc);
    signal.setDomainSignal(domainSignal);

    std::atomic<bool> stop{false};
    std::atomic<int64_t> sent{0};

    // the signal's single producer
    std::thread producer(
        [&]
        {
            int64_t offset = 0;
            while (!stop.load(std::memory_order_relaxed))
            {
                const auto domainPacket = DataPacket(domainDesc, 16, offset);
                const auto packet = DataPacketWithDomain(domainPacket, sigDesc, 16);
                signal.sendPacket(packet);
                offset += 160;
                sent.fetch_add(1, std::memory_order_relaxed);
            }
        });

    // this thread plays both the config role (connect/disconnect) and, per connection,
    // the consumer role. Gap checking makes the consumer validate the
    // descriptor-event-before-data invariant on every reconnect: a data packet arriving
    // before the initial event packet would throw InvalidStateException from dequeue.
    for (int round = 0; round < 200; ++round)
    {
        auto port = InputPort(ctx, nullptr, "ip", true /*gap checking*/);
        port.connect(signal);
        const auto connection = port.getConnection();

        bool first = true;
        for (int reads = 0; reads < 50; ++reads)
        {
            PacketPtr packet;
            ASSERT_NO_THROW(packet = connection.dequeue()) << "data-before-event or torn queue on round " << round;
            if (!packet.assigned())
                continue;
            if (first)
            {
                ASSERT_TRUE(packet.supportsInterface<IEventPacket>()) << "first packet on a new connection must be the descriptor event";
                first = false;
            }
        }

        port.disconnect();
    }

    stop.store(true);
    producer.join();
    ASSERT_GT(sent.load(), 0);
}

TEST_F(DataPathTest, StressSendVsGetLastValueAndActive)
{
    const auto ctx = NullContext();

    const auto sigDesc = DataDescriptorBuilder().setSampleType(SampleType::Int64).build();
    const auto signal = Signal(ctx, nullptr, "sig");
    signal.setDescriptor(sigDesc);

    std::atomic<bool> stop{false};

    std::thread producer(
        [&]
        {
            int64_t counter = 0;
            while (!stop.load(std::memory_order_relaxed))
            {
                const auto packet = DataPacket(sigDesc, 4);
                auto* data = static_cast<int64_t*>(packet.getRawData());
                for (int i = 0; i < 4; ++i)
                    data[i] = counter;
                signal.sendPacket(packet);
                ++counter;
            }
        });

    // config-path hammering: last-value pin/retire, keep-last toggles, active toggles
    for (int round = 0; round < 2000; ++round)
    {
        BaseObjectPtr value;
        ASSERT_NO_THROW(value = signal.getLastValue());
        if (value.assigned())
            ASSERT_TRUE(value.asPtrOrNull<IInteger>().assigned());

        if (round % 64 == 0)
        {
            signal.setActive(false);
            signal.setActive(true);
        }
        if (round % 128 == 0)
        {
            signal.asPtr<ISignalPrivate>(true).enableKeepLastValue(false);
            signal.asPtr<ISignalPrivate>(true).enableKeepLastValue(true);
        }
    }

    stop.store(true);
    producer.join();
}
