// A domain rule whose delta and start arrive as whole floating-point values is read as integers, with a warning

#include <gtest/gtest.h>
#include <opendaq/logger_sink_last_message_private_ptr.h>
#include <opendaq/opendaq.h>

#include <algorithm>
#include <chrono>
#include <thread>
#include <vector>

using namespace daq;

class MultiReader2FloatRule : public testing::Test
{
protected:
    // 2^32 ticks per second from the epoch, as an HBK device sends them: today's ticks are near 7.7e18
    static constexpr Int HbkTicksPerSecond = 4294967296;
    static constexpr Int HbkDelta = 16384;
    static constexpr Int HbkRate = HbkTicksPerSecond / HbkDelta;
    static constexpr Int NowSeconds = 1795096358;

    LoggerSinkPtr sink = LastMessageLoggerSink();
    // The scheduler logs to a logger of its own: a destroyed scheduler removes its component from its logger
    ContextPtr context = Context(Scheduler(LoggerWithSinks(List<ILoggerSink>()), 1),
                                 LoggerWithSinks(List<ILoggerSink>(sink)),
                                 TypeManager(),
                                 nullptr,
                                 nullptr);
    std::vector<SignalConfigPtr> domains;

    void SetUp() override
    {
        sink.setLevel(LogLevel::Warn);
    }

    SignalConfigPtr input(const std::string& id, Int ticksPerSecond, const NumberPtr& delta, const NumberPtr& start)
    {
        const auto domain = SignalWithDescriptor(context,
                                                 DataDescriptorBuilder()
                                                     .setSampleType(SampleType::Int64)
                                                     .setUnit(Unit("s", -1, "second", "time"))
                                                     .setTickResolution(Ratio(1, ticksPerSecond))
                                                     .setOrigin("1970-01-01T00:00:00Z")
                                                     .setRule(LinearDataRule(delta, start))
                                                     .build(),
                                                 nullptr,
                                                 id + "Time");
        const auto value = SignalWithDescriptor(context, DataDescriptorBuilder().setSampleType(SampleType::Float64).build(), nullptr, id);
        value.setDomainSignal(domain);
        domains.push_back(domain);
        return value;
    }

    static void send(const SignalConfigPtr& value, Int offset, SizeT count, double fill)
    {
        const SignalPtr domain = value.getDomainSignal();
        const auto domainPacket = DataPacket(domain.getDescriptor(), count, offset);
        const auto valuePacket = DataPacketWithDomain(domainPacket, value.getDescriptor(), count);
        std::fill_n(static_cast<double*>(valuePacket.getRawData()), count, fill);
        value.sendPacket(valuePacket);
    }

    static MultiReader2Ptr reader(const std::vector<SignalConfigPtr>& inputs)
    {
        auto list = List<IComponent>();
        for (const auto& signal : inputs)
            list.pushBack(signal);
        const auto params = MultiReader2Params();
        params.setInputs(list);
        return MultiReader2(params);
    }

    struct Read
    {
        SizeT count = 0;
        Int firstOffset = 0;
        MultiReader2StatusPtr status;
    };

    static Read readUpTo(const MultiReader2Ptr& reader, std::vector<std::vector<double>>& out, SizeT wanted)
    {
        Read result;
        const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(5);
        while (result.count < wanted && std::chrono::steady_clock::now() < deadline)
        {
            std::vector<void*> buffers;
            for (auto& values : out)
                buffers.push_back(values.data() + result.count);
            SizeT count = wanted - result.count;
            Int offset = 0;
            result.status = reader.read(buffers.data(), nullptr, &count, 0, offset);
            if (count > 0 && result.count == 0)
                result.firstOffset = offset;
            result.count += count;
            if (count == 0)
                std::this_thread::sleep_for(std::chrono::milliseconds(5));
        }
        return result;
    }

    static MultiReader2StatusPtr firstStatus(const MultiReader2Ptr& reader)
    {
        SizeT count = 0;
        Int offset = 0;
        return reader.read(nullptr, nullptr, &count, 0, offset);
    }

    std::string nextWarning(SizeT timeoutMs = 2000) const
    {
        const auto messages = sink.asPtr<ILastMessageLoggerSinkPrivate>(true);
        if (!messages.waitForMessage(timeoutMs))
            return {};
        return messages.getLastMessage().toStdString();
    }
};

