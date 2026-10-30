// White-box tests of the MultiReader2 parts below the data manager: tick arithmetic, sample conversion and the input
// queue. Their sources are compiled into this test, as they are not exported from the library.

#include <gtest/gtest.h>
#include <opendaq/multi_reader2_convert.h>
#include <opendaq/multi_reader2_domain.h>
#include <opendaq/multi_reader2_queue.h>
#include <opendaq/multi_reader2_sync.h>
#include <opendaq/opendaq.h>

#include <cmath>
#include <limits>

using namespace daq;
using namespace daq::multi_reader2;

namespace
{

DataDescriptorPtr tickDomain(Int num = 1, Int den = 1000, Int delta = 1, Int start = 0, const std::string& origin = "2026-01-01T00:00:00Z")
{
    return DataDescriptorBuilder()
        .setSampleType(SampleType::Int64)
        .setUnit(Unit("s", -1, "second", "time"))
        .setTickResolution(Ratio(num, den))
        .setRule(LinearDataRule(delta, start))
        .setOrigin(origin)
        .build();
}

DataPacketPtr packetOf(SampleType type, const std::vector<double>& values, const ScalingPtr& scaling = nullptr)
{
    auto builder = DataDescriptorBuilder().setSampleType(scaling.assigned() ? SampleType::Float64 : type);
    if (scaling.assigned())
        builder.setPostScaling(scaling);
    const auto packet = DataPacket(builder.build(), values.size());
    auto* raw = static_cast<uint8_t*>(packet.getRawData());
    const SampleType rawType = scaling.assigned() ? scaling.getInputSampleType() : type;
    for (SizeT i = 0; i < values.size(); ++i)
    {
        uint8_t* p = raw + i * getSampleSize(rawType);
        switch (rawType)
        {
            case SampleType::Float64:
            {
                const double v = values[i];
                std::memcpy(p, &v, 8);
                break;
            }
            case SampleType::Int64:
            {
                const int64_t v = static_cast<int64_t>(values[i]);
                std::memcpy(p, &v, 8);
                break;
            }
            case SampleType::Int16:
            {
                const int16_t v = static_cast<int16_t>(values[i]);
                std::memcpy(p, &v, 2);
                break;
            }
            default:
                throw std::runtime_error("unsupported");
        }
    }
    return packet;
}

DataPacketPtr dataPacket(SizeT count)
{
    return DataPacket(DataDescriptorBuilder().setSampleType(SampleType::Float64).build(), count);
}

}

TEST(MultiReader2Internals, ParsesAValidDomainWithAReducedResolution)
{
    const auto d = parseDomain(tickDomain(2, 2000, 1, 5));
    ASSERT_TRUE(d);
    EXPECT_EQ(d->resNum, 1);
    EXPECT_EQ(d->resDen, 1000);
    EXPECT_EQ(d->start, 5);
    EXPECT_EQ(d->originSeconds, 1767225600);
}

TEST(MultiReader2Internals, RejectsEachDomainConstraint)
{
    EXPECT_FALSE(parseDomain(tickDomain(1, 1000, 1, 0, "")));                        // no origin
    EXPECT_FALSE(parseDomain(tickDomain(1, 1000, 1, 0, "2026-01-01T00:00:00.5Z")));  // not on a full second
    EXPECT_FALSE(parseDomain(tickDomain(1, 1000, 0)));                               // delta 0
    EXPECT_FALSE(parseDomain(tickDomain(1, 1000, 3)));                               // 333.3 Hz is not an integer rate
    EXPECT_FALSE(parseDomain(DataDescriptorBuilder()
                                 .setSampleType(SampleType::Float64)
                                 .setUnit(Unit("s"))
                                 .setTickResolution(Ratio(1, 1000))
                                 .setRule(LinearDataRule(1, 0))
                                 .setOrigin("2026-01-01T00:00:00Z")
                                 .build()));  // not an integer type
    EXPECT_FALSE(parseDomain(DataDescriptorBuilder()
                                 .setSampleType(SampleType::Int64)
                                 .setUnit(Unit("ms"))
                                 .setTickResolution(Ratio(1, 1000))
                                 .setRule(LinearDataRule(1, 0))
                                 .setOrigin("2026-01-01T00:00:00Z")
                                 .build()));  // not seconds
    EXPECT_FALSE(parseDomain(DataDescriptorBuilder()
                                 .setSampleType(SampleType::Int64)
                                 .setUnit(Unit("s"))
                                 .setTickResolution(Ratio(1, 1000))
                                 .setRule(ExplicitDataRule())
                                 .setOrigin("2026-01-01T00:00:00Z")
                                 .build()));  // not linear
    EXPECT_FALSE(parseDomain(DataDescriptorBuilder()
                                 .setSampleType(SampleType::Int64)
                                 .setUnit(Unit("s"))
                                 .setTickResolution(Ratio(1, 1000))
                                 .setRule(LinearDataRule(1.5, 0))
                                 .setOrigin("2026-01-01T00:00:00Z")
                                 .build()));  // fractional delta
}

