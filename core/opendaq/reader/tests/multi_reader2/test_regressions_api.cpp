// Regression tests for the MultiReader2 facade, status and helper fixes found in review

#include <gtest/gtest.h>
#include <opendaq/logger_sink_last_message_private_ptr.h>
#include <opendaq/opendaq.h>
#include <opendaq/signal_impl.h>

#include <chrono>
#include <cmath>
#include <cstring>
#include <functional>
#include <limits>
#include <thread>
#include <utility>

using namespace daq;

// A signal that runs a hook, once, the next time its descriptor is read
class RegApiHookSignal : public SignalImpl
{
public:
    RegApiHookSignal(const ContextPtr& context, const DataDescriptorPtr& descriptor, std::shared_ptr<std::function<void()>> hook)
        : SignalImpl(context, descriptor, nullptr, "hooked")
        , hook(std::move(hook))
    {
    }

    ErrCode INTERFACE_FUNC getDescriptor(IDataDescriptor** descriptor) override
    {
        if (const auto run = std::exchange(*hook, nullptr))
            run();
        return SignalImpl::getDescriptor(descriptor);
    }

private:
    std::shared_ptr<std::function<void()>> hook;
};

class MultiReader2RegressionApi : public testing::Test
{
protected:
    // The scheduler logs to a logger of its own: a destroyed scheduler removes its component from its logger
    LoggerPtr logger = LoggerWithSinks(List<ILoggerSink>());
    ContextPtr context = Context(Scheduler(LoggerWithSinks(List<ILoggerSink>()), 1), logger, TypeManager(), nullptr, nullptr);

    DataDescriptorPtr valueDescriptor = DataDescriptorBuilder().setSampleType(SampleType::Float64).build();

    SignalConfigPtr regApiSignal(const std::string& id) const
    {
        return SignalWithDescriptor(context, valueDescriptor, nullptr, id);
    }
};

TEST_F(MultiReader2RegressionApi, FailedTakeOverLeavesNoSignalConnected)
{
    const auto healthy = regApiSignal("healthy");
    const auto removed = regApiSignal("removed");
    removed.asPtr<IRemovable>().remove();
    const auto params = MultiReader2Params();
    params.setInputs(List<IComponent>(healthy, removed));

    ASSERT_ANY_THROW(MultiReader2(params));
    EXPECT_EQ(healthy.getConnections().getCount(), 0u);

    const auto reader = MultiReader2(MultiReader2Params());
    ASSERT_ANY_THROW(reader.configure(params));
    EXPECT_EQ(healthy.getConnections().getCount(), 0u);
}

// A port that disconnects while the reader reads the descriptors of the ports it takes over reaches the reader as Disconnected
TEST_F(MultiReader2RegressionApi, DisconnectDuringTakeOverReachesTheReader)
{
    const auto hook = std::make_shared<std::function<void()>>();
    const auto first = regApiSignal("first");
    const SignalConfigPtr hooked = createWithImplementation<ISignalConfig, RegApiHookSignal>(context, valueDescriptor, hook);
    const auto portA = InputPort(context, nullptr, "a");
    const auto portB = InputPort(context, nullptr, "b");
    portA.connect(first);
    portB.connect(hooked);
    const auto reader = MultiReader2(MultiReader2Params());
    const auto params = MultiReader2Params();
    params.setInputs(List<IComponent>(portA, portB));

    // Reading port b's descriptors disconnects port a, after its own were read; the port lets go of its signal before it
    // notifies the reader, so the notification is under way, waiting on port a's intake lock, before the take-over ends
    std::thread disconnecting;
    *hook = [&disconnecting, portA]
    {
        disconnecting = std::thread([portA] { portA.disconnect(); });
        while (portA.getSignal().assigned())
            std::this_thread::yield();
    };
    reader.configure(params);
    disconnecting.join();

    SizeT count = 0;
    SizeT offset = 0;
    const auto status = reader.read(nullptr, nullptr, &count, 0, offset);
    EXPECT_EQ(status.getInputStatus(portA.asPtr<IComponent>()).getError(), MultiReader2InputError::Disconnected);
}

// Comparative review, debuggability finding 5: an exception thrown by acceptsDescriptor is a rejection, logged as a warning
TEST_F(MultiReader2RegressionApi, AcceptsDescriptorExceptionIsLoggedAsWarning)
{
    const auto sink = LastMessageLoggerSink();
    sink.setLevel(LogLevel::Warn);
    const auto logged = Context(
        Scheduler(LoggerWithSinks(List<ILoggerSink>()), 1), LoggerWithSinks(List<ILoggerSink>(sink)), TypeManager(), nullptr, nullptr);
    const auto params = MultiReader2Params();
    params.setInputs(List<IComponent>(SignalWithDescriptor(logged, valueDescriptor, nullptr, "only")));
    params.setAcceptsDescriptor(
        Function([](const BaseObjectPtr&, const BaseObjectPtr&) -> BaseObjectPtr
                 { throw std::runtime_error("owner bug"); }));
    const auto reader = MultiReader2(params);

    const auto messages = sink.asPtr<ILastMessageLoggerSinkPrivate>(true);
    ASSERT_TRUE(messages.waitForMessage(2000));  // the logger may hand the message to the sink on its own thread
    EXPECT_EQ(messages.getLastMessage(), "acceptsDescriptor threw for only: owner bug");
}

TEST_F(MultiReader2RegressionApi, StatusInputListCannotBeChanged)
{
    const auto params = MultiReader2Params();
    params.setInputs(List<IComponent>(regApiSignal("only")));
    const auto reader = MultiReader2(params);

    SizeT count = 0;
    SizeT offset = 0;
    auto inputs = reader.read(nullptr, nullptr, &count, 0, offset).getInputs();
    ASSERT_EQ(inputs.getCount(), 1u);
    EXPECT_THROW(inputs.clear(), FrozenException);
    EXPECT_EQ(reader.read(nullptr, nullptr, &count, 0, offset).getInputs().getCount(), 1u);
}