TEST_F(MultiReader2FloatRule, WholeFloatDeltaAndStartAreReadAndWarnedOncePerDescriptor)
{
    const auto integer = input("integer", 1000, Integer(1), Integer(0));
    const auto floating = input("floating", 1000, Float(1.0), Float(0.0));
    const auto multiReader = reader({integer, floating});

    // Debug builds append the source location
    EXPECT_EQ(nextWarning().rfind("floating: the domain rule's delta and start are floating point; read as the integers 1 and 0", 0), 0u);

    send(integer, 5000, 100, 1.0);
    send(floating, 5000, 100, 2.0);
    std::vector<std::vector<double>> out(2, std::vector<double>(100));
    const auto read = readUpTo(multiReader, out, 100);

    ASSERT_EQ(read.count, 100u);
    EXPECT_EQ(read.status.getStatus(), ComponentStatus::Ok);
    EXPECT_EQ(read.firstOffset, 5000);
    EXPECT_DOUBLE_EQ(out[0][99], 1.0);
    EXPECT_DOUBLE_EQ(out[1][99], 2.0);

    const auto params = MultiReader2Params();
    params.setInputs(List<IComponent>(integer, floating));
    multiReader.configure(params);
    EXPECT_EQ(nextWarning(300), "") << "the same descriptor is warned about once";
}

TEST_F(MultiReader2FloatRule, FractionalFloatDeltaIsRejected)
{
    const auto integer = input("integer", 1000, Integer(1), Integer(0));
    const auto floating = input("floating", 1000, Float(1.5), Float(0.0));
    const auto status = firstStatus(reader({integer, floating}));

    EXPECT_EQ(status.getInputStatus(floating).getError(), MultiReader2InputError::DomainDescriptorInvalid);
    EXPECT_NE(status.getStatusMessage().toStdString().find("floating: domain descriptor invalid, rule delta not an integer above 0"),
              std::string::npos)
        << status.getStatusMessage();
}

TEST_F(MultiReader2FloatRule, FloatStartBeyondExactDoublesIsRejected)
{
    const auto integer = input("integer", 1000, Integer(1), Integer(0));
    const auto floating = input("floating", 1000, Float(1.0), Float(9007199254740994.0));
    const auto status = firstStatus(reader({integer, floating}));

    EXPECT_EQ(status.getInputStatus(floating).getError(), MultiReader2InputError::DomainDescriptorInvalid);
    EXPECT_NE(status.getStatusMessage().toStdString().find("floating: domain descriptor invalid, rule start not an integer"),
              std::string::npos)
        << status.getStatusMessage();
}

TEST_F(MultiReader2FloatRule, HbkShapedInputsAreReadTogether)
{
    const auto first = input("first", HbkTicksPerSecond, Float(static_cast<double>(HbkDelta)), Float(0.0));
    const auto second = input("second", HbkTicksPerSecond, Float(static_cast<double>(HbkDelta)), Float(0.0));
    const auto multiReader = reader({first, second});

    const Int offset = NowSeconds * HbkTicksPerSecond;
    for (Int packet = 0; packet < 4; ++packet)
    {
        send(first, offset + packet * 8192 * HbkDelta, 8192, 1.0);
        send(second, offset + packet * 8192 * HbkDelta, 8192, 2.0);
    }
    std::vector<std::vector<double>> out(2, std::vector<double>(4 * 8192));
    const auto read = readUpTo(multiReader, out, 4 * 8192);

    ASSERT_EQ(read.count, 4u * 8192u) << read.status.getStatusMessage();
    EXPECT_EQ(read.status.getStatus(), ComponentStatus::Ok) << read.status.getStatusMessage();
    EXPECT_EQ(read.firstOffset, offset);
    EXPECT_DOUBLE_EQ(out[0].back(), 1.0);
    EXPECT_DOUBLE_EQ(out[1].back(), 2.0);
}

TEST_F(MultiReader2FloatRule, HbkShapedInputIsReadWithAnIntegerInputOnAnotherResolution)
{
    const auto integer = input("integer", HbkRate, Integer(1), Integer(0));
    const auto hbk = input("hbk", HbkTicksPerSecond, Float(static_cast<double>(HbkDelta)), Float(0.0));
    const auto multiReader = reader({integer, hbk});

    const Int sample = NowSeconds * HbkRate;
    for (Int packet = 0; packet < 4; ++packet)
    {
        send(integer, sample + packet * 8192, 8192, 1.0);
        send(hbk, (sample + packet * 8192) * HbkDelta, 8192, 2.0);
    }
    std::vector<std::vector<double>> out(2, std::vector<double>(4 * 8192));
    const auto read = readUpTo(multiReader, out, 4 * 8192);

    ASSERT_EQ(read.count, 4u * 8192u) << read.status.getStatusMessage();
    EXPECT_EQ(read.status.getStatus(), ComponentStatus::Ok) << read.status.getStatusMessage();
    EXPECT_DOUBLE_EQ(out[0].back(), 1.0);
    EXPECT_DOUBLE_EQ(out[1].back(), 2.0);
}