TEST(MultiReader2Internals, ParsesOriginForms)
{
    EXPECT_EQ(parseOrigin("1970-01-01T00:00:00Z"), 0);
    EXPECT_EQ(parseOrigin("1970-01-01"), 0);
    EXPECT_EQ(parseOrigin("1970"), 0);
    EXPECT_EQ(parseOrigin("1970-02"), 31 * 86400);
    EXPECT_EQ(parseOrigin("1970-01-01T01:00:00+01:00"), 0);
    EXPECT_EQ(parseOrigin("2026-01-01T00:00:00.000Z"), 1767225600);
    EXPECT_FALSE(parseOrigin("2026-01-01T00:00:00.001Z"));
    EXPECT_FALSE(parseOrigin("yesterday"));
}

TEST(MultiReader2Internals, RelatesInputTicksToMainTicks)
{
    const auto main = *parseDomain(tickDomain());
    const auto fine = *parseDomain(tickDomain(1, 2000, 2, 0, "2025-12-31T23:59:59Z"));  // 1 s earlier, half-millisecond ticks
    const auto r = relate(fine, main);
    ASSERT_TRUE(r);
    std::int64_t remainder = -1;
    EXPECT_EQ(r->toMain(2000, &remainder), 0);  // 1 s after its origin is the main origin
    EXPECT_EQ(remainder, 0);
    EXPECT_EQ(r->toMain(2001, &remainder), 0);  // between two main ticks
    EXPECT_EQ(remainder, 1);
    EXPECT_EQ(syncLimitTicks(main), 2000);
}

// Two devices whose ticks count from their own boot give the same time only with their reference domain offsets
TEST(MultiReader2Internals, RelationComparesTicksOnTheReferenceDomain)
{
    const auto withOffset = [](Int offset)
    {
        return DataDescriptorBuilderCopy(tickDomain(1, 1000000, 1000, 0, "1970-01-01T00:00:00Z"))
            .setReferenceDomainInfo(ReferenceDomainInfoBuilder().setReferenceDomainId("ptp:0").setReferenceDomainOffset(offset).build())
            .build();
    };
    const auto main = *parseDomain(withOffset(5000));
    const auto input = *parseDomain(withOffset(2000));
    EXPECT_EQ(main.referenceOffset, 5000);
    const auto r = relate(input, main);
    ASSERT_TRUE(r);
    EXPECT_EQ(r->toMain(10000), 7000);  // 12000 on the reference domain, which the main input reaches at its tick 7000
    EXPECT_EQ(relate(main, main)->toMain(10000), 10000);
    EXPECT_EQ(relate(*parseDomain(tickDomain(1, 1000000, 1000, 0, "1970-01-01T00:00:00Z")), main)->toMain(10000),
              5000);  // no info: offset 0
}

