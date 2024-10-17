#include <opendaq/input_port_factory.h>
#include <gtest/gtest.h>
#include <opendaq/gmock/context.h>
#include <opendaq/gmock/input_port.h>
#include <opendaq/gmock/input_port_notifications.h>
#include <opendaq/gmock/signal.h>
#include <opendaq/context_factory.h>
#include <opendaq/signal_factory.h>
#include <opendaq/event_packet.h>
#include <opendaq/packet_factory.h>
#include <thread>

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
