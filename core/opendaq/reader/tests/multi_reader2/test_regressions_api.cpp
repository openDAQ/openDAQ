// Regression tests for the MultiReader2 facade, status and helper fixes found in review

#include <gtest/gtest.h>
#include <opendaq/logger_sink_last_message_private_ptr.h>
#include <opendaq/multi_reader2_convert.h>
#include <opendaq/multi_reader2_domain.h>
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

    // Reading port b's descriptors disconnects port a, after its own were read; that disconnect waits on port a's intake lock
    std::thread disconnecting;
    *hook = [&disconnecting, portA]
    {
        disconnecting = std::thread([portA] { portA.disconnect(); });
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
    };
    reader.configure(params);
    disconnecting.join();

    SizeT count = 0;
    SizeT offset = 0;
    const auto status = reader.read(nullptr, &count, 0, offset);
    EXPECT_EQ(status.getInputStatus(portA.asPtr<IComponent>()).getError(), MultiReader2InputError::Disconnected);
}

TEST_F(MultiReader2RegressionApi, FloatTargetSaturatesFiniteValuesAndKeepsNanAndInfinity)
{
    const std::vector<double> values{1e300, -1e300, std::nan(""), -std::numeric_limits<double>::infinity()};
    const auto packet = DataPacket(valueDescriptor, values.size());
    std::memcpy(packet.getRawData(), values.data(), values.size() * sizeof(double));

    std::vector<float> out(values.size());
    multi_reader2::copySamples(packet, 0, values.size(), out.data(), SampleType::Float32);
    EXPECT_EQ(out[0], std::numeric_limits<float>::max());
    EXPECT_EQ(out[1], std::numeric_limits<float>::lowest());
    EXPECT_TRUE(std::isnan(out[2]));
    EXPECT_EQ(out[3], -std::numeric_limits<float>::infinity());
}

// -(2^65 - 1) / 2: the floor quotient's magnitude is 2^64, out of Int64
TEST_F(MultiReader2RegressionApi, TickWhoseFloorQuotientWrapsIsOutOfRange)
{
    const multi_reader2::Relation relation{31, 2, 0, 0};
    EXPECT_EQ(relation.toMain(-1190112520884487201), std::numeric_limits<std::int64_t>::min() / 2);
}

TEST_F(MultiReader2RegressionApi, TickWhoseShiftOrOffsetLeavesInt64IsOutOfRange)
{
    constexpr auto max = std::numeric_limits<std::int64_t>::max();
    constexpr auto min = std::numeric_limits<std::int64_t>::min();
    EXPECT_EQ((multi_reader2::Relation{1, 1, 0, 10}.toMain(max - 5)), max / 2);  // tick plus the reference domain offset
    EXPECT_EQ((multi_reader2::Relation{1, 1, 10, 0}.toMain(max - 5)), max / 2);  // main tick plus the origin offset
    EXPECT_EQ((multi_reader2::Relation{1, 1, -10, 0}.toMain(min + 5)), min / 2);
    EXPECT_EQ((multi_reader2::Relation{1, 1, 10, 5}.toMain(100)), 115);
}

TEST_F(MultiReader2RegressionApi, OriginParserRejectsInvalidDatesAndFieldsAndTakesShortForms)
{
    using multi_reader2::parseOrigin;
    EXPECT_FALSE(parseOrigin("2023-02-29"));
    EXPECT_FALSE(parseOrigin("2023-04-31"));
    EXPECT_FALSE(parseOrigin("1970-01-01T-1:00:00Z"));
    EXPECT_FALSE(parseOrigin("1970-+1-01"));
    EXPECT_FALSE(parseOrigin(" 970-01-01"));
    EXPECT_FALSE(parseOrigin("1970-01-01T00:00:00+24:00"));
    EXPECT_EQ(parseOrigin("2024-02-29"), 1709164800);
    EXPECT_EQ(parseOrigin("1970-01-01T01:00Z"), 3600);
    EXPECT_EQ(parseOrigin("1970-01-01T00:00:00+01"), -3600);
    EXPECT_EQ(parseOrigin("1970-01-01T00:00:00+0130"), -5400);
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
        Function([](const BaseObjectPtr&, const BaseObjectPtr&, const BaseObjectPtr&, const BaseObjectPtr&) -> BaseObjectPtr
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
    auto inputs = reader.read(nullptr, &count, 0, offset).getInputs();
    ASSERT_EQ(inputs.getCount(), 1u);
    EXPECT_THROW(inputs.clear(), FrozenException);
    EXPECT_EQ(reader.read(nullptr, &count, 0, offset).getInputs().getCount(), 1u);
}