TEST(MultiReader2Internals, RelationFailsOnDifferentRatesAndUnrepresentableOrigins)
{
    const auto main = *parseDomain(tickDomain());
    EXPECT_FALSE(relate(*parseDomain(tickDomain(1, 1000, 2)), main));  // 500 Hz
    const auto pico = *parseDomain(tickDomain(1, 1000000000000, 1000000000));
    const auto epochNs = *parseDomain(tickDomain(1, 1000000000, 1000000, 0, "1970-01-01T00:00:00Z"));
    EXPECT_FALSE(relate(epochNs, pico));  // 1.8e21 picoseconds apart does not fit Int64
    EXPECT_TRUE(relate(pico, epochNs));
}

TEST(MultiReader2Internals, TickArithmeticDoesNotOverflowWhileTheResultFits)
{
    const auto main = *parseDomain(tickDomain(1, 1000000, 1000, 0, "1970-01-01T00:00:00Z"));
    const auto ns = *parseDomain(tickDomain(1, 1000000000, 1000000, 0, "1970-01-01T00:00:00Z"));
    const auto r = relate(ns, main);
    ASSERT_TRUE(r);
    EXPECT_EQ(r->toMain(1767225600000000000), 1767225600000000);
}

TEST(MultiReader2Internals, SampleSizesFollowTheReadType)
{
    const auto vec = DataDescriptorBuilder()
                         .setSampleType(SampleType::Int16)
                         .setDimensions(List<IDimension>(Dimension(LinearDimensionRule(1, 0, 3))))
                         .build();
    EXPECT_EQ(readSampleSize(vec, SampleType::Float64), 24u);
    EXPECT_EQ(readSampleSize(vec, SampleType::Undefined), 6u);
    EXPECT_EQ(readSampleSize(DataDescriptorBuilder().setSampleType(SampleType::String).build(), SampleType::Float64), 0u);
    EXPECT_EQ(readSampleSize(nullptr, SampleType::Float64), 0u);
    EXPECT_FALSE(isReadType(SampleType::Struct));
    EXPECT_TRUE(isReadType(SampleType::Undefined));
}

TEST(MultiReader2Internals, ConversionRoundsHalfAwayFromZeroSaturatesAndZeroesNaN)
{
    const auto packet = packetOf(SampleType::Float64, {2.5, -2.5, 1e12, -1e12, std::numeric_limits<double>::quiet_NaN(), 1.49});
    int32_t out[6];
    copySamples(packet, 0, 6, out, SampleType::Int32);
    EXPECT_EQ(out[0], 3);
    EXPECT_EQ(out[1], -3);
    EXPECT_EQ(out[2], std::numeric_limits<int32_t>::max());
    EXPECT_EQ(out[3], std::numeric_limits<int32_t>::min());
    EXPECT_EQ(out[4], 0);
    EXPECT_EQ(out[5], 1);

    const auto big = packetOf(SampleType::Int64, {-5, 70000});
    uint16_t u[2];
    copySamples(big, 0, 2, u, SampleType::UInt16);
    EXPECT_EQ(u[0], 0);
    EXPECT_EQ(u[1], 65535);
}

TEST(MultiReader2Internals, ConversionAppliesPostScalingFirst)
{
    const auto packet = packetOf(SampleType::Int16, {1, 2, 3}, LinearScaling(0.5, 1, SampleType::Int16, ScaledSampleType::Float64));
    double out[2];
    copySamples(packet, 1, 2, out, SampleType::Float64);
    EXPECT_EQ(out[0], 2.0);
    EXPECT_EQ(out[1], 2.5);
}

TEST(MultiReader2Internals, QueueCountsSamplesInFrontOfTheFirstEvent)
{
    Queue q;
    q.pushData(dataPacket(10), 0, 10);
    q.pushData(dataPacket(10), 10, 10);
    EXPECT_EQ(q.frontCount(), 20u);
    multi_reader2::Event e{{true, nullptr, nullptr}, {true, nullptr, nullptr}};
    e.gap = true;
    EXPECT_TRUE(q.pushEvent(e));
    q.pushData(dataPacket(10), 50, 10);
    EXPECT_EQ(q.frontCount(), 20u);
    EXPECT_EQ(q.lastOwn(1), 19);

    SizeT seen = 0;
    q.consume(15, [&](const DataPacketPtr&, SizeT, SizeT n) { seen += n; });
    EXPECT_EQ(seen, 15u);
    EXPECT_EQ(q.frontCount(), 5u);
    EXPECT_EQ(q.firstOwn(1), 15);
    q.drop(5);
    EXPECT_TRUE(q.frontIsEvent());
    EXPECT_TRUE(q.popEvent().gap);
    EXPECT_EQ(q.frontCount(), 10u);
    EXPECT_EQ(q.firstOwn(1), 50);
}

TEST(MultiReader2Internals, QueueFoldsConsecutiveEventsAndDropsANetNoOp)
{
    Queue q;
    q.pushData(dataPacket(10), 0, 10);
    const Announced connected{true, nullptr, nullptr};
    const Announced disconnected{false, nullptr, nullptr};
    EXPECT_TRUE(q.pushEvent(multi_reader2::Event{connected, disconnected}));
    EXPECT_EQ(q.frontCount(), 10u);
    EXPECT_FALSE(q.pushEvent(multi_reader2::Event{disconnected, connected}));  // the reconnect cancels the disconnect
    EXPECT_FALSE(q.hasEvent());
    q.pushData(dataPacket(10), 10, 10);
    EXPECT_EQ(q.frontCount(), 20u);  // one run again
}

TEST(MultiReader2Internals, QueueFailureMarkerGoesInFrontOfTheData)
{
    Queue q;
    q.pushData(dataPacket(10), 0, 10);
    multi_reader2::Event marker{{true, nullptr, nullptr}, {true, nullptr, nullptr}};
    marker.syncFailed = SyncCause::Late;
    q.pushFrontEvent(marker);
    EXPECT_EQ(q.frontCount(), 0u);
    EXPECT_EQ(q.popEvent().syncFailed, SyncCause::Late);
    EXPECT_EQ(q.frontCount(), 10u);
}

// The main input holds ticks 0..999, the limit is 2000: input 1 proposes the common start, input 2 is ahead, input 3 ends
// before the start, input 4 is off the grid, and input 5 has nothing while the main input has sent 3000 since the start
TEST(MultiReader2Internals, SyncJudgementFailsEachCauseAndKeepsTheCommonStart)
{
    const std::vector<SyncInput> inputs{
        {500, 1500, false, false},
        {5000, 6000, false, false},
        {-5000, -3000, false, false},
        {100, 900, false, true},
        {std::nullopt, std::nullopt, true, false},
    };
    std::vector<SizeT> judged{0, 1, 2, 3, 4};
    std::vector<std::tuple<SizeT, SyncCause, bool>> failed;
    const auto common = judgeSync(
        0,
        999,
        3000,
        2000,
        judged,
        [&](SizeT i) { return inputs[i]; },
        [&](SizeT i, SyncCause cause, bool ahead) { failed.emplace_back(i, cause, ahead); });
    EXPECT_EQ(common, 500);
    EXPECT_EQ(judged, (std::vector<SizeT>{0, 2, 3, 4}));
    const std::vector<std::tuple<SizeT, SyncCause, bool>> expected{
        {1, SyncCause::Late, true},
        {2, SyncCause::Late, false},
        {3, SyncCause::OffGrid, false},
        {4, SyncCause::Late, false},
    };
    EXPECT_EQ(failed, expected);
}

TEST(MultiReader2Internals, SyncJudgementPassesInputsWithinTheLimit)
{
    const std::vector<SyncInput> inputs{{500, 1500, false, false}, {-1000, 2000, false, false}, {std::nullopt, std::nullopt, true, false}};
    std::vector<SizeT> judged{0, 1, 2};
    int failures = 0;
    const auto common =
        judgeSync(0, 999, 1000, 2000, judged, [&](SizeT i) { return inputs[i]; }, [&](SizeT, SyncCause, bool) { ++failures; });
    EXPECT_EQ(common, 500);
    EXPECT_EQ(failures, 0);
}
